// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SimpleButtonStyle.h"

class FSimpleButtonCommands : public TCommands<FSimpleButtonCommands>
{
public:

	FSimpleButtonCommands()
		: TCommands<FSimpleButtonCommands>(TEXT("SimpleButton"), NSLOCTEXT("Contexts", "SimpleButton", "SimpleButton Plugin"), NAME_None, FSimpleButtonStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PlayNiagaraAction;
};
