// Copyright AKaKLya 2024

#pragma once
#include "CoreMinimal.h"

struct FNodeButton;
class UMatHelperMgn;
class UMaterialGraphNode;
class FMatHelperModule;
class IMaterialEditor;

class SMatHelperWidget :public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMatHelperWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs,FMaterialEditor* InMatEditor);
	FReply InitialButton();
	
private:
	FMaterialEditor* MaterialEditor = nullptr;
	UMaterial* Material = nullptr;
	
	TSharedPtr<SScrollBox> NodeButtonScrollBox;
	
	void RefreshMaskPinSelection();
	
	FString PluginConfigPath;
	static inline bool CheckNode(UObject* Node);
	
	TSharedPtr<SEditableTextBox> GroupText;
	FReply SetNodeGroup(bool AutoGroup,bool AllGroup) const;
	
	//TSharedPtr<SEditableTextBox> MaskPinText;
	TArray<TSharedPtr<FString>> MaskPinOptions;
	TArray<FIntVector4> MaskPinInfo;
	FReply AddNodeMaskPin();
	int CurrentSelect = 0;

	TSharedPtr<SEditableTextBox> InstanceText;
	FReply CreateInstance();
	FString MIEmptyPath = "/MatHelper/Material/MI_Empty";
	
	FReply FixFunctionNode() const;
	FReply ToggleRefraction() const;
	TArray<TSharedPtr<SButton>> NodeButtons;
	
	FReply CreateMatNode(int32 Index) const;
	FReply RefreshButton();
	FReply RemoveParameterType() const;
	
};
