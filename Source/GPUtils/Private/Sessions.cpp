#include <GPUtils/Sessions.h>

#include <GPUtils/PlayerControllerRange.h>
#include <GPUtils/Range.h>

#include <GameFramework/GameSession.h>
#include <Engine/LocalPlayer.h>
#include <Engine/World.h>
#include <Kismet/GameplayStatics.h>
#include <OnlineSessionSettings.h>
#include <OnlineSubsystem.h>

DEFINE_LOG_CATEGORY_STATIC(Sessions, Display, Display);

template <class TError, class TCallback>
static IOnlineSessionPtr GetSessionsInterface(const TCallback& callback, TError ossNotFound, TError siNotFound)
{
	const auto onlineSubsystem = IOnlineSubsystem::Get();
	if (!onlineSubsystem)
	{
		UE_LOG(Sessions, Error, TEXT("Online subsystem not found."));
		callback.ExecuteIfBound(ossNotFound);
		return nullptr;
	}
	auto sessions = onlineSubsystem->GetSessionInterface();
	if (!sessions.IsValid())
	{
		UE_LOG(Sessions, Error, TEXT("Sessions interface not found."));
		callback.ExecuteIfBound(siNotFound);
		return nullptr;
	}

	return sessions;
}

FString USessionDetails::GetMapName() const
{
	const auto found = sessions->search->SearchResults[id].Session.SessionSettings.Settings.Find(SETTING_MAPNAME);
	if (found == nullptr)
		return "";
	FString ret;
	found->Data.GetValue(ret);
	return ret;
}

FString USessionDetails::GetOwnerName() const
{
	return sessions->search->SearchResults[id].Session.OwningUserName;
}

int32 USessionDetails::GetPing() const
{
	return sessions->search->SearchResults[id].PingInMs;
}

void USessionDetails::JoinAsync(UObject* worldContext, ULocalPlayer* player, FJoinSessionResultHandler callback)
{
	sessions->JoinSessionAsync(worldContext, player, id, callback);
}

void UCreateSessionTask::HostSessionAsync(ULocalPlayer* player, FString name, bool lan, bool usesPresence, int32 playersLimit, FSessionStartResultHandler callback)
{
	if (player == nullptr)
	{
		UE_LOG(Sessions, Error, TEXT("Player is invalid."));
		callback.ExecuteIfBound(ESessionCreationResult::InvalidPlayer);
		return;
	}

	auto settings = FOnlineSessionSettings{};
	settings.bIsLANMatch = lan;
	settings.bUsesPresence = usesPresence;
	settings.NumPublicConnections = playersLimit;
	settings.NumPrivateConnections = 0;
	settings.bAllowInvites = true;
	settings.bAllowJoinInProgress = true;
	settings.bShouldAdvertise = true;
	settings.bAllowJoinViaPresence = true;
	settings.bAllowJoinViaPresenceFriendsOnly = false;

	settings.Set(SETTING_MAPNAME, name, EOnlineDataAdvertisementType::ViaOnlineService);

	HostSessionAsync(player->GetPreferredUniqueNetId().GetUniqueNetId(), GameSessionName, settings, callback);
}

void UCreateSessionTask::HostSessionAsync(TSharedPtr<const FUniqueNetId> userId, FName name, const FOnlineSessionSettings& settings, FSessionStartResultHandler callback)
{
	if (!userId.IsValid())
	{
		UE_LOG(Sessions, Error, TEXT("User id is invalid."));
		callback.ExecuteIfBound(ESessionCreationResult::InvalidUserId);
		return;
	}

	auto sessions = GetSessionsInterface(callback, ESessionCreationResult::OnlineSubsystemNotFound, ESessionCreationResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return;

	auto task = NewObject<UCreateSessionTask>();

	if (callback.IsBound())
		task->callback = callback;

	const auto delegate = FOnCreateSessionCompleteDelegate::CreateUObject(task, &UCreateSessionTask::OnCreateSessionComplete);
	task->next = sessions->AddOnCreateSessionCompleteDelegate_Handle(delegate);

	sessions->DestroySession(name); // Todo: remove
	sessions->CreateSession(*userId, name, settings);
}

void UCreateSessionTask::OnCreateSessionComplete(FName name, bool successful)
{
	auto sessions = GetSessionsInterface(callback, ESessionCreationResult::OnlineSubsystemNotFound, ESessionCreationResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return;

	sessions->ClearOnCreateSessionCompleteDelegate_Handle(next);

	if (!successful)
	{
		UE_LOG(Sessions, Error, TEXT("Failed to create a session."));
		callback.ExecuteIfBound(ESessionCreationResult::FailedToCreateSession);
		return;
	}

	const auto delegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UCreateSessionTask::OnStartOnlineGameComplete);
	next = sessions->AddOnStartSessionCompleteDelegate_Handle(delegate);

	sessions->StartSession(name);
}

void UCreateSessionTask::OnStartOnlineGameComplete(FName name, bool successful)
{
	auto sessions = GetSessionsInterface(callback, ESessionCreationResult::OnlineSubsystemNotFound, ESessionCreationResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return;

	sessions->ClearOnStartSessionCompleteDelegate_Handle(next);

	if (!successful)
	{
		sessions->DestroySession(name);
		UE_LOG(Sessions, Error, TEXT("Failed to start session."));
		callback.ExecuteIfBound(ESessionCreationResult::FailedToStartSession);
		return;
	}

	callback.ExecuteIfBound(ESessionCreationResult::Success);
}

UFindSessionsTask::UFindSessionsTask()
{
	search = MakeShared<FOnlineSessionSearch>();
}

UFindSessionsTask* UFindSessionsTask::FindSessionsAsync(ULocalPlayer* player, bool lan, bool presence, int32 timeoutInSeconds, FFindSessionResultHandler callback_)
{
	if (player == nullptr)
	{
		UE_LOG(Sessions, Error, TEXT("Player is invalid."));
		callback_.ExecuteIfBound(EFindSessionsResult::InvalidPlayer);
		return nullptr;
	}

	auto task = NewObject<UFindSessionsTask>();

	task->search->bIsLanQuery = lan;
	task->search->MaxSearchResults = 1;
	task->search->PingBucketSize = 50;
	task->search->TimeoutInSeconds = timeoutInSeconds;

	if (presence)
		task->search->QuerySettings.Set(SEARCH_PRESENCE, presence, EOnlineComparisonOp::Equals);

	task->callback = callback_;

	if (!task->FindSessionsAsync(player->GetPreferredUniqueNetId().GetUniqueNetId()))
		return nullptr;
	return task;
}

bool UFindSessionsTask::FindSessionsAsync(TSharedPtr<const FUniqueNetId> userId)
{
	if (!userId.IsValid())
	{
		UE_LOG(Sessions, Error, TEXT("User id is invalid."));
		callback.ExecuteIfBound(EFindSessionsResult::InvalidUserId);
		return false;
	}

	auto sessions = GetSessionsInterface(callback, EFindSessionsResult::OnlineSubsystemNotFound, EFindSessionsResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return false;

	if (search->SearchState != EOnlineAsyncTaskState::Done && search->SearchState != EOnlineAsyncTaskState::NotStarted)
	{
		sessions->CancelFindSessions();
	}

	const auto delegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UFindSessionsTask::OnFindSessionsComplete);
	next = sessions->AddOnFindSessionsCompleteDelegate_Handle(delegate);

	return sessions->FindSessions(*userId, search.ToSharedRef());
}

void UFindSessionsTask::JoinSessionAsync(UObject* worldContext, ULocalPlayer* player, int32 sessionId, FJoinSessionResultHandler callback_)
{
	if (player == nullptr)
	{
		UE_LOG(Sessions, Error, TEXT("Player is invalid."));
		callback_.ExecuteIfBound(EJoinSessionResult::InvalidPlayer);
		return;
	}

	auto task = NewObject<UJoinSessionTask>(this);
	task->world = worldContext->GetWorld();
	task->callback = callback_;
	task->JoinSessionAsync(player->GetPreferredUniqueNetId().GetUniqueNetId(), GameSessionName, search->SearchResults[sessionId]);
}

TArray<USessionDetails*> UFindSessionsTask::GetSessions()
{
	TArray<USessionDetails*> ret;

	for (auto i : Range::Indexes<int32>(search->SearchResults))
	{
		auto session = NewObject<USessionDetails>();
		session->sessions = this;
		session->id = i;
		ret.Add(session);
	}

	return ret;
}

void UFindSessionsTask::OnFindSessionsComplete(bool successful)
{
	auto sessions = GetSessionsInterface(callback, EFindSessionsResult::OnlineSubsystemNotFound, EFindSessionsResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return;

	sessions->ClearOnFindSessionsCompleteDelegate_Handle(next);

	if (!successful)
	{
		UE_LOG(Sessions, Error, TEXT("Failed to find sessions."));
		callback.ExecuteIfBound(EFindSessionsResult::FailedToFindSessions);
		return;
	}

	callback.ExecuteIfBound(EFindSessionsResult::Success);
}

void UJoinSessionTask::JoinSessionAsync(TSharedPtr<const FUniqueNetId> userId, FName sessionName, const FOnlineSessionSearchResult& searchResult)
{
	if (!userId.IsValid())
	{
		UE_LOG(Sessions, Error, TEXT("User id is invalid."));
		callback.ExecuteIfBound(EJoinSessionResult::InvalidUserId);
		return;
	}

	auto sessions = GetSessionsInterface(callback, EJoinSessionResult::OnlineSubsystemNotFound, EJoinSessionResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return;

	const auto delegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UJoinSessionTask::OnJoinSessionComplete);
	next = sessions->AddOnJoinSessionCompleteDelegate_Handle(delegate);

	sessions->JoinSession(*userId, sessionName, searchResult);
}

void UJoinSessionTask::OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result)
{
	auto sessions = GetSessionsInterface(callback, EJoinSessionResult::OnlineSubsystemNotFound, EJoinSessionResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return;

	sessions->ClearOnJoinSessionCompleteDelegate_Handle(next);

	switch (result)
	{
	case EOnJoinSessionCompleteResult::SessionIsFull:
		UE_LOG(Sessions, Log, TEXT("Session is full."));
		callback.ExecuteIfBound(EJoinSessionResult::SessionIsFull);
		return;
	case EOnJoinSessionCompleteResult::SessionDoesNotExist:
		UE_LOG(Sessions, Log, TEXT("Session doesn't exist."));
		callback.ExecuteIfBound(EJoinSessionResult::SessionDoesNotExist);
		return;
	case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
		UE_LOG(Sessions, Log, TEXT("Could not retrieve session address."));
		callback.ExecuteIfBound(EJoinSessionResult::CouldNotRetrieveAddress);
		return;
	case EOnJoinSessionCompleteResult::AlreadyInSession:
		UE_LOG(Sessions, Log, TEXT("Already in session."));
		callback.ExecuteIfBound(EJoinSessionResult::AlreadyInSession);
		return;
	case EOnJoinSessionCompleteResult::UnknownError:
		UE_LOG(Sessions, Error, TEXT("Unknown error while joining a session."));
		callback.ExecuteIfBound(EJoinSessionResult::UnknownError);
		return;
	case EOnJoinSessionCompleteResult::Success:
		break;
	}

	const auto controllers = PlayerControllerRange(world);
	if (controllers.begin() == controllers.end())
	{
		UE_LOG(Sessions, Error, TEXT("Can't find controller to travel."));
		callback.ExecuteIfBound(EJoinSessionResult::FailedToFindController);
		return;
	}

	const auto controller = *PlayerControllerRange(world).begin();
	if (!controller.IsValid())
	{
		UE_LOG(Sessions, Error, TEXT("Can't find controller to travel."));
		callback.ExecuteIfBound(EJoinSessionResult::FailedToFindController);
		return;
	}

	FString travelURL;

	if (!sessions->GetResolvedConnectString(sessionName, travelURL))
	{
		UE_LOG(Sessions, Error, TEXT("Can't travel."));
		callback.ExecuteIfBound(EJoinSessionResult::FailedToTravel);
		return;
	}

	UE_LOG(Sessions, Warning, TEXT("%s."), *travelURL);
	controller->ClientTravel(travelURL, ETravelType::TRAVEL_Absolute);
	callback.ExecuteIfBound(EJoinSessionResult::Success);
}

void UDestroySessionTask::DestroySessionAsync(FName name, FDestroySessionResultHandler callback_)
{
	auto task = NewObject<UDestroySessionTask>();
	task->callback = callback_;
	task->DestroySessionAsync(name);
}

void UDestroySessionTask::DestroySessionAsync(FName name)
{
	auto sessions = GetSessionsInterface(callback, EDestroySessionResult::OnlineSubsystemNotFound, EDestroySessionResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return;

	const auto delegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UDestroySessionTask::OnDestroySessionComplete);
	next = sessions->AddOnDestroySessionCompleteDelegate_Handle(delegate);

	sessions->DestroySession(name);
}

void UDestroySessionTask::OnDestroySessionComplete(FName name, bool successful)
{
	auto sessions = GetSessionsInterface(callback, EDestroySessionResult::OnlineSubsystemNotFound, EDestroySessionResult::SessionsInterfaceNotFound);
	if (!sessions.IsValid())
		return;

	sessions->ClearOnDestroySessionCompleteDelegate_Handle(next);

	if (!successful)
		UE_LOG(Sessions, Error, TEXT("Failed to destroy a session."));

	callback.ExecuteIfBound(successful ? EDestroySessionResult::Success : EDestroySessionResult::FailedToDestroySession);
}
