// Copyright AKaKLya 2024


#include "EngineClass/CusAssetDefinition_Material.h"

#include "IMaterialEditor.h"
#include "MaterialEditorModule.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"


FLinearColor UCusAssetDefinition_Material::GetAssetColor() const
{
	return FLinearColor(Color);
}

UThumbnailInfo* UCusAssetDefinition_Material::LoadThumbnailInfo(const FAssetData& InAsset) const
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

EAssetCommandResult UCusAssetDefinition_Material::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UMaterial* Material : OpenArgs.LoadObjects<UMaterial>())
	{
		IMaterialEditorModule* MaterialEditorModule = &FModuleManager::LoadModuleChecked<IMaterialEditorModule>( "MaterialEditor" );
		TSharedRef<IMaterialEditor> Interface = MaterialEditorModule->CreateMaterialEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Material);
	}

	return EAssetCommandResult::Handled;
}
