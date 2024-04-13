// Copyright AKaKLya 2024
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MINOR_VERSION == 4

#include "EngineClass/CusMaterialEditorInstanceDetailCustomization.h"
#include "Misc/MessageDialog.h"
#include "Misc/Guid.h"
#include "UObject/UnrealType.h"
#include "Layout/Margin.h"
#include "Misc/Attribute.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Materials/MaterialInterface.h"
#include "MaterialEditor/DEditorFontParameterValue.h"
#include "MaterialEditor/DEditorMaterialLayersParameterValue.h"
#include "MaterialEditor/DEditorRuntimeVirtualTextureParameterValue.h"
#include "MaterialEditor/DEditorSparseVolumeTextureParameterValue.h"
#include "MaterialEditor/DEditorScalarParameterValue.h"
#include "MaterialEditor/DEditorStaticComponentMaskParameterValue.h"
#include "MaterialEditor/DEditorStaticSwitchParameterValue.h"
#include "MaterialEditor/DEditorTextureParameterValue.h"
#include "MaterialEditor/DEditorVectorParameterValue.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialExpressionFontSampleParameter.h"
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "MaterialShared.h"
#include "EditorSupportDelegates.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "IDetailPropertyRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialPropertyHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Curves/CurveLinearColor.h"
#include "IPropertyUtilities.h"
#include "MatHelper.h"
#include "Engine/Texture.h"
#include "HAL/PlatformApplicationMisc.h"

#include "MaterialInstanceEditor.h"
#include "SMaterialLayersFunctionsTree.h"
#include "TAccessPrivate.inl"

#define LOCTEXT_NAMESPACE "MaterialInstanceEditor"





TSharedRef<IDetailCustomization> FCusMaterialInstanceParameterDetails::MakeInstance(UMaterialEditorInstanceConstant* MaterialInstance)
{
	return MakeShareable(new FCusMaterialInstanceParameterDetails(MaterialInstance));
}


FCusMaterialInstanceParameterDetails::FCusMaterialInstanceParameterDetails(UMaterialEditorInstanceConstant* MaterialInstance)
{
	EnablePrams = false;
	MaterialEditorInstance = MaterialInstance;
	ShowHiddenDelegate = FCusGetShowHiddenParameters::CreateLambda([](bool& b)
	{
		b = false;
	});
}



TOptional<float> FCusMaterialInstanceParameterDetails::OnGetValue(TSharedRef<IPropertyHandle> PropertyHandle)
{
	float Value = 0.0f;
	if (PropertyHandle->GetValue(Value) == FPropertyAccess::Success)
	{
		return TOptional<float>(Value);
	}

	// Value couldn't be accessed. Return an unset value
	return TOptional<float>();
}

void FCusMaterialInstanceParameterDetails::OnValueCommitted(float NewValue, ETextCommit::Type CommitType, TSharedRef<IPropertyHandle> PropertyHandle)
{	
	// Try setting as float, if that fails then set as int
	ensure(PropertyHandle->SetValue(NewValue) == FPropertyAccess::Success);
}

FString FCusMaterialInstanceParameterDetails::GetFunctionParentPath() const
{
	FString PathString;
	if (MaterialEditorInstance->SourceFunction)
	{
		PathString = MaterialEditorInstance->SourceFunction->Parent->GetPathName();
	}
	return PathString;
}

void FCusMaterialInstanceParameterDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	PropertyUtilities = DetailLayout.GetPropertyUtilities();

	// Create a new category for a custom layout for the MIC parameters at the very top
	FName GroupsCategoryName = TEXT("ParameterGroups");
	IDetailCategoryBuilder& GroupsCategory = DetailLayout.EditCategory(GroupsCategoryName, LOCTEXT("MICParamGroupsTitle", "Parameter Groups"));
	TSharedRef<IPropertyHandle> ParameterGroupsProperty = DetailLayout.GetProperty("ParameterGroups");

	CreateGroupsWidget(ParameterGroupsProperty, GroupsCategory);

	// Create default category for class properties
	const FName DefaultCategoryName = NAME_None;
	IDetailCategoryBuilder& DefaultCategory = DetailLayout.EditCategory(DefaultCategoryName);
	DetailLayout.HideProperty("MaterialLayersParameterValues");
	if (MaterialEditorInstance->bIsFunctionPreviewMaterial)
	{
		// Customize Parent property so we can check for recursively set parents
		bool bShowParent = false;
		if(MaterialEditorInstance->SourceFunction->GetMaterialFunctionUsage() != EMaterialFunctionUsage::Default)
		{
			bShowParent = true;
		}
		if (bShowParent)
		{
			TSharedRef<IPropertyHandle> ParentPropertyHandle = DetailLayout.GetProperty("Parent");
			IDetailPropertyRow& ParentPropertyRow = DefaultCategory.AddProperty(ParentPropertyHandle);
			ParentPropertyHandle->MarkResetToDefaultCustomized();

			TSharedPtr<SWidget> NameWidget;
			TSharedPtr<SWidget> ValueWidget;
			FDetailWidgetRow Row;

			ParentPropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

			ParentPropertyHandle->ClearResetToDefaultCustomized();

			const bool bShowChildren = true;
			ParentPropertyRow.CustomWidget(bShowChildren)
				.NameContent()
				.MinDesiredWidth(Row.NameWidget.MinWidth)
				.MaxDesiredWidth(Row.NameWidget.MaxWidth)
				[
					NameWidget.ToSharedRef()
				]
			.ValueContent()
				.MinDesiredWidth(Row.ValueWidget.MinWidth)
				.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
				[
					SNew(SObjectPropertyEntryBox)
					.ObjectPath(this, &FCusMaterialInstanceParameterDetails::GetFunctionParentPath)
					.AllowedClass(UMaterialFunctionInterface::StaticClass())
					.ThumbnailPool(DetailLayout.GetThumbnailPool())
					.AllowClear(true)
					.OnObjectChanged(this, &FCusMaterialInstanceParameterDetails::OnAssetChanged, ParentPropertyHandle)
					.OnShouldSetAsset(this, &FCusMaterialInstanceParameterDetails::OnShouldSetAsset)
					.NewAssetFactories(TArray<UFactory*>())
				];

			ValueWidget.Reset();


		}
		else
		{
			DetailLayout.HideProperty("Parent");
		}

		DetailLayout.HideProperty("PhysMaterial");
		DetailLayout.HideProperty("LightmassSettings");
		DetailLayout.HideProperty("bUseOldStyleMICEditorGroups");
		DetailLayout.HideProperty("ParameterGroups");
		DetailLayout.HideProperty("RefractionDepthBias");
		DetailLayout.HideProperty("bOverrideSubsurfaceProfile");
		DetailLayout.HideProperty("SubsurfaceProfile");
		DetailLayout.HideProperty("BasePropertyOverrides");
	}
	else
	{
		// Add PhysMaterial property
		DefaultCategory.AddProperty("PhysMaterial");

		// Customize Parent property so we can check for recursively set parents
		TSharedRef<IPropertyHandle> ParentPropertyHandle = DetailLayout.GetProperty("Parent");
		IDetailPropertyRow& ParentPropertyRow = DefaultCategory.AddProperty(ParentPropertyHandle);

		ParentPropertyHandle->MarkResetToDefaultCustomized();
	
		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		FDetailWidgetRow Row;

		ParentPropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);
	
		ParentPropertyHandle->ClearResetToDefaultCustomized();

		const bool bShowChildren = true;
		ParentPropertyRow.CustomWidget(bShowChildren)
			.NameContent()
			.MinDesiredWidth(Row.NameWidget.MinWidth)
			.MaxDesiredWidth(Row.NameWidget.MaxWidth)
			[
				NameWidget.ToSharedRef()
			]
			.ValueContent()
			.MinDesiredWidth(Row.ValueWidget.MinWidth)
			.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
			[
				SNew(SObjectPropertyEntryBox)
				.PropertyHandle(ParentPropertyHandle)
				.AllowedClass(UMaterialInterface::StaticClass())
				.ThumbnailPool(DetailLayout.GetThumbnailPool())
				.AllowClear(true)
				.OnShouldSetAsset(this, &FCusMaterialInstanceParameterDetails::OnShouldSetAsset)
			];

		ValueWidget.Reset();


		// Add/hide other properties
		DetailLayout.HideProperty("LightmassSettings");
		CreateLightmassOverrideWidgets(DetailLayout);
		DetailLayout.HideProperty("bUseOldStyleMICEditorGroups");
		DetailLayout.HideProperty("ParameterGroups");

		{
			FIsResetToDefaultVisible IsRefractionDepthBiasPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
				float BiasValue;
				float ParentBiasValue;
				return MaterialEditorInstance->SourceInstance->GetRefractionSettings(BiasValue) 
					&& MaterialEditorInstance->Parent->GetRefractionSettings(ParentBiasValue)
					&& BiasValue != ParentBiasValue;
			});
			FResetToDefaultHandler ResetRefractionDepthBiasPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
				MaterialEditorInstance->Parent->GetRefractionSettings(MaterialEditorInstance->RefractionDepthBias);
			});
			FResetToDefaultOverride ResetRefractionDepthBiasPropertyOverride = FResetToDefaultOverride::Create(IsRefractionDepthBiasPropertyResetVisible, ResetRefractionDepthBiasPropertyHandler);
			IDetailPropertyRow& PropertyRow = DefaultCategory.AddProperty("RefractionDepthBias");
			PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::ShouldShowMaterialRefractionSettings)));
			PropertyRow.OverrideResetToDefault(ResetRefractionDepthBiasPropertyOverride);
		}

		{
			// Add the material property override group
			static FName GroupName(TEXT("MaterialPropertyOverrideGroup"));
			IDetailGroup& MaterialPropertyOverrideGroup = DefaultCategory.AddGroup(GroupName, LOCTEXT("MaterialPropertyOverrideGroup", "Material Property Overrides"), false, false);
			
			// Hide the originals, these will be recreated manually
			DetailLayout.HideProperty("bOverrideSubsurfaceProfile");
			DetailLayout.HideProperty("SubsurfaceProfile");
			DetailLayout.HideProperty("BasePropertyOverrides");

			// Set up the override logic for the subsurface profile
			TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this](){ return (bool)MaterialEditorInstance->bOverrideSubsurfaceProfile; }));

			IDetailPropertyRow& PropertyRow = MaterialPropertyOverrideGroup.AddPropertyRow(DetailLayout.GetProperty("SubsurfaceProfile"));
			PropertyRow
				.EditCondition(IsParamEnabled, 
					FOnBooleanValueChanged::CreateLambda([this](bool NewValue) {
						MaterialEditorInstance->bOverrideSubsurfaceProfile = (uint32)NewValue;
						MaterialEditorInstance->PostEditChange();
						FEditorSupportDelegates::RedrawAllViewports.Broadcast();
				}))
				.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::ShouldShowSubsurfaceProfile)));
			
			// Append the base property overrides to the Material Property Override Group
			CreateBasePropertyOverrideWidgets(DetailLayout, MaterialPropertyOverrideGroup);

			// Append the nanite material override.
			MaterialPropertyOverrideGroup.AddPropertyRow(DetailLayout.GetProperty("NaniteOverrideMaterial"));
		}
	}

	// Add the preview mesh property directly from the material instance 
	FName PreviewingCategoryName = TEXT("Previewing");
	IDetailCategoryBuilder& PreviewingCategory = DetailLayout.EditCategory(PreviewingCategoryName, LOCTEXT("MICPreviewingCategoryTitle", "Previewing"));

	TArray<UObject*> ExternalObjects;
	ExternalObjects.Add(MaterialEditorInstance->SourceInstance);

	PreviewingCategory.AddExternalObjectProperty(ExternalObjects, TEXT("PreviewMesh"));

	DefaultCategory.AddExternalObjectProperty(ExternalObjects, TEXT("AssetUserData"), EPropertyLocation::Advanced);
}

void FCusMaterialInstanceParameterDetails::CreateGroupsWidget(TSharedRef<IPropertyHandle> ParameterGroupsProperty, IDetailCategoryBuilder& GroupsCategory)
{
	bool bShowSaveButtons = false;
	check(MaterialEditorInstance);
	for (int32 GroupIdx = 0; GroupIdx < MaterialEditorInstance->ParameterGroups.Num(); ++GroupIdx)
	{
		FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[GroupIdx];
		if (ParameterGroup.GroupAssociation == EMaterialParameterAssociation::GlobalParameter
			&& ParameterGroup.GroupName != FMaterialPropertyHelpers::LayerParamName)
		{
			bShowSaveButtons = true;
			bool bCreateGroup = false;
			for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num() && !bCreateGroup; ++ParamIdx)
			{
				UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
				const bool bIsVisible = MaterialEditorInstance->VisibleExpressions.Contains(Parameter->ParameterInfo);
				bCreateGroup = bIsVisible && (!MaterialEditorInstance->bShowOnlyOverrides || FMaterialPropertyHelpers::IsOverriddenExpression(Parameter));
			}
			
			if (bCreateGroup)
			{
				IDetailGroup& DetailGroup = GroupsCategory.AddGroup(ParameterGroup.GroupName, FText::FromName(ParameterGroup.GroupName), false, false);
				FUIAction CopyAction(
					FExecuteAction::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnCopyParameterValues, GroupIdx),
					FCanExecuteAction::CreateSP(this, &FCusMaterialInstanceParameterDetails::CanCopyParameterValues, GroupIdx));
				FUIAction PasteAction(
					FExecuteAction::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnPasteParameterValues, GroupIdx),
					FCanExecuteAction::CreateSP(this, &FCusMaterialInstanceParameterDetails::CanPasteParameterValues, GroupIdx));
				FDetailWidgetRow& HeaderRow = DetailGroup.HeaderRow()
					.CopyAction(CopyAction)
					.PasteAction(PasteAction)
					.NameContent()
					[
						SNew(STextBlock)
						.Text(FText::FromName(DetailGroup.GetGroupName()))
					];

				CreateSingleGroupWidget(ParameterGroup, ParameterGroupsProperty->GetChildHandle(GroupIdx), DetailGroup);

				HeaderRow.AddCustomContextMenuAction(FUIAction(
						FExecuteAction::CreateLambda([&]()mutable 
						{
							EnableGroupParameters(ParameterGroup, true);
						})),
							LOCTEXT("ToggleParametersEnable", "Enable All Parameters"),
							LOCTEXT("ToggleParametersEnableTooltip", "Enable All Parameters in group"),
							FSlateIcon());

				HeaderRow.AddCustomContextMenuAction(FUIAction(
						FExecuteAction::CreateLambda([&]()mutable 
						{
							EnableGroupParameters(ParameterGroup, false);
						})),
							LOCTEXT("ToggleParametersDisable", "Disable All Parameters"),
							LOCTEXT("ToggleParametersDisableTooltip", "Disable All Parameters in group"),
							FSlateIcon());
			}
		}
	}
	if (bShowSaveButtons)
	{
		FDetailWidgetRow& SaveInstanceRow = GroupsCategory.AddCustomRow(LOCTEXT("SaveInstances", "Save Instances"));
		FOnClicked OnChildButtonClicked;
		FOnClicked OnSiblingButtonClicked;
		UMaterialInterface* LocalSourceInstance = MaterialEditorInstance->SourceInstance;
		UObject* LocalEditorInstance = MaterialEditorInstance;
		if (!MaterialEditorInstance->bIsFunctionPreviewMaterial)
		{
			OnChildButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, LocalSourceInstance, LocalEditorInstance);
			OnSiblingButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, ToRawPtr(MaterialEditorInstance->SourceInstance->Parent), LocalEditorInstance);
		}
		else
		{
			OnChildButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewFunctionInstance, 
				ImplicitConv<UMaterialFunctionInterface*>(MaterialEditorInstance->SourceFunction), LocalSourceInstance, LocalEditorInstance);
			OnSiblingButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewFunctionInstance,
				ImplicitConv<UMaterialFunctionInterface*>(MaterialEditorInstance->SourceFunction->Parent), LocalSourceInstance, LocalEditorInstance);
		}
		SaveInstanceRow.ValueContent()
			.HAlign(HAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNullWidget::NullWidget
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("SaveSibling", "Save Sibling"))
						.HAlign(HAlign_Center)
						.OnClicked(OnSiblingButtonClicked)
						.ToolTipText(LOCTEXT("SaveToSiblingInstance", "Save to Sibling Instance"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("SaveChild", "Save Child"))
						.HAlign(HAlign_Center)
						.OnClicked(OnChildButtonClicked)
						.ToolTipText(LOCTEXT("SaveToChildInstance", "Save to Child Instance"))
					]
				]
				// T Hook
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNullWidget::NullWidget
					]
					
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f)
					[
						SNew(SButton)
						.Text(FText::FromString("Enable Params"))
						.HAlign(HAlign_Center)
						.ToolTipText(LOCTEXT("SaveToChildInstance", "Enable/Disable Params"))
						.OnClicked_Lambda([&]()
						{
							EnablePrams = !EnablePrams;
							for (int32 GroupIdx = 0; GroupIdx < MaterialEditorInstance->ParameterGroups.Num(); ++GroupIdx)
							{
								FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[GroupIdx];
								for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ++ParamIdx)
								{
									UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
									FMaterialPropertyHelpers::OnOverrideParameter(EnablePrams,Parameter,MaterialEditorInstance);
								}
							}
							return FReply::Handled();
						})
					]
					
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f)
					[
						SNew(SButton)
						.Text(FText::FromString("Scene View"))
						.HAlign(HAlign_Center)
						.ToolTipText(LOCTEXT("SaveToChildInstance", "Scene View"))
						.OnClicked_Lambda([&]()
						{
							FGlobalTabmanager::Get()->TryInvokeTab(FMatHelperModule::MaterialInstanceSceneViewEditorTabName);
							return FReply::Handled();
						})
					]
				]
			];
	}
}

void FCusMaterialInstanceParameterDetails::EnableGroupParameters(FEditorParameterGroup& ParameterGroup, bool ShouldEnable)
{
	// loop through each parameter in the group and toggle to enable/disable them all
	for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ++ParamIdx)
	{
		UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
		Parameter->bOverride = ShouldEnable;
	}
}

void FCusMaterialInstanceParameterDetails::CreateSingleGroupWidget(FEditorParameterGroup& ParameterGroup, TSharedPtr<IPropertyHandle> ParameterGroupProperty, IDetailGroup& DetailGroup )
{
	TSharedPtr<IPropertyHandle> ParametersArrayProperty = ParameterGroupProperty->GetChildHandle("Parameters");

	// Create a custom widget for each parameter in the group
	for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ++ParamIdx)
	{
		TSharedPtr<IPropertyHandle> ParameterProperty = ParametersArrayProperty->GetChildHandle(ParamIdx);

		UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
		UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
		UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(Parameter);
		UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
		UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
		UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
		UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
		UDEditorRuntimeVirtualTextureParameterValue* RuntimeVirtualTextureParam = Cast<UDEditorRuntimeVirtualTextureParameterValue>(Parameter);
		UDEditorSparseVolumeTextureParameterValue* SparseVolumeTextureParam = Cast<UDEditorSparseVolumeTextureParameterValue>(Parameter);
		UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);
	
		if (Parameter->ParameterInfo.Association == EMaterialParameterAssociation::GlobalParameter)
		{
			if (VectorParam && VectorParam->bIsUsedAsChannelMask)
			{
				CreateVectorChannelMaskParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
			}
			if (ScalarParam && ScalarParam->AtlasData.bIsUsedAsAtlasPosition)
			{
				CreateScalarAtlasPositionParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
			}
			if (TextureParam && 
				( !TextureParam->ChannelNames.R.IsEmpty()
				|| !TextureParam->ChannelNames.G.IsEmpty()
				|| !TextureParam->ChannelNames.B.IsEmpty()
				|| !TextureParam->ChannelNames.A.IsEmpty()))
			{
				CreateLabeledTextureParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
			}
			else if (LayersParam)
			{
			}
			else if (CompMaskParam)
			{
				CreateMaskParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
			}
			else
			{
				if (ScalarParam && ScalarParam->SliderMax > ScalarParam->SliderMin)
				{
					TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");
					ParameterValueProperty->SetInstanceMetaData("UIMin", FString::Printf(TEXT("%f"), ScalarParam->SliderMin));
					ParameterValueProperty->SetInstanceMetaData("UIMax", FString::Printf(TEXT("%f"), ScalarParam->SliderMax));
				}

				if (VectorParam)
				{
					static const FName Red("R");
					static const FName Green("G");
					static const FName Blue("B");
					static const FName Alpha("A");
					if (!VectorParam->ChannelNames.R.IsEmpty())
					{
						ParameterProperty->GetChildHandle(Red)->SetPropertyDisplayName(VectorParam->ChannelNames.R);
					}
					if (!VectorParam->ChannelNames.G.IsEmpty())
					{
						ParameterProperty->GetChildHandle(Green)->SetPropertyDisplayName(VectorParam->ChannelNames.G);
					}
					if (!VectorParam->ChannelNames.B.IsEmpty())
					{
						ParameterProperty->GetChildHandle(Blue)->SetPropertyDisplayName(VectorParam->ChannelNames.B);
					}
					if (!VectorParam->ChannelNames.A.IsEmpty())
					{
						ParameterProperty->GetChildHandle(Alpha)->SetPropertyDisplayName(VectorParam->ChannelNames.A);
					}
				}

				CreateParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
			}
		}
	}
}

void FCusMaterialInstanceParameterDetails::CreateParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup)
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");
	
	if (ParameterValueProperty->IsValidHandle())
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, Parameter));

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow(ParameterValueProperty.ToSharedRef());

		TAttribute<bool> IsResetVisible = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowResetToDefault, Parameter, MaterialEditorInstance));
		FSimpleDelegate ResetHandler = FSimpleDelegate::CreateStatic(&FMaterialPropertyHelpers::ResetToDefault, Parameter, MaterialEditorInstance);
		FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);

		PropertyRow
			.DisplayName(FText::FromName(Parameter->ParameterInfo.Name))
			.ToolTip(FMaterialPropertyHelpers::GetParameterTooltip(Parameter, MaterialEditorInstance))
			.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, Parameter, MaterialEditorInstance))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowExpression, Parameter, MaterialEditorInstance, ShowHiddenDelegate)))
			.OverrideResetToDefault(ResetOverride);

		// Textures need a special widget that filters based on VT or not
		UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
		if (TextureParam != nullptr)
		{
			UMaterial *Material = MaterialEditorInstance->SourceInstance->GetMaterial();
			if (Material != nullptr)
			{
				UMaterialExpressionTextureSampleParameter* Expression = Material->FindExpressionByGUID<UMaterialExpressionTextureSampleParameter>(TextureParam->ExpressionId);
				if (Expression != nullptr)
				{
					TWeakObjectPtr<UMaterialExpressionTextureSampleParameter> SamplerExpression = Expression;

					PropertyRow.CustomWidget()
					.NameContent()
					[
						ParameterValueProperty->CreatePropertyNameWidget()
					]
					.ValueContent()
					.MaxDesiredWidth(TOptional<float>())
					[
						SNew(SObjectPropertyEntryBox)
						.PropertyHandle(ParameterValueProperty)
						.AllowedClass(UTexture::StaticClass())
						.ThumbnailPool(PropertyUtilities.Pin()->GetThumbnailPool())
						.OnShouldFilterAsset_Lambda([SamplerExpression](const FAssetData& AssetData)
						{
							if (SamplerExpression.Get())
							{
								bool VirtualTextured = false;
								AssetData.GetTagValue<bool>("VirtualTextureStreaming", VirtualTextured);

								bool ExpressionIsVirtualTextured = IsVirtualSamplerType(SamplerExpression->SamplerType);

								return VirtualTextured != ExpressionIsVirtualTextured;
							}
							else
							{
								return false;
							}
						})
					];
				}
			}
		}
	}
}

void FCusMaterialInstanceParameterDetails::CreateMaskParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup )
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");
	TSharedPtr<IPropertyHandle> RMaskProperty = ParameterValueProperty->GetChildHandle("R");
	TSharedPtr<IPropertyHandle> GMaskProperty = ParameterValueProperty->GetChildHandle("G");
	TSharedPtr<IPropertyHandle> BMaskProperty = ParameterValueProperty->GetChildHandle("B");
	TSharedPtr<IPropertyHandle> AMaskProperty = ParameterValueProperty->GetChildHandle("A");

	if (ParameterValueProperty->IsValidHandle())
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, Parameter));

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow(ParameterValueProperty.ToSharedRef());
		PropertyRow.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, Parameter, MaterialEditorInstance));
		// Handle reset to default manually
		PropertyRow.OverrideResetToDefault(FResetToDefaultOverride::Create(FSimpleDelegate::CreateStatic(&FMaterialPropertyHelpers::ResetToDefault, Parameter, MaterialEditorInstance)));
		PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowExpression, Parameter, MaterialEditorInstance, ShowHiddenDelegate)));

		const FText ParameterName = FText::FromName(Parameter->ParameterInfo.Name);

		FDetailWidgetRow& CustomWidget = PropertyRow.CustomWidget();
		CustomWidget
			.FilterString(ParameterName)
			.NameContent()
			[
				SNew(STextBlock)
				.Text(ParameterName)
				.ToolTipText(FMaterialPropertyHelpers::GetParameterTooltip(Parameter, MaterialEditorInstance))
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		.ValueContent()
			.MaxDesiredWidth(200.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				RMaskProperty->CreatePropertyNameWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				RMaskProperty->CreatePropertyValueWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
			.AutoWidth()
			[
				GMaskProperty->CreatePropertyNameWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				GMaskProperty->CreatePropertyValueWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
			.AutoWidth()
			[
				BMaskProperty->CreatePropertyNameWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				BMaskProperty->CreatePropertyValueWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
			.AutoWidth()
			[
				AMaskProperty->CreatePropertyNameWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				AMaskProperty->CreatePropertyValueWidget()
			]
			]
			];
	}
}

void FCusMaterialInstanceParameterDetails::CreateVectorChannelMaskParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup)
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");

	if (ParameterValueProperty->IsValidHandle())
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, Parameter));

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow(ParameterValueProperty.ToSharedRef());
		PropertyRow.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, Parameter, MaterialEditorInstance));
		// Handle reset to default manually
		PropertyRow.OverrideResetToDefault(FResetToDefaultOverride::Create(FSimpleDelegate::CreateStatic(&FMaterialPropertyHelpers::ResetToDefault, Parameter, MaterialEditorInstance)));
		PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowExpression, Parameter, MaterialEditorInstance, ShowHiddenDelegate)));

		const FText ParameterName = FText::FromName(Parameter->ParameterInfo.Name);

		// Combo box hooks for converting between our "enum" and colors
		FOnGetPropertyComboBoxStrings GetMaskStrings = FOnGetPropertyComboBoxStrings::CreateStatic(&FMaterialPropertyHelpers::GetVectorChannelMaskComboBoxStrings);
		FOnGetPropertyComboBoxValue GetMaskValue = FOnGetPropertyComboBoxValue::CreateStatic(&FMaterialPropertyHelpers::GetVectorChannelMaskValue, Parameter);
		FOnPropertyComboBoxValueSelected SetMaskValue = FOnPropertyComboBoxValueSelected::CreateStatic(&FMaterialPropertyHelpers::SetVectorChannelMaskValue, ParameterValueProperty, Parameter, (UObject*)MaterialEditorInstance);

		// Widget replaces color picker with combo box
		FDetailWidgetRow& CustomWidget = PropertyRow.CustomWidget();
		CustomWidget
		.FilterString(ParameterName)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(ParameterName)
			.ToolTipText(FMaterialPropertyHelpers::GetParameterTooltip(Parameter, MaterialEditorInstance))
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
		]
		.ValueContent()
		.MaxDesiredWidth(200.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					PropertyCustomizationHelpers::MakePropertyComboBox(ParameterValueProperty, GetMaskStrings, GetMaskValue, SetMaskValue)
				]
			]
		];
	}
}

void FCusMaterialInstanceParameterDetails::CreateScalarAtlasPositionParameterValueWidget(class UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup)
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");

	if (ParameterValueProperty->IsValidHandle())
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, Parameter));

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow(ParameterValueProperty.ToSharedRef());
		PropertyRow.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, Parameter, MaterialEditorInstance));
		// Handle reset to default manually
		PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowExpression, Parameter, MaterialEditorInstance, ShowHiddenDelegate)));

		const FText ParameterName = FText::FromName(Parameter->ParameterInfo.Name);
		UDEditorScalarParameterValue* AtlasParameter = Cast<UDEditorScalarParameterValue>(Parameter);

		TAttribute<bool> IsResetVisible = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowResetToDefault, Parameter, MaterialEditorInstance));
		FSimpleDelegate ResetHandler = FSimpleDelegate::CreateStatic(&FMaterialPropertyHelpers::ResetCurveToDefault, Parameter, MaterialEditorInstance);
		FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);

		PropertyRow.OverrideResetToDefault(ResetOverride);

		FDetailWidgetRow& CustomWidget = PropertyRow.CustomWidget();
		CustomWidget
			.FilterString(ParameterName)
			.NameContent()
			[
				SNew(STextBlock)
				.Text(ParameterName)
				.ToolTipText(FMaterialPropertyHelpers::GetParameterTooltip(Parameter, MaterialEditorInstance))
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
			.ValueContent()
			.HAlign(HAlign_Fill)
			.MaxDesiredWidth(400.0f)
			[
				SNew(SObjectPropertyEntryBox)
				.ObjectPath(this, &FCusMaterialInstanceParameterDetails::GetCurvePath, AtlasParameter)
				.AllowedClass(UCurveLinearColor::StaticClass())
				.NewAssetFactories(TArray<UFactory*>())
				.DisplayThumbnail(true)
				.ThumbnailPool(PropertyUtilities.Pin()->GetThumbnailPool())
				.OnShouldFilterAsset(FOnShouldFilterAsset::CreateStatic(&FMaterialPropertyHelpers::OnShouldFilterCurveAsset, AtlasParameter->AtlasData.Atlas))
				.OnShouldSetAsset(FOnShouldSetAsset::CreateStatic(&FMaterialPropertyHelpers::OnShouldSetCurveAsset, AtlasParameter->AtlasData.Atlas))
				.OnObjectChanged(FOnSetObject::CreateStatic(&FMaterialPropertyHelpers::SetPositionFromCurveAsset, AtlasParameter->AtlasData.Atlas, AtlasParameter, ParameterProperty, (UObject*)MaterialEditorInstance))
				.DisplayCompactSize(true)
			];
	}
}

void FCusMaterialInstanceParameterDetails::CreateLabeledTextureParameterValueWidget(class UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup)
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");

	if (ParameterValueProperty->IsValidHandle())
	{
		UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
		if (TextureParam)
		{
			UMaterial *Material = MaterialEditorInstance->SourceInstance->GetMaterial();
			if (Material != nullptr)
			{
				UMaterialExpressionTextureSampleParameter* Expression = Material->FindExpressionByGUID<UMaterialExpressionTextureSampleParameter>(TextureParam->ExpressionId);
				if (Expression != nullptr)
				{
					TWeakObjectPtr<UMaterialExpressionTextureSampleParameter> SamplerExpression = Expression;
					TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, Parameter));

					IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow(ParameterValueProperty.ToSharedRef());

					TAttribute<bool> IsResetVisible = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowResetToDefault, Parameter, MaterialEditorInstance));
					FSimpleDelegate ResetHandler = FSimpleDelegate::CreateStatic(&FMaterialPropertyHelpers::ResetToDefault, Parameter, MaterialEditorInstance);
					FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);

					PropertyRow
						.DisplayName(FText::FromName(Parameter->ParameterInfo.Name))
						.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, Parameter, MaterialEditorInstance))
						.ToolTip(FMaterialPropertyHelpers::GetParameterTooltip(Parameter, MaterialEditorInstance))
						.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowExpression, Parameter, MaterialEditorInstance, ShowHiddenDelegate)));


					TSharedPtr<SWidget> NameWidget;
					TSharedPtr<SWidget> ValueWidget;
					FDetailWidgetRow Row;
					PropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

					FDetailWidgetRow &DetailWidgetRow = PropertyRow.CustomWidget();
					TSharedPtr<SVerticalBox> NameVerticalBox;
					DetailWidgetRow.NameContent()
						[
							SAssignNew(NameVerticalBox, SVerticalBox)
							+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromName(Parameter->ParameterInfo.Name))
						.ToolTipText(FMaterialPropertyHelpers::GetParameterTooltip(Parameter, MaterialEditorInstance))
						.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
						]
						];
					DetailWidgetRow.ValueContent()
						.MinDesiredWidth(Row.ValueWidget.MinWidth)
						.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
						[
							SNew(SObjectPropertyEntryBox)
							.PropertyHandle(ParameterValueProperty)
							.AllowedClass(UTexture::StaticClass())
							.ThumbnailPool(PropertyUtilities.Pin()->GetThumbnailPool())
							.OnShouldFilterAsset_Lambda([SamplerExpression](const FAssetData& AssetData)
							{
								if (SamplerExpression.Get())
								{
									bool VirtualTextured = false;
									AssetData.GetTagValue<bool>("VirtualTextureStreaming", VirtualTextured);

									bool ExpressionIsVirtualTextured = IsVirtualSamplerType(SamplerExpression->SamplerType);

									return VirtualTextured != ExpressionIsVirtualTextured;
								}
								else
								{
									return false;
								}
							})
						];

					DetailWidgetRow.OverrideResetToDefault(ResetOverride);

					static const FName Red("R");
					static const FName Green("G");
					static const FName Blue("B");
					static const FName Alpha("A");

					if (!TextureParam->ChannelNames.R.IsEmpty())
					{
						NameVerticalBox->AddSlot()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(20.0, 2.0, 4.0, 2.0)
								[
									SNew(STextBlock)
									.Text(FText::FromName(Red))
									.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
								]
								+ SHorizontalBox::Slot()
								.HAlign(HAlign_Left)
								.Padding(4.0, 2.0)
								[
									SNew(STextBlock)
									.Text(TextureParam->ChannelNames.R)
									.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
								]
							];
					}
					if (!TextureParam->ChannelNames.G.IsEmpty())
					{
						NameVerticalBox->AddSlot()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
							.Padding(20.0, 2.0, 4.0, 2.0)
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(FText::FromName(Green))
							.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
							]
						+ SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.Padding(4.0, 2.0)
							[
								SNew(STextBlock)
								.Text(TextureParam->ChannelNames.G)
							.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
							]
							];
					}
					if (!TextureParam->ChannelNames.B.IsEmpty())
					{
						NameVerticalBox->AddSlot()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
							.Padding(20.0, 2.0, 4.0, 2.0)
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(FText::FromName(Blue))
							.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
							]
						+ SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.Padding(4.0, 2.0)
							[
								SNew(STextBlock)
								.Text(TextureParam->ChannelNames.B)
							.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
							]
							];
					}
					if (!TextureParam->ChannelNames.A.IsEmpty())
					{
						NameVerticalBox->AddSlot()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
							.Padding(20.0, 2.0, 4.0, 2.0)
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(FText::FromName(Alpha))
							.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
							]
						+ SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.Padding(4.0, 2.0)
							[
								SNew(STextBlock)
								.Text(TextureParam->ChannelNames.A)
							.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
							]
							];
					}
				}
			}
		}
	}
}

FString FCusMaterialInstanceParameterDetails::GetCurvePath(UDEditorScalarParameterValue* Parameter) const
{
	FString Path = Parameter->AtlasData.Curve->GetPathName();
	return Path;
}

bool FCusMaterialInstanceParameterDetails::IsVisibleExpression(UDEditorParameterValue* Parameter)
{
	return MaterialEditorInstance->VisibleExpressions.Contains(Parameter->ParameterInfo);
}

EVisibility FCusMaterialInstanceParameterDetails::ShouldShowExpression(UDEditorParameterValue* Parameter) const
{
	return FMaterialPropertyHelpers::ShouldShowExpression(Parameter, MaterialEditorInstance, ShowHiddenDelegate);
}

bool FCusMaterialInstanceParameterDetails::OnShouldSetAsset(const FAssetData& AssetData) const
{
	if (MaterialEditorInstance->bIsFunctionPreviewMaterial)
	{
		if (MaterialEditorInstance->SourceFunction->GetMaterialFunctionUsage() == EMaterialFunctionUsage::Default)
		{
			return false;
		}
		else
		{
			UMaterialFunctionInstance* FunctionInstance = Cast<UMaterialFunctionInstance>(AssetData.GetAsset());
			if (FunctionInstance != nullptr)
			{
				bool bIsChild = FunctionInstance->IsDependent(MaterialEditorInstance->SourceFunction);
				if (bIsChild)
				{
					FMessageDialog::Open(
						EAppMsgType::Ok,
						FText::Format(LOCTEXT("CannotSetExistingChildFunctionAsParent", "Cannot set {0} as a parent as it is already a child of this material function instance."), FText::FromName(AssetData.AssetName)));
				}
				return !bIsChild;
			}
		}
	}

	UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(AssetData.GetAsset());

	if (MaterialInstance != nullptr)
	{
		bool bIsChild = MaterialInstance->IsChildOf(MaterialEditorInstance->SourceInstance);
		if (bIsChild)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				FText::Format(LOCTEXT("CannotSetExistingChildAsParent", "Cannot set {0} as a parent as it is already a child of this material instance."), FText::FromName(AssetData.AssetName)));
		}

		if (bIsChild)
		{
			return false;
		}
	}

	return true;
}

void FCusMaterialInstanceParameterDetails::OnAssetChanged(const FAssetData & InAssetData, TSharedRef<IPropertyHandle> InHandle)
{
	if (MaterialEditorInstance->bIsFunctionPreviewMaterial &&
		MaterialEditorInstance->SourceFunction->GetMaterialFunctionUsage() != EMaterialFunctionUsage::Default)
	{
		UMaterialFunctionInterface* NewParent = Cast<UMaterialFunctionInterface>(InAssetData.GetAsset());
		if (NewParent != nullptr)
		{
			MaterialEditorInstance->SourceFunction->SetParent(NewParent);
			FPropertyChangedEvent ParentChanged = FPropertyChangedEvent(InHandle->GetProperty());
			MaterialEditorInstance->PostEditChangeProperty(ParentChanged);
		}
	}
}

EVisibility FCusMaterialInstanceParameterDetails::ShouldShowMaterialRefractionSettings() const
{
	return (MaterialEditorInstance->SourceInstance->GetMaterial()->bUsesDistortion && IsTranslucentBlendMode(*MaterialEditorInstance->SourceInstance)) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FCusMaterialInstanceParameterDetails::ShouldShowSubsurfaceProfile() const
{
	FMaterialShadingModelField ShadingModels = MaterialEditorInstance->SourceInstance->GetShadingModels();

	return UseSubsurfaceProfile(ShadingModels) ? EVisibility::Visible : EVisibility::Collapsed;
}

void FCusMaterialInstanceParameterDetails::OnCopyParameterValues(int32 ParameterGroupIndex)
{
	if (!MaterialEditorInstance || !MaterialEditorInstance->ParameterGroups.IsValidIndex(ParameterGroupIndex))
	{
		return;
	}
	FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[ParameterGroupIndex];

	TStringBuilder<4096> CombinedValue;

	const int32 NumParams = ParameterGroup.Parameters.Num();
	for (int32 ParamIdx = 0; ParamIdx < NumParams; ++ParamIdx)
	{
		UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
		const FName ParamName = Parameter->ParameterInfo.Name;

		const TCHAR* Prefix = (ParamIdx == 0) ? TEXT("") : TEXT(",");

		// Include the value in the result entry only if the parameter is overridden.
		const bool bOverride = FMaterialPropertyHelpers::IsOverriddenExpression(Parameter);
		if (bOverride)
		{
			FProperty* ParameterValueProperty = Parameter->GetClass()->FindPropertyByName("ParameterValue");
			if (ParameterValueProperty != nullptr)
			{
				FString ParameterValueString;
				if (ParameterValueProperty->ExportText_InContainer(0, ParameterValueString, Parameter, Parameter, Parameter, PPF_Copy))
				{
					CombinedValue.Appendf(TEXT("%s%s.Override=True,%s.Value=\"%s\""),
						Prefix,
						*(ParamName.ToString()),
						*(ParamName.ToString()),
						*(ParameterValueString.ReplaceCharWithEscapedChar()));
				}
			}
		}
		else
		{
			CombinedValue.Appendf(TEXT("%s%s.Override=False"), Prefix, *(ParamName.ToString()));
		}
	}

	if (CombinedValue.Len())
	{
		// Copy.
		FPlatformApplicationMisc::ClipboardCopy(*CombinedValue);
	}
}

bool FCusMaterialInstanceParameterDetails::CanCopyParameterValues(int32 ParameterGroupIndex)
{
	return MaterialEditorInstance && MaterialEditorInstance->ParameterGroups.IsValidIndex(ParameterGroupIndex)
		&& (MaterialEditorInstance->ParameterGroups[ParameterGroupIndex].Parameters.Num() > 0);
}

void FCusMaterialInstanceParameterDetails::OnPasteParameterValues(int32 ParameterGroupIndex)
{
	if (!MaterialEditorInstance || !MaterialEditorInstance->ParameterGroups.IsValidIndex(ParameterGroupIndex))
	{
		return;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	if (!ClipboardContent.IsEmpty())
	{
		FScopedTransaction Transaction(LOCTEXT("PasteMaterialInstanceParameters", "Paste Material Instance Parameters"));

		MaterialEditorInstance->Modify();

		for (int32 ParamIdx = 0; ParamIdx < MaterialEditorInstance->ParameterGroups[ParameterGroupIndex].Parameters.Num(); ++ParamIdx)
		{
			UDEditorParameterValue* Parameter = MaterialEditorInstance->ParameterGroups[ParameterGroupIndex].Parameters[ParamIdx];
			Parameter->Modify();

			const FName ParamName = Parameter->ParameterInfo.Name;

			const FString OverrideKey = FString::Printf(TEXT("%s.Override="), *(ParamName.ToString()));
			bool bParsedOverride = false;
			if (FParse::Bool(*ClipboardContent, *OverrideKey, bParsedOverride))
			{
				Parameter->bOverride = bParsedOverride;
				if (bParsedOverride)
				{
					// Paste value.
					const FString ValueKey = FString::Printf(TEXT("%s.Value="), *(ParamName.ToString()));
					FString ParsedValueString;
					if (FParse::Value(*ClipboardContent, *ValueKey, ParsedValueString))
					{
						ParsedValueString = ParsedValueString.ReplaceEscapedCharWithChar();
						FProperty* ParameterValueProperty = Parameter->GetClass()->FindPropertyByName("ParameterValue");
						if (ParameterValueProperty != nullptr)
						{
							ParameterValueProperty->ImportText_InContainer(*ParsedValueString, Parameter, Parameter, PPF_Copy);
						}
					}
				}
			}
		}

		MaterialEditorInstance->PostEditChange();
		FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	}
}

bool FCusMaterialInstanceParameterDetails::CanPasteParameterValues(int32 ParameterGroupIndex)
{
	// First check the same criteria as copying.
	if (!CanCopyParameterValues(ParameterGroupIndex))
	{
		return false;
	}

	// Now see if there's anything to paste from the clipboard.
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	return !ClipboardContent.IsEmpty();
}

void FCusMaterialInstanceParameterDetails::CreateLightmassOverrideWidgets(IDetailLayoutBuilder& DetailLayout)
{
	IDetailCategoryBuilder& DetailCategory = DetailLayout.EditCategory(NAME_None);

	static FName GroupName(TEXT("LightmassSettings"));
	IDetailGroup& LightmassSettingsGroup = DetailCategory.AddGroup(GroupName, LOCTEXT("LightmassSettingsGroup", "Lightmass Settings"), false, false);

	TAttribute<bool> IsOverrideCastShadowAsMaskedEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this] { return (bool)MaterialEditorInstance->LightmassSettings.CastShadowAsMasked.bOverride; }));
	TAttribute<bool> IsOverrideEmissiveBoostEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this] { return (bool)MaterialEditorInstance->LightmassSettings.EmissiveBoost.bOverride; }));
	TAttribute<bool> IsOverrideDiffuseBoostEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this] { return (bool)MaterialEditorInstance->LightmassSettings.DiffuseBoost.bOverride; }));
	TAttribute<bool> IsOverrideExportResolutionScaleEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this] { return (bool)MaterialEditorInstance->LightmassSettings.ExportResolutionScale.bOverride; }));
	
	TSharedRef<IPropertyHandle> LightmassSettings = DetailLayout.GetProperty("LightmassSettings");
	TSharedPtr<IPropertyHandle> CastShadowAsMaskedProperty = LightmassSettings->GetChildHandle("CastShadowAsMasked");
	TSharedPtr<IPropertyHandle> EmissiveBoostProperty = LightmassSettings->GetChildHandle("EmissiveBoost");
	TSharedPtr<IPropertyHandle> DiffuseBoostProperty = LightmassSettings->GetChildHandle("DiffuseBoost");
	TSharedPtr<IPropertyHandle> ExportResolutionScaleProperty = LightmassSettings->GetChildHandle("ExportResolutionScale");

	
	FIsResetToDefaultVisible IsCastShadowAsMaskedPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
		return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->LightmassSettings.CastShadowAsMasked.ParameterValue != MaterialEditorInstance->Parent->GetCastShadowAsMasked() : false;
	});
	FResetToDefaultHandler ResetCastShadowAsMaskedPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
		if (MaterialEditorInstance->Parent != nullptr)
		{
			MaterialEditorInstance->LightmassSettings.CastShadowAsMasked.ParameterValue = MaterialEditorInstance->Parent->GetCastShadowAsMasked();
		}
	});
	FResetToDefaultOverride ResetCastShadowAsMaskedPropertyOverride = FResetToDefaultOverride::Create(IsCastShadowAsMaskedPropertyResetVisible, ResetCastShadowAsMaskedPropertyHandler);
	IDetailPropertyRow& CastShadowAsMaskedPropertyRow = LightmassSettingsGroup.AddPropertyRow(CastShadowAsMaskedProperty->GetChildHandle(0).ToSharedRef());
	CastShadowAsMaskedPropertyRow
		.DisplayName(CastShadowAsMaskedProperty->GetPropertyDisplayName())
		.ToolTip(CastShadowAsMaskedProperty->GetToolTipText())
		.EditCondition(IsOverrideCastShadowAsMaskedEnabled, FOnBooleanValueChanged::CreateLambda([this](bool NewValue) {
		MaterialEditorInstance->LightmassSettings.CastShadowAsMasked.bOverride = (uint32)NewValue;
		MaterialEditorInstance->PostEditChange();
		FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	}))
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideCastShadowAsMaskedEnabled)))
		.OverrideResetToDefault(ResetCastShadowAsMaskedPropertyOverride);

	FIsResetToDefaultVisible IsEmissiveBoostPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
		return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->LightmassSettings.EmissiveBoost.ParameterValue != MaterialEditorInstance->Parent->GetEmissiveBoost() : false;
	});
	FResetToDefaultHandler ResetEmissiveBoostPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
		if (MaterialEditorInstance->Parent != nullptr)
		{
			MaterialEditorInstance->LightmassSettings.EmissiveBoost.ParameterValue = MaterialEditorInstance->Parent->GetEmissiveBoost();
		}
	});
	FResetToDefaultOverride ResetEmissiveBoostPropertyOverride = FResetToDefaultOverride::Create(IsEmissiveBoostPropertyResetVisible, ResetEmissiveBoostPropertyHandler);
	IDetailPropertyRow& EmissiveBoostPropertyRow = LightmassSettingsGroup.AddPropertyRow(EmissiveBoostProperty->GetChildHandle(0).ToSharedRef());
	EmissiveBoostPropertyRow
		.DisplayName(EmissiveBoostProperty->GetPropertyDisplayName())
		.ToolTip(EmissiveBoostProperty->GetToolTipText())
		.EditCondition(IsOverrideEmissiveBoostEnabled, FOnBooleanValueChanged::CreateLambda([this](bool NewValue) {
		MaterialEditorInstance->LightmassSettings.EmissiveBoost.bOverride = (uint32)NewValue;
		MaterialEditorInstance->PostEditChange();
		FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	}))
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideEmissiveBoostEnabled)))
		.OverrideResetToDefault(ResetEmissiveBoostPropertyOverride);

	FIsResetToDefaultVisible IsDiffuseBoostPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
		return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->LightmassSettings.DiffuseBoost.ParameterValue != MaterialEditorInstance->Parent->GetDiffuseBoost() : false;
	});
	FResetToDefaultHandler ResetDiffuseBoostPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
		if (MaterialEditorInstance->Parent != nullptr)
		{
			MaterialEditorInstance->LightmassSettings.DiffuseBoost.ParameterValue = MaterialEditorInstance->Parent->GetDiffuseBoost();
		}
	});
	FResetToDefaultOverride ResetDiffuseBoostPropertyOverride = FResetToDefaultOverride::Create(IsDiffuseBoostPropertyResetVisible, ResetDiffuseBoostPropertyHandler);
	IDetailPropertyRow& DiffuseBoostPropertyRow = LightmassSettingsGroup.AddPropertyRow(DiffuseBoostProperty->GetChildHandle(0).ToSharedRef());
	DiffuseBoostPropertyRow
		.DisplayName(DiffuseBoostProperty->GetPropertyDisplayName())
		.ToolTip(DiffuseBoostProperty->GetToolTipText())
		.EditCondition(IsOverrideDiffuseBoostEnabled, FOnBooleanValueChanged::CreateLambda([this](bool NewValue) {
		MaterialEditorInstance->LightmassSettings.DiffuseBoost.bOverride = (uint32)NewValue;
		MaterialEditorInstance->PostEditChange();
		FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	}))
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideDiffuseBoostEnabled)))
		.OverrideResetToDefault(ResetDiffuseBoostPropertyOverride);

	FIsResetToDefaultVisible IsExportResolutionScalePropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
		return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->LightmassSettings.ExportResolutionScale.ParameterValue != MaterialEditorInstance->Parent->GetDiffuseBoost() : false;
	});
	FResetToDefaultHandler ResetExportResolutionScalePropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
		if (MaterialEditorInstance->Parent != nullptr)
		{
			MaterialEditorInstance->LightmassSettings.ExportResolutionScale.ParameterValue = MaterialEditorInstance->Parent->GetDiffuseBoost();
		}
	});
	FResetToDefaultOverride ResetExportResolutionScalePropertyOverride = FResetToDefaultOverride::Create(IsExportResolutionScalePropertyResetVisible, ResetExportResolutionScalePropertyHandler);
	IDetailPropertyRow& ExportResolutionScalePropertyRow = LightmassSettingsGroup.AddPropertyRow(ExportResolutionScaleProperty->GetChildHandle(0).ToSharedRef());
	ExportResolutionScalePropertyRow
		.DisplayName(ExportResolutionScaleProperty->GetPropertyDisplayName())
		.ToolTip(ExportResolutionScaleProperty->GetToolTipText())
		.EditCondition(IsOverrideExportResolutionScaleEnabled, FOnBooleanValueChanged::CreateLambda([this](bool NewValue) {
		MaterialEditorInstance->LightmassSettings.ExportResolutionScale.bOverride = (uint32)NewValue;
		MaterialEditorInstance->PostEditChange();
		FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	}))
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideExportResolutionScaleEnabled)))
		.OverrideResetToDefault(ResetExportResolutionScalePropertyOverride);
}

UEnum* CGetBlendModeEnum()
{
	UEnum* BlendModeEnum = StaticEnum<EBlendMode>();
	if (Substrate::IsSubstrateEnabled())
	{
		// BLEND_Translucent & BLEND_TranslucentGreyTransmittance are mapped onto the same enum index
		BlendModeEnum->SetMetaData(TEXT("DisplayName"), TEXT("TranslucentGreyTransmittance"), BLEND_Translucent);

		// BLEND_Modulate & BLEND_ColoredTransmittanceOnly are mapped onto the same enum index
		BlendModeEnum->SetMetaData(TEXT("DisplayName"), TEXT("ColoredTransmittanceOnly"), BLEND_Modulate);

		// BLEND_TranslucentColoredTransmittance is only supported in Substrate mode
		BlendModeEnum->SetMetaData(TEXT("DisplayName"), TEXT("TranslucentColoredTransmittance"), BLEND_TranslucentColoredTransmittance);
	}
	else
	{
		// BLEND_TranslucentColoredTransmittance is not supported in legacy mode
		BlendModeEnum->SetMetaData(TEXT("Hidden"), TEXT("True"), BLEND_TranslucentColoredTransmittance);
	}
	return BlendModeEnum;
}

void FCusMaterialInstanceParameterDetails::CreateBasePropertyOverrideWidgets(IDetailLayoutBuilder& DetailLayout, IDetailGroup& MaterialPropertyOverrideGroup)
{
	IDetailGroup& BasePropertyOverrideGroup = MaterialPropertyOverrideGroup;

	TAttribute<bool> IsOverrideOpacityClipMaskValueEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideOpacityClipMaskValueEnabled));
	TAttribute<bool> IsOverrideBlendModeEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideBlendModeEnabled));
	TAttribute<bool> IsOverrideShadingModelEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideShadingModelEnabled));
	TAttribute<bool> IsOverrideTwoSidedEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideTwoSidedEnabled));
	TAttribute<bool> IsOverrideIsThinSurfaceEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideIsThinSurfaceEnabled));
	TAttribute<bool> IsOverrideDitheredLODTransitionEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideDitheredLODTransitionEnabled));
	TAttribute<bool> IsOverrideOutputTranslucentVelocityEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideOutputTranslucentVelocityEnabled));
	TAttribute<bool> IsOverrideHasPixelAnimationEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideHasPixelAnimationEnabled));
	TAttribute<bool> IsOverrideTessellationEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideTessellationEnabled));
	TAttribute<bool> IsOverrideDisplacementScalingEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideDisplacementScalingEnabled));
	TAttribute<bool> IsOverrideMaxWorldPositionOffsetDisplacementEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideMaxWorldPositionOffsetDisplacementEnabled));
	TAttribute<bool> IsOverrideCastDynamicShadowAsMaskedEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::OverrideCastDynamicShadowAsMaskedEnabled));

	TSharedRef<IPropertyHandle> BasePropertyOverridePropery = DetailLayout.GetProperty("BasePropertyOverrides");
	TSharedPtr<IPropertyHandle> OpacityClipMaskValueProperty = BasePropertyOverridePropery->GetChildHandle("OpacityMaskClipValue");
	TSharedPtr<IPropertyHandle> BlendModeProperty = BasePropertyOverridePropery->GetChildHandle("BlendMode");
	TSharedPtr<IPropertyHandle> ShadingModelProperty = BasePropertyOverridePropery->GetChildHandle("ShadingModel");
	TSharedPtr<IPropertyHandle> TwoSidedProperty = BasePropertyOverridePropery->GetChildHandle("TwoSided");
	TSharedPtr<IPropertyHandle> IsThinSurfaceProperty = BasePropertyOverridePropery->GetChildHandle("IsThinSurface");
	TSharedPtr<IPropertyHandle> DitheredLODTransitionProperty = BasePropertyOverridePropery->GetChildHandle("DitheredLODTransition");
	TSharedPtr<IPropertyHandle> OutputTranslucentVelocityProperty = BasePropertyOverridePropery->GetChildHandle("bOutputTranslucentVelocity");
	TSharedPtr<IPropertyHandle> HasPixelAnimationProperty = BasePropertyOverridePropery->GetChildHandle("bHasPixelAnimation");
	TSharedPtr<IPropertyHandle> EnableTessellationProperty = BasePropertyOverridePropery->GetChildHandle("bEnableTessellation");
	TSharedPtr<IPropertyHandle> DisplacementScalingProperty = BasePropertyOverridePropery->GetChildHandle("DisplacementScaling");
	TSharedPtr<IPropertyHandle> MaxWorldPositionOffsetDisplacementProperty = BasePropertyOverridePropery->GetChildHandle("MaxWorldPositionOffsetDisplacement");
	TSharedPtr<IPropertyHandle> CastDynamicShadowAsMaskedProperty = BasePropertyOverridePropery->GetChildHandle("bCastDynamicShadowAsMasked");

	const FText ParameterDisabledToolTipString = FText::FromString(TEXT("This material instance parent restricts the creation of new shader permutations. Overriding this parameter would result in the generation of additional shader permutations."));
	const bool bStaticParametersOverrideDisabled = MaterialEditorInstance->SourceInstance->bDisallowStaticParameterPermutations;

	// Update blend mode display names
	if (FByteProperty* BlendModeByteProperty = (FByteProperty*)BlendModeProperty->GetProperty())
	{
		BlendModeByteProperty->Enum = CGetBlendModeEnum();
	}

	{
		FIsResetToDefaultVisible IsOpacityClipMaskValuePropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.OpacityMaskClipValue != MaterialEditorInstance->Parent->GetOpacityMaskClipValue() : false;
			});
		FResetToDefaultHandler ResetOpacityClipMaskValuePropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.OpacityMaskClipValue = MaterialEditorInstance->Parent->GetOpacityMaskClipValue();
			}
			});
		FResetToDefaultOverride ResetOpacityClipMaskValuePropertyOverride = FResetToDefaultOverride::Create(IsOpacityClipMaskValuePropertyResetVisible, ResetOpacityClipMaskValuePropertyHandler);
		IDetailPropertyRow& OpacityClipMaskValuePropertyRow = BasePropertyOverrideGroup.AddPropertyRow(OpacityClipMaskValueProperty.ToSharedRef());
		OpacityClipMaskValuePropertyRow
			.DisplayName(OpacityClipMaskValueProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : OpacityClipMaskValueProperty->GetToolTipText())
			.EditCondition(IsOverrideOpacityClipMaskValueEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideOpacityClipMaskValueChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideOpacityClipMaskValueEnabled)))
			.OverrideResetToDefault(ResetOpacityClipMaskValuePropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsBlendModePropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.BlendMode != MaterialEditorInstance->Parent->GetBlendMode() : false;
			});
		FResetToDefaultHandler ResetBlendModePropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.BlendMode = MaterialEditorInstance->Parent->GetBlendMode();
			}
			});
		FResetToDefaultOverride ResetBlendModePropertyOverride = FResetToDefaultOverride::Create(IsBlendModePropertyResetVisible, ResetBlendModePropertyHandler);
		IDetailPropertyRow& BlendModePropertyRow = BasePropertyOverrideGroup.AddPropertyRow(BlendModeProperty.ToSharedRef());
		BlendModePropertyRow
			.DisplayName(BlendModeProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : BlendModeProperty->GetToolTipText())
			.EditCondition(IsOverrideBlendModeEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideBlendModeChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideBlendModeEnabled)))
			.OverrideResetToDefault(ResetBlendModePropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsShadingModelPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				if (MaterialEditorInstance->Parent->IsShadingModelFromMaterialExpression())
				{
					return MaterialEditorInstance->BasePropertyOverrides.ShadingModel != MSM_FromMaterialExpression;
				}
				else
				{
					return MaterialEditorInstance->BasePropertyOverrides.ShadingModel != MaterialEditorInstance->Parent->GetShadingModels().GetFirstShadingModel();
				}
			}
			else
			{
				return false;
			}
			});
		FResetToDefaultHandler ResetShadingModelPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				if (MaterialEditorInstance->Parent->IsShadingModelFromMaterialExpression())
				{
					MaterialEditorInstance->BasePropertyOverrides.ShadingModel = MSM_FromMaterialExpression;
				}
				else
				{
					MaterialEditorInstance->BasePropertyOverrides.ShadingModel = MaterialEditorInstance->Parent->GetShadingModels().GetFirstShadingModel();
				}
			}
			});
		FResetToDefaultOverride ResetShadingModelPropertyOverride = FResetToDefaultOverride::Create(IsShadingModelPropertyResetVisible, ResetShadingModelPropertyHandler);
		IDetailPropertyRow& ShadingModelPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(ShadingModelProperty.ToSharedRef());
		ShadingModelPropertyRow
			.DisplayName(ShadingModelProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : ShadingModelProperty->GetToolTipText())
			.EditCondition(IsOverrideShadingModelEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideShadingModelChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideShadingModelEnabled)))
			.OverrideResetToDefault(ResetShadingModelPropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsTwoSidedPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.TwoSided != MaterialEditorInstance->Parent->IsTwoSided() : false;
			});
		FResetToDefaultHandler ResetTwoSidedValuePropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.TwoSided = MaterialEditorInstance->Parent->IsTwoSided();
			}
			});
		FResetToDefaultOverride ResetTwoSidedPropertyOverride = FResetToDefaultOverride::Create(IsTwoSidedPropertyResetVisible, ResetTwoSidedValuePropertyHandler);
		IDetailPropertyRow& TwoSidedPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(TwoSidedProperty.ToSharedRef());
		TwoSidedPropertyRow
			.DisplayName(TwoSidedProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : TwoSidedProperty->GetToolTipText())
			.EditCondition(IsOverrideTwoSidedEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideTwoSidedChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideTwoSidedEnabled)))
			.OverrideResetToDefault(ResetTwoSidedPropertyOverride);
	}
	if (IsThinSurfaceProperty)
	{
		FIsResetToDefaultVisible IsThinSurfacePropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.bIsThinSurface != MaterialEditorInstance->Parent->IsThinSurface() : false;
			});
		FResetToDefaultHandler ResetIsThinSurfaceValuePropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.bIsThinSurface = MaterialEditorInstance->Parent->IsThinSurface();
			}
			});
		FResetToDefaultOverride ResetIsThinSurfacePropertyOverride = FResetToDefaultOverride::Create(IsThinSurfacePropertyResetVisible, ResetIsThinSurfaceValuePropertyHandler);
		IDetailPropertyRow& IsThinSurfacePropertyRow = BasePropertyOverrideGroup.AddPropertyRow(IsThinSurfaceProperty.ToSharedRef());
		IsThinSurfacePropertyRow
			.DisplayName(IsThinSurfaceProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : IsThinSurfaceProperty->GetToolTipText())
			.EditCondition(IsOverrideIsThinSurfaceEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideIsThinSurfaceChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideIsThinSurfaceEnabled)))
			.OverrideResetToDefault(ResetIsThinSurfacePropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsDitheredLODTransitionPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.DitheredLODTransition != MaterialEditorInstance->Parent->IsDitheredLODTransition() : false;
			});
		FResetToDefaultHandler ResetDitheredLODTransitionPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.DitheredLODTransition = MaterialEditorInstance->Parent->IsDitheredLODTransition();
			}
			});
		FResetToDefaultOverride ResetDitheredLODTransitionPropertyOverride = FResetToDefaultOverride::Create(IsDitheredLODTransitionPropertyResetVisible, ResetDitheredLODTransitionPropertyHandler);
		IDetailPropertyRow& DitheredLODTransitionPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(DitheredLODTransitionProperty.ToSharedRef());
		DitheredLODTransitionPropertyRow
			.DisplayName(DitheredLODTransitionProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : DitheredLODTransitionProperty->GetToolTipText())
			.EditCondition(IsOverrideDitheredLODTransitionEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideDitheredLODTransitionChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideDitheredLODTransitionEnabled)))
			.OverrideResetToDefault(ResetDitheredLODTransitionPropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsOutputTranslucentVelocityPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.bOutputTranslucentVelocity != MaterialEditorInstance->Parent->IsTranslucencyWritingVelocity() : false;
			});
		FResetToDefaultHandler ResetOutputTranslucentVelocityPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.bOutputTranslucentVelocity = MaterialEditorInstance->Parent->IsTranslucencyWritingVelocity();
			}
			});
		FResetToDefaultOverride ResetOutputTranslucentVelocityPropertyOverride = FResetToDefaultOverride::Create(IsOutputTranslucentVelocityPropertyResetVisible, ResetOutputTranslucentVelocityPropertyHandler);
		IDetailPropertyRow& OutputTranslucentVelocityPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(OutputTranslucentVelocityProperty.ToSharedRef());
		OutputTranslucentVelocityPropertyRow
			.DisplayName(OutputTranslucentVelocityProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : OutputTranslucentVelocityProperty->GetToolTipText())
			.EditCondition(IsOverrideOutputTranslucentVelocityEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideOutputTranslucentVelocityChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideOutputTranslucentVelocityEnabled)))
			.OverrideResetToDefault(ResetOutputTranslucentVelocityPropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsHasPixelAnimationPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.bHasPixelAnimation != MaterialEditorInstance->Parent->HasPixelAnimation() : false;
			});
		FResetToDefaultHandler ResetHasPixelAnimationPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.bHasPixelAnimation = MaterialEditorInstance->Parent->HasPixelAnimation();
			}
			});
		FResetToDefaultOverride ResetHasPixelAnimationPropertyOverride = FResetToDefaultOverride::Create(IsHasPixelAnimationPropertyResetVisible, ResetHasPixelAnimationPropertyHandler);
		IDetailPropertyRow& HasPixelAnimationPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(HasPixelAnimationProperty.ToSharedRef());
		HasPixelAnimationPropertyRow
			.DisplayName(HasPixelAnimationProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : HasPixelAnimationProperty->GetToolTipText())
			.EditCondition(IsOverrideHasPixelAnimationEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideHasPixelAnimationChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideHasPixelAnimationEnabled)))
			.OverrideResetToDefault(ResetHasPixelAnimationPropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsEnableTessellationPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.bEnableTessellation != MaterialEditorInstance->Parent->IsTessellationEnabled() : false;
			});
		FResetToDefaultHandler ResetEnableTessellationPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.bEnableTessellation = MaterialEditorInstance->Parent->IsTessellationEnabled();
			}
			});
		FResetToDefaultOverride ResetEnableTessellationPropertyOverride = FResetToDefaultOverride::Create(IsEnableTessellationPropertyResetVisible, ResetEnableTessellationPropertyHandler);
		IDetailPropertyRow& EnableTessellationPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(EnableTessellationProperty.ToSharedRef());
		EnableTessellationPropertyRow
			.DisplayName(EnableTessellationProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : EnableTessellationProperty->GetToolTipText())
			.EditCondition(IsOverrideTessellationEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideEnableTessellationChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideTessellationEnabled)))
			.OverrideResetToDefault(ResetEnableTessellationPropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsDisplacementScalingPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle)
		{
			return MaterialEditorInstance->Parent != nullptr ?
				MaterialEditorInstance->BasePropertyOverrides.DisplacementScaling != MaterialEditorInstance->Parent->GetDisplacementScaling() : false;
		});
		FResetToDefaultHandler ResetDisplacementScalingPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle)
		{
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.DisplacementScaling = MaterialEditorInstance->Parent->GetDisplacementScaling();
			}
		});
		FResetToDefaultOverride ResetDisplacementScalingPropertyOverride = FResetToDefaultOverride::Create(IsDisplacementScalingPropertyResetVisible, ResetDisplacementScalingPropertyHandler);
		IDetailPropertyRow& DisplacementScalingPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(DisplacementScalingProperty.ToSharedRef());
		DisplacementScalingPropertyRow
			.DisplayName(DisplacementScalingProperty->GetPropertyDisplayName())
			.ToolTip(DisplacementScalingProperty->GetToolTipText())
			.EditCondition(IsOverrideDisplacementScalingEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideDisplacementScalingChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideDisplacementScalingEnabled)))
			.OverrideResetToDefault(ResetDisplacementScalingPropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsMaxWorldPositionOffsetDisplacementPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.MaxWorldPositionOffsetDisplacement != MaterialEditorInstance->Parent->GetMaxWorldPositionOffsetDisplacement() : false;
			});
		FResetToDefaultHandler ResetMaxWorldPositionOffsetDisplacementPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.MaxWorldPositionOffsetDisplacement = MaterialEditorInstance->Parent->GetMaxWorldPositionOffsetDisplacement();
			}
			});
		FResetToDefaultOverride ResetMaxWorldPositionOffsetDisplacementPropertyOverride = FResetToDefaultOverride::Create(IsMaxWorldPositionOffsetDisplacementPropertyResetVisible, ResetMaxWorldPositionOffsetDisplacementPropertyHandler);
		IDetailPropertyRow& MaxWorldPositionOffsetDisplacementPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(MaxWorldPositionOffsetDisplacementProperty.ToSharedRef());
		MaxWorldPositionOffsetDisplacementPropertyRow
			.DisplayName(MaxWorldPositionOffsetDisplacementProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : MaxWorldPositionOffsetDisplacementProperty->GetToolTipText())
			.EditCondition(IsOverrideMaxWorldPositionOffsetDisplacementEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideMaxWorldPositionOffsetDisplacementChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideMaxWorldPositionOffsetDisplacementEnabled)))
			.OverrideResetToDefault(ResetMaxWorldPositionOffsetDisplacementPropertyOverride);
	}
	{
		FIsResetToDefaultVisible IsCastDynamicShadowAsMaskedPropertyResetVisible = FIsResetToDefaultVisible::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			return MaterialEditorInstance->Parent != nullptr ? MaterialEditorInstance->BasePropertyOverrides.bCastDynamicShadowAsMasked != MaterialEditorInstance->Parent->GetCastDynamicShadowAsMasked() : false;
			});
		FResetToDefaultHandler ResetCastDynamicShadowAsMaskedPropertyHandler = FResetToDefaultHandler::CreateLambda([this](TSharedPtr<IPropertyHandle> InHandle) {
			if (MaterialEditorInstance->Parent != nullptr)
			{
				MaterialEditorInstance->BasePropertyOverrides.bCastDynamicShadowAsMasked = MaterialEditorInstance->Parent->GetCastDynamicShadowAsMasked();
			}
			});
		FResetToDefaultOverride ResetCastDynamicShadowAsMaskedPropertyOverride = FResetToDefaultOverride::Create(IsCastDynamicShadowAsMaskedPropertyResetVisible, ResetCastDynamicShadowAsMaskedPropertyHandler);
		IDetailPropertyRow& CastDynamicShadowAsMaskedPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(CastDynamicShadowAsMaskedProperty.ToSharedRef());
		CastDynamicShadowAsMaskedPropertyRow
			.DisplayName(CastDynamicShadowAsMaskedProperty->GetPropertyDisplayName())
			.ToolTip(bStaticParametersOverrideDisabled ? ParameterDisabledToolTipString : CastDynamicShadowAsMaskedProperty->GetToolTipText())
			.EditCondition(IsOverrideCastDynamicShadowAsMaskedEnabled, FOnBooleanValueChanged::CreateSP(this, &FCusMaterialInstanceParameterDetails::OnOverrideCastDynamicShadowAsMaskedChanged))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible, IsOverrideCastDynamicShadowAsMaskedEnabled)))
			.OverrideResetToDefault(ResetCastDynamicShadowAsMaskedPropertyOverride);
	}
}

EVisibility FCusMaterialInstanceParameterDetails::IsOverriddenAndVisible(TAttribute<bool> IsOverridden) const
{
	bool bShouldBeVisible = true;
	if (MaterialEditorInstance->bShowOnlyOverrides)
	{
		bShouldBeVisible = IsOverridden.Get();
	}
	return bShouldBeVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

bool FCusMaterialInstanceParameterDetails::OverrideOpacityClipMaskValueEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_OpacityMaskClipValue;
}

bool FCusMaterialInstanceParameterDetails::OverrideBlendModeEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_BlendMode;
}

bool FCusMaterialInstanceParameterDetails::OverrideShadingModelEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_ShadingModel;
}

bool FCusMaterialInstanceParameterDetails::OverrideTwoSidedEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_TwoSided;
}

bool FCusMaterialInstanceParameterDetails::OverrideIsThinSurfaceEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_bIsThinSurface;
}

bool FCusMaterialInstanceParameterDetails::OverrideDitheredLODTransitionEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_DitheredLODTransition;
}

bool FCusMaterialInstanceParameterDetails::OverrideOutputTranslucentVelocityEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_OutputTranslucentVelocity;
}

bool FCusMaterialInstanceParameterDetails::OverrideHasPixelAnimationEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_bHasPixelAnimation;
}

bool FCusMaterialInstanceParameterDetails::OverrideTessellationEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_bEnableTessellation;
}

bool FCusMaterialInstanceParameterDetails::OverrideDisplacementScalingEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_DisplacementScaling;
}

bool FCusMaterialInstanceParameterDetails::OverrideMaxWorldPositionOffsetDisplacementEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_MaxWorldPositionOffsetDisplacement;
}

bool FCusMaterialInstanceParameterDetails::OverrideCastDynamicShadowAsMaskedEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_CastDynamicShadowAsMasked;
}

/** Helper function used by some parameters to verify that they are allowed to be overridden. This
  * must be prevented if the source material instance disallows the creation of new static parameter
  * permutations as that would trigger a new shader creation. */
static bool DoesSourceMaterialInstanceDisallowStaticParameterPermutation(const UMaterialEditorInstanceConstant* mi, bool NewValue)
{
	if (NewValue && mi->SourceInstance->bDisallowStaticParameterPermutations)
	{
		return true;
	}
	return false;
}

void FCusMaterialInstanceParameterDetails::OnOverrideCastDynamicShadowAsMaskedChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_CastDynamicShadowAsMasked = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideOpacityClipMaskValueChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_OpacityMaskClipValue = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideBlendModeChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_BlendMode = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideShadingModelChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_ShadingModel = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideTwoSidedChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_TwoSided = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideIsThinSurfaceChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_bIsThinSurface = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideDitheredLODTransitionChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_DitheredLODTransition = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideOutputTranslucentVelocityChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_OutputTranslucentVelocity = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideHasPixelAnimationChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_bHasPixelAnimation = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideEnableTessellationChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_bEnableTessellation = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideDisplacementScalingChanged(bool NewValue)
{
	MaterialEditorInstance->BasePropertyOverrides.bOverride_DisplacementScaling = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FCusMaterialInstanceParameterDetails::OnOverrideMaxWorldPositionOffsetDisplacementChanged(bool NewValue)
{
	if (DoesSourceMaterialInstanceDisallowStaticParameterPermutation(MaterialEditorInstance, NewValue))
	{
		return;
	}
	MaterialEditorInstance->BasePropertyOverrides.bOverride_MaxWorldPositionOffsetDisplacement = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

#undef LOCTEXT_NAMESPACE

#endif