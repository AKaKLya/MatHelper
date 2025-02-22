// Copyright AKaKLya 2024

#include "MatHelperWidget.h"
#include "TAccessPrivate.inl"
#include "AssetViewUtils.h"
#include "EditorWidgetsModule.h"
#include "IContentBrowserSingleton.h"
#include "MaterialGraphNode_Knot.h"
#include "MaterialPropertyHelpers.h"
#include "MatHelper.h"
#include "MatHelperMgn.h"
#include "Editor/MaterialEditor/Private/MaterialEditor.h"
#include "Kismet/KismetMathLibrary.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Comment.h"
#include "MaterialGraph/MaterialGraphNode_Composite.h"
#include "MaterialGraph/MaterialGraphNode_PinBase.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Windows/WindowsPlatformApplicationMisc.h"



#define LOCTEXT_NAMESPACE "MaterialPalette"

struct AccessGraph
{
	typedef TWeakPtr<class SGraphEditor> (FMaterialEditor::*Type);
};

template struct TAccessPrivateStub<AccessGraph,&FMaterialEditor::FocusedGraphEdPtr>;

void SMatHelperWidget::Construct(const FArguments& InArgs,FMaterialEditor* InMatEditor)
{
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	const TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_Vertical);
	
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	
	MatEditorInterface = InMatEditor;
	Material = Cast<UMaterial>(MatEditorInterface->OriginalMaterialObject);
	
	PluginConfigPath = MatHelper.GetPluginPath().Append("/Config/");
	
	SAssignNew(GroupText,SEditableTextBox);
	SAssignNew(InstanceText,SEditableTextBox);
	SAssignNew(NodeButtonScrollBox,SScrollBox);

	RefreshMaskPinSelection();

	this->ChildSlot
	[
		NodeButtonScrollBox.ToSharedRef()
	];
	
	NodeButtonScrollBox->AddSlot()
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

	
	NodeButtonScrollBox->AddSlot()
	.Padding(5.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Scene View"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Lambda([&]()
		{
			MatEditorInterface->GetTabManager()->TryInvokeTab(FMatHelperModule::MaterialSceneViewEditorTabName);
			return FReply::Handled();
		})
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(5.0f)
	[
		GroupText.ToSharedRef()	
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Set Group"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::SetNodeGroup,false,false)
	];

	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Auto Group"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::SetNodeGroup,true,false)
	];
	

	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Auto All Group"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::SetNodeGroup,true,true)
	];
	
	if(MaskPinOptions.Num() > 0 )
	{
		NodeButtonScrollBox->AddSlot()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&MaskPinOptions)
			.OnGenerateWidget_Lambda([](const TSharedPtr<FString>& InString)
			{
				return SNew(STextBlock)
				.Text(FText::FromString(*InString));
			})
			.OnSelectionChanged_Lambda([&](const TSharedPtr<FString>& NewOption,ESelectInfo::Type SelectInfo)
			{
				CurrentSelect = MaskPinOptions.Find(NewOption);
			})
			
			[
				SNew(STextBlock)
				.Text_Lambda([&]()
				{
					return FText::FromString(*MaskPinOptions[CurrentSelect]);
				})
			]
		];
	
	
		NodeButtonScrollBox->AddSlot()
		.Padding(3.0f)
		[
			SNew(SButton)
			.Text(FText::FromString("Add Mask Pin"))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked_Raw(this,&SMatHelperWidget::AddNodeMaskPin)
		];
	}
	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Show Pin Name"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Lambda([&]()
		{
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
			MatNode->MaterialExpression->bShowOutputNameOnPin = !MatNode->MaterialExpression->bShowOutputNameOnPin;
			MatNode->RecreateAndLinkNode();
			MatEditorInterface->UpdateMaterialAfterGraphChange();
			return FReply::Handled();
		})
	];


	
	NodeButtonScrollBox->AddSlot()
	.Padding(5.0f)
	[
		InstanceText.ToSharedRef()
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Create Instance"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::CreateInstance)
	];
	
	NodeButtonScrollBox->AddSlot()
    	.Padding(3.0f)
    	[
    		SNew(SButton)
    		.Text(FText::FromString("Refraction"))
    		.VAlign(VAlign_Center)
    		.HAlign(HAlign_Center)
    		.OnClicked_Raw(this,&SMatHelperWidget::ToggleRefraction)
    	];

	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Fix Function Node"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::FixFunctionNode)
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Auto Name"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::RemoveParameterType)
	];
	
	InitialButton();
}

FReply SMatHelperWidget::SetNodeGroup(bool AutoGroup,bool AllGroup) const
{
	bool ShouldRefresh = false;

	if(AllGroup == true)
	{
		const auto GraphEdPtr =  MatEditorInterface->*TAccessPrivate<AccessGraph>::Value;
		if(const auto GraphEd = GraphEdPtr.Pin().Get())
		{
			GraphEd->SelectAllNodes();
		}
	}
	const FMatHelperModule& MatHelper = FMatHelperModule::Get();
	TArray<FString> Names = MatHelper.MatHelperMgn->AutoGroupKeys;
	
	auto SelectedNodes = MatEditorInterface->GetSelectedNodes();
	MatEditorInterface->FocusWindow();
	
	FString GroupName = GroupText->GetText().ToString(); // 提前获取组名

	const auto ProcessGroup = [&](auto* Parameter)
	{
		if (AutoGroup)
		{
			for (const FString& Name : Names)
			{
				if (Parameter->ParameterName.ToString().Contains(Name))
				{
					Parameter->Group = *Name;
					break; // 找到第一个匹配项后退出
				}
			}
		}
		else
		{
			Parameter->Group = *GroupName;
		}
		ShouldRefresh = true;
	};
	
	for(UObject* Node : SelectedNodes)
	{
		if(UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node))
		{
			if(CheckNode(MatNode) == false) {continue;}

			if(UMaterialExpressionParameter* Parameter = Cast<UMaterialExpressionParameter>(MatNode->MaterialExpression))
			{
				ProcessGroup(Parameter);
			}
			else if(UMaterialExpressionTextureSampleParameter* TexParameter = Cast<UMaterialExpressionTextureSampleParameter>(MatNode->MaterialExpression))
			{
				ProcessGroup(TexParameter);
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
	
	const TObjectPtr<UMaterialExpression> Expression = MatNode->MaterialExpression;
	TArray<FExpressionOutput>& Outputs = Expression->Outputs;

	bool Found = false;
	int Index = -1;
	for(const auto& Output : Outputs)
	{
		FIntVector4 PinMask = FIntVector4(Output.MaskR,Output.MaskG,Output.MaskB,Output.MaskA);
		Index = Index + 1;
		if(PinMask == MaskPinInfo[CurrentSelect])
		{
			Found = true;
			break;
		}
	}
	
	if(!Found)
	{
		Outputs.Add(FExpressionOutput(FName(**MaskPinOptions[CurrentSelect].Get()), 1,
			MaskPinInfo[CurrentSelect].X, MaskPinInfo[CurrentSelect].Y,
			MaskPinInfo[CurrentSelect].Z, MaskPinInfo[CurrentSelect].W));
	}
	else
	{
		Outputs.RemoveAt(Index);
	}
	
	MatEditorInterface->FocusWindow();
	MatNode->RecreateAndLinkNode();
	MatEditorInterface->UpdateMaterialAfterGraphChange();
	
	return FReply::Handled();
}



FReply SMatHelperWidget::CreateInstance()
{
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	FString TargetPath = Material->GetPathName();
	const FString BaseName = Material->GetName();
	TargetPath.ReplaceInline(*BaseName,*FString(""));
	TargetPath.ReplaceInline(*FString("."),*FString(""));

	FString NewBaseName = BaseName;
	if(BaseName.Left(2) == "M_")
	{
		NewBaseName.ReplaceInline(*FString("M_"),*FString("MI_"));
	}
	else
	{
		NewBaseName = "MI_" + BaseName;
	}
	FString InText = InstanceText->GetText().ToString();

	if(InText == "")
	{
		InText = "Inst" + FString::FromInt(UKismetMathLibrary::RandomIntegerInRange(0,99));
	}
	
	const FString NewName = NewBaseName + "_" + InText;

	
	const FString NewPath = TargetPath + NewName;
	
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if(EditorAssetSubsystem->DoesAssetExist(NewPath))
	{
		MatHelper.EditorNotify("Create Fail - This Instance Exists",SNotificationItem::CS_Fail);
		return FReply::Handled();
	}
	UMaterialInstance* NewMi = Cast<UMaterialInstance>(EditorAssetSubsystem->DuplicateAsset(MIEmptyPath, NewPath));
	NewMi->Parent=Material;
	
	UMaterialInstanceConstant* ConstMat = static_cast<UMaterialInstanceConstant*>(NewMi);
	const auto MaterialEditorInstance = NewObject<UMaterialEditorInstanceConstant>(GetTransientPackage(), NAME_None, RF_Transactional);
	MaterialEditorInstance->SetSourceInstance(ConstMat);

	const int32 GroupNum = MaterialEditorInstance->ParameterGroups.Num();
	for (int32 GroupIdx = 0; GroupIdx <GroupNum ; ++GroupIdx)
	{
		FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[GroupIdx];
		
		int32 ParameterNum = ParameterGroup.Parameters.Num();
		for (int32 ParamIdx = 0; ParamIdx < ParameterNum; ++ParamIdx)
		{
			UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
			FMaterialPropertyHelpers::OnOverrideParameter(true,Parameter,MaterialEditorInstance);
		}
	}
	
	TArray<UObject*> AssetList;
	AssetList.Add(NewMi);
	IContentBrowserSingleton::Get().SyncBrowserToAssets(AssetList);
	AssetViewUtils::OpenEditorForAsset(NewMi);
	
	return FReply::Handled();
}

FReply SMatHelperWidget::ToggleRefraction() const
{
	auto& Ref = MatEditorInterface->GetMaterialInterface()->GetMaterial()->RefractionMethod;
	if (Ref == RM_None)
	{
		Ref = RM_IndexOfRefraction;
	}
	else
	{
		Ref = RM_None;
	}
	const auto BaseRootNode =  MatEditorInterface->GetMaterialInterface()->GetMaterial()->MaterialGraph->RootNode;
	Cast<UMaterialGraphNode_Root>(BaseRootNode)->ReconstructNode();
	MatEditorInterface->UpdateMaterialAfterGraphChange();
	return FReply::Handled();
}

FReply SMatHelperWidget::FixFunctionNode() const
{
	bool bNeedRefreshNode = false;
	auto Nodes = MatEditorInterface->GetSelectedNodes().Array();
	for(const auto Node : Nodes)
	{
		UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
		if(Cast<UMaterialExpressionMaterialFunctionCall>(MatNode->MaterialExpression))
		{
			MatNode->RecreateAndLinkNode();
			bNeedRefreshNode = true;
		}
	}
	if(bNeedRefreshNode)
	{
		MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
		
}

FReply SMatHelperWidget::InitialButton()
{
	const FMatHelperModule& MatHelper = FMatHelperModule::Get();
	for(auto Button : NodeButtons)
	{
		NodeButtonScrollBox->RemoveSlot(Button.ToSharedRef());
	}
	NodeButtons.Empty();

	const int32 Num = MatHelper.MatHelperMgn->NodeButtonInfo.Num();
	
	for(int i = 0 ; i < Num ; i++)
	{
		FNodeButton ButtonInfo = MatHelper.MatHelperMgn->NodeButtonInfo[i];
		TSharedPtr<SButton> Button = SNew(SButton)
		.Text(FText::FromString( ButtonInfo.ButtonName))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this,&SMatHelperWidget::CreateMatNode,i));
		
		NodeButtonScrollBox->AddSlot()
		.Padding(3.0f)
		[
			Button.ToSharedRef()
		];
		
		NodeButtons.Add(Button);
	}
	
	
	return FReply::Handled();
}

FReply SMatHelperWidget::CreateMatNode(int32 Index) const
{
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	const FString NodeFileName = PluginConfigPath + "AddNodeFile/" + MatHelper.MatHelperMgn->NodeButtonInfo[Index].ButtonName + ".txt";
	
	if(FPaths::FileExists(NodeFileName) == false)
	{
		return FReply::Handled();
	}
	
	FString NodeText;
	FFileHelper::LoadFileToString(NodeText,*NodeFileName);
	if(NodeText.Len() == 0)
	{
		
		MatHelper.EditorNotify("This Text Maybe Empty.",SNotificationItem::CS_Fail);
		return FReply::Handled();
	}
	
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	FPlatformApplicationMisc::ClipboardCopy(*NodeText);
	
	MatEditorInterface->FocusWindow();
	auto SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
	
	FVector2D RootOffset;
	if(MatHelper.MatHelperMgn->NodeButtonInfo[Index].RootOffsetOverride)
	{
		RootOffset = MatHelper.MatHelperMgn->NodeButtonInfo[Index].RootOffset;
	}
	else
	{
		RootOffset = MatHelper.MatHelperMgn->RootOffset;
	}

	const FVector2D BaseOffset = MatHelper.MatHelperMgn->BaseOffset;

	const TObjectPtr<UMaterialGraph> Graph = MatEditorInterface->GetMaterialInterface()->GetMaterial()->MaterialGraph;
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
		const auto BaseRootNode =  Graph->RootNode;
		const FVector2D Location = FVector2D(BaseRootNode->NodePosX + RootOffset.X,BaseRootNode->NodePosY + RootOffset.Y);
		MatEditorInterface->PasteNodesHere(Location);
	}
	
	auto NewNodes = MatEditorInterface->GetSelectedNodes().Array();
	
	for(const auto Node : NewNodes)
	{
		UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
		if(Cast<UMaterialExpressionMaterialFunctionCall>(MatNode->MaterialExpression))
		{
			MatNode->RecreateAndLinkNode();
		}
		MatEditorInterface->AddToSelection(MatNode->MaterialExpression);
	}

	const TWeakPtr<SGraphEditor> GraphEdPtr = MatEditorInterface->*TAccessPrivate<AccessGraph>::Value;
	GraphEdPtr.Pin().Get()->JumpToNode(Cast<UMaterialGraphNode>(NewNodes[0]),false,false);
	
	
	FPlatformApplicationMisc::ClipboardCopy(*ClipboardContent);
	return FReply::Handled();
}


FReply SMatHelperWidget::RefreshButton()
{
	InitialButton();
	return FReply::Handled();
}

bool ModifyName(FString& Name)
{
	const FString VersionsToReplace[] ={ TEXT(" (V2)"), TEXT(" (V3)"), TEXT(" (V4)"), TEXT(" (S)"), TEXT(" (T2d)"), TEXT(" (SB)")};
	
	for (auto& Version : VersionsToReplace)
	{
		if (Name.Contains(Version))
		{
			Name.ReplaceInline(*Version, TEXT(""));
			return true;
		}
	}
	return false; 
}

FReply SMatHelperWidget::RemoveParameterType() const
{
	bool ShouldRefresh = false;

	const auto GraphEdPtr = MatEditorInterface->*TAccessPrivate<AccessGraph>::Value;
	if (const auto GraphEd = GraphEdPtr.Pin().Get()) {
	    GraphEd->SelectAllNodes();
	}
	
	auto SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
	if (SelectedNodes.Num() == 0) {
	    return FReply::Handled();
	}
	
	for (const auto Node : SelectedNodes)
	{
		if (const auto MatNode = Cast<UMaterialGraphNode>(Node))
	    {
	       	if (CheckNode(MatNode) == false)
	       	{
	       	    continue;
	       	}
			
	       	if (const auto Parameter = Cast<UMaterialExpressionParameter>(MatNode->MaterialExpression); Parameter != nullptr)
	       	{
	       	    FString Name = Parameter->ParameterName.ToString();
	       	    if (ModifyName(Name))
	       	    {
	       	        ShouldRefresh = true;
	       	        Parameter->ParameterName = *Name;
	       	    }
	       	}
	    }
	}

	if (ShouldRefresh) {
	    MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
}

void SMatHelperWidget::RefreshMaskPinSelection()
{
	const FMatHelperModule& MatHelper = FMatHelperModule::Get();
	TArray<FNodeMaskPin> Array = MatHelper.MatHelperMgn->MaskPinInfo;
	for(auto& Info : Array)
	{
		MaskPinOptions.Add(MakeShareable(new FString(Info.ButtonName)));
		MaskPinInfo.Add(Info.MaskValue);
	}
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

#undef LOCTEXT_NAMESPACE