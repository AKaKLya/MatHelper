// Copyright AKaKLya 2024

#include "MatHelper.h"

#include "Engine/DeveloperSettings.h"
#include "TAccessPrivate.inl"
#include "AssetDefinitionRegistry.h"
#include "AssetSelection.h"
#include "AssetViewUtils.h"
#include "ButtonInfoEditor.h"
#include "ILevelSequenceModule.h"
#include "MatHelperMgn.h"
#include "MatHelperWidget.h"
#include "IMaterialEditor.h"
#include "MaterialInstanceEditor.h"
#include "MaterialEditorModule.h"
#include "EngineClass/CusAssetDefinition_Material.h"
#include "MaterialEditor.h"
#include "MovieScene.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemEditorData.h"
#include "OuterlineSelectionCol.h"
#include "SceneOutlinerModule.h"
#include "SMaterialPalette.h"
#include "ButtonClass/SimpleButtonCommands.h"
#include "Editor/Sequencer/Public/ISequencer.h"
#include "EngineClass/CusAssetDefinition_MatInstance.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "MovieScene/MovieSceneNiagaraSystemSpawnSection.h"
#include "MovieScene/MovieSceneNiagaraSystemTrack.h"


#define LOCTEXT_NAMESPACE "FMatHelperModule"

namespace MatHelperSpace
{
	TArray<TWeakPtr<SMatHelperWidget>> MhWidgets;
}
using namespace MatHelperSpace;


#pragma region PrivateAccess
// 用于访问私有成员变量.

// AccessPalette 
struct AccessPalette
{
	typedef TSharedPtr<class SMaterialPalette> (FMaterialEditor::*Type);
};

template struct TAccessPrivateStub<AccessPalette,&FMaterialEditor::Palette>;

// AccessNiagaraTrackHandle
struct AccessNiagaraTrackHandle
{
	typedef FDelegateHandle (FNiagaraEditorModule::*Type);
};
template struct TAccessPrivateStub<AccessNiagaraTrackHandle,&FNiagaraEditorModule::DefaultTrackHandle>;

// AccessNiagaraAgeUpdateMode
struct AccessNiagaraAgeUpdateMode
{
	typedef ENiagaraAgeUpdateMode (UMovieSceneNiagaraSystemSpawnSection::*Type);
};
template struct TAccessPrivateStub<AccessNiagaraAgeUpdateMode,&UMovieSceneNiagaraSystemSpawnSection::AgeUpdateMode>;


#pragma endregion


FMatHelperModule& FMatHelperModule::Get()
{
	return FModuleManager::Get().GetModuleChecked<FMatHelperModule>("MatHelper");
}


template<typename... Args>
void AddStringToTArray(TArray<TSharedPtr<FString>>& Array, Args&&... args)
{
	(Array.Add(MakeShareable(new FString(args))), ...);
}

void FMatHelperModule::StartupModule()
{
	PluginPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir();
	MatHelperMgn = LoadObject<UMatHelperMgn>(nullptr,TEXT("/MatHelper/MatHelper.MatHelper"));
	
	FSimpleButtonStyle::Initialize();
	FSimpleButtonStyle::ReloadTextures();
	FSimpleButtonCommands::Register();
	
	PlayNiagaraCommands = MakeShareable(new FUICommandList);
	PlayNiagaraCommands->MapAction(
		FSimpleButtonCommands::Get().PlayNiagaraAction,
		FExecuteAction::CreateRaw(this, &FMatHelperModule::PlayNiagaraOnEditorWorld),
		FCanExecuteAction());
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMatHelperModule::RegisterButton));
	
	//AddStringToTArray(MaskPinOptions, "R","G","B","A","RGB","RGBA","RG","BA","RG - BA","ShowName","ClearAllPin","Only RG - BA");
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ButtonInfoEditorTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnButtonInfoEditor))
		.SetDisplayName(FText::FromString("ButtonInfoEditor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	
	const UCusAssetDefinition_Material* MaterialDefinition = Cast<UCusAssetDefinition_Material>(UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(UMaterial::StaticClass()));
	UCusAssetDefinition_Material* NonConstMaterialDefinition = const_cast<UCusAssetDefinition_Material*>(MaterialDefinition);
	NonConstMaterialDefinition->Color = MatHelperMgn->MaterialAssetColor;

	const UCusAssetDefinition_MatInstance* MaterialInstanceDefinition = Cast<UCusAssetDefinition_MatInstance>(UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(UMaterialInstanceConstant::StaticClass()));
	UCusAssetDefinition_MatInstance* NonConstMaterialInstanceDefinition = const_cast<UCusAssetDefinition_MatInstance*>(MaterialInstanceDefinition);
	NonConstMaterialInstanceDefinition->Color = MatHelperMgn->MaterialInstanceAssetColor;
	
	IMaterialEditorModule& MatInterface = IMaterialEditorModule::Get();
	MatInterface.OnMaterialEditorOpened().AddLambda([&](const TWeakPtr<IMaterialEditor>& InMatEditor)
		{
			IMaterialEditor* IMatEditor = InMatEditor.Pin().Get();
			FMaterialEditor* MatEditor = static_cast<FMaterialEditor*>(IMatEditor);
			TSharedPtr<SMaterialPalette>& Palette = MatEditor->*TAccessPrivate<AccessPalette>::Value;
	
			IMatEditor->OnRegisterTabSpawners().AddLambda([&](const TSharedRef<class FTabManager>& TabManager)
			{
				const auto MhWidget = SNew(SMatHelperWidget, MatEditor);
				MhWidgets.Add(MhWidget);
				Palette = MhWidget;
			});
		});

	

	if(MatHelperMgn->OverrideNiagaraSequenceMode)
	{
		ILevelSequenceModule& LevelSequenceModule = FModuleManager::LoadModuleChecked<ILevelSequenceModule>("LevelSequence");
		const FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");

		const FDelegateHandle NiagaraTrackHandle = &NiagaraEditorModule->*TAccessPrivate<AccessNiagaraTrackHandle>::Value;
		LevelSequenceModule.OnNewActorTrackAdded().Remove(NiagaraTrackHandle);
		DefaultTrackHandle = LevelSequenceModule.OnNewActorTrackAdded().AddRaw(this,&FMatHelperModule::AddDefaultSystemTracks);
	}

	if(MatHelperMgn->CreateNiagaraAutoPlaySelection)
	{
		RegisterNiagaraAutoPlayer();
	}
}

TSharedRef<ISceneOutlinerColumn> FMatHelperModule::OnCreateOutlinerColumn(ISceneOutliner& SceneOutliner)
{
	return MakeShareable(new FOuterlineSelectionLockCol(SceneOutliner));
}

void FMatHelperModule::PlayNiagaraOnEditorWorld()
{
	TArray<AActor*> NiagaraActors;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	UGameplayStatics::GetAllActorsOfClass(World,ANiagaraActor::StaticClass(),NiagaraActors);
	for(AActor* NiagaraActor : NiagaraActors)
	{
		auto Niagara = Cast<ANiagaraActor>(NiagaraActor);
		if(Niagara->Tags.Contains("NiagaraAutoPlay"))
		{
			UNiagaraComponent* NiagaraComponent = Niagara->GetNiagaraComponent();
			NiagaraComponent->Activate(true);
			NiagaraComponent->ReregisterComponent();
		}
	}
}


void FMatHelperModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("LevelSequence"))
	{
		ILevelSequenceModule& LevelSequenceModule = FModuleManager::LoadModuleChecked<ILevelSequenceModule>("LevelSequence");
		LevelSequenceModule.OnNewActorTrackAdded().Remove(DefaultTrackHandle);
	}
	if (FModuleManager::Get().IsModuleLoaded("MaterialEditor"))
	{
		IMaterialEditorModule& MatInterface = IMaterialEditorModule::Get();
		MatInterface.OnMaterialEditorOpened().Remove(MaterialOpenHandle);	
	}
}


void FMatHelperModule::EditorNotify(const FString& NotifyInfo, SNotificationItem::ECompletionState State)
{
	FNotificationInfo Info( FText::FromString(NotifyInfo) );
	Info.FadeInDuration = 0.5f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 5.0f;
	const auto NotificationItem = FSlateNotificationManager::Get().AddNotification( Info );
	NotificationItem->SetCompletionState(State);
	NotificationItem->ExpireAndFadeout();
}

void FMatHelperModule::RefreshAllWidgetButton()
{
	for(auto MhWidget : MhWidgets)
	{
		if(MhWidget.Pin().IsValid())
		{
			MhWidget.Pin().Get()->InitialButton();	
		}
	}
}

TSharedRef<SDockTab> FMatHelperModule::OnSpawnButtonInfoEditor(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
					SNew(SButtonInfoEditor,MatHelperMgn->NodeButtonInfo)
			];
}




void FMatHelperModule::ProcessLockingForOutliner(AActor* ActorToProcess, bool bShouldLock)
{
	if(bShouldLock)
	{
		ActorToProcess->Tags.Add("NiagaraAutoPlay");
	}
	else
	{
		ActorToProcess->Tags.Remove("NiagaraAutoPlay");
	}
}


void FMatHelperModule::RegisterButton()
{
	
	
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Tools");
	{
		FToolMenuSection& MenuSection = Menu->AddSection("MatHelper", LOCTEXT("FSwitchCNModule", "MatHelper"));
		MenuSection.AddDynamicEntry("MatHelper", FNewToolMenuSectionDelegate::CreateLambda([&](FToolMenuSection& InSection)
		{
			InSection.AddEntry(FToolMenuEntry::InitSubMenu(
			"MatHelper",
			FText::FromString("MatHelper"),
			FText::GetEmpty(),
			FNewToolMenuDelegate::CreateLambda([&](UToolMenu* InToolMenu)
			{
				FToolMenuSection& Section = InToolMenu->FindOrAddSection("MatHelper");
				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"MatHelper",
					LOCTEXT("FMatHelperModule", "MatHelper"),
					LOCTEXT("FMatHelperModule", "Open MatHelper Manager"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						AssetViewUtils::OpenEditorForAsset("/MatHelper/MatHelper.MatHelper");
					}))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"SwitchCN",
					LOCTEXT("FMatHelperModule", "SwitchCN"),
					LOCTEXT("FMatHelperModule", "Switch Language Between CN And EN"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						if(UKismetInternationalizationLibrary::GetCurrentLanguage() == "en")
						{
							UKismetInternationalizationLibrary::SetCurrentLanguage("zh-cn",true);
						}
						else
						{
							UKismetInternationalizationLibrary::SetCurrentLanguage("en",true);
						}
					}))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"Lock",
					LOCTEXT("FMatHelperModule", "Lock"),
					LOCTEXT("FMatHelperModule", "LockSelectedAssets"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateRaw(this, &FMatHelperModule::ToggleAssetFlag,true))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"UnLock",
					LOCTEXT("FMatHelperModule", "UnLock"),
					LOCTEXT("FMatHelperModule", "UnLockSelectedAssets"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateRaw(this, &FMatHelperModule::ToggleAssetFlag,false))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"Restart",
					LOCTEXT("FMatHelperModule", "Restart UE"),
					LOCTEXT("FMatHelperModule", "Restart Engine"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						bool bRestart = false;
						bRestart = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo,FText::FromString("Restart UE ?"));
						if(bRestart)
						{
							FUnrealEdMisc::Get().RestartEditor(false);
						}
					}))
				));

			}),
			false,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.SaveLayout") 
			));
			}));
		}
}

void FMatHelperModule::RegisterNiagaraAutoPlayer()
{
	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(
						FSimpleButtonCommands::Get().PlayNiagaraAction,
						FText::FromString("Play Niagara"),
						FText::FromString("Play Niagara"),
						FSlateIcon(TEXT("SimpleButtonStyle"),"SimpleButton.Niagara")
					));
				Entry.SetCommandList(PlayNiagaraCommands);
			}
		}
	}
	
	auto& SceneOutlinerModule = FModuleManager::LoadModuleChecked<FSceneOutlinerModule>("SceneOutliner");

	FSceneOutlinerColumnInfo SceneOutlinerColumnInfo(
		ESceneOutlinerColumnVisibility::Visible,
		1,
		FCreateSceneOutlinerColumn::CreateStatic(&FMatHelperModule::OnCreateOutlinerColumn)
		);
	SceneOutlinerModule.RegisterDefaultColumnType<FOuterlineSelectionLockCol>(SceneOutlinerColumnInfo);
	
}

void FMatHelperModule::ToggleAssetFlag(bool bIsLock)
{
	int32 ConvertFileNum = 0;
	TArray<FAssetData> ObjectsToExport;
	AssetSelectionUtils::GetSelectedAssets(ObjectsToExport);
	if(bIsLock)
	{
		for(auto AssetData : ObjectsToExport)
		{
			if(UPackage* Package = AssetData.GetAsset()->GetOutermost(); !Package->HasAnyPackageFlags(PKG_DisallowExport))
			{
				Package->SetPackageFlags(PKG_DisallowExport);
				ConvertFileNum = ConvertFileNum + 1;
			}
		}
	}
	else
	{
		for(auto AssetData : ObjectsToExport)
		{
			if(UPackage* Package = AssetData.GetAsset()->GetOutermost(); Package->HasAnyPackageFlags(PKG_DisallowExport))
			{
				Package->ClearPackageFlags(PKG_DisallowExport);
				ConvertFileNum = ConvertFileNum + 1;
			}
		}
	}

	EditorNotify(FString::Printf(TEXT("%s %d Files"),bIsLock ? TEXT("Lock") : TEXT("UnLock"),ConvertFileNum),SNotificationItem::CS_Success);
}


// 原始函数 void FNiagaraSystemTrackEditor::AddDefaultSystemTracks(const AActor& SourceActor, const FGuid& Binding,	TSharedPtr<ISequencer> Sequencer) 
void FMatHelperModule::AddDefaultSystemTracks(const AActor& SourceActor, const FGuid& Binding,	TSharedPtr<ISequencer> Sequencer)
{
	//ENiagaraAddDefaultsTrackMode TrackMode = GetDefault<UNiagaraEditorSettings>()->DefaultsSequencerSubtracks;
	if (!SourceActor.IsA<ANiagaraActor>() )
	{
		return;
	}

	const ANiagaraActor* NiagaraActor = Cast<ANiagaraActor>(&SourceActor);
	UNiagaraComponent* NiagaraComponent = NiagaraActor->GetNiagaraComponent();
	if (NiagaraComponent && NiagaraComponent->GetAsset())
	{
		const UMovieSceneSequence* Sequence = Sequencer->GetFocusedMovieSceneSequence();
		UMovieScene* MovieScene = Sequence->GetMovieScene();
		const FGuid ComponentBinding = Sequencer->GetHandleToObject(NiagaraComponent);
		
		if (MovieScene->FindSpawnable(Binding))
		{
			// we only want to add tracks for possessables
			return;
		}
		
		UClass* TrackClass = UMovieSceneNiagaraSystemTrack::StaticClass();
		const UMovieSceneTrack* NewTrack = MovieScene->FindTrack(TrackClass, ComponentBinding);
		if (!NewTrack)
		{
			UNiagaraSystem* System = NiagaraComponent->GetAsset();

			UMovieSceneNiagaraSystemTrack* NiagaraSystemTrack = MovieScene->AddTrack<UMovieSceneNiagaraSystemTrack>(ComponentBinding);
			NiagaraSystemTrack->SetDisplayName(LOCTEXT("SystemLifeCycleTrackName", "System Life Cycle"));
			UMovieSceneNiagaraSystemSpawnSection* SpawnSection = NewObject<UMovieSceneNiagaraSystemSpawnSection>(NiagaraSystemTrack, NAME_None, RF_Transactional);
				
			ENiagaraAgeUpdateMode& NiagaraAgeUpdateMode = SpawnSection->*TAccessPrivate<AccessNiagaraAgeUpdateMode>::Value;
			NiagaraAgeUpdateMode = ENiagaraAgeUpdateMode::DesiredAge;

			const FFrameRate FrameResolution = MovieScene->GetTickResolution();
			const FFrameTime SpawnSectionStartTime = Sequencer->GetLocalTime().ConvertTo(FrameResolution);
			FFrameTime SpawnSectionDuration;

			const UNiagaraSystemEditorData* SystemEditorData = Cast<UNiagaraSystemEditorData>(System->GetEditorData());
			if (SystemEditorData != nullptr && SystemEditorData->GetPlaybackRange().HasLowerBound() && SystemEditorData->GetPlaybackRange().HasUpperBound())
			{
				SpawnSectionDuration = FrameResolution.AsFrameTime(SystemEditorData->GetPlaybackRange().Size<float>());
			}
			else
			{
				SpawnSectionDuration = FrameResolution.AsFrameTime(5.0);
			}

			SpawnSection->SetRange(TRange<FFrameNumber>(
				SpawnSectionStartTime.RoundToFrame(),
				(SpawnSectionStartTime + SpawnSectionDuration).RoundToFrame()));
			NiagaraSystemTrack->AddSection(*SpawnSection);
		}
	}
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMatHelperModule, MatHelper)
