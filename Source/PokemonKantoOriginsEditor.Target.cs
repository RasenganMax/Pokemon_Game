

using UnrealBuildTool;
using System.Collections.Generic;

public class PokemonKantoOriginsEditorTarget : TargetRules
{
	public PokemonKantoOriginsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "PokemonKantoOrigins" } );
	}
}
