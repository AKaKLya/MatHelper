// Fill out your copyright notice in the Description page of Project Settings.


#include "MatHelperMgn.h"

#include "Interfaces/IPluginManager.h"


UMatHelperMgn::UMatHelperMgn()
{
	PluginConfigPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() + "/Config/AddNodeFile";
	AutoGroupKeys.Add("Dissolve");
}

void UMatHelperMgn::OpenNodesConfigFolder()
{
	FString Path = PluginConfigPath;
	Path.ReplaceCharInline('/','\\');
	FWindowsPlatformProcess::CreateProc(L"explorer.exe",*Path,false,false,false,nullptr,0,nullptr,nullptr);
}
