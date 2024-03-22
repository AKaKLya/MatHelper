// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"

class UMatHelperMgn;
class SMatHelperWidget;
class IMaterialEditor;

class FMatHelperModule : public IModuleInterface
{
public:
	
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	FString GetPluginPath() {return PluginPath;};
	TArray<TWeakPtr<IMaterialEditor>> GetMatEditors(){ return MatEditors;};
	
	void EditorNotify(const FString&  NotifyInfo, SNotificationItem::ECompletionState State);
	TArray<TSharedPtr<FString>> MaskPinOptions;
	static void CreateMat(UMaterial* InMaterial,TWeakPtr<IMaterialEditor> InMatEditor);
	UMatHelperMgn* MatHelperMgn; 
private:
	FString PluginPath;
	TArray<TWeakPtr<IMaterialEditor>> MatEditors;
	
};
