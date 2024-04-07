// Copyright AKaKLya 2024

#include "MatHelperWidget.h"
#include "TAccessPrivate.inl"
#include "AssetViewUtils.h"
#include "EditorWidgetsModule.h"
#include "GraphEditorDragDropAction.h"
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
#include "MaterialGraph/MaterialGraphSchema.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Widgets/Input/STextComboBox.h"
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

	CategoryNames.Add(MakeShareable(new FString(TEXT("All"))));
	CategoryNames.Add(MakeShareable(new FString(TEXT("Expressions"))));
	CategoryNames.Add(MakeShareable(new FString(TEXT("Functions"))));

	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	
	MatEditorInterface = InMatEditor;
	Material = Cast<UMaterial>(MatEditorInterface->OriginalMaterialObject);
	
	PluginConfigPath = MatHelper.GetPluginPath().Append("/Config/");
	
	SAssignNew(GroupText,SEditableTextBox);
	SAssignNew(InstanceText,SEditableTextBox);
	SAssignNew(NodeButtonScrollBox,SScrollBox);

	
	this->ChildSlot
	[
		SNew(SBorder)
		.Padding(2.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
		
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(MatHelper.MatHelperMgn->HeightRatio)
			[
			
				NodeButtonScrollBox.ToSharedRef()
			]

			// Filter UI
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				// Comment
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Category", "Category: "))
				]

				// Combo button to select a class
				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SAssignNew(CategoryComboBox, STextComboBox)
						.OptionsSource(&CategoryNames)
						.OnSelectionChanged(this, &SMatHelperWidget::MHCategorySelectionChanged)
						.InitiallySelectedItem(CategoryNames[0])
					]
			]
			
			// Content list
			+SVerticalBox::Slot()
					[
						SNew(SOverlay)

						+SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						[
							// Old Expression and Function lists were auto expanded so do the same here for now
							SAssignNew(GraphActionMenu, SGraphActionMenu)
							.OnActionDragged(this, &SMatHelperWidget::OnActionDragged)
							.OnCreateWidgetForAction(this, &SMatHelperWidget::OnCreateWidgetForAction)
							.OnCollectAllActions(this, &SMatHelperWidget::CollectAllActions)
							.AutoExpandActionMenu(true)
						]

						+SOverlay::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Bottom)
							.Padding(FMargin(24, 0, 24, 0))
							[
								// Asset discovery indicator
								AssetDiscoveryIndicator
							]
					]
	]];
	
	
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
		.Text(FText::FromString("Auto Name"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::RemoveParameterType)
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
	

	NodeButtonScrollBox->AddSlot()
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

	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Add Mask Pin"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::AddNodeMaskPin)
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
		.Text(FText::FromString("-Refresh Button-"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::RefreshButton)
	];


	
	InitialButton();
}

FReply SMatHelperWidget::SetNodeGroup(bool AutoGroup,bool AllGroup)
{
	bool ShouldRefresh = false;

	if(AllGroup == true)
	{
		auto GraphEdPtr =  MatEditorInterface->*TAccessPrivate<AccessGraph>::Value;
		if(auto GraphEd = GraphEdPtr.Pin().Get())
		{
			GraphEd->SelectAllNodes();
		}
	}
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
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
	case 10:
		{
			TArray<FExpressionOutput>& Outputs = MatNode->MaterialExpression->Outputs;
			Outputs.Empty();
			IsAddSuccess = true;
			break;
		}
	case 11:
		{
			TArray<FExpressionOutput>& Outputs = MatNode->MaterialExpression->Outputs;
			Outputs.Empty();
			AddMaskPin(MatNode,"RG",FIntVector4(1,1,0,0),IsAddSuccess);
			AddMaskPin(MatNode,"BA",FIntVector4(0,0,1,1),IsAddSuccess);
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


void SMatHelperWidget::AddMaskPin(const UMaterialGraphNode* MatNode, const FString& Name, const FIntVector4& Mask,bool& Out_IsAddSuccess)
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
	for (int32 GroupIdx = 0; GroupIdx < MaterialEditorInstance->ParameterGroups.Num(); ++GroupIdx)
	{
		FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[GroupIdx];
		for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ++ParamIdx)
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

FReply SMatHelperWidget::ToggleRefraction()
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

FReply SMatHelperWidget::FixFunctionNode()
{
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
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
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
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

FReply SMatHelperWidget::CreateMatNode(int32 Index)
{
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	const FString NodeFileName = PluginConfigPath + "AddNodeFile/" + MatHelper.MatHelperMgn->NodeButtonInfo[Index].ButtonName + ".txt";

	const bool Exist = FPaths::FileExists(NodeFileName);
	if(Exist == false)
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
	
	auto Graph = MatEditorInterface->GetMaterialInterface()->GetMaterial()->MaterialGraph;
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
		const auto BaseRootNode =  MatEditorInterface->GetMaterialInterface()->GetMaterial()->MaterialGraph->RootNode;
		const FVector2D Location = FVector2D(BaseRootNode->NodePosX + RootOffset.X,BaseRootNode->NodePosY + RootOffset.Y);
		MatEditorInterface->PasteNodesHere(Location);
	}
	
	auto NewNodes = MatEditorInterface->GetSelectedNodes().Array();
	
//	MatEditorInterface->JumpToExpression(Cast<UMaterialGraphNode>(NewNodes[0])->MaterialExpression);
	
	for(const auto Node : NewNodes)
	{
		UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
		if(Cast<UMaterialExpressionMaterialFunctionCall>(MatNode->MaterialExpression))
		{
			MatNode->RecreateAndLinkNode();
		}
		MatEditorInterface->AddToSelection(MatNode->MaterialExpression);
	}

	TWeakPtr<SGraphEditor> GraphEdPtr = MatEditorInterface->*TAccessPrivate<AccessGraph>::Value;
	GraphEdPtr.Pin().Get()->JumpToNode(Cast<UMaterialGraphNode>(NewNodes[0]),false,false);
	
	
	FPlatformApplicationMisc::ClipboardCopy(*ClipboardContent);
	return FReply::Handled();
}


FReply SMatHelperWidget::RefreshButton()
{
	InitialButton();
	return FReply::Handled();
}

FReply SMatHelperWidget::RemoveParameterType()
{
	bool ShouldRefresh = false;
	auto GraphEdPtr =  MatEditorInterface->*TAccessPrivate<AccessGraph>::Value;
	if(auto GraphEd = GraphEdPtr.Pin().Get())
	{
		GraphEd->SelectAllNodes();
	}

	auto SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();

	for(auto Node : SelectedNodes)
	{
		if(auto MatNode = Cast<UMaterialGraphNode>(Node))
		{
			if(CheckNode(MatNode) == false)
			{
				continue;
			}
			if(UMaterialExpressionParameter* Parameter = Cast<UMaterialExpressionParameter>(MatNode->MaterialExpression))
			{
				FString Name = Parameter->ParameterName.ToString();
				if(Name.Contains(" (V2)"))
				{
					Name.ReplaceInline(TEXT(" (V2)"),TEXT(""));
					goto End;
				}
				if(Name.Contains((" (V3)")))
				{
					Name.ReplaceInline(TEXT(" (V3)"),TEXT(""));
					goto End;
				}
				if(Name.Contains((" (V4)")))
				{
					Name.ReplaceInline(TEXT(" (V4)"),TEXT(""));
					goto End;
				}
				if(Name.Contains((" (S)")))
				{
					Name.ReplaceInline(TEXT(" (S)"),TEXT(""));
					goto End;
				}
				if(Name.Contains((" (T2d)")))
				{
					Name.ReplaceInline(TEXT(" (T2d)"),TEXT(""));
					goto End;
				}
				if(Name.Contains((" (SB)")))
				{
					Name.ReplaceInline(TEXT(" (SB)"),TEXT(""));
					goto End;
				}
				
				End:
				ShouldRefresh = true;
				Parameter->ParameterName = *Name;
			}
			
		}
	}
	
	if(ShouldRefresh)
	{
		MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
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

#pragma region Palette

TSharedRef<SWidget> SMatHelperWidget::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return SMaterialPalette::OnCreateWidgetForAction(InCreateData);
}

void SMatHelperWidget::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	const UMaterialGraphSchema* Schema = GetDefault<UMaterialGraphSchema>();

	FGraphActionMenuBuilder ActionMenuBuilder;

	// Determine all possible actions
	Schema->GetPaletteActions(ActionMenuBuilder, SMatHelperWidget::MHGetFilterCategoryName(), MatEditorInterface->MaterialFunction != NULL);

	//@TODO: Avoid this copy
	OutAllActions.Append(ActionMenuBuilder);
}

void SMatHelperWidget::MHCategorySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	GraphActionMenu->RefreshAllActions(true);
}


FReply SMatHelperWidget::OnActionDragged(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions,
	const FPointerEvent& MouseEvent)
{
	if( InActions.Num() > 0 && InActions[0].IsValid() )
	{
		const TSharedPtr<FEdGraphSchemaAction> InAction = InActions[0];

		return FReply::Handled().BeginDragDrop(FGraphSchemaActionDragDropAction::New(InAction));
	}

	return FReply::Unhandled();
}

void SMatHelperWidget::MHRefreshAssetInRegistry(const FAssetData& InAddedAssetData)
{
	if (InAddedAssetData.IsInstanceOf(UMaterialFunction::StaticClass()))
	{
		RefreshActionsList(true);
	}
}

FString SMatHelperWidget::MHGetFilterCategoryName() const
{
	if (CategoryComboBox.IsValid())
	{
		return *CategoryComboBox->GetSelectedItem();
	}
	else
	{
		return TEXT("All");
	}
}
#pragma endregion 

#pragma region EngineClassCpp

void SMaterialPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData)
{
	check(InCreateData->Action.IsValid());

	const TSharedPtr<FEdGraphSchemaAction> GraphAction = InCreateData->Action;
	ActionPtr = InCreateData->Action;

	// Get the Hotkey chord if one exists for this action
	const TSharedPtr<const FInputChord> HotkeyChord;
	
	// Find icons
	const FSlateBrush* IconBrush = FAppStyle::GetBrush(TEXT("NoBrush"));
	const FSlateColor IconColor = FSlateColor::UseForeground();
	const FText IconToolTip = GraphAction->GetTooltipDescription();
	const bool bIsReadOnly = false;

	const TSharedRef<SWidget> IconWidget = CreateIconWidget( IconToolTip, IconBrush, IconColor );
	const TSharedRef<SWidget> NameSlotWidget = CreateTextSlotWidget(InCreateData, bIsReadOnly );
	const TSharedRef<SWidget> HotkeyDisplayWidget = CreateHotkeyDisplayWidget(HotkeyChord );
	// Create the actual widget
	this->ChildSlot
	[
		SNew(SHorizontalBox)
		// Icon slot
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			IconWidget
		]
		// Name slot
		+SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.Padding(3,0)
		[
			NameSlotWidget
		]
		// Hotkey slot
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		[
			HotkeyDisplayWidget
		]
	];
}

TSharedRef<SWidget> SMaterialPaletteItem::CreateHotkeyDisplayWidget(const TSharedPtr<const FInputChord> HotkeyChord)
{
	FText HotkeyText;
	if (HotkeyChord.IsValid())
	{
		HotkeyText = HotkeyChord->GetInputText();
	}
	return SNew(STextBlock)
		.Text(HotkeyText);
}

FText SMaterialPaletteItem::GetItemTooltip() const
{
	return ActionPtr.Pin()->GetTooltipDescription();
}

FString SMaterialPalette::GetFilterCategoryName() const
{
	if (CategoryComboBox.IsValid())
	{
		return *CategoryComboBox->GetSelectedItem();
	}
	else
	{
		return TEXT("All");
	}
}

TSharedRef<SWidget> SMaterialPalette::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return	SNew(SMaterialPaletteItem, InCreateData);
}

void SMaterialPalette::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	const UMaterialGraphSchema* Schema = GetDefault<UMaterialGraphSchema>();

	FGraphActionMenuBuilder ActionMenuBuilder;

	// Determine all possible actions
	Schema->GetPaletteActions(ActionMenuBuilder, GetFilterCategoryName(), MaterialEditorPtr.Pin()->MaterialFunction != NULL);

	//@TODO: Avoid this copy
	OutAllActions.Append(ActionMenuBuilder);
}

#pragma endregion 

#undef LOCTEXT_NAMESPACE