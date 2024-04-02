// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Widgets/Notifications/SNotificationList.h"

class ISceneOutliner;
class ISceneOutlinerColumn;
class ISequencer;
class UCusAssetDefinition_MatInstance;
class UMatHelperMgn;
class SMatHelperWidget;
class IMaterialEditor;


class FMatHelperModule : public IModuleInterface
{
public:
	static FMatHelperModule& Get();
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void EditorNotify(const FString&  NotifyInfo, SNotificationItem::ECompletionState State);
	static void RefreshAllWidgetButton();
	
	FString GetPluginPath() {return PluginPath;};
	TSharedRef<SDockTab> OnSpawnButtonInfoEditor(const FSpawnTabArgs& SpawnTabArgs);
	inline static const FName ButtonInfoEditorTabName = "ButtonInfoEditor";
	
	
	inline static FString CurrentCreateInstanceName = "";
	UMatHelperMgn* MatHelperMgn;
	TArray<TSharedPtr<FString>> MaskPinOptions;
	static TSharedRef<ISceneOutlinerColumn> OnCreateOutlinerColumn(ISceneOutliner& SceneOutliner);
	void ProcessLockingForOutliner(AActor* ActorToProcess,bool bShouldLock);
private:
	FString PluginPath;
	void TestLog(UToolMenu* Menu);
	void RegisterButton();
	void RegisterNiagaraAutoPlayer();
	void ToggleAssetFlag(bool bIsLock);
	//TSharedPtr<class FUICommandList> PluginCommands;
	
	TSharedPtr<class FUICommandList> PlayNiagaraCommands;
	void PlayNiagaraOnEditorWorld();
	
	FDelegateHandle DefaultTrackHandle;
	void AddDefaultSystemTracks(const AActor& SourceActor, const FGuid& Binding,TSharedPtr<ISequencer> Sequencer);
	FDelegateHandle MaterialOpenHandle;
};

