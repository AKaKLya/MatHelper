// Copyright AKaKLya 2024


#include "ButtonInfoEditor.h"

#include "MatHelper.h"
#include "MatHelperMgn.h"
#include "Interfaces/IPluginManager.h"
#include "Windows/WindowsPlatformApplicationMisc.h"


void SButtonInfoEditor::Construct(const FArguments& InArgs, TArray<FNodeButton> InButtonInfo)
{
	SAssignNew(ScrollBox, SScrollBox);
	SAssignNew(CurrentEditText, SEditableText)
	.Justification(ETextJustify::Center);
	CurrentEditText->SetFont(FSlateFontInfo(FPaths::EngineContentDir()  /  TEXT("Slate/Fonts/Roboto-Regular.ttf"),  12));
	
	SAssignNew(EditText, SMultiLineEditableTextBox);
	
	SAssignNew(SaveButton, SButton)
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	.Text(FText::FromString("Save to"))
	.IsEnabled(false)
	.OnClicked(this, &SButtonInfoEditor::OnConfirmClicked);

	SAssignNew(PasteSaveButton, SButton)
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	.Text(FText::FromString("Paste and Save to "))
	.IsEnabled(false)
	.OnClicked(this, &SButtonInfoEditor::PasteSave);

	CurrentEditText->SetEnabled(false);
	CurrentEditText->SetText(FText::FromString("Editing None"));

	EditText->SetText(FText::FromString("Select A Button Info . "));
	NodeConfigPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() + "/Config/AddNodeFile/";
	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.5)
		[
		
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(5.0)
			.AutoHeight()
			[
				CurrentEditText.ToSharedRef()
			]
			
			+ SVerticalBox::Slot()
			.Padding(5.0)
			.AutoHeight()
			[
				SaveButton.ToSharedRef()
			]
			
			+ SVerticalBox::Slot()
			.Padding(5.0)
			.AutoHeight()
			[
				PasteSaveButton.ToSharedRef()
			]

			/*+ SVerticalBox::Slot()
			.Padding(5.0)
			.AutoHeight()
			[
				SNew(SButton)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Refresh Mh Buttons"))
				.OnClicked_Lambda([]()
				{
					FMatHelperModule::RefreshAllWidgetButton();
					return FReply::Handled();
				})
			]*/
			
			+ SVerticalBox::Slot()
			.Padding(10.0)
			.AutoHeight()
			[
				ScrollBox.ToSharedRef()
			]
		]
		
		+ SHorizontalBox::Slot()
		.FillWidth(1.5)
		[
			EditText.ToSharedRef()
		]
	];

	InitialButtons(InButtonInfo);
}

FReply SButtonInfoEditor::OnButtonClicked(FString InTextName)
{
	CurrentEditText->SetText(FText::FromString("Editing " + InTextName));
	SaveButton->SetEnabled(true);
	PasteSaveButton->SetEnabled(true);
	CurrentEditFileName = NodeConfigPath + InTextName + ".txt";
	FString NodeText;
	FFileHelper::LoadFileToString(NodeText, *CurrentEditFileName);
	EditText->SetText(FText::FromString(NodeText));

	return FReply::Handled();
}

FReply SButtonInfoEditor::OnConfirmClicked()
{
	FFileHelper::SaveStringToFile(EditText->GetText().ToString(), *CurrentEditFileName);
	return FReply::Handled();
}

FReply SButtonInfoEditor::PasteSave()
{
	FString Content;
	FPlatformApplicationMisc::ClipboardPaste(Content);
	EditText->SetText(FText::FromString(Content));
	FFileHelper::SaveStringToFile(EditText->GetText().ToString(), *CurrentEditFileName);
	return FReply::Handled();
}

void SButtonInfoEditor::InitialButtons(TArray<FNodeButton> InButtonInfo)
{
	for (auto ButtonInfo : InButtonInfo)
	{
		if(ButtonInfo.ButtonName == "None")
		{
			continue;
		}
		ScrollBox->AddSlot()
		.Padding(3.0)
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Text(FText::FromString(ButtonInfo.ButtonName))
			.OnClicked_Raw(this, &SButtonInfoEditor::OnButtonClicked, ButtonInfo.ButtonName)
		];
	}
}
