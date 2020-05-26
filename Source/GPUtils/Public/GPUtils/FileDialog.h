#pragma once

#include <Containers/UnrealString.h>

TArray<FString> GPUTILS_API OpenFileDialog(const FString& dialogTitle, const FString& defaultPath, bool allowMultiple = false, const FString& fileTypes = TEXT(""), const FString& defaultFile = TEXT(""));
