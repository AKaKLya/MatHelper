// Copyright AKaKLya 2024


#include "MatHelperWidget.h"

#include "GraphActionNode.h"
#include "MaterialGraphNode_Knot.h"
#include "MatHelper.h"
#include "Editor/MaterialEditor/Public/IMaterialEditor.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Comment.h"
#include "MaterialGraph/MaterialGraphNode_Composite.h"
#include "MaterialGraph/MaterialGraphNode_PinBase.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Windows/WindowsPlatformApplicationMisc.h"

namespace MatHelperWidget
{
	static FMatHelperModule MatHelper;
}

using namespace MatHelperWidget;


void SMatHelperWidget::Construct(const FArguments& InArgs)
{
	SScrollBox::Construct(SScrollBox::FArguments());
	
	
	MatHelper = FModuleManager::Get().GetModuleChecked<FMatHelperModule>("MatHelper");
	PluginConfigPath = MatHelper.GetPluginPath().Append("/Config/");
	
	
	SAssignNew(GroupText,SEditableTextBox);
//	SAssignNew(MaskPinText,SEditableTextBox);
	
	AddSlot()
	.Padding(5.0f)
	[
		GroupText.ToSharedRef()	
	];
	
	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Set Group"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this,&SMatHelperWidget::SetNodeGroup,false))
	];

	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Auto Group"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this,&SMatHelperWidget::SetNodeGroup,true))
	];


	
/*	AddSlot()
	.Padding(5.0f)
	[
		MaskPinText.ToSharedRef()	
	];*/

	AddSlot()
	.Padding(5.0f)
	[
		SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&MatHelper.MaskPinOptions)
		.OnGenerateWidget_Lambda([](const TSharedPtr<FString>& InString)
		{
			return SNew(STextBlock)
			.Text(FText::FromString(*InString));
		})
		.OnSelectionChanged_Lambda([&](const TSharedPtr<FString>& NewOption,ESelectInfo::Type SelectInfo)
		{
			CurrentSelect = MatHelper.MaskPinOptions.Find(NewOption);
		})
		[
			SNew(STextBlock)
			.Text_Lambda([&]()
			{
				return FText::FromString(*MatHelper.MaskPinOptions[CurrentSelect]);
			})
		]
	];

	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Add Mask Pin"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this,&SMatHelperWidget::AddNodeMaskPin))
	];

	

	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Open Config Folder"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateLambda([&]()
		{
			FString Path = PluginConfigPath;
			Path.ReplaceCharInline('/','\\');
			FWindowsPlatformProcess::CreateProc(L"explorer.exe",*Path,false,false,false,nullptr,0,nullptr,nullptr);
			return FReply::Handled();
		}))
	];


	
	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Refresh Button"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this,&SMatHelperWidget::InitialButton))
	];

	
	InitialButton();
}

bool SMatHelperWidget::InitialMatEditorInterface()
{
	TArray<TWeakPtr<IMaterialEditor>> MatEditors = MatHelper.GetMatEditors();
	TArray<IMaterialEditor*> HasSelectNodesMatEditor;
	for(TWeakPtr<IMaterialEditor> MatEditor : MatEditors)
	{
		if(MatEditor.Pin().IsValid())
		{
			if( MatEditor.Pin().Get()->GetNumberOfSelectedNodes() > 0)
			{
				HasSelectNodesMatEditor.Add(MatEditor.Pin().Get());
			}
		}
	}

	if( HasSelectNodesMatEditor.Num() < 1)
	{
		return false;
	}

	if( HasSelectNodesMatEditor.Num() > 1)
	{
		MatHelper.EditorNotify("Error - The Selected Nodes Maybe From Different Material Editor",SNotificationItem::CS_Fail);
		return false;
	}
	MatEditorInterface = HasSelectNodesMatEditor[0];
	return true;
}

FReply SMatHelperWidget::SetNodeGroup(bool AutoGroup)
{
	bool ShouldRefresh = false;
	
	if( InitialMatEditorInterface() == false )
	{
		return FReply::Handled();
	}
	
	int32 NodeNum = MatEditorInterface->GetNumberOfSelectedNodes();
	
	TArray<UObject*> SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
	MatEditorInterface->FocusWindow();
	
	for(UObject* Node : SelectedNodes)
	{
		if(UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node))
		{

			if(CheckNode(MatNode) == false)
			{
				continue;
			}
			
			if(UMaterialExpressionParameter* Parameter = Cast<UMaterialExpressionParameter>(MatNode->MaterialExpression))
			{
				if(AutoGroup)
				{
					FString AutoFile = PluginConfigPath + "AutoGroup.ini";
					TArray<FString> Names;
					FFileHelper::LoadFileToStringArray(Names,*AutoFile);
					for(FString Name : Names)
					{
						if(Parameter->ParameterName.ToString().Contains(Name))
						{
							Parameter->Group = *Name;
						}
					}
				}
				else
				{
					Parameter->Group = *GroupText->GetText().ToString();
				}
				ShouldRefresh=true;
				
			}
			else if(UMaterialExpressionTextureSampleParameter* TexParameter = Cast<UMaterialExpressionTextureSampleParameter>(MatNode->MaterialExpression))
			{
				if(AutoGroup)
				{
					FString AutoFile = PluginConfigPath + "AutoGroup.ini";
					TArray<FString> Names;
					FFileHelper::LoadFileToStringArray(Names,*AutoFile);
					for(FString Name : Names)
					{
						if(TexParameter->ParameterName.ToString().Contains(Name))
						{
							TexParameter->Group = *Name;
						}
					}
				}
				else
				{
					TexParameter->Group = *GroupText->GetText().ToString();
				}
				ShouldRefresh=true;
				
			}
		}
	}
	
	if(ShouldRefresh)
	{
		MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
}

FReply SMatHelperWidget::AddNodeMaskPin()
{
//	int32 MaskIndex = FCString::Atoi(*MaskPinText->GetText().ToString());
	
	if( InitialMatEditorInterface() == false )
	{
		return FReply::Handled();
	}
	
	MatEditorInterface->FocusWindow();
	TArray<UObject*> SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
	if(SelectedNodes.Num() == 0)
	{
		return FReply::Handled();
	}
	
	if(CheckNode(SelectedNodes[0]) == false)
	{
		return FReply::Handled();
	}

	UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(SelectedNodes[0]);
	
	bool IsAddSuccess = false;
	switch (CurrentSelect)
	{
	case 0 :
		{
			AddMaskPin(MatNode,"R",FIntVector4(1,0,0,0),IsAddSuccess);
			break;
		}
	case 1 :
		{
			AddMaskPin(MatNode,"G",FIntVector4(0,1,0,0),IsAddSuccess);
			break;
		}
	case 2:
		{
			AddMaskPin(MatNode,"B",FIntVector4(0,0,1,0),IsAddSuccess);
			break;
		}
	case 3:
		{
			AddMaskPin(MatNode,"A",FIntVector4(0,0,0,1),IsAddSuccess);
			break;
		}
	case 4:
		{
			AddMaskPin(MatNode,"RGB",FIntVector4(1,1,1,0),IsAddSuccess);
			break;
		}
	case 5:
		{
			AddMaskPin(MatNode,"RGBA",FIntVector4(1,1,1,1),IsAddSuccess);
			break;
		}
	case 6:
		{
			AddMaskPin(MatNode,"RG",FIntVector4(1,1,0,0),IsAddSuccess);
			break;
		}
	case 7:
		{
			AddMaskPin(MatNode,"BA",FIntVector4(0,0,1,1),IsAddSuccess);
			break;
		}
	case 8:
		{
			MatNode->MaterialExpression->bShowOutputNameOnPin = !MatNode->MaterialExpression->bShowOutputNameOnPin;
			IsAddSuccess = true;
			break;
		}
	default: break;
	}
	
	if(IsAddSuccess)
	{
		MatEditorInterface->FocusWindow();
		MatNode->RecreateAndLinkNode();
		MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
	
	
	return FReply::Handled();
}

inline void SMatHelperWidget::AddMaskPin(UMaterialGraphNode* MatNode, const FString& Name, const FIntVector4& Mask,bool& Out_IsAddSuccess)
{
	bool HasPin = false;
	TArray<FExpressionOutput>& Outputs = MatNode->MaterialExpression->Outputs;
	for(FExpressionOutput& Output : Outputs)
	{
		if(Output.OutputName.ToString() == Name)
		{
			HasPin = true;
		}
	}
	if(!HasPin)
	{
		Outputs.Add(FExpressionOutput(FName(*Name), 1, Mask.X, Mask.Y, Mask.Z, Mask.W));
		Out_IsAddSuccess = true;
	}
}

FReply SMatHelperWidget::InitialButton()
{
	
	for(auto Button : NodeButtons)
	{
		RemoveSlot(Button.ToSharedRef());
	}
	NodeButtons.Empty();
	
	int32 Num = 0;
	FString FileName = PluginConfigPath+"AddNodeInfo.ini";
	GConfig->LoadFile(FileName);
	GConfig->GetInt(L"mgn",L"Num",Num,FileName);
	for(int i = 0 ; i < Num ; i++)
	{
		FString ButtonName;
		FString Key = FString::Printf(TEXT("%d"),i+1);
		GConfig->GetString(L"name",*Key,ButtonName,FileName);
		ButtonName = ButtonName.Len() == 0 ? "None" : ButtonName;
		TSharedPtr<SButton> Button = SNew(SButton)
		.Text(FText::FromString(ButtonName))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this,&SMatHelperWidget::CreateMatNode,i+1));

		AddSlot()
		.Padding(3.0f)
		[
			Button.ToSharedRef()
		];
		NodeButtons.Add(Button);
	}
	return FReply::Handled();
}

FReply SMatHelperWidget::CreateMatNode(int32 index)
{
	FString ConfigFileName = PluginConfigPath + "AddNodeInfo.ini";
	FString NodeFileName = PluginConfigPath + "AddNodeFile/" + FString::FromInt(index) + ".txt";
	FString NodeText;
	FFileHelper::LoadFileToString(NodeText,*NodeFileName);
	FPlatformApplicationMisc::ClipboardCopy(*NodeText);
	
	if( InitialMatEditorInterface() == false )
	{
		return FReply::Handled();
	}
	
	MatEditorInterface->FocusWindow();
	IMaterialEditor* IMatEditorPtr = MatEditorInterface;
	
	UObject* SelectedNode = IMatEditorPtr->GetSelectedNodes().Array()[0];
	
	GConfig->LoadFile(ConfigFileName);
	
	FIntVector2 RootOffset = FIntVector2(100,800);
	GConfig->GetInt(L"mgn",L"RootoffsetX",RootOffset.X,*ConfigFileName);
	GConfig->GetInt(L"mgn",L"RootoffsetY",RootOffset.Y,*ConfigFileName);

	FIntVector2 BaseOffset = FIntVector2(50,50);
	GConfig->GetInt(L"mgn",L"BaseoffsetX",BaseOffset.X,*ConfigFileName);
	GConfig->GetInt(L"mgn",L"BaseoffsetY",BaseOffset.Y,*ConfigFileName);
	

	if(auto RootNode = Cast<UMaterialGraphNode_Root>(SelectedNode))
	{
		FVector2D Location = FVector2D(RootNode->NodePosX + RootOffset.X,RootNode->NodePosY + RootOffset.Y);
		IMatEditorPtr->PasteNodesHere(Location);
	}
	else if(auto BaseNode = Cast<UMaterialGraphNode>(SelectedNode))
	{
		FVector2D Location = FVector2D(BaseNode->NodePosX + BaseOffset.X,BaseNode->NodePosY + BaseOffset.Y);
		IMatEditorPtr->PasteNodesHere(Location);
	}
	
	return FReply::Handled();
};


inline bool SMatHelperWidget::CheckNode(UObject* Node)
{
	bool CheckSuccess = true;
	if (Cast<UMaterialGraphNode_Comment>(Node)) { CheckSuccess = false; }
	if (Cast<UMaterialGraphNode_Root>(Node)) { CheckSuccess = false; }
	if (Cast<UMaterialGraphNode_Composite>(Node)) { CheckSuccess = false; }
	if (Cast<UMaterialGraphNode_Knot>(Node)) { CheckSuccess = false; }
	if (Cast<UMaterialGraphNode_PinBase>(Node)) { CheckSuccess = false; }
	return CheckSuccess;
}