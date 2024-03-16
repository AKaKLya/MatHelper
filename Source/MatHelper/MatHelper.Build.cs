// Copyright AKaKLya 2024

using UnrealBuildTool;
using System.IO;
public class MatHelper : ModuleRules
{
	public MatHelper(ReadOnlyTargetRules Target) : base(Target)
	{
		// ...
		// Get the engine path. Ends with "Engine/"
		string engine_path = Path.GetFullPath(Target.RelativeEnginePath);
		// Now get the base of UE4's modules dir (could also be Developer, Editor, ThirdParty)
		string Curve_path = engine_path + "Plugins/Editor/EngineAssetDefinitions/Source/Private";
    
		// now you can include the module's private paths!
		// as an example, you can expose UE4's abstraction of D3D11, located in Source/Runtime/Windows/D3D11RHI
		
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(Curve_path);
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core","AssetDefinition", "EngineAssetDefinitions",
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
				"CurveEditor",
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