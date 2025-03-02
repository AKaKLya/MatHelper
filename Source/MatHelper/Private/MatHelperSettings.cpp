// Fill out your copyright notice in the Description page of Project Settings.


#include "MatHelperSettings.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "MatHelper.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

UMatHelperSettings::UMatHelperSettings()
{
	PluginButtonConfigPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() + "/Config/AddNodeFile";
	MaskPinInfo =
	{
		DECLARE_NODE_MASK_PIN(R, 1, 0, 0, 0),
		DECLARE_NODE_MASK_PIN(G, 0, 1, 0, 0),
		DECLARE_NODE_MASK_PIN(B, 0, 0, 1, 0),
		DECLARE_NODE_MASK_PIN(A, 0, 0, 0, 1),
		DECLARE_NODE_MASK_PIN(RGB, 1, 1, 1, 0),
		DECLARE_NODE_MASK_PIN(RGBA, 1, 1, 1, 1),
		DECLARE_NODE_MASK_PIN(RG, 1, 1, 0, 0),
		DECLARE_NODE_MASK_PIN(BA, 0, 0, 1, 1)
	};
}

FName UMatHelperSettings::GetCategoryName() const
{
	return FName("Plugins");
}


FMatHelperSettingsCustomization::FMatHelperSettingsCustomization()
{
}

FMatHelperSettingsCustomization::~FMatHelperSettingsCustomization()
{
}

TSharedRef<IDetailCustomization> FMatHelperSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FMatHelperSettingsCustomization());
}


void FMatHelperSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	IDetailCategoryBuilder& MatHelperSettingCategory = DetailLayout.EditCategory("MatHelper");
	TSharedPtr<IPropertyHandle> IconNameHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UMatHelperSettings, IConName), UMatHelperSettings::StaticClass());

	
	MatHelperSettingCategory.AddCustomRow(FText::FromString("CustRow"), false).NameContent() //NameContent是属性名的显示内容
	[
		SNew(STextBlock).Text(FText::FromString(""))
	].ValueContent()

	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(2, 0, 2, 0)
		.HAlign(HAlign_Fill).AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Edit Button Info"))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([]()
			{
				FGlobalTabmanager::Get()->TryInvokeTab(FMatHelperModule::ButtonInfoEditorTabName);
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot()
		.Padding(2, 0, 2, 0)
		.HAlign(HAlign_Fill).AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Open Nodes Config Folder"))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([]()
			{
				FString Path = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() + "/Config/AddNodeFile";;
				Path.ReplaceCharInline('/', '\\');
				FWindowsPlatformProcess::CreateProc(L"explorer.exe", *Path, false,
					false, false, nullptr, 0, nullptr,nullptr);
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot()
		.Padding(2, 0, 2, 0)
		.HAlign(HAlign_Fill).AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Refresh Helpers Button"))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([]()
			{
				FMatHelperModule::RefreshAllWidgetButton();
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot()
		.Padding(2, 0, 2, 0)
		.HAlign(HAlign_Fill).AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Modify ICON"))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([=]()
			{
				FString IConName;
				IconNameHandle->GetValue(IConName);
				auto PluginPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir();
				
				FSlateStyleSet* Style = static_cast<FSlateStyleSet*>(const_cast<ISlateStyle*>(&FAppStyle::Get()));
				FString TheIConName = "Graph/" + IConName;
				Style->SetContentRoot(PluginPath + "/Resources/");
				
				Style->Set("AppIcon", new IMAGE_BRUSH_SVG(TheIConName, FVector2f(50.f, 50.f), FStyleColors::White));
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot()
		.Padding(2, 0, 2, 0)
		.HAlign(HAlign_Fill).AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Restart Editor"))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([]()
			{
				FUnrealEdMisc::Get().RestartEditor(false);
				return FReply::Handled();
			})
		]
	];
}