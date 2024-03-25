// Fill out your copyright notice in the Description page of Project Settings.


#include "EngineClass/CusAssetDefinition_MatInstance.h"

#include "IMaterialEditor.h"
#include "MaterialEditorModule.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"

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
		TSharedRef<IMaterialEditor> Editor = MaterialEditorModule->CreateMaterialInstanceEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, MIC);
	}


	return EAssetCommandResult::Handled;
}
