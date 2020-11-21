// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class TrueSkyEditorPlugin : ModuleRules
	{
		public TrueSkyEditorPlugin(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
				}
				);
			
			PublicIncludePaths.AddRange(new string[] {
				"Editor/LevelEditor/Public",
				"Editor/PlacementMode/Private",
				"Editor/MainFrame/Public/Interfaces",
                "Developer/AssetTools/Private",
				"Editor/ContentBrowser/Private",
			});

			string SDL2Path = Target.UEThirdPartySourceDirectory + "SDL2/SDL-gui-backend/";
			string SDL2LibPath = SDL2Path + "lib/";
			// assume SDL to be built with extensions
			PublicDefinitions.Add("SDL_WITH_EPIC_EXTENSIONS=1");
			if (Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
			{
				PublicIncludePaths.Add(SDL2Path + "include");
			}

			PublicIncludePathModuleNames.AddRange(
				new string[] {
				"SceneOutliner",
				}
			);
			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
				"SceneOutliner",
				}
			);
			// ... Add private include paths required here ...
			PrivateIncludePaths.AddRange(
					new string[] {
						"TrueSkyEditorPlugin/Private"
						,"TrueSkyPlugin/Private"
						,"TrueSkyPlugin/Public"
						,"TrueSkyPlugin/Classes"
					}
				);

			// Add public dependencies that we statically link with here ...
			PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"Core",
						"CoreUObject",
						"Slate",
						"SlateCore",
						"Engine",
						"AppFramework",
					}
				);

				PublicDependencyModuleNames.AddRange(
						new string[]
						{
							"UnrealEd",
							"EditorStyle",
							"CollectionManager",
							"EditorStyle",
							"AssetTools",
							"PlacementMode",
							"ContentBrowser",
							"TrueSkyPlugin"
						}
					);


			// ... add private dependencies that you statically link with here ...
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"RenderCore",
                    "RHI",
					"Slate",
					"SlateCore",
					"InputCore",
					"ContentBrowser",
					"PropertyEditor",
					"LevelEditor",
					"Renderer",
					"DesktopPlatform"
				}
				);

            if(Target.Platform== UnrealTargetPlatform.Win32 || Target.Platform==UnrealTargetPlatform.Win64)
            {
                    AddEngineThirdPartyPrivateStaticDependencies(Target,
                        "DX11"
                        );
                    PrivateDependencyModuleNames.AddRange( new string[] { "D3D11RHI" }   );
            }
            else if(Target.Platform==UnrealTargetPlatform.Linux)
            {
                PublicIncludePaths.AddRange(new string[] {
                    "Runtime/ApplicationCore/Public",
                });
            }

			PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		}
	}
}
