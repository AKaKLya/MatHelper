// Copyright AKaKLya

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

	void Construct(const FArguments& InArgs);
	
private:
	IMaterialEditor* MatEditorInterface = nullptr;
	bool InitialMatEditorInterface();
	
	FString PluginConfigPath;
	inline bool CheckNode(UObject* Node);
	
	TSharedPtr<SEditableTextBox> GroupText;
	FReply SetNodeGroup(bool AutoGroup);
	
	TSharedPtr<SEditableTextBox> MaskPinText;
	FReply AddNodeMaskPin();
	inline void AddMaskPin(UMaterialGraphNode* MatNode, const FString& Name, const FIntVector4& Mask,bool& Out_IsAddSuccess);
	
	TArray<TSharedPtr<SButton>> NodeButtons;
	FReply InitialButton();
	FReply CreateMatNode(int32 index);

	
};
