// Copyright AKaKLya 2024


#include "SceneEditorViewToolBar.h"

#include "MatHelper.h"
#include "SAssetEditorViewport.h"
#include "SceneEditorView.h"
#include "TAccessPrivate.inl"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"


#define LOCTEXT_NAMESPACE "LevelViewportToolBar"

void SceneEditorViewToolBar::Construct(const FArguments& InArgs,SceneEditorView* InSceneEditorView,TSharedPtr<class ICommonEditorViewportToolbarInfoProvider> InInfoProvider)
{
	SceneView = InSceneEditorView;
	SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments(),InInfoProvider);
}



void SceneEditorViewToolBar::ExtendLeftAlignedToolbarSlots(TSharedPtr<SHorizontalBox> MainBoxPtr,TSharedPtr<SViewportToolBar> ParentToolBarPtr) const
{
	MainBoxPtr->AddSlot()
	.AutoWidth()
	[
		SNew(SButton)
		.Text(FText::FromString("FX"))
		.ButtonColorAndOpacity(FSlateColor(FLinearColor(1,1,1,0.45)))
		.OnClicked_Lambda([]()
		{
			FMatHelperModule::PlayNiagaraOnEditorWorld();
			return FReply::Handled();
		})
	];
}



TSharedRef<SWidget> SceneEditorViewToolBar::GenerateShowMenu() const
{
	GetInfoProvider().OnFloatingButtonClicked();
	TSharedRef<SEditorViewport> ViewportRef = GetInfoProvider().GetViewportWidget();

	UWorld* World = GEditor->GetEditorWorldContext().World();
	UGameplayStatics::GetAllActorsOfClass(World,ACameraActor::StaticClass(),CameraActors);
	
	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder ShowMenuBuilder(bInShouldCloseWindowAfterMenuSelection, ViewportRef->GetCommandList());
	{
		
		ShowMenuBuilder.BeginSection("ShowTool");
		{
			ShowMenuBuilder.AddMenuEntry(FText::FromString("Reset View"),FText::GetEmpty(),
				FSlateIcon(),FUIAction(FExecuteAction::CreateLambda([&]()
				{
					SceneView->UpdateViewPortLocation();
				})));
			
			
			
			for(auto& CameraActor : CameraActors)
			{
				ShowMenuBuilder.AddMenuEntry(FText::FromString(CameraActor->GetActorLabel()),FText::GetEmpty(),
				FSlateIcon(),FUIAction(FExecuteAction::CreateLambda([&]()
				{
					SceneView->GetViewportClient()->SetViewLocation(CameraActor->GetActorLocation());
					SceneView->GetViewportClient()->SetViewRotation(CameraActor->GetActorRotation());
				})));
			}
			
		}
		ShowMenuBuilder.EndSection();
	}

	return ShowMenuBuilder.MakeWidget();
}


void SceneEditorViewToolBar::ExtendOptionsMenu(FMenuBuilder& OptionsMenuBuilder) const
{
	
}

#undef LOCTEXT_NAMESPACE