#pragma once
#include "Toolkits/NiagaraSystemToolkit.h"


DECLARE_EVENT_OneParam(FMatHelperModule, FNiagaraRegisterTabSpawnersEvent, const TSharedRef<FTabManager>&);
DECLARE_EVENT_TwoParams(FMatHelperModule, FNiagaraExtendToolbar, FToolBarBuilder&,FNiagaraSystemToolkit*);

namespace MatHelperHook
{
	void HookMatEditor();
	void HookNiagaraTabRegister();
	inline FNiagaraRegisterTabSpawnersEvent OnNiagaraRegisterTabFactories;
	inline FNiagaraExtendToolbar OnNiagaraExtendToolbar;
};
