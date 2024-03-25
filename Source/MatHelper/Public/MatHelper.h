// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"

class UCusAssetDefinition_MatInstance;
class UMatHelperMgn;
class SMatHelperWidget;
class IMaterialEditor;


class FMatHelperModule : public IModuleInterface
{
public:
	inline static FMatHelperModule& Get();
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void EditorNotify(const FString&  NotifyInfo, SNotificationItem::ECompletionState State);
	static void RefreshAllWidgetButton();
	static void CreateMat(UMaterial* InMaterial,TWeakPtr<IMaterialEditor> InMatEditor);
	
	FString GetPluginPath() {return PluginPath;};
	TSharedRef<SDockTab> OnSpawnButtonInfoEditor(const FSpawnTabArgs& SpawnTabArgs);
	inline static const FName ButtonInfoEditorTabName = "ButtonInfoEditor";
	//TArray<TWeakPtr<IMaterialEditor>> GetMatEditors(){ return MatEditors;};
	
	inline static FString CurrentCreateInstanceName = "";
	UMatHelperMgn* MatHelperMgn;
	TArray<TSharedPtr<FString>> MaskPinOptions;
	
private:
	FString PluginPath;
	//TArray<TWeakPtr<IMaterialEditor>> MatEditors;
	void InitialMaskOptions();
};
