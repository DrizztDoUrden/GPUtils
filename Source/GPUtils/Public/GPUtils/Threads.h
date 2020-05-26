#pragma once

#include <CoreMinimal.h>

void GPUTILS_API ExecuteInGameThread(TUniqueFunction<void()>&& delegate);
