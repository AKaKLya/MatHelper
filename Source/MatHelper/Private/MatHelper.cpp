// Copyright AKaKLya 2024

#include "MatHelper.h"

#include "MatHelperWidget.h"
#include "Editor/MaterialEditor/Public/IMaterialEditor.h"
#include "Editor/MaterialEditor/Public/MaterialEditorModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FMatHelperModule"



void FMatHelperModule::StartupModule()
{
	
	PluginPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir();

	IMaterialEditorModule& MatInterface = IMaterialEditorModule::Get();
	MatInterface.OnMaterialEditorOpened().AddLambda([&](const TWeakPtr<IMaterialEditor>& InMatEditor)
	{
		IMaterialEditor* MatEditor = InMatEditor.Pin().Get();
		MatEditors.Add(InMatEditor);
		
		MatEditor->OnRegisterTabSpawners().AddLambda([&](const TSharedRef<class FTabManager>& TabManager)
		{
			const TSharedPtr<FWorkspaceItem> WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString("Material Editor"));
			TabManager->UnregisterTabSpawner("MatHelper");
		
			TabManager->RegisterTabSpawner("MatHelper",FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& Args)
			{
				TSharedRef<SMatHelperWidget> MhWidget = SNew(SMatHelperWidget);
				TSharedRef<SDockTab> Dock = SNew(SDockTab)
				[
					MhWidget
				];
				return Dock;
			}))
			.SetGroup(WorkspaceMenuCategory.ToSharedRef())
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Palette"));
		});

		MatEditor->OnMaterialEditorClosed().AddLambda([&]()
		{
			for(int i=0;i<MatEditors.Num();i++)
			{
				if( MatEditors[i].Pin().IsValid()==false )
				{
					MatEditors.RemoveAt(i);
				}
			}
		});
	});
	
	MaskPinOptions.Add(MakeShareable(new FString("R")));
	MaskPinOptions.Add(MakeShareable(new FString("G")));
	MaskPinOptions.Add(MakeShareable(new FString("B")));
	MaskPinOptions.Add(MakeShareable(new FString("A")));
	MaskPinOptions.Add(MakeShareable(new FString("RGB")));
	MaskPinOptions.Add(MakeShareable(new FString("RGBA")));
	MaskPinOptions.Add(MakeShareable(new FString("RG")));
	MaskPinOptions.Add(MakeShareable(new FString("BA")));
	MaskPinOptions.Add(MakeShareable(new FString("ShowName")));
	
}

void FMatHelperModule::ShutdownModule()
{

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




#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMatHelperModule, MatHelper)