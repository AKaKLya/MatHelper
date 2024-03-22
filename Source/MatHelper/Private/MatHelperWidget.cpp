// Copyright AKaKLya 2024

#include "MatHelperWidget.h"

#include "AssetViewUtils.h"
#include "TAccessPrivate.inl"
#include "IContentBrowserSingleton.h"
#include "MaterialGraphNode_Knot.h"
#include "MatHelper.h"
#include "MatHelperMgn.h"

#include "Editor/MaterialEditor/Public/IMaterialEditor.h"
#include "Kismet/KismetMathLibrary.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Comment.h"
#include "MaterialGraph/MaterialGraphNode_Composite.h"
#include "MaterialGraph/MaterialGraphNode_PinBase.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Windows/WindowsPlatformApplicationMisc.h"

namespace MatHelperWidget
{
	FMatHelperModule MatHelper;
}
using namespace MatHelperWidget;

void SMatHelperWidget::Construct(const FArguments& InArgs,IMaterialEditor* InMatEditor)
{
	SScrollBox::Construct(SScrollBox::FArguments());
	MatEditorInterface = InMatEditor;
	
	MatHelper = FModuleManager::Get().GetModuleChecked<FMatHelperModule>("MatHelper");
	PluginConfigPath = MatHelper.GetPluginPath().Append("/Config/");
	
	
	SAssignNew(GroupText,SEditableTextBox);
	SAssignNew(InstanceText,SEditableTextBox);

	AddSlot()
	.Padding(5.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("MatHelper Manager"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Lambda([]()
		{
			AssetViewUtils::OpenEditorForAsset("/MatHelper/MatHelper.MatHelper");
			return FReply::Handled();
		})
	];
	
	
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
		.OnClicked_Raw(this,&SMatHelperWidget::SetNodeGroup,false)
	];

	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Auto Group"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::SetNodeGroup,true)
	];
	

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
		.OnClicked_Raw(this,&SMatHelperWidget::AddNodeMaskPin)
	];

	
	
	AddSlot()
	.Padding(5.0f)
	[
		InstanceText.ToSharedRef()
	];
	
	
	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Create Instance"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::CreateInstance)
	];

	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Refraction"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::ToggleRefraction)
	];

	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Fix Function Node"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::FixFunctionNode)
	];
	
	
	AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Refresh Button"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::RefreshButton)
	];

	
	InitialButton();
}

bool SMatHelperWidget::InitialMatEditorInterface()
{
	if(MatEditorInterface != nullptr)
	{
		return true;
	}
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
	
	TArray<FString> Names = MatHelper.MatHelperMgn->AutoGroupKeys;
	
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
			AddMaskPin(MatNode,"RG",FIntVector4(1,1,0,0),IsAddSuccess);
			AddMaskPin(MatNode,"BA",FIntVector4(0,0,1,1),IsAddSuccess);
			break;
		}
	case 9:
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

inline void SMatHelperWidget::AddMaskPin(const UMaterialGraphNode* MatNode, const FString& Name, const FIntVector4& Mask,bool& Out_IsAddSuccess)
{
	const TObjectPtr<UMaterialExpression> Expression = MatNode->MaterialExpression;
	TArray<FExpressionOutput>& Outputs = Expression->Outputs;

	bool Found = false;
	int Index = -1;
	for(const auto& Output : Outputs)
	{
		FIntVector4 PinMask = FIntVector4(Output.MaskR,Output.MaskG,Output.MaskB,Output.MaskA);
		Index = Index + 1;
		if(PinMask == Mask)
		{
			Found = true;
			break;
		}
	}
	
	if(!Found)
	{
		Outputs.Add(FExpressionOutput(FName(*Name), 1, Mask.X, Mask.Y, Mask.Z, Mask.W));
	}
	else
	{
		Outputs.RemoveAt(Index);
	}
	Out_IsAddSuccess = true;
}

FReply SMatHelperWidget::CreateInstance()
{
	if( InitialMatEditorInterface() == false || Material == nullptr)
	{
		return FReply::Handled();
	}
	FString Target = Material->GetPathName();
	const FString BaseName = Material->GetName();
	Target.ReplaceInline(*BaseName,*FString(""));
	Target.ReplaceInline(*FString("."),*FString(""));

	FString NewPath = BaseName;
	if(BaseName.Left(2) == "M_")
	{
		NewPath.ReplaceInline(*FString("M_"),*FString("MI_"));
	}
	else
	{
		NewPath = "MI_" + BaseName;
	}
	FString InText = InstanceText->GetText().ToString();

	

	if(InText == "")
	{
		InText = "Inst" + FString::FromInt(UKismetMathLibrary::RandomIntegerInRange(0,99));
	}
	
	NewPath = Target + NewPath + "_" + InText;
	
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if(EditorAssetSubsystem->DoesAssetExist(NewPath))
	{
		MatHelper.EditorNotify("Create Fail - This Instance Exists",SNotificationItem::CS_Fail);
		return FReply::Handled();
	}
	UMaterialInstance* NewMi = Cast<UMaterialInstance>(EditorAssetSubsystem->DuplicateAsset(MIEmptyPath, NewPath));
	NewMi->Parent=Material;
	
	TArray<UObject*> AssetList;
	AssetList.Add(NewMi);
	IContentBrowserSingleton::Get().SyncBrowserToAssets(AssetList);
	AssetViewUtils::OpenEditorForAsset(NewMi);
	return FReply::Handled();
}

FReply SMatHelperWidget::ToggleRefraction()
{
	if(InitialMatEditorInterface() == false)
	{
		return FReply::Handled();
	}
	auto& Ref = MatEditorInterface->GetMaterialInterface()->GetMaterial()->RefractionMethod;
	if (Ref == RM_None)
	{
		Ref = RM_IndexOfRefraction;
	}
	else
	{
		Ref = RM_None;
	}
	MatEditorInterface->UpdateMaterialAfterGraphChange();
	return FReply::Handled();
}

FReply SMatHelperWidget::FixFunctionNode()
{
		
	if(InitialMatEditorInterface() == false)
	{
		return FReply::Handled();
	}
	bool bNeedRefresh = false;
	auto Nodes = MatEditorInterface->GetSelectedNodes().Array();
	for(const auto Node : Nodes)
	{
		UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
		if(Cast<UMaterialExpressionMaterialFunctionCall>(MatNode->MaterialExpression))
		{
			MatNode->RecreateAndLinkNode();
			bNeedRefresh = true;
		}
	}
	if(bNeedRefresh)
	{
		MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
		
}

FReply SMatHelperWidget::InitialButton()
{
	
	for(auto Button : NodeButtons)
	{
		RemoveSlot(Button.ToSharedRef());
	}
	NodeButtons.Empty();
	
	int32 Num = MatHelper.MatHelperMgn->NodeButtonInfo.Num();
	
	for(int i = 0 ; i < Num ; i++)
	{
		FNodeButton ButtonInfo = MatHelper.MatHelperMgn->NodeButtonInfo[i];
		TSharedPtr<SButton> Button = SNew(SButton)
		.Text(FText::FromString( ButtonInfo.ButtonName))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this,&SMatHelperWidget::CreateMatNode,ButtonInfo));
		
		AddSlot()
		.Padding(3.0f)
		[
			Button.ToSharedRef()
		];
		
		NodeButtons.Add(Button);
	}
	return FReply::Handled();
}

FReply SMatHelperWidget::CreateMatNode(FNodeButton ButtonInfo)
{
	
	const FString NodeFileName = PluginConfigPath + "AddNodeFile/" + ButtonInfo.ButtonName + ".txt";
	FString NodeText;
	FFileHelper::LoadFileToString(NodeText,*NodeFileName);
	FPlatformApplicationMisc::ClipboardCopy(*NodeText);
	
	if( InitialMatEditorInterface() == false )
	{
		return FReply::Handled();
	}

	
	MatEditorInterface->FocusWindow();
	auto SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
	
	FVector2D RootOffset;
	if(ButtonInfo.RootOffsetOverride)
	{
		RootOffset = ButtonInfo.RootOffset;
	}
	else
	{
		RootOffset = MatHelper.MatHelperMgn->RootOffset;
	}
	
	FVector2D BaseOffset = MatHelper.MatHelperMgn->BaseOffset;
	
	
	if(SelectedNodes.Num() > 0)
	{
		UObject* SelectedNode = SelectedNodes[0];
		if(const auto RootNode = Cast<UMaterialGraphNode_Root>(SelectedNode))
		{
			const FVector2D Location = FVector2D(RootNode->NodePosX + RootOffset.X,RootNode->NodePosY + RootOffset.Y);
			MatEditorInterface->PasteNodesHere(Location);
		}
		else if(const auto BaseNode = Cast<UMaterialGraphNode>(SelectedNode))
		{
			const FVector2D Location = FVector2D(BaseNode->NodePosX + BaseOffset.X,BaseNode->NodePosY + BaseOffset.Y);
			MatEditorInterface->PasteNodesHere(Location);
		}
	}
	else
	{
		auto BaseRootNode =  MatEditorInterface->GetMaterialInterface()->GetMaterial()->MaterialGraph->RootNode;
		MatEditorInterface->PasteNodesHere(FVector2D(BaseRootNode->NodePosX + RootOffset.X,BaseRootNode->NodePosY + RootOffset.Y));
		MatEditorInterface->JumpToExpression(Cast<UMaterialGraphNode>(MatEditorInterface->GetSelectedNodes().Array()[0])->MaterialExpression);
	}
	return FReply::Handled();
}

FReply SMatHelperWidget::RefreshButton()
{
	
	InitialButton();
	return FReply::Handled();
}


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
