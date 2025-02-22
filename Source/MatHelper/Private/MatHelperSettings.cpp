// Fill out your copyright notice in the Description page of Project Settings.


#include "MatHelperSettings.h"

UMatHelperSettings::UMatHelperSettings()
{
}

FName UMatHelperSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}
