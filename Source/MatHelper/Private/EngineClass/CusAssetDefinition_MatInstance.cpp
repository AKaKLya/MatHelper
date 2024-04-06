// Copyright AKaKLya 2024


#include "EngineClass/CusAssetDefinition_MatInstance.h"
#include "TAccessPrivate.inl"
#include "IMaterialEditor.h"
#include "IPropertyRowGenerator.h"
#include "MaterialEditorModule.h"
#include "MaterialInstanceEditor.h"
#include "SMaterialLayersFunctionsTree.h"
#include "MaterialPropertyHelpers.h"
#include "EngineClass/CusMaterialEditorInstanceDetailCustomization.h"
#include "MaterialEditor/DEditorMaterialLayersParameterValue.h"
#include "MaterialEditor/DEditorParameterValue.h"
#include "MaterialEditor/DEditorScalarParameterValue.h"
#include "MaterialEditor/DEditorStaticComponentMaskParameterValue.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"

#define LOCTEXT_NAMESPACE "MaterialInstanceEditor"

struct AccessPalette
{
	typedef TSharedPtr<class IDetailsView> (FMaterialInstanceEditor::*Type);
};
template struct TAccessPrivateStub<AccessPalette,&FMaterialInstanceEditor::MaterialInstanceDetails>;

struct AccessInstance
{
	typedef TObjectPtr<UMaterialEditorInstanceConstant> (FMaterialInstanceEditor::*Type);
};
template struct TAccessPrivateStub<AccessInstance,&FMaterialInstanceEditor::MaterialEditorInstance>;

struct AccessLayer
{
	typedef TSharedPtr<class SMaterialLayersFunctionsInstanceWrapper> (FMaterialInstanceEditor::*Type);
};
template struct TAccessPrivateStub<AccessLayer,&FMaterialInstanceEditor::MaterialLayersFunctionsInstance>;

UThumbnailInfo* UCusAssetDefinition_MatInstance::LoadThumbnailInfo(const FAssetData& InAsset) const
{
	if (UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(InAsset.GetAsset()))
	{
		if (USceneThumbnailInfoWithPrimitive* ThumbnailInfo = UE::Editor::FindOrCreateThumbnailInfo<USceneThumbnailInfoWithPrimitive>(MaterialInterface))
		{
			const UMaterial* Material = MaterialInterface->GetBaseMaterial();
			if (Material && Material->bUsedWithParticleSprites)
			{
				ThumbnailInfo->DefaultPrimitiveType = TPT_Plane;
			}

			return ThumbnailInfo;
		}
	}

	return nullptr;
}

EAssetCommandResult UCusAssetDefinition_MatInstance::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UMaterialInstanceConstant* MIC : OpenArgs.LoadObjects<UMaterialInstanceConstant>())
	{
		IMaterialEditorModule* MaterialEditorModule = &FModuleManager::LoadModuleChecked<IMaterialEditorModule>( "MaterialEditor" );
		TSharedRef<IMaterialEditor> EditorRef = MaterialEditorModule->CreateMaterialInstanceEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, MIC);
		
		auto Editor = EditorRef.ToSharedPtr().Get();
		auto MatInstanceEditor = static_cast<FMaterialInstanceEditor*>(Editor);
		auto Detail = MatInstanceEditor->*TAccessPrivate<AccessPalette>::Value;
		auto Instance = MatInstanceEditor->*TAccessPrivate<AccessInstance>::Value;
		auto Layer = MatInstanceEditor->*TAccessPrivate<AccessLayer>::Value;
		
		FOnGetDetailCustomizationInstance LayoutMICDetails = FOnGetDetailCustomizationInstance::CreateStatic( 
		&FCusMaterialInstanceParameterDetails::MakeInstance, Instance.Get(), FCusGetShowHiddenParameters::CreateLambda([](bool& b){b = true;}) );
		Detail->RegisterInstancedCustomPropertyLayout(UMaterialEditorInstanceConstant::StaticClass(),LayoutMICDetails);
		
		TArray<UObject*> SelectedObjects;
		SelectedObjects.Add( Instance );
		Detail->SetObjects( SelectedObjects, true );
		if (Layer.IsValid())
		{
			Layer->NestedTree->MaterialEditorInstance=Instance;
			Layer->Refresh();
		}
	}

	return EAssetCommandResult::Handled;
}

#pragma region EngineClassCpp

void SMaterialLayersFunctionsInstanceTree::AddLayer()
{
	const FScopedTransaction Transaction(LOCTEXT("AddLayerAndBlend", "Add a new Layer and a Blend into it"));
	FunctionInstanceHandle->NotifyPreChange();
	FunctionInstance->AppendBlendedLayer();
	FunctionInstanceHandle->NotifyPostChange(EPropertyChangeType::ArrayAdd);
	CreateGroupsWidget();
	RequestTreeRefresh();
}

FReply SMaterialLayersFunctionsInstanceTree::RelinkLayersToParent()
{
	const FScopedTransaction Transaction(LOCTEXT("RelinkLayersToParent", "Relink layers to parent"));
	FunctionInstanceHandle->NotifyPreChange();
	FunctionInstance->RelinkLayersToParent();
	FunctionInstanceHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	MaterialEditorInstance->RegenerateArrays();
	CreateGroupsWidget();
	return FReply::Handled();
}

EVisibility SMaterialLayersFunctionsInstanceTree::GetRelinkLayersToParentVisibility() const
{
	if (FunctionInstance->HasAnyUnlinkedLayers())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}


TSharedPtr<IDetailTreeNode> FindParameterGroupsNode(TSharedPtr<IPropertyRowGenerator> PropertyRowGenerator)
{
	const TArray<TSharedRef<IDetailTreeNode>> RootNodes = PropertyRowGenerator->GetRootTreeNodes();
	if (RootNodes.Num() > 0)
	{
		TSharedPtr<IDetailTreeNode> Category = RootNodes[0];
		TArray<TSharedRef<IDetailTreeNode>> Children;
		Category->GetChildren(Children);

		for (int32 ChildIdx = 0; ChildIdx < Children.Num(); ChildIdx++)
		{
			TSharedPtr<IPropertyHandle> PropertyHandle = Children[ChildIdx]->CreatePropertyHandle();
			if (PropertyHandle.IsValid() && PropertyHandle->GetProperty() && PropertyHandle->GetProperty()->GetName() == "ParameterGroups")
			{
				return Children[ChildIdx];
			}
		}
	}
	return nullptr;
}



void SMaterialLayersFunctionsInstanceTree::SetParentsExpansionState()
{
	for (const auto& Pair : LayerProperties)
	{
		if (Pair->Children.Num())
		{
			bool* bIsExpanded = MaterialEditorInstance->SourceInstance->LayerParameterExpansion.Find(Pair->NodeKey);
			if (bIsExpanded)
			{
				SetItemExpansion(Pair, *bIsExpanded);
			}
		}
	}
}


void SMaterialLayersFunctionsInstanceTree::ShowSubParameters(TSharedPtr<FSortedParamData> ParentParameter)
{
	for (FUnsortedParamData Property : NonLayerProperties)
	{
		UDEditorParameterValue* Parameter = Property.Parameter;
		if (Parameter->ParameterInfo.Index == ParentParameter->ParameterInfo.Index
			&& Parameter->ParameterInfo.Association == ParentParameter->ParameterInfo.Association)
		{
			TSharedPtr<FSortedParamData> GroupProperty(new FSortedParamData());
			GroupProperty->StackDataType = EStackDataType::Group;
			GroupProperty->ParameterInfo.Index = Parameter->ParameterInfo.Index;
			GroupProperty->ParameterInfo.Association = Parameter->ParameterInfo.Association;
			GroupProperty->Group = Property.ParameterGroup;
			GroupProperty->NodeKey = FString::FromInt(GroupProperty->ParameterInfo.Index) + FString::FromInt(GroupProperty->ParameterInfo.Association) + Property.ParameterGroup.GroupName.ToString();

			bool bAddNewGroup = true;
			for (TSharedPtr<struct FSortedParamData> GroupChild : ParentParameter->Children)
			{
				if (GroupChild->NodeKey == GroupProperty->NodeKey)
				{
					bAddNewGroup = false;
				}
			}
			if (bAddNewGroup)
			{
				ParentParameter->Children.Add(GroupProperty);
			}

			TSharedPtr<FSortedParamData> ChildProperty(new FSortedParamData());
			ChildProperty->StackDataType = EStackDataType::Property;
			ChildProperty->Parameter = Parameter;
			ChildProperty->ParameterInfo.Index = Parameter->ParameterInfo.Index;
			ChildProperty->ParameterInfo.Association = Parameter->ParameterInfo.Association;
			ChildProperty->ParameterNode = Property.ParameterNode;
			ChildProperty->PropertyName = Property.UnsortedName;
			ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association) +  Property.ParameterGroup.GroupName.ToString() + Property.UnsortedName.ToString();


			UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
			if (!CompMaskParam)
			{
				TArray<TSharedRef<IDetailTreeNode>> ParamChildren;
				Property.ParameterNode->GetChildren(ParamChildren);
				for (int32 ParamChildIdx = 0; ParamChildIdx < ParamChildren.Num(); ParamChildIdx++)
				{
					TSharedPtr<FSortedParamData> ParamChildProperty(new FSortedParamData());
					ParamChildProperty->StackDataType = EStackDataType::PropertyChild;
					ParamChildProperty->ParameterNode = ParamChildren[ParamChildIdx];
					ParamChildProperty->ParameterHandle = ParamChildProperty->ParameterNode->CreatePropertyHandle();
					ParamChildProperty->ParameterInfo.Index = Parameter->ParameterInfo.Index;
					ParamChildProperty->ParameterInfo.Association = Parameter->ParameterInfo.Association;
					ParamChildProperty->Parameter = ChildProperty->Parameter;
					ChildProperty->Children.Add(ParamChildProperty);
				}
			}
			for (TSharedPtr<struct FSortedParamData> GroupChild : ParentParameter->Children)
			{
				if (GroupChild->Group.GroupName == Property.ParameterGroup.GroupName
					&& GroupChild->ParameterInfo.Association == ChildProperty->ParameterInfo.Association
					&&  GroupChild->ParameterInfo.Index == ChildProperty->ParameterInfo.Index)
				{
					GroupChild->Children.Add(ChildProperty);
				}
			}

		}
	}
}


void SMaterialLayersFunctionsInstanceTree::CreateGroupsWidget()
{
	check(MaterialEditorInstance);
	MaterialEditorInstance->RegenerateArrays();
	NonLayerProperties.Empty();
	LayerProperties.Empty();
	FunctionParameter = nullptr;
	FPropertyEditorModule& Module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	if (!Generator.IsValid())
	{
		FPropertyRowGeneratorArgs Args;
		Generator = Module.CreatePropertyRowGenerator(Args);
		// the sizes of the parameter lists are only based on the parent material and not changed out from under the details panel 
		// When a parameter is added open MI editors are refreshed
		// the tree should also refresh if one of the layer or blend assets is swapped

		auto ValidationLambda = ([](const FRootPropertyNodeList& PropertyNodeList) { return true; });
		Generator->SetCustomValidatePropertyNodesFunction(FOnValidatePropertyRowGeneratorNodes::CreateLambda(MoveTemp(ValidationLambda)));

		TArray<UObject*> Objects;
		Objects.Add(MaterialEditorInstance);
		Generator->SetObjects(Objects);
	}
	else
	{
		TArray<UObject*> Objects;
		Objects.Add(MaterialEditorInstance);
		Generator->SetObjects(Objects);
	}

	TSharedPtr<IDetailTreeNode> ParameterGroups = FindParameterGroupsNode(Generator);
	if (ParameterGroups.IsValid())
	{
		TArray<TSharedRef<IDetailTreeNode>> Children;
		ParameterGroups->GetChildren(Children);
		// the order of DeferredSearches should correspond to NonLayerProperties exactly
		TArray<TSharedPtr<IPropertyHandle>> DeferredSearches;
		for (int32 GroupIdx = 0; GroupIdx < Children.Num(); ++GroupIdx)
		{
			TArray<void*> GroupPtrs;
			TSharedPtr<IPropertyHandle> ChildHandle = Children[GroupIdx]->CreatePropertyHandle();
			ChildHandle->AccessRawData(GroupPtrs);
			auto GroupIt = GroupPtrs.CreateConstIterator();
			const FEditorParameterGroup* ParameterGroupPtr = reinterpret_cast<FEditorParameterGroup*>(*GroupIt);
			const FEditorParameterGroup& ParameterGroup = *ParameterGroupPtr;

			for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ParamIdx++)
			{
				UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];

				TSharedPtr<IPropertyHandle> ParametersArrayProperty = ChildHandle->GetChildHandle("Parameters");
				TSharedPtr<IPropertyHandle> ParameterProperty = ParametersArrayProperty->GetChildHandle(ParamIdx);
				TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");

				if (Cast<UDEditorMaterialLayersParameterValue>(Parameter))
				{
					if (FunctionParameter == nullptr)
					{
						FunctionParameter = Parameter;
					}
					TArray<void*> StructPtrs;
					ParameterValueProperty->AccessRawData(StructPtrs);
					auto It = StructPtrs.CreateConstIterator();
					FunctionInstance = reinterpret_cast<FMaterialLayersFunctions*>(*It);
					FunctionInstanceHandle = ParameterValueProperty;

					TSharedPtr<IPropertyHandle>	LayerHandle = ChildHandle->GetChildHandle("Layers").ToSharedRef();
					TSharedPtr<IPropertyHandle> BlendHandle = ChildHandle->GetChildHandle("Blends").ToSharedRef();
					uint32 LayerChildren;
					LayerHandle->GetNumChildren(LayerChildren);
					uint32 BlendChildren;
					BlendHandle->GetNumChildren(BlendChildren);
					if (MaterialEditorInstance->StoredLayerPreviews.Num() != LayerChildren)
					{
						MaterialEditorInstance->StoredLayerPreviews.Empty();
						MaterialEditorInstance->StoredLayerPreviews.AddDefaulted(LayerChildren);
					}
					if (MaterialEditorInstance->StoredBlendPreviews.Num() != BlendChildren)
					{
						MaterialEditorInstance->StoredBlendPreviews.Empty();
						MaterialEditorInstance->StoredBlendPreviews.AddDefaulted(BlendChildren);
					}

					TSharedRef<FSortedParamData> StackProperty = MakeShared<FSortedParamData>();
					StackProperty->StackDataType = EStackDataType::Stack;
					StackProperty->Parameter = Parameter;
					StackProperty->ParameterInfo.Index = LayerChildren - 1;
					StackProperty->NodeKey = FString::FromInt(StackProperty->ParameterInfo.Index);


					TSharedRef<FSortedParamData> ChildProperty = MakeShared<FSortedParamData>();
					ChildProperty->StackDataType = EStackDataType::Asset;
					ChildProperty->Parameter = Parameter;
					ChildProperty->ParameterHandle = LayerHandle->AsArray()->GetElement(LayerChildren - 1);
					ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
					ChildProperty->ParameterInfo.Index = LayerChildren - 1;
					ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::LayerParameter;
					ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);

					{
						UObject* AssetObject = nullptr;
						ChildProperty->ParameterHandle->GetValue(AssetObject);
						if (AssetObject)
						{
							if (MaterialEditorInstance->StoredLayerPreviews[LayerChildren - 1] == nullptr)
							{
								MaterialEditorInstance->StoredLayerPreviews[LayerChildren - 1] = (NewObject<UMaterialInstanceConstant>(MaterialEditorInstance, NAME_None));
							}
							UMaterialInterface* EditedMaterial = Cast<UMaterialFunctionInterface>(AssetObject)->GetPreviewMaterial();
							if (MaterialEditorInstance->StoredLayerPreviews[LayerChildren - 1] && MaterialEditorInstance->StoredLayerPreviews[LayerChildren - 1]->Parent != EditedMaterial)
							{
								MaterialEditorInstance->StoredLayerPreviews[LayerChildren - 1]->SetParentEditorOnly(EditedMaterial);
							}
						}
					}

					StackProperty->Children.Add(ChildProperty);
					LayerProperties.Add(StackProperty);

					if (BlendChildren > 0 && LayerChildren > BlendChildren)
					{
						for (int32 Counter = BlendChildren - 1; Counter >= 0; Counter--)
						{
							ChildProperty = MakeShared<FSortedParamData>();
							ChildProperty->StackDataType = EStackDataType::Asset;
							ChildProperty->Parameter = Parameter;
							ChildProperty->ParameterHandle = BlendHandle->AsArray()->GetElement(Counter);
							ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
							ChildProperty->ParameterInfo.Index = Counter;
							ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::BlendParameter;
							ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);
							{
								UObject* AssetObject = nullptr;
								ChildProperty->ParameterHandle->GetValue(AssetObject);
								if (AssetObject)
								{
									if (MaterialEditorInstance->StoredBlendPreviews[Counter] == nullptr)
									{
										MaterialEditorInstance->StoredBlendPreviews[Counter] = (NewObject<UMaterialInstanceConstant>(MaterialEditorInstance, NAME_None));
									}
									UMaterialInterface* EditedMaterial = Cast<UMaterialFunctionInterface>(AssetObject)->GetPreviewMaterial();
									if (MaterialEditorInstance->StoredBlendPreviews[Counter] && MaterialEditorInstance->StoredBlendPreviews[Counter]->Parent != EditedMaterial)
									{
										MaterialEditorInstance->StoredBlendPreviews[Counter]->SetParentEditorOnly(EditedMaterial);
									}
								}
							}
							LayerProperties.Last()->Children.Add(ChildProperty);

							StackProperty = MakeShared<FSortedParamData>();
							StackProperty->StackDataType = EStackDataType::Stack;
							StackProperty->Parameter = Parameter;
							StackProperty->ParameterInfo.Index = Counter;
							StackProperty->NodeKey = FString::FromInt(StackProperty->ParameterInfo.Index);
							LayerProperties.Add(StackProperty);

							ChildProperty = MakeShared<FSortedParamData>();
							ChildProperty->StackDataType = EStackDataType::Asset;
							ChildProperty->Parameter = Parameter;
							ChildProperty->ParameterHandle = LayerHandle->AsArray()->GetElement(Counter);
							ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
							ChildProperty->ParameterInfo.Index = Counter;
							ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::LayerParameter;
							ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);
							{
								UObject* AssetObject = nullptr;
								ChildProperty->ParameterHandle->GetValue(AssetObject);
								if (AssetObject)
								{
									if (MaterialEditorInstance->StoredLayerPreviews[Counter] == nullptr)
									{
										MaterialEditorInstance->StoredLayerPreviews[Counter] = (NewObject<UMaterialInstanceConstant>(MaterialEditorInstance, NAME_None));
									}
									UMaterialInterface* EditedMaterial = Cast<UMaterialFunctionInterface>(AssetObject)->GetPreviewMaterial();
									if (MaterialEditorInstance->StoredLayerPreviews[Counter] && MaterialEditorInstance->StoredLayerPreviews[Counter]->Parent != EditedMaterial)
									{
										MaterialEditorInstance->StoredLayerPreviews[Counter]->SetParentEditorOnly(EditedMaterial);
									}
								}
							}
							LayerProperties.Last()->Children.Add(ChildProperty);
						}
					}
				}
				else
				{
					FUnsortedParamData NonLayerProperty;
					UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);

					if (ScalarParam && ScalarParam->SliderMax > ScalarParam->SliderMin)
					{
						ParameterValueProperty->SetInstanceMetaData("UIMin", FString::Printf(TEXT("%f"), ScalarParam->SliderMin));
						ParameterValueProperty->SetInstanceMetaData("UIMax", FString::Printf(TEXT("%f"), ScalarParam->SliderMax));
					}

					NonLayerProperty.Parameter = Parameter;
					NonLayerProperty.ParameterGroup = ParameterGroup;

					DeferredSearches.Add(ParameterValueProperty);
					NonLayerProperty.UnsortedName = Parameter->ParameterInfo.Name;

					NonLayerProperties.Add(NonLayerProperty);
				}
			}
		}

		checkf(NonLayerProperties.Num() == DeferredSearches.Num(), TEXT("Internal inconsistency: number of node searches does not match the number of properties"));
		TArray<TSharedPtr<IDetailTreeNode>> DeferredResults = Generator->FindTreeNodes(DeferredSearches);
		checkf(NonLayerProperties.Num() == DeferredResults.Num(), TEXT("Internal inconsistency: number of node search results does not match the number of properties"));

		for (int Idx = 0, NumUnsorted = NonLayerProperties.Num(); Idx < NumUnsorted; ++Idx)
		{
			FUnsortedParamData& NonLayerProperty = NonLayerProperties[Idx];
			NonLayerProperty.ParameterNode = DeferredResults[Idx];
			NonLayerProperty.ParameterHandle = NonLayerProperty.ParameterNode->CreatePropertyHandle();
		}

		DeferredResults.Empty();
		DeferredSearches.Empty();

		for (int32 LayerIdx = 0; LayerIdx < LayerProperties.Num(); LayerIdx++)
		{
			for (int32 ChildIdx = 0; ChildIdx < LayerProperties[LayerIdx]->Children.Num(); ChildIdx++)
			{
				ShowSubParameters(LayerProperties[LayerIdx]->Children[ChildIdx]);
			}
		}
	}

	SetParentsExpansionState();
}


void SMaterialLayersFunctionsInstanceWrapper::Refresh()
{
	LayerParameter.Reset();
	TSharedPtr<SHorizontalBox> HeaderBox;
	NestedTree->CreateGroupsWidget();
	LayerParameter = NestedTree->FunctionParameter;
	FOnClicked 	OnChildButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, ImplicitConv<UMaterialInterface*>(MaterialEditorInstance->SourceInstance), ImplicitConv<UObject*>(MaterialEditorInstance));
	FOnClicked	OnSiblingButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, MaterialEditorInstance->SourceInstance->Parent, ImplicitConv<UObject*>(MaterialEditorInstance));

	if (LayerParameter != nullptr)
	{
		FOnClicked OnRelinkToParent = FOnClicked::CreateSP(NestedTree.ToSharedRef(), &SMaterialLayersFunctionsInstanceTree::RelinkLayersToParent);

		this->ChildSlot
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(0.0f)
				.AutoHeight()
				[
					SAssignNew(HeaderBox, SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(FMargin(4.0f, 0.0f))
					.HAlign(HAlign_Left)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MaterialLayers", "Material Layers"))
					]
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin(0.0f))
				[
					NestedTree.ToSharedRef()
				]
		];
		if (FMaterialPropertyHelpers::IsOverriddenExpression(NestedTree->FunctionParameter))
		{
			HeaderBox->AddSlot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					PropertyCustomizationHelpers::MakeAddButton(FSimpleDelegate::CreateSP(NestedTree.Get(), &SMaterialLayersFunctionsInstanceTree::AddLayer))
				];
		}
		HeaderBox->AddSlot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			];
		HeaderBox->AddSlot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Relink", "Relink"))
				.HAlign(HAlign_Center)
				.OnClicked(OnRelinkToParent)
				.ToolTipText(LOCTEXT("RelinkToParentLayers", "Relink to Parent Layers and Blends"))
				.Visibility(NestedTree.Get(), &SMaterialLayersFunctionsInstanceTree::GetRelinkLayersToParentVisibility)
			];
		HeaderBox->AddSlot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("SaveSibling", "Save Sibling"))
					.HAlign(HAlign_Center)
					.OnClicked(OnSiblingButtonClicked)
					.ToolTipText(LOCTEXT("SaveToSiblingInstance", "Save to Sibling Instance"))
				];
		HeaderBox->AddSlot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("SaveChild", "Save Child"))
				.HAlign(HAlign_Center)
				.OnClicked(OnChildButtonClicked)
				.ToolTipText(LOCTEXT("SaveToChildInstance", "Save To Child Instance"))
			];
	}
	else
	{
		this->ChildSlot
			[
				SNew(SBox)
				.Padding(FMargin(10.0f))
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AddLayerParameterPrompt", "Add a Material Attribute Layers parameter to see it here."))
				]
			];
	}
}
#pragma endregion



#undef LOCTEXT_NAMESPACE
