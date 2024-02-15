// Copyright AKaKLya

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
	

private:
	FString PluginPath;
	TArray<TWeakPtr<IMaterialEditor>> MatEditors;
	
};
