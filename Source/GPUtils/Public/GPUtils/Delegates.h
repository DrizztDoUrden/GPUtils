#pragma once

#include <CoreMinimal.h>

TScriptDelegate<> MakeDelegate(UObject* object, const FName& method)
{
	auto delegate = TScriptDelegate<>{};
	delegate.BindUFunction(object, method);
	return delegate;
}
