// Copyright AKaKLya 2024


#include "EngineClass/CusAssetDefinition_MatInstance.h"
#include "TAccessPrivate.inl"
#include "IMaterialEditor.h"
#include "MaterialEditorModule.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"

#define LOCTEXT_NAMESPACE "MaterialInstanceEditor"



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
		
		/*IMaterialEditor* EditorPtr = EditorRef.ToSharedPtr().Get();
		FMaterialInstanceEditor* MatInstanceEditor = static_cast<FMaterialInstanceEditor*>(EditorPtr);
		SimpleRunnable* Simple = new SimpleRunnable(MatInstanceEditor);
		FRunnableThread* RunnableThread = FRunnableThread::Create(Simple,TEXT("MatCusThread"));*/
	}

//DetailLayouts
	return EAssetCommandResult::Handled;
}
#undef LOCTEXT_NAMESPACE