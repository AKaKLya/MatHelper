// ReSharper disable CppCStyleCast
#include "FunctionHook.h"

namespace FunctionHook
{
	void HookFunction(HMODULE HModule, uintptr_t FunctionOffset, void*& OriginalFunction,void* InHookFunction,Hook_Normal_Tag )
	{
		if (!HModule) {
			return ;
		}

		uintptr_t BaseAddress = reinterpret_cast<uintptr_t>(HModule);
		void* FunctionAddress = reinterpret_cast<void*>(BaseAddress + FunctionOffset);
		OriginalFunction = FunctionAddress;
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)OriginalFunction, InHookFunction);
		DetourTransactionCommit();
	}


	void HookFunction(HMODULE HModule, uintptr_t FunctionOffset, void*& OriginalFunction,void* InHookFunction,Hook_Debug_Tag)
	{
		if (!HModule) {
			UE_LOG(LogTemp, Warning, TEXT("Failed to get module handle"));
			return ;
		}

		uintptr_t BaseAddress = reinterpret_cast<uintptr_t>(HModule);
		void* FunctionAddress = reinterpret_cast<void*>(BaseAddress + FunctionOffset);
		OriginalFunction = FunctionAddress;
		LONG Error = DetourTransactionBegin();
		if (Error != NO_ERROR) {
			UE_LOG(LogTemp, Warning, TEXT("DetourTransactionBegin failed with error code: %d"), Error);
			return ;
		}

		Error = DetourUpdateThread(GetCurrentThread());
		if (Error != NO_ERROR) {
			UE_LOG(LogTemp, Warning, TEXT("DetourUpdateThread failed with error code: %d"), Error);
			DetourTransactionAbort();
			return ;
		}
    
		Error = DetourAttach(&(PVOID&)OriginalFunction, InHookFunction);
		if (Error != NO_ERROR) {
			UE_LOG(LogTemp, Warning, TEXT("DetourAttach failed with error code: %d"), Error);
			DetourTransactionAbort();
			return ;
		}

		Error = DetourTransactionCommit();
		if (Error != NO_ERROR) {
			UE_LOG(LogTemp, Warning, TEXT("DetourTransactionCommit failed with error code: %d"), Error);
			return ;
		}
	}

	void HookFunction(const char* ModuleName, uintptr_t FunctionOffset, void*& OriginalFunction,void* InHookFunction,Hook_Normal_Tag Tag)
	{
		HMODULE HModule = GetModuleHandleA(ModuleName);
		HookFunction(HModule,FunctionOffset,OriginalFunction,InHookFunction,Tag);
	}


	void HookFunction(const char* ModuleName, uintptr_t FunctionOffset, void*& OriginalFunction,void* InHookFunction,Hook_Debug_Tag Tag)
	{
		HMODULE HModule = GetModuleHandleA(ModuleName);
		HookFunction(HModule,FunctionOffset,OriginalFunction,InHookFunction,Tag);
	}
}
