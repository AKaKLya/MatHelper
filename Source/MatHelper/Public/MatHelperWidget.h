// Copyright AKaKLya 2024

#pragma once
#include "CoreMinimal.h"
#include "Widgets/Layout/SScrollBox.h"

class UMaterialGraphNode;
class FMatHelperModule;
class IMaterialEditor;

class SMatHelperWidget : public SScrollBox
{
public:

	SLATE_BEGIN_ARGS(SMatHelperWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs,UMaterial* InMaterial = nullptr);
	IMaterialEditor* MatEditorInterface = nullptr;
	UMaterial* Material = nullptr;
private:
	bool InitialMatEditorInterface();
	
	
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

	FReply ToggleRefraction();
	FReply FixFunctionNode();
	
	TArray<TSharedPtr<SButton>> NodeButtons;
	FReply InitialButton();
	FReply CreateMatNode(int32 Index);
	
};
