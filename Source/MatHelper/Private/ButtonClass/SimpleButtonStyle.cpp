// Copyright Epic Games, Inc. All Rights Reserved.

#include "Buttonclass/SimpleButtonStyle.h"
#include "MatHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FSimpleButtonStyle::StyleInstance = nullptr;

void FSimpleButtonStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSimpleButtonStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSimpleButtonStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SimpleButtonStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FSimpleButtonStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SimpleButtonStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() / TEXT("Resources"));
	Style->Set("SimpleButton.PluginAction", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
	Style->Set("SimpleButton.Niagara", new IMAGE_BRUSH(TEXT("NiagaraIcon"), FVector2D(50,50)));
	return Style;
}

void FSimpleButtonStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSimpleButtonStyle::Get()
{
	return *StyleInstance;
}
