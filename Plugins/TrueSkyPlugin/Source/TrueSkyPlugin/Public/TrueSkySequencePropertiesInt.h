// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkySequencePropertiesInt.generated.h"

// Enumeration for the int-based sky sequencer sky-layer properties.
UENUM( BlueprintType )
enum class ETrueSkySkySequenceLayerPropertiesInt : uint8
{
	TSPROPINT_NumAltitudes			UMETA( DisplayName = "Altitude" ),
	TSPROPINT_NumElevations			UMETA( DisplayName = "Elevation" ),
	TSPROPINT_NumDistances			UMETA( DisplayName = "Distances" ),
	TSPROPINT_StartDayNumber		UMETA( DisplayName = "Start Day Number (From 1/1/2000)" ),

	TSPROPINT_Max					UMETA( Hidden )
};

// Enumeration for the int-based sky sequencer sky-keyframe properties.
UENUM( BlueprintType )
enum class ETrueSkySkySequenceKeyframePropertiesInt : uint8
{
	TSPROPINT_StoreAsColours			UMETA( DisplayName = "Store Keyframe as Colours" ),
	TSPROPINT_NumColourAltitudes		UMETA( DisplayName = "Colour Altitude Layer Count" ),
	TSPROPINT_NumColourElevations		UMETA( DisplayName = "Colour Elevation Layer Count" ),
	TSPROPINT_NumColourDistances		UMETA( DisplayName = "Colour Distance Layer Count" ),
	TSPROPINT_AutomaticSunPosition		UMETA( DisplayName = "Automatic Sun Position" ),
	TSPROPINT_AutomaticMoonPosition		UMETA( DisplayName = "Automatic Moon Position" ),

	TSPROPINT_Max						UMETA( Hidden )
};

// Enumeration for the int-based sky sequencer cloud-layer properties.
UENUM( BlueprintType )
enum class ETrueSkyCloudSequenceLayerPropertiesInt : uint8
{
	TSPROPINT_Clouds_RenderMode					UMETA(DisplayName = "Enable Cirrus Style Clouds (Int)"),
	TSPROPINT_Clouds_DefaultNumSlices			UMETA(DisplayName = "Default Slices (Int)"),
	TSPROPINT_Clouds_GridWidth					UMETA(DisplayName = "Cloud Detail Grid Width (Int)"),
	TSPROPINT_Clouds_GridHeight					UMETA(DisplayName = "Cloud Detail Grid Height (Int)"),
	TSPROPINT_Clouds_NoiseResolution			UMETA(DisplayName = "3D Noise Resolution (Int)"),

	TSPROPINT_Max							UMETA( Hidden )
};	

// Enumeration for the float-based sky sequencer cloud-keyframe properties.
UENUM( BlueprintType )
enum class ETrueSkyCloudSequenceKeyframePropertiesInt : uint8
{
	TSPROPINT_Octaves					UMETA( DisplayName = "Noise Generation Octaves" ),
	TSPROPINT_RegionalPrecipitation	UMETA( DisplayName = "Regional Precipitation" ),
	TSPROPINT_LockRainToClouds		UMETA( DisplayName = "Lock Rain to Clouds" ),

	TSPROPINT_Max						UMETA( Hidden )
};
