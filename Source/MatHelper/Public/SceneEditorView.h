// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"

/**
 * 
 */

class MATHELPER_API SceneEditorView : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SceneEditorView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	SceneEditorView();
	~SceneEditorView();
	virtual void OnFloatingButtonClicked() override;
	void UpdateViewPortLocation();
	
protected:
	/** SEditorViewport interface */
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
	
	// ICommonEditorViewportToolbarInfoProvider interface
	virtual TSharedRef<class SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;

	// End of ICommonEditorViewportToolbarInfoProvider interface
	
};
