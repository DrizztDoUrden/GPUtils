#pragma once

#include <CoreMinimal.h>
#include <EngineUtils.h>

class UWorld;

template <class TActor>
class ActorRange
{
public:
	using Element = TActor*;

	ActorRange(UWorld* world_)
		: world(world_)
	{
	}

	FORCEINLINE_DEBUGGABLE auto begin() const { return TActorIterator<TActor>{ world }; }
	FORCEINLINE_DEBUGGABLE auto end() const { return TActorIterator<TActor>{ EActorIteratorType::End }; }

private:
	UWorld* world;
};
