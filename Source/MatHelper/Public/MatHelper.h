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
	TSharedRef<SDockTab> OnSpawnSceneEditorView(const FSpawnTabArgs& SpawnTabArgs);
	UMatHelperMgn* MatHelperMgn;
	static TSharedRef<ISceneOutlinerColumn> OnCreateOutlinerColumn(ISceneOutliner& SceneOutliner);
	void ProcessLockingForOutliner(AActor* ActorToProcess,bool bShouldLock);
	static void PlayNiagaraOnEditorWorld();
	
	inline static const FName ButtonInfoEditorTabName = "ButtonInfoEditor";
	inline static const FName SceneViewEditorTabName = "SceneEditorView";
	inline static const FName MaterialSceneViewEditorTabName = "MaterialSceneEditorView";
	inline static const FName MaterialInstanceSceneViewEditorTabName = "MaterialInstanceSceneEditorView";
	inline static const FName NiagaraSceneViewEditorTabName = "NiagaraSceneViewEditorTabName";
	static const FName SceneViewEditorTabName1;
	static const FName SceneViewEditorTabName2;
	static const FName SceneViewEditorTabName3;
	static const FName SceneViewEditorTabName4;
	static const FName SceneViewEditorTabName5;
	static const FName SceneViewEditorTabName6;
	static const FName SceneViewEditorTabName7;
	static const FName SceneViewEditorTabName8;
	static const FName SceneViewEditorTabName9;

private:
	FString PluginPath;
	TSharedPtr<class FUICommandList> PlayNiagaraCommands;
	void RegisterTab();
	void RegisterButton();
	void RegisterNiagaraAutoPlayer();
	void ToggleAssetFlag(bool bIsLock);
	//TSharedPtr<class FUICommandList> PluginCommands;
	void NiagaraToolBarExtend(FToolBarBuilder& ToolbarBuilder);
	
	
	
	void AddDefaultSystemTracks(const AActor& SourceActor, const FGuid& Binding,TSharedPtr<ISequencer> Sequencer);
	FDelegateHandle DefaultTrackHandle;
	FDelegateHandle MaterialOpenHandle;

	
};

