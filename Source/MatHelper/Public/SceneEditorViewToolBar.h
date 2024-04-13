// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "SceneEditorView.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SViewportToolBar.h"



class SceneEditorViewToolBar : public SCommonEditorViewportToolbarBase
{
public:
	SLATE_BEGIN_ARGS(SceneEditorViewToolBar)
		: _AddRealtimeButton(false)
		{}

		SLATE_ARGUMENT(bool, AddRealtimeButton)
	
	SLATE_END_ARGS()
	

	void Construct(const FArguments& InArgs, SceneEditorView* InSceneEditorView,TSharedPtr<class ICommonEditorViewportToolbarInfoProvider> InInfoProvider);
	virtual void ExtendLeftAlignedToolbarSlots(TSharedPtr<SHorizontalBox> MainBoxPtr, TSharedPtr<SViewportToolBar> ParentToolBarPtr) const override;
	
	virtual void ExtendOptionsMenu(FMenuBuilder& OptionsMenuBuilder) const override;

	mutable TArray<AActor*> CameraActors;
//	virtual TSharedRef<SWidget> GenerateOptionsMenu() const override;
	virtual TSharedRef<SWidget>	GenerateShowMenu() const override;
	SceneEditorView* SceneView;
};
