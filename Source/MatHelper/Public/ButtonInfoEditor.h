// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

struct FNodeButton;

class SButtonInfoEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SButtonInfoEditor) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs,TArray<FNodeButton> InButtonInfo);

	FString NodeConfigPath;
	FString CurrentEditFileName;
	FString SaveButtonText;
	
	TSharedPtr<SScrollBox> ScrollBox;
	TSharedPtr<SEditableText> CurrentEditText;
	TSharedPtr<SMultiLineEditableTextBox> EditText;
	TSharedPtr<SButton> SaveButton;
	TSharedPtr<SButton> PasteSaveButton;
	
	FReply OnButtonClicked(FString InTextName);
	FReply OnConfirmClicked();
	FReply PasteSave();
	
	void InitialButtons(TArray<FNodeButton> InButtonInfo);
	
};
