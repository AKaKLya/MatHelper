// Copyright AKaKLya 2024


#include "MatHelperMgn.h"

#include "AssetViewUtils.h"
#include "MatHelper.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

UMatHelperMgn::UMatHelperMgn()
{
	PluginButtonConfigPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() + "/Config/AddNodeFile";
}

void UMatHelperMgn::ModifyICON()
{

	auto PluginPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir();
	
	FSlateStyleSet* Style = (FSlateStyleSet*)&FAppStyle::Get();
	
	FSlateBrush* MatIcon = CreateHeaderBrush();
	
	FString TheIConName = "Graph/" + IConName;
	Style->SetContentRoot(PluginPath + "/Resources/");
	if(IConName == "MatIcon")
	{
		Style->Set("AppIcon", MatIcon);
	}
	else
	{
		Style->Set("AppIcon", new IMAGE_BRUSH_SVG(TheIConName, FVector2f(50.f, 50.f), FStyleColors::White));
	}
	
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditorSubsystem->CloseAllEditorsForAsset(this);
	AssetViewUtils::OpenEditorForAsset(this);
}

FSlateBrush* UMatHelperMgn::CreateHeaderBrush()
{
	FSlateBrush* SlateBrush = new FSlateBrush();
	
	const FString MaterialPath = FString("/MatHelper/Material/M_SlateIcon");
	
	UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
	
	DynamicMaterial->AddToRoot();

	SlateBrush->SetResourceObject(DynamicMaterial);
	SlateBrush->ImageSize = FVector2D(50.f, 50.f);
	return SlateBrush;
}

void UMatHelperMgn::OpenNodesConfigFolder()
{
	FString Path = PluginButtonConfigPath;
	Path.ReplaceCharInline('/','\\');
	FWindowsPlatformProcess::CreateProc(L"explorer.exe",*Path,false,false,false,nullptr,0,nullptr,nullptr);
}


void UMatHelperMgn::EditButtonInfo()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FMatHelperModule::ButtonInfoEditorTabName);
}

void UMatHelperMgn::RefreshHelpersButton()
{
	FMatHelperModule::RefreshAllWidgetButton();
}

void UMatHelperMgn::RestartEditor()
{
	FUnrealEdMisc::Get().RestartEditor(false);
}