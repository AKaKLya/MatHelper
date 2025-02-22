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
	
	inline static const FName ButtonInfoEditorTabName = "ButtonInfoEditor";
	inline static const FName SceneViewEditorTabName = "SceneEditorView";
	inline static const FName MaterialSceneViewEditorTabName = "MaterialSceneEditorView";
	
	static void PlayNiagaraOnEditorWorld();
	
private:
	FString PluginPath;
	TSharedPtr<class FUICommandList> PlayNiagaraCommands;
	
	void RegisterTab();
	void RegisterButton();
	void RegisterNiagaraAutoPlayer();
	void ToggleAssetFlag(bool bIsLock);
	void NiagaraToolBarExtend(FToolBarBuilder& ToolbarBuilder);
	
	void InitMatEditorHook();
	void InitNiagaraEditorHook();
	void InitPluginInfo();
	
	void AddDefaultSystemTracks(const AActor& SourceActor, const FGuid& Binding,TSharedPtr<ISequencer> Sequencer);
	FDelegateHandle DefaultTrackHandle;
	FDelegateHandle MaterialOpenHandle;

	
};

