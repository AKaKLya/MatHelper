// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "CusAssetDefinition_Material.generated.h"



/**
 * 
 */
UCLASS()
class MATHELPER_API UCusAssetDefinition_Material : public UAssetDefinitionDefault
{
	UCusAssetDefinition_Material();
	GENERATED_BODY()
public:
	// UAssetDefinition Begin
	FColor Color = FColor(64,192,64);
	virtual FLinearColor GetAssetColor() const override;
	virtual UThumbnailInfo* LoadThumbnailInfo(const FAssetData& InAsset) const override;
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Material", "Material"); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UMaterial::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { EAssetCategoryPaths::Material, EAssetCategoryPaths::Basic };
		return Categories;
	}
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	
	/*DECLARE_EVENT_TwoParams(IMaterialEditorModule, FCusMatEditorOpenedEvent, UMaterial*,TSharedRef<IMaterialEditor>);
	FCusMatEditorOpenedEvent& OnMaterialEditorOpened() { return MatEditorOpenedEvent; };
private:
	FCusMatEditorOpenedEvent MatEditorOpenedEvent;*/
};
