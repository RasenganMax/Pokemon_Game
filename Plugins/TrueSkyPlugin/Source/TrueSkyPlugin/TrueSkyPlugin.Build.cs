// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.
using System.IO;
using System;

namespace UnrealBuildTool.Rules
{
	public class TrueSkyPlugin : ModuleRules
	{
		public TrueSkyPlugin(ReadOnlyTargetRules Target) : base(Target)
		{
			string SIMUL_BUILD = System.Environment.GetEnvironmentVariable("SIMUL_BUILD");
			if(SIMUL_BUILD!=""&& SIMUL_BUILD!="0")
				PublicDefinitions.Add("SIMUL_BUILD=1");
			else
				PublicDefinitions.Add("SIMUL_BUILD=1");
			string SIMUL_BINARY_PLUGIN = System.Environment.GetEnvironmentVariable("SIMUL_BINARY_PLUGIN");
			if (SIMUL_BINARY_PLUGIN != "" && SIMUL_BINARY_PLUGIN != "0")
			{
				PublicDefinitions.Add("SIMUL_BINARY_PLUGIN=1");
				PublicDefinitions.Add("SIMUL_ALTITUDE_DIRECTIONAL_LIGHT=0");
			}
			else
			{
				PublicDefinitions.Add("SIMUL_BINARY_PLUGIN=0");
				PublicDefinitions.Add("SIMUL_ALTITUDE_DIRECTIONAL_LIGHT=1");
			}

			if (Target.Configuration != UnrealTargetConfiguration.Debug&&Target.Configuration!= UnrealTargetConfiguration.DebugGame )
			{
				PublicDefinitions.Add("NDEBUG=1");
			}
			else
			{
				PublicDefinitions.Add("DEBUG=1");
			}
			PublicDefinitions.Add("ENABLE_TRUESKY_WATER=1");
			//PublicDefinitions.Add("INTEL_METRICSDISCOVERY=0"); // for build warnings in D3D11RHIPrivate.h

			PublicIncludePaths.AddRange(new string[]
			{
				"Runtime/Renderer/Private"
			});

			if(Target.bBuildEditor==true)
			{
				PublicIncludePaths.AddRange(new string[] 
				{
					"Editor/LevelEditor/Public",
					"Editor/PlacementMode/Private",
					"Editor/MainFrame/Public/Interfaces",
					"Developer/AssetTools/Private",
				});
			}

			// ... Add private include paths required here ...
			PrivateIncludePaths.AddRange(new string[] 
			{
				"TrueSkyPlugin/Private"
			});

			// Add public dependencies that we statically link with here ...
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"Engine",
			});

			if(Target.bBuildEditor==true)
			{
				PublicDependencyModuleNames.AddRange(new string[]
				{
					"UnrealEd",
					"EditorStyle",
					"CollectionManager",
					"EditorStyle",
					"AssetTools",
					"PlacementMode",
					"ContentBrowser"
				});
			}

			string configName		= "";
			string platformName		= Target.Platform.ToString();

			string platformMidName  = "";
			string linkExtension	= "";
			string dllExtension	 	= "";
			string libPrefix		= "";
			var EngineDir			= Path.GetFullPath(Target.RelativeEnginePath);
			string BasePath		 	= Path.Combine(EngineDir, @"Binaries/ThirdParty/Simul");
			BasePath				= Path.Combine(BasePath, platformName);

			string copyThirdPartyPath		= "";
			bool bAddRuntimeDependencies	= false;
			bool bLinkFromBinaries			= true;

			if(Target.Platform==UnrealTargetPlatform.Win64)
			{
				linkExtension   = "_MT.lib";
				dllExtension	= "_MT.dll";
			}
			else if(Target.Platform==UnrealTargetPlatform.Mac)
			{
				linkExtension	   = dllExtension = ".dylib";
				libPrefix		   = "lib";
				bLinkFromBinaries	= false;
			}
			else if(Target.Platform==UnrealTargetPlatform.XboxOne)
			{
				linkExtension   = "_MD.lib";
				dllExtension	= "_MD.dll";
			}
			else if(Target.Platform==UnrealTargetPlatform.PS4)
			{
				linkExtension   = "_stub.a";
				dllExtension	= ".prx";
				libPrefix	   = "lib";
			}
			else if(Target.Platform==UnrealTargetPlatform.Linux)
			{
				BasePath		= System.IO.Path.Combine(BasePath, "x86_64");
				linkExtension   = ".so";
				dllExtension	= ".so";
				libPrefix	   = "lib";
			}
			else if(Target.Platform==UnrealTargetPlatform.Switch)
			{
				linkExtension   = ".so";
				dllExtension	= ".so";
				libPrefix	   = "lib";
			}
			else
			{
				throw new System.Exception(System.String.Format("Unsupported platform {0}", Target.Platform.ToString()));
			}
			//PublicLibraryPaths.Add(BasePath);
			
			string libName = System.String.Format("{0}trueskypluginrender{1}{2}{3}", libPrefix, configName, platformMidName, linkExtension);
			string dllName = System.String.Format("{0}trueskypluginrender{1}{2}{3}", libPrefix, configName, platformMidName, dllExtension);

			string libPath = System.IO.Path.Combine(BasePath, libName);
			string dllPath = System.IO.Path.Combine(BasePath, dllName);

			System.Collections.Generic.List<string> plugins = GetPlugins(BasePath);
			if (bLinkFromBinaries)
			{
				//PublicAdditionalLibraries.Add(libPath);
			}
			else
			{
				string LibPath = System.IO.Path.Combine(ModuleDirectory, "../../Libs/Mac/");
				PublicAdditionalLibraries.Add(System.String.Format("{0}trueskypluginrender{1}.dylib", LibPath, configName));			
			}

			if (bAddRuntimeDependencies)
			{
				RuntimeDependencies.Add(dllPath);
				foreach (string plugin in plugins)
				{
					string pluginPath = System.IO.Path.Combine(BasePath, plugin + dllExtension);
					System.Console.WriteLine("Adding reference to trueSKY plugin: " + pluginPath);
					RuntimeDependencies.Add(pluginPath);
				}
			}

			if (copyThirdPartyPath.Length != 0)
			{
				string destPath = System.IO.Path.Combine(Target.UEThirdPartyBinariesDirectory, copyThirdPartyPath);
				System.IO.Directory.CreateDirectory(destPath);
				string dllDest = System.IO.Path.Combine(destPath,dllName);
				CopyFile(dllPath, dllDest);
			}

			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"RenderCore",
				"RHI",
				"Slate",
				"SlateCore",
				"Renderer"
				// ... add private dependencies that you statically link with here ...
			});
            bool bHaveVulkan = false;
			string VulkanSDKPath="";
			// Win64 Platform (D3D11 & D3D12)
			if (Target.Platform==UnrealTargetPlatform.Win64)
            {
                VulkanSDKPath = System.Environment.GetEnvironmentVariable("VULKAN_SDK");
                bHaveVulkan = !String.IsNullOrEmpty(VulkanSDKPath);
                PublicIncludePaths.AddRange(new string[] 
				{
					"Runtime/Windows/D3D11RHI/Public"
					,"Runtime/Windows/D3D11RHI/Private"
					,"Runtime/Windows/D3D11RHI/Private/Windows"
					,"Runtime/D3D12RHI/Public"
					,"Runtime/D3D12RHI/Private"
					,"Runtime/VulkanRHI/Public"
					,"Runtime/VulkanRHI/Private"
					,"Runtime/VulkanRHI/Private/Windows"
				});
				PrivateIncludePathModuleNames.AddRange(new string[]
				{
					"VulkanRHI","HeadMountedDisplay"
				});
				string SimulBinariesDir = "$(EngineDir)/Binaries/ThirdParty/Simul/Win64/";
				RuntimeDependencies.Add(SimulBinariesDir + "TrueSkyPluginRender_MT.dll");
				string SimulShaderbinDir = "$(EngineDir)/Plugins/TrueSkyPlugin/shaderbin/Win64/";
				RuntimeDependencies.Add(SimulShaderbinDir + "*.*");
				PrivateDependencyModuleNames.AddRange(new string[] 
				{
					"D3D11RHI", "D3D12RHI"
				});
				AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
			}
			else if (Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
            {
                VulkanSDKPath = System.Environment.GetEnvironmentVariable("VULKAN_SDK");
                bHaveVulkan = !String.IsNullOrEmpty(VulkanSDKPath);
                if (Target.Platform == UnrealTargetPlatform.Linux)
				{
					PublicIncludePaths.AddRange(new string[]
					{
					"Runtime/VulkanRHI/Private/Linux"
					});
					if (BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Linux || !bHaveVulkan)
					{
						AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
					}
					else
					{
						PrivateIncludePaths.Add(VulkanSDKPath + "/include");
						PrivateIncludePaths.Add(VulkanSDKPath + "/include/vulkan");
					}
				}
				else
				{
					AddEngineThirdPartyPrivateStaticDependencies(Target, "VkHeadersExternal");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
            {
                VulkanSDKPath = System.Environment.GetEnvironmentVariable("VULKAN_SDK");
                bHaveVulkan = !String.IsNullOrEmpty(VulkanSDKPath);
                if (bHaveVulkan)
				{
					PrivateIncludePaths.Add(VulkanSDKPath + "/Include");
					if (Target.Configuration != UnrealTargetConfiguration.Shipping)
					{
						PrivateIncludePathModuleNames.AddRange(
							new string[]
							{
							"TaskGraph",
							}
						);
					}
				}
			}
			// XBox One Platform (D3D11 & D3D12)
			else if (Target.Platform==UnrealTargetPlatform.XboxOne)
			{
				PublicIncludePaths.AddRange(new string[] 
				{
					"Runtime/D3D12RHI/Public"
					,"Runtime/D3D12RHI/Private"
					,"Runtime/Core/Public/Windows"
				});
				PrivateIncludePathModuleNames.AddRange(new string[]
				{
					"D3D12RHI"
				});
				PrivateDependencyModuleNames.AddRange(new string[] 
				{
					"DX11",
					"XboxOneSDK"
				});
				string SimulBinariesDir = "$(EngineDir)/Binaries/ThirdParty/Simul/XboxOne/";
				RuntimeDependencies.Add(SimulBinariesDir + "TrueSkyPluginRender_MD.dll");
				string SimulShaderbinDir = "$(EngineDir)/Plugins/TrueSkyPlugin/Shaderbin/XboxOne/";
				RuntimeDependencies.Add(SimulShaderbinDir + "*.*");
			}
			// PS4 Platform
			else if(Target.Platform==UnrealTargetPlatform.PS4)
			{
				string SimulBinariesDir = "$(EngineDir)/Binaries/ThirdParty/Simul/PS4/";

				PrivateDependencyModuleNames.Add("PS4RHI");
				PublicIncludePaths.AddRange(new string[] 
				{
					"Runtime/PS4/PS4RHI/Public"
					,"Runtime/PS4/PS4RHI/Private"
					,"../Platforms/PS4/Source/Runtime/PS4RHI/Public"
					,"../Platforms/PS4/Source/Runtime/PS4RHI/Private"
				});
				string trueskypluginrender = "trueskypluginrender";
				if (Target.Configuration == UnrealTargetConfiguration.Debug)
				{
					//trueskypluginrender += "-debug";
				}
				RuntimeDependencies.Add(SimulBinariesDir + trueskypluginrender+".prx");
				string SimulShaderbinDir = "$(EngineDir)/plugins/trueskyplugin/Shaderbin/ps4/";
				RuntimeDependencies.Add(SimulShaderbinDir + "*.*");
				//PublicAdditionalLibraries.Add("../binaries/thirdparty/simul/ps4/"+trueskypluginrender+"_stub.a");
				PublicDelayLoadDLLs.AddRange(
					new string[] {
					trueskypluginrender+".prx"
				}
				);
				string SDKDir = System.Environment.GetEnvironmentVariable("SCE_ORBIS_SDK_DIR");
				if ((SDKDir == null) || (SDKDir.Length <= 0))
				{
					SDKDir = "C:/Program Files (x86)/SCE/ORBIS SDKs/5.000";
				}
				PrivateIncludePaths.AddRange(new string[] 
				{
						SDKDir+"/target/include_common/gnmx"
				});
			}
			// Nintendo Switch
			else if(Target.Platform == UnrealTargetPlatform.Switch)
			{
				string SimulBinariesDir = "$(EngineDir)/Binaries/ThirdParty/Simul/Switch/";

				PrivateDependencyModuleNames.Add("NVNRHI");
				PublicIncludePaths.AddRange(new string[] 
				{
					"Runtime/Switch/NVNRHI/Public"
					,"Runtime/Switch/NVNRHI/Private"
				});
				string trueskypluginrender = "TrueSkyPluginRender";
				if (Target.Configuration == UnrealTargetConfiguration.Debug)
				{
					trueskypluginrender += "-debug";
				}
				RuntimeDependencies.Add(SimulBinariesDir + trueskypluginrender+".nro");
				string SimulShaderbinDir = "$(EngineDir)/Plugins/TrueSkyPlugin/shaderbin/Switch/";
				RuntimeDependencies.Add(SimulShaderbinDir + "*.*");
				PublicDelayLoadDLLs.AddRange
				(
					new string[] {
					trueskypluginrender+".nro"
				}
				);
			}
			if (bHaveVulkan)
			{
				PublicDefinitions.Add("SIMUL_SUPPORT_VULKAN=1");
				PublicIncludePaths.AddRange(new string[]
				{
					"Runtime/VulkanRHI/Public"
					,"Runtime/VulkanRHI/Private"
				});
				PrivateIncludePaths.Add(VulkanSDKPath + "/include");
				PrivateIncludePaths.Add(VulkanSDKPath + "/include/vulkan");
				PrivateDependencyModuleNames.AddRange(new string[]
					{
						 "VulkanRHI","HeadMountedDisplay"
					});
					AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
				
			}
			else
				PublicDefinitions.Add("SIMUL_SUPPORT_VULKAN=0");
			// The following to duplicate the D3D12RHI dependencies
			if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
			{
				AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAPI");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "AMD_AGS");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
				AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
			}
			// Load SIMUL dlls
			PublicDelayLoadDLLs.Add("SimulBase_MD.dll");
			PublicDelayLoadDLLs.Add("SimulCamera_MD.dll");
			PublicDelayLoadDLLs.Add("SimulClouds_MD.dll");
			PublicDelayLoadDLLs.Add("SimulCrossPlatform_MD.dll");
			PublicDelayLoadDLLs.Add("SimulDirectX11_MD.dll");
			PublicDelayLoadDLLs.Add("SimulGeometry_MD.dll");
			PublicDelayLoadDLLs.Add("SimulMath_MD.dll");
			PublicDelayLoadDLLs.Add("SimulMeta_MD.dll");
			PublicDelayLoadDLLs.Add("SimulScene_MD.dll");
			PublicDelayLoadDLLs.Add("SimulSky_MD.dll");
			PublicDelayLoadDLLs.Add("SimulTerrain_MD.dll");
			PublicDelayLoadDLLs.Add("TrueSkyPluginRender_MT.dll");
			PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		}
		private void CopyFile(string source, string dest)
		{
			//System.Console.WriteLine("Copying {0} to {1}", source, dest);
			if (System.IO.File.Exists(dest))
			{
				System.IO.File.SetAttributes(dest, System.IO.File.GetAttributes(dest) & ~System.IO.FileAttributes.ReadOnly);
			}
			try
			{
				System.IO.File.Copy(source, dest, true);
			}
			catch (System.Exception ex)
			{
				System.Console.WriteLine("Failed to copy file: {0}", ex.Message);
			}
		}

		private System.Collections.Generic.List<string> GetPlugins(string BasePath)
		{
			System.Collections.Generic.List<string> AllPlugins = new System.Collections.Generic.List<string>();
			string PluginListName = System.IO.Path.Combine(BasePath, "plugins.txt");
			if (System.IO.File.Exists(PluginListName))
			{
				try
				{
					foreach (string FullEntry in System.IO.File.ReadAllLines(PluginListName))
					{
						string Entry = FullEntry.Trim();
						if (Entry.Length > 0)
						{
							AllPlugins.Add(Entry);
						}
					}
				}
				catch (System.Exception ex)
				{
					System.Console.WriteLine("Failed to read plugin list file: {0}", ex.Message);
				}
			}
			return AllPlugins;
		}
	}
}
