// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "CusFactory_MatInterface.generated.h"

/**
 * 
 */
UCLASS()
class MATHELPER_API UCusFactory_MatInterface : public UMaterialInstanceConstantFactoryNew
{
	GENERATED_BODY()
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
};
