// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "CusAssetDefinition_MatInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "CusAssetDefinition_MatInstance.generated.h"

/**
 * 
 */
UCLASS()
class MATHELPER_API UCusAssetDefinition_MatInstance : public UAssetDefinitionDefault
{
	GENERATED_BODY()
public:
	// UAssetDefinition Begin
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { EAssetCategoryPaths::Material };
		return Categories;
	}
	virtual UThumbnailInfo* LoadThumbnailInfo(const FAssetData& InAsset) const override;
	// UAssetDefinition End
	
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialInstanceConstant", "Material Instance"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(0,128,0)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UMaterialInstanceConstant::StaticClass(); }
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	virtual bool CanImport() const override { return true; }
	// UAssetDefinition End
};
