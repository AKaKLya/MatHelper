// Copyright AKaKLya 2024

#include "MatHelper.h"
#include "TAccessPrivate.inl"
#include "AssetDefinitionRegistry.h"
#include "ButtonInfoEditor.h"
#include "MatHelperMgn.h"
#include "MatHelperWidget.h"
#include "Editor/MaterialEditor/Public/IMaterialEditor.h"
#include "Editor/MaterialEditor/Public/MaterialEditorModule.h"
#include "EngineClass/CusAssetDefinition_Material.h"
#include "Editor/MaterialEditor/Private/MaterialEditor.h"
#include "Editor/MaterialEditor/Private/SMaterialPalette.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/SCompoundWidget.h"
#define LOCTEXT_NAMESPACE "FMatHelperModule"

namespace MatHelperSpace
{
	TArray<TWeakPtr<SMatHelperWidget>> MhWidgets;
}
using namespace MatHelperSpace;

FMatHelperModule& FMatHelperModule::Get()
{
	return FModuleManager::Get().GetModuleChecked<FMatHelperModule>("MatHelper");
}

struct AccessPalatte
{
	typedef TSharedPtr<class SMaterialPalette> (FMaterialEditor::*Type);
};

template struct TAccessPrivateStub<AccessPalatte,&FMaterialEditor::Palette>;


PROTECTED_MEMBER_ACCESS_FUNCTION_DEFINE(SMaterialPalette,MaterialEditorPtr)

void FMatHelperModule::StartupModule()
{
	InitialMaskOptions();
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ButtonInfoEditorTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnButtonInfoEditor))
		.SetDisplayName(FText::FromString("ButtonInfoEditor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	
	PluginPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir();
	MatHelperMgn = LoadObject<UMatHelperMgn>(nullptr,TEXT("/MatHelper/MatHelper.MatHelper"));
	
	const UCusAssetDefinition_Material* MaterialDefinition = Cast<UCusAssetDefinition_Material>(UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(UMaterial::StaticClass()));
	UCusAssetDefinition_Material* NonConstMaterialDefinition = const_cast<UCusAssetDefinition_Material*>(MaterialDefinition);
	NonConstMaterialDefinition->Color = MatHelperMgn->MaterialAssetColor;
	
	IMaterialEditorModule& MatInterface = IMaterialEditorModule::Get();
	MatInterface.OnMaterialEditorOpened().AddLambda([&](const TWeakPtr<IMaterialEditor>& InMatEditor)
	{
		IMaterialEditor* IMatEditor = InMatEditor.Pin().Get();
		FMaterialEditor* MatEditor = static_cast<FMaterialEditor*>(IMatEditor);
		TSharedPtr<SMaterialPalette>& Palette = MatEditor->*TAccessPrivate<AccessPalatte>::Value;
		
		
		MatEditors.Add(InMatEditor);
		
		IMatEditor->OnRegisterTabSpawners().AddLambda([&](const TSharedRef<class FTabManager>& TabManager)
		{
			const TSharedPtr<FWorkspaceItem> WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString("Material Editor"));
			auto MhWidget = SNew(SMatHelperWidget,MatEditor);
			MhWidgets.Add(MhWidget);
			Palette =  MhWidget;
		});
	});
}

void FMatHelperModule::ShutdownModule()
{

}

void FMatHelperModule::CreateMat(UMaterial* InMaterial, TWeakPtr<IMaterialEditor> InMatEditor)
{
	/*for(auto MhWidget : MhWidgets)
	{
		if(MhWidget.Pin().IsValid())
		{
			SMatHelperWidget* MhWdiget = MhWidget.Pin().Get();
			if(MhWdiget->MatEditorInterface == InMatEditor.Pin().Get())
			{
				MhWdiget->Material = InMaterial;
				break;
			}
		}
	}*/
}

void FMatHelperModule::EditorNotify(const FString& NotifyInfo, SNotificationItem::ECompletionState State)
{
	FNotificationInfo Info( FText::FromString(NotifyInfo) );
	Info.FadeInDuration = 0.5f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 5.0f;
	auto NotificationItem = FSlateNotificationManager::Get().AddNotification( Info );
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
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMatHelperModule, MatHelper)

/*
IMaterialEditor* IMatEditor = InMatEditor.Pin().Get();
FMaterialEditor* MatEditor = static_cast<FMaterialEditor*>(IMatEditor);
TSharedPtr<SMaterialPalette> Palette = MatEditor->*TAccessPrivate<AccessPalatte>::Value;
	
MatEditors.Add(InMatEditor);
IMatEditor->OnRegisterTabSpawners().AddLambda([&](const TSharedRef<class FTabManager>& TabManager)
{
const TSharedPtr<FWorkspaceItem> WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString("Material Editor"));
TabManager->UnregisterTabSpawner("MatHelper");
		
TabManager->RegisterTabSpawner("MatHelper",FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& Args)
{
TSharedRef<SMatHelperWidget> MhWidget = SNew(SMatHelperWidget,MatEditor);
MhWidget->Material = Cast<UMaterial>(MatEditor->OriginalMaterialObject);
				
MhWidgets.Add(MhWidget);
TSharedRef<SDockTab> Dock = SNew(SDockTab)
[
MhWidget
];
return Dock;
}))
.SetGroup(WorkspaceMenuCategory.ToSharedRef())
.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Palette"));*/