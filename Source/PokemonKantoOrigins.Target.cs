

using UnrealBuildTool;
using System.Collections.Generic;

public class PokemonKantoOriginsTarget : TargetRules
{
	public PokemonKantoOriginsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "PokemonKantoOrigins" } );
	}
}
