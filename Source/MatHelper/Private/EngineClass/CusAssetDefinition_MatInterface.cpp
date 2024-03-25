// Fill out your copyright notice in the Description page of Project Settings.


#include "EngineClass/CusAssetDefinition_MatInterface.h"

#include "ContentBrowserMenuContexts.h"
#include "CusFactory_MatInterface.h"
#include "IAssetTools.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UThumbnailInfo* UCusAssetDefinition_MatInterface::LoadThumbnailInfo(const FAssetData& InAsset) const
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

namespace MenuExtension_MatInterface
{
	static void ExecuteNewMIC(const FToolMenuContext& MenuContext)
	{
		const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext);

		IAssetTools::Get().CreateAssetsFrom<UMaterialInterface>(CBContext->LoadSelectedObjects<UMaterialInterface>(), UMaterialInstanceConstant::StaticClass(), TEXT("_Inst"), [](UMaterialInterface* SourceObject)
			{
				UCusFactory_MatInterface* Factory = NewObject<UCusFactory_MatInterface>();
				Factory->InitialParent = SourceObject;
				return Factory;
			}
		);
	}
	
	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []{ 
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UMaterialInterface::StaticClass());
	        
			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
			Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				{
					const TAttribute<FText> Label = LOCTEXT("Material_NewMIC", "Create Material Instance 2");
					const TAttribute<FText> ToolTip = LOCTEXT("Material_NewMICTooltip", "Creates a parameterized material using this material as a base.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.MaterialInstanceActor");
					const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateStatic(&ExecuteNewMIC);

					InSection.AddMenuEntry("Material_NewMIC", Label, ToolTip, Icon, UIAction);
				}
			}));
		}));
	});
}







#undef LOCTEXT_NAMESPACE

