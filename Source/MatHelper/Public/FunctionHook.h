// ReSharper disable CppCStyleCast
#pragma once
#include "Windows/AllowWindowsPlatformTypes.h"
#include <detours.h>
#include "Windows/HideWindowsPlatformTypes.h"

//-----------------------Hook Function-----------------//


namespace FunctionHook
{
    struct Hook_Debug_Tag {};  
    struct Hook_Normal_Tag {};
    void HookFunction(HMODULE HModule, uintptr_t FunctionOffset, void*& OriginalFunction, void* InHookFunction, Hook_Normal_Tag = Hook_Normal_Tag{});
    void HookFunction(HMODULE HModule, uintptr_t FunctionOffset, void*& OriginalFunction, void* InHookFunction, Hook_Debug_Tag);
    void HookFunction(const char* ModuleName, uintptr_t FunctionOffset, void*& OriginalFunction, void* InHookFunction, Hook_Normal_Tag Tag = Hook_Normal_Tag{});
    void HookFunction(const char* ModuleName, uintptr_t FunctionOffset, void*& OriginalFunction, void* InHookFunction, Hook_Debug_Tag Tag);

    template<typename MODULE, typename T>
    void HookFunction(MODULE Module, uintptr_t Offset, T*& Original, T* Hook, Hook_Normal_Tag = Hook_Normal_Tag{})
    {
        HookFunction(Module, Offset, (void*&)Original, (void*)Hook);
    }

    template<typename MODULE, typename T>
    void HookFunction(MODULE Module, uintptr_t Offset, T*& Original, T* Hook, Hook_Debug_Tag Tag)
    {
        HookFunction(Module, Offset, (void*&)Original, (void*)Hook, Tag);
    }
}



#define DEFINE_HOOK_TYPE(HookName, RetType, CallConv, ...)               \
using HookName##FuncType = RetType(CallConv*)(__VA_ARGS__);              \
HookName##FuncType Original##HookName = nullptr;                         \
RetType CallConv Hook##HookName(__VA_ARGS__)

/*
↓ 1.If you Want To Hook This Function ↓
FMaterialEditor::PasteNodesHere(const FVector2D& Location, const class UEdGraph* Graph)

↓ 2.USE THIS MACRO TO DEFINE ↓
DEFINE_HOOK_TYPE(PasteNodesHere, void, __fastcall, FMaterialEditor*, const FVector2D&, const UEdGraph*)

↓ 3.DEFINE YOUR FUNCTION ↓
void __fastcall HookedPasteNodes(FMaterialEditor* This, const FVector2D& Location, const UEdGraph* Graph)
{}

↓ 4.1 Get DLL ↓
constexpr const char* MaterialDll = "UnrealEditor-MaterialEditor.dll";

↓ 4.2 Get Function Offset  ↓
const uint64 FunctionOffset = GetDefault<UMatHelperSettings>()->PasteFromBufferIndex;// PasteNodesHere()

↓ 4.3 Hook ↓
HOOK_FUNCTION(Module, FunctionOffset, PasteNodesHere, HookedPasteNodes);

e.g:

Hook This Function: FMaterialEditor::PasteNodesHere(const FVector2D& Location, const class UEdGraph* Graph)
DEFINE_HOOK_TYPE(PasteNodesHere, void, __fastcall, FMaterialEditor* This, const FVector2D& Location, const UEdGraph* Graph)
{
    OriginalPasteNodesHere(This, Location, Graph);
    
    //Hook
   //do something...
}


void HookPasteFromBuffer()
{
	constexpr const char* MaterialDll = "UnrealEditor-MaterialEditor.dll";
const uint64 FunctionOffset = GetDefault<UMatHelperSettings>()->PasteNodeHereIndex;// PasteNodesHere()
     HookFunction(MaterialDll, FunctionOffset, OriginalPasteNodesHere, HookPasteNodesHere);
    //HookFunction(MaterialDll, FunctionOffset, OriginalPasteNodesHere, HookPasteNodesHere,Hook_Debug_Tag{});
}
*/
