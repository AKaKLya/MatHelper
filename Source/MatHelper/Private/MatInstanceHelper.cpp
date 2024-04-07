// Copyright AKaKLya 2024


#include "MatInstanceHelper.h"

#include "MaterialPropertyHelpers.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/MaterialInstanceConstant.h"

void SMatInstanceHelper::Construct(const FArguments& InArgs,TSharedPtr<UMaterialInstanceConstant> InMatInstanceConstant)
{
	SScrollBox::Construct(SScrollBox::FArguments());
	MatInstanceConstant = InMatInstanceConstant;

	
	AddSlot()
	.Padding(5.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Enable Params"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatInstanceHelper::ToogleParams,true)
	];
	
	AddSlot()
	.Padding(5.0f)
	[
		SNew(SButton)
		.Text(FText::FromString("Disable Params"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatInstanceHelper::ToogleParams,false)
	];
}

FReply SMatInstanceHelper::ToogleParams(bool bIsOpen)
{
	const auto MaterialEditorInstance = NewObject<UMaterialEditorInstanceConstant>(GetTransientPackage(), NAME_None, RF_Transactional);
	MaterialEditorInstance->SetSourceInstance(MatInstanceConstant.Get());
	for (int32 GroupIdx = 0; GroupIdx < MaterialEditorInstance->ParameterGroups.Num(); ++GroupIdx)
	{
		FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[GroupIdx];
		for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ++ParamIdx)
		{
			UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
			FMaterialPropertyHelpers::OnOverrideParameter(bIsOpen,Parameter,MaterialEditorInstance);
		}
	}
	return FReply::Handled();
}
