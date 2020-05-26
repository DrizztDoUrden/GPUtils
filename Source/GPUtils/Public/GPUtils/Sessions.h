#pragma once

#include <CoreMinimal.h>
#include <GameFramework/OnlineReplStructs.h>
#include <Interfaces/OnlineSessionInterface.h>
#include <OnlineSessionSettings.h>

#include "Sessions.generated.h"

using namespace UC;
using namespace US;
using namespace UM;
using namespace UP;
using namespace UF;

UENUM(BlueprintType)
enum class ESessionCreationResult : uint8
{
	Success,
	InvalidPlayer,
	InvalidUserId,
	OnlineSubsystemNotFound,
	SessionsInterfaceNotFound,
	FailedToInitializeSessionCreation,
	FailedToCreateSession,
	FailedToStartSession,
};

UENUM(BlueprintType)
enum class EFindSessionsResult : uint8
{
	Success,
	InvalidPlayer,
	InvalidUserId,
	OnlineSubsystemNotFound,
	SessionsInterfaceNotFound,
	FailedToFindSessions,
};

UENUM(BlueprintType)
enum class EJoinSessionResult : uint8
{
	Success,
	InvalidPlayer,
	InvalidUserId,
	OnlineSubsystemNotFound,
	SessionsInterfaceNotFound,
	FailedToJoinSession,
	FailedToFindController,
	FailedToTravel,
	SessionIsFull,
	SessionDoesNotExist,
	CouldNotRetrieveAddress,
	AlreadyInSession,
	UnknownError,
};

UENUM(BlueprintType)
enum class EDestroySessionResult : uint8
{
	Success,
	OnlineSubsystemNotFound,
	SessionsInterfaceNotFound,
	FailedToDestroySession,
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FSessionStartResultHandler, ESessionCreationResult, status);
DECLARE_DYNAMIC_DELEGATE_OneParam(FFindSessionResultHandler, EFindSessionsResult, status);
DECLARE_DYNAMIC_DELEGATE_OneParam(FJoinSessionResultHandler, EJoinSessionResult, status);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDestroySessionResultHandler, EDestroySessionResult, status);

class ULocalPlayer;

UCLASS(BlueprintType)
class GPUTILS_API USessionDetails : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class UFindSessionsTask* sessions;

	UPROPERTY()
	int32 id;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetMapName() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetOwnerName() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetPing() const;

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContext", HidePin = "worldContext"))
	void JoinAsync(UObject* worldContext, ULocalPlayer* player, FJoinSessionResultHandler callback);
};

UCLASS()
class GPUTILS_API UCreateSessionTask : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void HostSessionAsync(ULocalPlayer* player, FString name, bool lan, bool usesPresence, int32 playersLimit, FSessionStartResultHandler callback);

	static void HostSessionAsync(TSharedPtr<const FUniqueNetId> userId, FName name, const FOnlineSessionSettings& settings, FSessionStartResultHandler callback);

private:
	FSessionStartResultHandler callback;
	FDelegateHandle next;

	UFUNCTION()
	virtual void OnCreateSessionComplete(FName name, bool successful);

	UFUNCTION()
	virtual void OnStartOnlineGameComplete(FName name, bool successful);
};

UCLASS(BlueprintType)
class GPUTILS_API UFindSessionsTask : public UObject
{
	GENERATED_BODY()

public:
	FFindSessionResultHandler callback;
	TSharedPtr<FOnlineSessionSearch> search;

	UFindSessionsTask();

	UFUNCTION(BlueprintCallable)
	static UFindSessionsTask* FindSessionsAsync(ULocalPlayer* player, bool lan, bool presence, int32 timeoutInSeconds, FFindSessionResultHandler callback);

	bool FindSessionsAsync(TSharedPtr<const FUniqueNetId> userId);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContext", HidePin = "worldContext"))
	void JoinSessionAsync(UObject* worldContext, ULocalPlayer* player, int32 sessionId, FJoinSessionResultHandler callback);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<USessionDetails*> GetSessions();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetSearchState() const { return EOnlineAsyncTaskState::ToString(search->SearchState); }

private:
	FDelegateHandle next;

	UFUNCTION()
	virtual void OnFindSessionsComplete(bool successful);
};

class UWorld;

UCLASS()
class GPUTILS_API UJoinSessionTask : public UObject
{
	GENERATED_BODY()

public:
	UWorld* world;
	FJoinSessionResultHandler callback;

	void JoinSessionAsync(TSharedPtr<const FUniqueNetId> userId, FName sessionName, const FOnlineSessionSearchResult& searchResult);

private:
	FDelegateHandle next;

	virtual void OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result);
};

UCLASS()
class GPUTILS_API UDestroySessionTask : public UObject
{
	GENERATED_BODY()

public:
	FDestroySessionResultHandler callback;

	static void DestroySessionAsync(FName name, FDestroySessionResultHandler callback);

	void DestroySessionAsync(FName name);

private:
	FDelegateHandle next;

	virtual void OnDestroySessionComplete(FName name, bool successful);
};
