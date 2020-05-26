#include <GPUtils/Threads.h>

#include <Async/Async.h>

void ExecuteInGameThread(TUniqueFunction<void()>&& delegate)
{
	if (IsInGameThread())
		delegate();
	else
		AsyncTask(ENamedThreads::GameThread, MoveTemp(delegate));
}
