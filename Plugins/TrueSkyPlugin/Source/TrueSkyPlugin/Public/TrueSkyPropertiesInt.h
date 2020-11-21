// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkyPropertiesInt.generated.h"

// Enumeration for the int-based sky properties.
UENUM( BlueprintType )
enum class ETrueSkyPropertiesInt: uint8
{
	TSPROPINT_OnscreenProfiling				UMETA( DisplayName = "Onscreen Profiling" ),
	TSPROPINT_InterpolationMode				UMETA( DisplayName = "Interpolation Mode"),
	TSPROPINT_InterpolationSubdivisions		UMETA( DisplayName = "Interpolation Subdivisions" ),
	TSPROPINT_InstantUpdate					UMETA( DisplayName = "Instant Update on property change" ),
	TSPROPINT_WorleyTextureSize				UMETA( DisplayName = "Worley Texture Size"),
	TSPROPINT_CloudSteps					UMETA( DisplayName = "Cloud Steps"),
	TSPROPINT_MaxViewAgeFrames				UMETA( DisplayName = "Max View Age Frames"),
	TSPROPINT_PrecipitationOptions			UMETA( DisplayName = "Precipitation Options"),
	TSPROPINT_MaximumCubemapResolution		UMETA( DisplayName = "Maximum Cubemap Resolution"),
	TSPROPINT_GridRendering					UMETA( DisplayName = "Grid Rendering / Fixed Rendering"),
	TSPROPINT_GodraysGridX					UMETA( DisplayName = "Godrays Grid X"),
	TSPROPINT_GodraysGridY					UMETA( DisplayName = "Godrays Grid Y"),
	TSPROPINT_GodraysGridZ					UMETA( DisplayName = "Godrays Grid Z"),
	TSPROPINT_CloudShadowTextureSize		UMETA( DisplayName = "Cloud Shadow Texture Size"),
	TSPROPINT_Amortization					UMETA( DisplayName = "Cloud Amortization"),
	TSPROPINT_AtmosphericsAmortization		UMETA( DisplayName = "Atmospherics Amortization"),
	TSPROPINT_RenderWater					UMETA( DisplayName = "Render Water"),
	TSPROPINT_WaterFullResolution			UMETA( DisplayName = "Water Full Resolution"),
	TSPROPINT_EnableWaterReflection			UMETA( DisplayName = "Enable Water Reflections"),
	TSPROPINT_ReflectionFullResolution		UMETA( DisplayName = "Reflections Full Resolution"),
	TSPROPINT_ReflectionSteps				UMETA( DisplayName = "Max number of relfection tests"),
	TSPROPINT_ReflectionPixelStep			UMETA( DisplayName = "Pixel space between reflection samples"),
	TSPROPINT_EnableBoundlessOcean			UMETA( DisplayName = "Enable Boundless Ocean"),
	TSPROPINT_WaterUseGameTime				UMETA( DisplayName = "Does the water use Real Time or Game Time"),
	TSPROPINT_TrueSkyLightUnits				UMETA( DisplayName = "Light Units"),
	TSPROPINT_Max				UMETA( Hidden )
};

// Enumeration for the int-based water properties.
UENUM(BlueprintType)
enum class ETrueSkyWaterPropertiesInt : uint8
{
	TSPROPINT_Water_Render						UMETA(DisplayName = "Render"),
	TSPROPINT_Water_EnableWaveGrid				UMETA(DisplayName = "Enable Wave Grid"),
	TSPROPINT_Water_WaveGridDistance			UMETA(DisplayName = "Wave Grid Extent"),
	TSPROPINT_Water_EnableFoam					UMETA(DisplayName = "Enable Foam"),
	TSPROPINT_Water_EnableShoreEffects			UMETA(DisplayName = "Enable Shore Effects"),
	TSPROPINT_Water_ProfileBufferResolution		UMETA(DisplayName = "Profile Buffer Resolution"),
	TSPROPINT_Water_Max							UMETA(Hidden)
};
