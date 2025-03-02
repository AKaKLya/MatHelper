// ReSharper disable CppPassValueParameterByConstReference
#include "MatHelperHook.h"




#include "DetailCategoryBuilder.h"
#include "FunctionHook.h"

#include "MaterialEditor.h"
#include "MatHelperSettings.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialEditorInstanceDetailCustomization.h"
#include "TAccessPrivate.inl"
#include "MaterialEditor/DEditorParameterValue.h"
#include "Toolkits/NiagaraSystemToolkit.h"
#include "Toolkits/SystemToolkitModes/NiagaraSystemToolkitMode_Default.h"

//--------------------Access Private--------------------//
DEFINE_ACCESS_PRIVATE(AccessNiagaraToolbarExtender,FApplicationMode,TSharedPtr<FExtender>,ToolbarExtender)
DEFINE_ACCESS_PRIVATE(AccessNiagaraSystemToolkit,FNiagaraSystemToolkitModeBase,TWeakPtr<FNiagaraSystemToolkit>,SystemToolkit)
DEFINE_ACCESS_PRIVATE(AccessMatInstance,FMaterialInstanceParameterDetails,UMaterialEditorInstanceConstant*, MaterialEditorInstance)

//-----------------------Hook MaterialEditor FixFunctionNode------------------------------------------//
//FMaterialEditor::PasteNodesHere(const FVector2D& Location, const class UEdGraph* Graph)
using PasteNodesHereFuncType = void(__fastcall*)(FMaterialEditor* This, const FVector2D& Location, const UEdGraph* Graph);
PasteNodesHereFuncType OriginalPasteNodesHere = nullptr;
PVOID& OriginalPasteNodesHereRef()
{ return (PVOID&)OriginalPasteNodesHere; }
void __fastcall HookPasteNodesHere(FMaterialEditor* This, const FVector2D& Location, const UEdGraph* Graph)
{
	OriginalPasteNodesHere(This, Location, Graph);
	
	//Hook
	auto NewNodes = This->GetSelectedNodes();
	for (const auto Node : NewNodes)
	{
		UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
		if (Cast<UMaterialExpressionMaterialFunctionCall>(MatNode->MaterialExpression))
		{
			MatNode->RecreateAndLinkNode();
		}
		This->AddToSelection(MatNode->MaterialExpression);
	}
}

//-----------------------Hook MaterialInstanceEditor Button------------------------------------------//
//void FMaterialInstanceParameterDetails::CreateGroupsWidget(TSharedRef<IPropertyHandle> ParameterGroupsProperty, IDetailCategoryBuilder& GroupsCategory)
DEFINE_HOOK_TYPE(CreateGroupsWidget, void, __fastcall, FMaterialInstanceParameterDetails* This, TSharedRef<IPropertyHandle> ParameterGroupsProperty, IDetailCategoryBuilder&GroupsCategory)
{
	OriginalCreateGroupsWidget(This, ParameterGroupsProperty, GroupsCategory);
	
	FDetailWidgetRow& SaveInstanceRow = GroupsCategory.AddCustomRow(FText::FromString("MatHelper"));
	
	SaveInstanceRow.ValueContent()
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNullWidget::NullWidget
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString("Enable Params"))
					.HAlign(HAlign_Center)
					.OnClicked_Lambda([=]()
					{
						const auto Instance = This->*TAccessPrivate<AccessMatInstance>::Value;;
						for (int32 GroupIdx = 0; GroupIdx < Instance->ParameterGroups.Num(); ++GroupIdx)
						{
							FEditorParameterGroup& ParameterGroup = Instance->ParameterGroups[GroupIdx];
							for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ++ParamIdx)
							{
								UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
								FMaterialPropertyHelpers::OnOverrideParameter(!Parameter->bOverride,Parameter,Instance);
							}
						}
						return FReply::Handled();
					})
				]
			];
}

#define LOCTEXT_NAMESPACE "NiagaraSystemToolkitModeBase"
//--------------------------Hook Niagara TabRegister-------------------------------//
//void FNiagaraSystemToolkitMode_Default::ExtendToolbar()
DEFINE_HOOK_TYPE(NiagaraExtendToolbar, void, __fastcall, FNiagaraSystemToolkitMode_Default* This)
{
	OriginalNiagaraExtendToolbar(This);
	
	const auto SystemToolKit=  This->*TAccessPrivate<AccessNiagaraSystemToolkit>::Value;
	const auto Extender = This->*TAccessPrivate<AccessNiagaraToolbarExtender>::Value;
	
	Extender->AddToolBarExtension("Asset",EExtensionHook::After,SystemToolKit.Pin()->GetToolkitCommands(),
	FToolBarExtensionDelegate::CreateLambda([=](FToolBarBuilder& ToolbarBuilder) {
	   MatHelperHook::OnNiagaraExtendToolbar.Broadcast(ToolbarBuilder, SystemToolKit.Pin().Get());
	}));
}

//------------------------//
//void FNiagaraSystemToolkitModeBase::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
DEFINE_HOOK_TYPE(NiagaraRegisterTabFactories, void, __fastcall, FNiagaraSystemToolkitModeBase* This, TSharedPtr<FTabManager> InTabManager)
{
	OriginalNiagaraRegisterTabFactories(This, InTabManager);
	MatHelperHook::OnNiagaraRegisterTabFactories.Broadcast(InTabManager.ToSharedRef());
}

//----------------------//

#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------//

namespace MatHelperHook
{
	using namespace FunctionHook;
	
	void HookMatEditor()
	{
		constexpr const char* MaterialDll = "UnrealEditor-MaterialEditor.dll";
		HMODULE MatModule = GetModuleHandleA(MaterialDll);
		const UMatHelperSettings* Settings = GetDefault<UMatHelperSettings>();
	
		HookFunction(MatModule, Settings->PasteNodeHereOffset, OriginalPasteNodesHere, HookPasteNodesHere);
		HookFunction(MatModule, Settings->MIEditorCreateWidgetOffset, OriginalCreateGroupsWidget,HookCreateGroupsWidget);
	}

	void HookNiagaraTabRegister()
	{
		constexpr const char* NiagaraDll = "UnrealEditor-NiagaraEditor.dll";
		HMODULE NSModule = GetModuleHandleA(NiagaraDll);
		const UMatHelperSettings* Settings = GetDefault<UMatHelperSettings>();
		
		HookFunction(NSModule, Settings->NiagaraExtendToolbarOffset, OriginalNiagaraExtendToolbar, HookNiagaraExtendToolbar);
		HookFunction(NSModule, Settings->NiagaraRegisterTabFactoriesOffset, OriginalNiagaraRegisterTabFactories, HookNiagaraRegisterTabFactories);
	}
}
