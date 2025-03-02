// Copyright AKaKLya 2024

using UnrealBuildTool;
using System.IO;
public class MatHelper : ModuleRules
{
	public MatHelper(ReadOnlyTargetRules Target) : base(Target)
	{
		// ...
		// Get the engine path. Ends with "Engine/"
		var EnginePath = Path.GetFullPath(Target.RelativeEnginePath);
		
		// Now get the base of UE4's modules dir (could also be Developer, Editor, ThirdParty)
		var MaterialPath = EnginePath + "Source/Editor/MaterialEditor/Private/";
		var NiagaraPath = EnginePath + "Plugins/FX/Niagara/Source/NiagaraEditor/Private/Sequencer/LevelSequence/";
		var NiagaraPathB = EnginePath + "Plugins/FX/Niagara/Source/NiagaraEditor/Private/";
		// now you can include the module's private paths!
		// as an example, you can expose UE4's abstraction of D3D11, located in Source/Runtime/Windows/D3D11RHI
	
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty/Detours/include"));
		PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "ThirdParty/Detours/lib/x64/detours.lib"));
		
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				
			}
		);
		
		PrivateIncludePaths.AddRange(
			new string[] {
				MaterialPath,NiagaraPath,NiagaraPathB
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core","AssetDefinition", "EngineAssetDefinitions", "MaterialEditor","GraphEditor", "NiagaraEditor",
				// ... add other public dependencies that you statically link with here ...
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"MaterialEditor",
				"Projects",
				"ApplicationCore",
				"InputCore",
				"ContentBrowser",
				"AssetTools",
				"EngineAssetDefinitions", "CurveAssetEditor",
				"CurveEditor", "GraphEditor",
				"EditorWidgets","ToolMenus","EditorStyle","DeveloperSettings", "StaticMeshEditor", "LevelSequence", "NiagaraEditor", "Niagara", "Sequencer",
				"MovieScene","SceneOutliner","PropertyEditor","RenderCore","LevelEditor",
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}