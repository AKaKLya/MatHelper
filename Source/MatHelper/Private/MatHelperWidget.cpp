// Copyright AKaKLya 2024

#include "MatHelperWidget.h"

#include "AssetToolsModule.h"
#include "TAccessPrivate.inl"
#include "AssetViewUtils.h"
#include "EditorWidgetsModule.h"
#include "IContentBrowserSingleton.h"
#include "ISettingsModule.h"
#include "MaterialGraphNode_Knot.h"
#include "MatHelper.h"
#include "MatHelperSettings.h"
#include "Editor/MaterialEditor/Private/MaterialEditor.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Comment.h"
#include "MaterialGraph/MaterialGraphNode_Composite.h"
#include "MaterialGraph/MaterialGraphNode_PinBase.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Windows/WindowsPlatformApplicationMisc.h"


#define LOCTEXT_NAMESPACE "MaterialPalette"

DEFINE_ACCESS_PRIVATE(AccessGraph,FMaterialEditor,TWeakPtr<SGraphEditor>,FocusedGraphEdPtr)

void SMatHelperWidget::Construct(const FArguments& InArgs,FMaterialEditor* InMatEditor)
{
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	const TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_Vertical);
	
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	
	MaterialEditor = InMatEditor;
	Material = Cast<UMaterial>(MaterialEditor->OriginalMaterialObject);
	
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
			FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "Plugins", "MatHelperSettings");
		
			//AssetViewUtils::OpenEditorForAsset("/MatHelper/MatHelper.MatHelper");
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
			MaterialEditor->GetTabManager()->TryInvokeTab(FMatHelperModule::MaterialSceneViewEditorTabName);
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
			MaterialEditor->FocusWindow();
			TArray<UObject*> SelectedNodes = MaterialEditor->GetSelectedNodes().Array();
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
			MaterialEditor->UpdateMaterialAfterGraphChange();
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
		const auto GraphEdPtr =  MaterialEditor->*TAccessPrivate<AccessGraph>::Value;
		if(const auto GraphEd = GraphEdPtr.Pin().Get())
		{
			GraphEd->SelectAllNodes();
		}
	}

	TArray<FString> Names = GetDefault<UMatHelperSettings>()->AutoGroupKeys;
	
	auto SelectedNodes = MaterialEditor->GetSelectedNodes();
	MaterialEditor->FocusWindow();
	
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
		MaterialEditor->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
}

FReply SMatHelperWidget::AddNodeMaskPin()
{
	MaterialEditor->FocusWindow();
	TArray<UObject*> SelectedNodes = MaterialEditor->GetSelectedNodes().Array();
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
	
	MaterialEditor->FocusWindow();
	MatNode->RecreateAndLinkNode();
	MaterialEditor->UpdateMaterialAfterGraphChange();
	
	return FReply::Handled();
}

FReply SMatHelperWidget::CreateInstance()
{
	if (!Material || !Material->IsValidLowLevel())
	{
		FMatHelperModule::EditorNotify("Invalid Parent Material", SNotificationItem::CS_Fail);
		return FReply::Handled();
	}

	// 1. 生成材质实例名称
	FString BaseName = Material->GetName();
	FString NewBaseName;

	if (BaseName.StartsWith("M_"))
	{
		NewBaseName = BaseName.Replace(*FString("M_"), *FString("MI_"), ESearchCase::CaseSensitive);
	}
	else
	{
		NewBaseName = FString("MI_") + BaseName;
	}
	NewBaseName = NewBaseName + "_" + InstanceText->GetText().ToString();
	
	// 2. 生成唯一资产路径
	FString PackagePath = Material->GetOutermost()->GetName();
	PackagePath = FPackageName::GetLongPackagePath(PackagePath); // 去掉文件名，只保留路径

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString FinalAssetName;
	FString UniquePackageName;
	AssetToolsModule.Get().CreateUniqueAssetName(PackagePath / NewBaseName,"",UniquePackageName,FinalAssetName);

	// 3. 创建材质实例工厂
	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = Material; // 直接设置父材质

	// 4. 通过AssetTools创建材质实例
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(FinalAssetName,FPackageName::GetLongPackagePath(UniquePackageName),
		UMaterialInstanceConstant::StaticClass(),Factory);

	UMaterialInstanceConstant* NewMIC = Cast<UMaterialInstanceConstant>(NewAsset);
	if (!NewMIC)
	{
		FMatHelperModule::EditorNotify("Failed to Create Material Instance", SNotificationItem::CS_Fail);
		return FReply::Handled();
	}
	
	// 5. 自动打开材质编辑器
	TArray<UObject*> AssetsToSync = { NewMIC };
	IContentBrowserSingleton::Get().SyncBrowserToAssets(AssetsToSync);
	AssetViewUtils::OpenEditorForAsset(NewMIC);

	return FReply::Handled();
}


FReply SMatHelperWidget::ToggleRefraction() const
{
	auto& Ref = MaterialEditor->GetMaterialInterface()->GetMaterial()->RefractionMethod;
	if (Ref == RM_None)
	{
		Ref = RM_IndexOfRefraction;
	}
	else
	{
		Ref = RM_None;
	}
	const auto BaseRootNode =  MaterialEditor->GetMaterialInterface()->GetMaterial()->MaterialGraph->RootNode;
	Cast<UMaterialGraphNode_Root>(BaseRootNode)->ReconstructNode();
	MaterialEditor->UpdateMaterialAfterGraphChange();
	return FReply::Handled();
}

FReply SMatHelperWidget::FixFunctionNode() const
{
	bool bNeedRefreshNode = false;
	auto Nodes = MaterialEditor->GetSelectedNodes().Array();
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
		MaterialEditor->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
		
}

FReply SMatHelperWidget::InitialButton()
{
	for(auto Button : NodeButtons)
	{
		NodeButtonScrollBox->RemoveSlot(Button.ToSharedRef());
	}
	NodeButtons.Empty();

	const UMatHelperSettings* MatHelperSettings = GetDefault<UMatHelperSettings>();
	const int32 Num = MatHelperSettings->NodeButtonInfo.Num();
	
	for(int i = 0 ; i < Num ; i++)
	{
		FNodeButton ButtonInfo = MatHelperSettings->NodeButtonInfo[i];
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
	const UMatHelperSettings* MatHelperSettings = GetDefault<UMatHelperSettings>();
	const FString NodeFileName = PluginConfigPath + "AddNodeFile/" + MatHelperSettings->NodeButtonInfo[Index].ButtonName + ".txt";
	
	if(FPaths::FileExists(NodeFileName) == false)
	{
		return FReply::Handled();
	}
	
	FString NodeText;
	FFileHelper::LoadFileToString(NodeText,*NodeFileName);
	if(NodeText.Len() == 0)
	{
		
		FMatHelperModule::EditorNotify("This Text Maybe Empty.",SNotificationItem::CS_Fail);
		return FReply::Handled();
	}
	
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	FPlatformApplicationMisc::ClipboardCopy(*NodeText);
	
	MaterialEditor->FocusWindow();
	auto SelectedNodes = MaterialEditor->GetSelectedNodes().Array();
	
	FVector2D RootOffset;
	if(MatHelperSettings->NodeButtonInfo[Index].RootOffsetOverride)
	{
		RootOffset = MatHelperSettings->NodeButtonInfo[Index].RootOffset;
	}
	else
	{
		RootOffset = MatHelperSettings->RootOffset;
	}

	const FVector2D BaseOffset = MatHelperSettings->BaseOffset;

	const TObjectPtr<UMaterialGraph> Graph = MaterialEditor->GetMaterialInterface()->GetMaterial()->MaterialGraph;
	if(SelectedNodes.Num() > 0)
	{
		UObject* SelectedNode = SelectedNodes[0];
		if(const auto RootNode = Cast<UMaterialGraphNode_Root>(SelectedNode))
		{
			const FVector2D Location = FVector2D(RootNode->NodePosX + RootOffset.X,RootNode->NodePosY + RootOffset.Y);
			MaterialEditor->PasteNodesHere(Location);
		}
		else if(const auto BaseNode = Cast<UMaterialGraphNode>(SelectedNode))
		{
			const FVector2D Location = FVector2D(BaseNode->NodePosX + BaseOffset.X,BaseNode->NodePosY + BaseOffset.Y);
			MaterialEditor->PasteNodesHere(Location);
		}
	}
	else
	{
		const auto BaseRootNode =  Graph->RootNode;
		const FVector2D Location = FVector2D(BaseRootNode->NodePosX + RootOffset.X,BaseRootNode->NodePosY + RootOffset.Y);
		MaterialEditor->PasteNodesHere(Location);
	}

	auto NewNodes = MaterialEditor->GetSelectedNodes().Array();
	
	for(const auto Node : NewNodes)
	{
		UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
		if(Cast<UMaterialExpressionMaterialFunctionCall>(MatNode->MaterialExpression))
		{
			MatNode->RecreateAndLinkNode();
		}
		MaterialEditor->AddToSelection(MatNode->MaterialExpression);
	}

	const TWeakPtr<SGraphEditor> GraphEdPtr = MaterialEditor->*TAccessPrivate<AccessGraph>::Value;
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

	const auto GraphEdPtr = MaterialEditor->*TAccessPrivate<AccessGraph>::Value;
	if (const auto GraphEd = GraphEdPtr.Pin().Get()) {
	    GraphEd->SelectAllNodes();
	}
	
	auto SelectedNodes = MaterialEditor->GetSelectedNodes().Array();
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
	    MaterialEditor->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
}

void SMatHelperWidget::RefreshMaskPinSelection()
{
	TArray<FNodeMaskPin> Array = GetDefault<UMatHelperSettings>()->MaskPinInfo;
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