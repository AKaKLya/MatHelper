// Fill out your copyright notice in the Description page of Project Settings.


#include "CusFactory_MatInterface.h"

#include "MaterialPropertyHelpers.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/MaterialInstanceConstant.h"

UObject* UCusFactory_MatInterface::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
                                                    UObject* Context, FFeedbackContext* Warn)
{
	FString NewBaseName = Name.ToString();
	if(NewBaseName.Left(2) == "M_")
	{
		NewBaseName.ReplaceInline(*FString("M_"),*FString("MI_"));
	}
	else
	{
		NewBaseName = "MI_" + NewBaseName;
	}

	UObject* NewObj = Super::FactoryCreateNew(Class, InParent, *NewBaseName, Flags, Context, Warn);

	UMaterialInstanceConstant* ConstMat = static_cast<UMaterialInstanceConstant*>(NewObj);
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
	
	return NewObj;
}
