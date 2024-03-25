// Copyright AKaKLya 2024


#include "MatHelperMgn.h"

#include "MatHelper.h"
#include "Interfaces/IPluginManager.h"


UMatHelperMgn::UMatHelperMgn()
{
	PluginButtonConfigPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() + "/Config/AddNodeFile";
	AutoGroupKeys.Add("Dissolve");
}

void UMatHelperMgn::OpenNodesConfigFolder()
{
	FString Path = PluginButtonConfigPath;
	Path.ReplaceCharInline('/','\\');
	FWindowsPlatformProcess::CreateProc(L"explorer.exe",*Path,false,false,false,nullptr,0,nullptr,nullptr);
}



void UMatHelperMgn::EditButtonInfo()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FMatHelperModule::ButtonInfoEditorTabName);
}

void UMatHelperMgn::RefreshHelpersButton()
{
	FMatHelperModule::RefreshAllWidgetButton();
}

void UMatHelperMgn::RestartEditor()
{
	FUnrealEdMisc::Get().RestartEditor(false);
}

/*
void UMatHelperMgn::CreateButtonFile()
{
	for(FNodeButton& Info : NodeButtonInfo)
	{
		if(Info.ButtonName == "None")
		{
			continue;
		}
		FString FilePath = PluginButtonConfigPath + "/" + Info.ButtonName + ".txt";
		FilePath.ReplaceCharInline('/','\\');
		bool Exist = FPaths::FileExists(FilePath);
		if(Exist == false)
		{
			FFileHelper::SaveStringToFile(FString(""),  *FilePath);
			FWindowsPlatformProcess::CreateProc(L"notepad.exe",*FilePath,false,false,false,nullptr,0,nullptr,nullptr);
		}
	}
}*/
