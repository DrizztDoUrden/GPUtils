#pragma once

#include <Algo/Accumulate.h>
#include <CoreMinimal.h>
#include <Engine/World.h>

class UWorld;

class PlayerControllerRange
{
public:
	using Element = TWeakObjectPtr<APlayerController>;

	PlayerControllerRange(UWorld* world_)
		: world(world_)
	{
	}

	FORCEINLINE_DEBUGGABLE auto begin() const { return world->GetPlayerControllerIterator(); }

	FORCEINLINE_DEBUGGABLE auto end() const
	{
		auto ret = world->GetPlayerControllerIterator();
		ret.SetToEnd();
		return ret;
	}

	FORCEINLINE_DEBUGGABLE auto Num() const { return Algo::Accumulate(*this, 0, [](auto v, auto p) { return v + 1; }); }

private:
	UWorld* world;
};
