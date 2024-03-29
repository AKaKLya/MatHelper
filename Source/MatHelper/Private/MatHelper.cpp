// Copyright AKaKLya 2024

#include "MatHelper.h"
#include "ISequencerModule.h"
#include "Engine/DeveloperSettings.h"
#include "TAccessPrivate.inl"
#include "AssetDefinitionRegistry.h"
#include "AssetViewUtils.h"
#include "ButtonInfoEditor.h"
#include "ILevelSequenceModule.h"
#include "MatHelperMgn.h"
#include "MatHelperWidget.h"
#include "IMaterialEditor.h"
#include "MaterialEditorModule.h"
#include "EngineClass/CusAssetDefinition_Material.h"
#include "MaterialEditor.h"
#include "MaterialEditorActions.h"
#include "MovieScene.h"
#include "MovieScenePossessable.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraEditorSettings.h"
#include "NiagaraSystemEditorData.h"
#include "OuterlineSelectionCol.h"
#include "SceneOutlinerModule.h"
#include "SMaterialPalette.h"
#include "ButtonClass/SimpleButtonCommands.h"
#include "Editor/Sequencer/Public/ISequencer.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/GameplayStatics.h"
#include "MovieScene/MovieSceneNiagaraSystemSpawnSection.h"
#include "MovieScene/MovieSceneNiagaraSystemTrack.h"
#include "Subsystems/EditorActorSubsystem.h"

#define LOCTEXT_NAMESPACE "FMatHelperModule"

namespace MatHelperSpace
{
	TArray<TWeakPtr<SMatHelperWidget>> MhWidgets;
}
using namespace MatHelperSpace;

#pragma region PrivateAccesser
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


void FMatHelperModule::StartupModule()
{
	PluginPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir();
	
	
	FSimpleButtonStyle::Initialize();
	FSimpleButtonStyle::ReloadTextures();
	FSimpleButtonCommands::Register();
	
	PlayNiagaraCommands = MakeShareable(new FUICommandList);
	PlayNiagaraCommands->MapAction(
		FSimpleButtonCommands::Get().PlayNiagaraAction,
		FExecuteAction::CreateRaw(this, &FMatHelperModule::PlayNiagaraOnEditorWorld),
		FCanExecuteAction());
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMatHelperModule::RegisterButton));

	InitialMaskOptions();

	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ButtonInfoEditorTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnButtonInfoEditor))
		.SetDisplayName(FText::FromString("ButtonInfoEditor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	
	
	MatHelperMgn = LoadObject<UMatHelperMgn>(nullptr,TEXT("/MatHelper/MatHelper.MatHelper"));
	
	const UCusAssetDefinition_Material* MaterialDefinition = Cast<UCusAssetDefinition_Material>(UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(UMaterial::StaticClass()));
	UCusAssetDefinition_Material* NonConstMaterialDefinition = const_cast<UCusAssetDefinition_Material*>(MaterialDefinition);
	NonConstMaterialDefinition->Color = MatHelperMgn->MaterialAssetColor;
	
	IMaterialEditorModule& MatInterface = IMaterialEditorModule::Get();
	MaterialOpenHandle = MatInterface.OnMaterialEditorOpened().AddLambda([&](const TWeakPtr<IMaterialEditor>& InMatEditor)
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

	auto& SceneOutlinerModule = FModuleManager::LoadModuleChecked<FSceneOutlinerModule>("SceneOutliner");

	FSceneOutlinerColumnInfo SceneOutlinerColumnInfo(
		ESceneOutlinerColumnVisibility::Visible,
		1,
		FCreateSceneOutlinerColumn::CreateRaw(this,&FMatHelperModule::OnCreateOutlinerColumn)
		);
	SceneOutlinerModule.RegisterDefaultColumnType<FOuterlineSelectionLockCol>(SceneOutlinerColumnInfo);
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
			Niagara->GetNiagaraComponent()->ResetSystem();
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

void FMatHelperModule::InitialMaskOptions()
{
	MaskPinOptions.Add(MakeShareable(new FString("R")));
	MaskPinOptions.Add(MakeShareable(new FString("G")));
	MaskPinOptions.Add(MakeShareable(new FString("B")));
	MaskPinOptions.Add(MakeShareable(new FString("A")));
	MaskPinOptions.Add(MakeShareable(new FString("RGB")));
	MaskPinOptions.Add(MakeShareable(new FString("RGBA")));
	MaskPinOptions.Add(MakeShareable(new FString("RG")));
	MaskPinOptions.Add(MakeShareable(new FString("BA")));
	MaskPinOptions.Add(MakeShareable(new FString("RG - BA")));
	MaskPinOptions.Add(MakeShareable(new FString("ShowName")));
	MaskPinOptions.Add(MakeShareable(new FString("ClearAllPin")));
	MaskPinOptions.Add(MakeShareable(new FString("Only RG - BA")));
}

void FMatHelperModule::RegisterButton()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Tools");
		{
			FToolMenuSection& Section = Menu->AddSection("MatHelper", LOCTEXT("FSwitchCNModule", "MatHelper"));
			Section.AddEntry(FToolMenuEntry::InitMenuEntry(
				"MatHelper",
				LOCTEXT("FSwitchCNModule", "MatHelper"),
				LOCTEXT("FSwitchCNModule", "Open MatHelper Manager"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					AssetViewUtils::OpenEditorForAsset("/MatHelper/MatHelper.MatHelper");
				}))
			));
		}
	}
	
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
	
/*	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.StatusBar.ToolBar");

		FToolMenuSection& StallDetectorSection = Menu->AddSection(
			"Fuck", FText::GetEmpty(), FToolMenuInsert("Insights", EToolMenuInsertType::Before));
		
	
		StallDetectorSection.AddEntry(
			FToolMenuEntry::InitToolBarButton("Fuck",
				FToolUIActionChoice(FExecuteAction::CreateLambda([]()
				{
					AssetViewUtils::OpenEditorForAsset("/MatHelper/MatHelper.MatHelper");
				})),
				FText::FromString(""),
				FText::FromString(""),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon")
				)
				);
	}*/
	
	//InitToolBarButton(const FName InName,
	//const FToolUIActionChoice& InAction,
	//const TAttribute<FText>& InLabel = TAttribute<FText>(),
	//const TAttribute<FText>& InToolTip = TAttribute<FText>(),
	//const TAttribute<FSlateIcon>& InIcon = TAttribute<FSlateIcon>(),
	//const EUserInterfaceActionType UserInterfaceActionType = EUserInterfaceActionType::Button,
	//FName InTutorialHighlightName = NAME_None);
	
	/*FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenus* ToolMenus = UToolMenus::Get();
	
	UToolMenu* MyMenu = ToolMenus->RegisterMenu("MainFrame.MainMenu.MatHelper");
	FToolMenuSection& SectionMgn = MyMenu->AddSection("Manager");

	SectionMgn.AddEntry(FToolMenuEntry::InitMenuEntry("Manager",LOCTEXT("FAppLauncherModule", "Manager"),FText::FromString(""),FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FUIAction(FExecuteAction::CreateLambda([&]()
		{
			AssetViewUtils::OpenEditorForAsset("/MatHelper/MatHelper.MatHelper");
		}))
		));*/
}

// 原始函数 void FNiagaraSystemTrackEditor::AddDefaultSystemTracks(const AActor& SourceActor, const FGuid& Binding,	TSharedPtr<ISequencer> Sequencer) 
void FMatHelperModule::AddDefaultSystemTracks(const AActor& SourceActor, const FGuid& Binding,	TSharedPtr<ISequencer> Sequencer)
{
	ENiagaraAddDefaultsTrackMode TrackMode = GetDefault<UNiagaraEditorSettings>()->DefaultsSequencerSubtracks;
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
