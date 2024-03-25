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
	
	FString PluginConfigPath;
	inline bool CheckNode(UObject* Node);
	
	TSharedPtr<SEditableTextBox> GroupText;
	FReply SetNodeGroup(bool AutoGroup);
	
	//TSharedPtr<SEditableTextBox> MaskPinText;
	FReply AddNodeMaskPin();
	inline void AddMaskPin(const UMaterialGraphNode* MatNode, const FString& Name, const FIntVector4& Mask, bool& Out_IsAddSuccess);
	int CurrentSelect = 0;

	TSharedPtr<SEditableTextBox> InstanceText;
	FReply CreateInstance();
	FString MIEmptyPath = "/MatHelper/Material/MI_Empty";
	
	FReply FixFunctionNode();
	FReply ToggleRefraction();
	TArray<TSharedPtr<SButton>> NodeButtons;
	
	FReply CreateMatNode(int32 Index);
	FReply RefreshButton();

	
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
