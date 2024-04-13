// Copyright AKaKLya 2024

#pragma once
#include "CoreMinimal.h"
#include "Widgets/Layout/SScrollBox.h"
#include "SMaterialPalette.h"

struct FNodeButton;
class UMatHelperMgn;
class UMaterialGraphNode;
class FMatHelperModule;
class IMaterialEditor;



class SMatHelperWidget :public SMaterialPalette
{
public:

	SLATE_BEGIN_ARGS(SMatHelperWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs,FMaterialEditor* InMatEditor);
	FReply InitialButton();
	TSharedPtr<SGraphActionMenu> GraphActionMenu;
	
private:
	FMaterialEditor* MatEditorInterface = nullptr;
	UMaterial* Material = nullptr;
	
	TSharedPtr<SScrollBox> NodeButtonScrollBox;
	
	void RefreshMaskPinSelection();
	
	FString PluginConfigPath;
	inline bool CheckNode(UObject* Node);
	
	TSharedPtr<SEditableTextBox> GroupText;
	FReply SetNodeGroup(bool AutoGroup,bool AllGroup);
	
	//TSharedPtr<SEditableTextBox> MaskPinText;
	TArray<TSharedPtr<FString>> MaskPinOptions;
	TArray<FIntVector4> MaskPinInfo;
	FReply AddNodeMaskPin();
	int CurrentSelect = 0;

	TSharedPtr<SEditableTextBox> InstanceText;
	FReply CreateInstance();
	FString MIEmptyPath = "/MatHelper/Material/MI_Empty";
	
	FReply FixFunctionNode();
	FReply ToggleRefraction();
	TArray<TSharedPtr<SButton>> NodeButtons;
	
	FReply CreateMatNode(int32 Index);
	FReply RefreshButton();
	FReply RemoveParameterType();
	
protected:
	// SMaterialPalette Function  Begin
	virtual TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData) override;
	virtual void CollectAllActions(FGraphActionListBuilderBase& OutAllActions) override;
	virtual FReply OnActionDragged(const TArray< TSharedPtr<FEdGraphSchemaAction> >& InActions, const FPointerEvent& MouseEvent) override;
	
	void MHCategorySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void MHRefreshAssetInRegistry(const FAssetData& InAddedAssetData);
	FString MHGetFilterCategoryName() const;
	TSharedPtr<STextComboBox> CategoryComboBox;
	// SMaterialPalette Function End
};
