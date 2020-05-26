#include <GPUtils/FileDialog.h>

#include <DesktopPlatform/Public/DesktopPlatformModule.h>
#include <Framework/Application/SlateApplication.h>
#include <Widgets/SWindow.h>

TArray<FString> OpenFileDialog(const FString& dialogTitle, const FString& defaultPath, bool allowMultiple, const FString& fileTypes, const FString& defaultFile)
{
    const auto windowPtr = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
    const auto desktopPlatform = FDesktopPlatformModule::Get();
    if (!desktopPlatform)
        return {};
    auto ret = TArray<FString>{};
    const uint32 flags = allowMultiple ? 1 : 0;
    desktopPlatform->OpenFileDialog(windowPtr, dialogTitle, defaultPath, defaultFile, fileTypes, flags, ret);
    return MoveTemp(ret);
}
