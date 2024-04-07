// Copyright Epic Games, Inc. All Rights Reserved.

#include "ButtonClass/SimpleButtonCommands.h"

#define LOCTEXT_NAMESPACE "FSimpleButtonModule"

void FSimpleButtonCommands::RegisterCommands()
{
	UI_COMMAND(PlayNiagaraAction, "MatHelper", "PlayNiagara", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
