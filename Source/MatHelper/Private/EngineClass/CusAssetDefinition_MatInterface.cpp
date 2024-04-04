// Copyright AKaKLya 2024


#include "EngineClass/CusAssetDefinition_MatInterface.h"

#include "ContentBrowserMenuContexts.h"
#include "IAssetTools.h"
#include "MaterialPropertyHelpers.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Subsystems/EditorAssetSubsystem.h"
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
				FString TargetPath = SourceObject->GetPathName();
				const FString BaseName = SourceObject->GetName();
				TargetPath.ReplaceInline(*BaseName,*FString(""));
				TargetPath.ReplaceInline(*FString("."),*FString(""));

				FString NewBaseName = BaseName;
				if(BaseName.Left(2) == "M_")
				{
					NewBaseName.ReplaceInline(*FString("M_"),*FString("MI_"));
				}
				else
				{
					NewBaseName = "MI_" + BaseName;
				}
				int RenameTime = 1;
				
				Rename:
				RenameTime = RenameTime + 1 ;
				
				const FString NewName = NewBaseName + "_" + FString::FromInt(RenameTime);

				
				const FString NewPath = TargetPath + NewName;
				
				UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
				if(EditorAssetSubsystem->DoesAssetExist(NewPath))
				{
					goto Rename;
				}
				
				UMaterialInstance* NewMi = Cast<UMaterialInstance>(EditorAssetSubsystem->DuplicateAsset("/MatHelper/Material/MI_Empty", NewPath));
				NewMi->Parent=SourceObject;
				
				UMaterialInstanceConstant* ConstMat = static_cast<UMaterialInstanceConstant*>(NewMi);
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
				return nullptr;
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

