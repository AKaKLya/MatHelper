// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "MatHelperSettings.generated.h"

/**
 * 
 */
UCLASS(config=EditorPerProjectUserSettings)
class MATHELPER_API UMatHelperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()
	
public:
	UMatHelperSettings();
	virtual FName GetCategoryName() const override;
	
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> CommonEditorMaps;
};
