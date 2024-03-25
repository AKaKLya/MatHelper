// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IMaterialEditor.h"


/**
 * 
 */

class UMaterialInstanceConstant;

class SMatInstanceHelper : public SScrollBox
{
public:
	SLATE_BEGIN_ARGS(SMatInstanceHelper) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs,TSharedPtr<UMaterialInstanceConstant> InMatInstanceConstant);

private:
	TSharedPtr<UMaterialInstanceConstant> MatInstanceConstant;
	FReply ToogleParams(bool bIsOpen);
};
