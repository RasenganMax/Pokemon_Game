// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkyPropertiesFloat.generated.h"

// Enumeration for the float-based truesky properties.
UENUM( BlueprintType )
enum class ETrueSkyPropertiesFloat : uint8
{
	TSPROPFLOAT_DepthSamplingPixelRange		UMETA( DisplayName = "Depth sampling pixel range" ),
	TSPROPFLOAT_NearestRainMetres			UMETA( DisplayName = "Nearest rain metres" ),
	TSPROPFLOAT_CloudThresholdDistanceKm	UMETA( DisplayName = "Cloud Threshold Distance Km" ),
	TSPROPFLOAT_CloudshadowRangeKm			UMETA( DisplayName = "Cloud Shadowrange km" ),
	TSPROPFLOAT_CloudShadowHeight			UMETA( DisplayName = "Cloud Shadow Height (Read Only)"),
	TSPROPFLOAT_LatitudeDegrees				UMETA( DisplayName = "Latitude degrees" ),
	TSPROPFLOAT_LongitudeDegrees			UMETA( DisplayName = "Longitude degrees" ),
	TSPROPFLOAT_MaxSunRadiance				UMETA( DisplayName = "Max sun radiance" ),
	TSPROPFLOAT_ReferenceSpectralRadiance	UMETA( DisplayName = "Reference Spectral Radiance" ),
	TSPROPFLOAT_DepthTemporalAlpha			UMETA( DisplayName = "Alpha for blending depth over time for use in cloud rendering. 1.0=instant update, 0.01=slow update" ),
	TSPROPFLOAT_MinimumStarPixelSize		UMETA( DisplayName = "Minimum Star pixel size"),
	TSPROPFLOAT_Time						UMETA( DisplayName = "Time"),
	TSPROPFLOAT_InterpolationTimeSeconds	UMETA( DisplayName = "Interpolation Time Seconds"),
	TSPROPFLOAT_InterpolationTimeDays		UMETA( DisplayName = "Interpolation Time Days"),
	TSPROPFLOAT_RainDepthTextureScale		UMETA( DisplayName = "RainDepth Texture Scale"),
	TSPROPFLOAT_WaterGameTime				UMETA( DisplayName = "Water Uses Game Time"),
	TSPROPFLOAT_Max							UMETA( Hidden )
};

// Enumeration for the float-based water properties.
UENUM(BlueprintType)
enum class ETrueSkyWaterPropertiesFloat : uint8
{
	TSPROPFLOAT_Water_BeaufortScale				UMETA(DisplayName = "Beaufort Scale"),
	TSPROPFLOAT_Water_WindSpeed					UMETA(DisplayName = "Wind Speed"),
	TSPROPFLOAT_Water_WindDirection				UMETA(DisplayName = "Wind Direction"),
	TSPROPFLOAT_Water_WindDependency			UMETA(DisplayName = "Wind Dependency"),
	TSPROPFLOAT_Water_WaveAmplitude				UMETA(DisplayName = "Wave Amplitude"),
	TSPROPFLOAT_Water_FoamStrength				UMETA(DisplayName = "Foam Strength"),
	TSPROPFLOAT_Water_Rotation					UMETA(DisplayName = "Rotation"),
	TSPROPFLOAT_Water_MaxWavelength				UMETA(DisplayName = "Max Wavelength"),
	TSPROPFLOAT_Water_MinWavelength				UMETA(DisplayName = "Min Wavelength"),
	TSPROPFLOAT_Water_ShoreDepthExtent			UMETA(DisplayName = "Shore Depth Extent"),
	TSPROPFLOAT_Water_ShoreDepthWidth			UMETA(DisplayName = "Shore Depth Width"),
	TSPROPFLOAT_Water_Max						UMETA(Hidden)
};

// Enumeration for the positional float-based truesky properties.
UENUM( BlueprintType )
enum class ETrueSkyPositionalPropertiesFloat : uint8
{
	TSPROPFLOAT_Cloudiness				UMETA( DisplayName = "Cloudiness" ),
	TSPROPFLOAT_Precipitation			UMETA( DisplayName = "Precipitation" ),
	TSPROPFLOAT_Max						UMETA( Hidden )
};

// Enumeration for the vector-based truesky properties.
UENUM( BlueprintType )
enum class ETrueSkyPropertiesVector : uint8
{
	TSPROPVECTOR_CloudShadowOriginKm					UMETA( DisplayName = "Cloud Shadow Origin Km" ),
	TSPROPVECTOR_CloudShadowScaleKm						UMETA( DisplayName = "Cloud Shadow Scale Km" ),
	TSPROPVECTOR_SunDirection							UMETA( DisplayName = "Override direction of the sun." ),
	TSPROPVECTOR_MoonDirection							UMETA( DisplayName = "Override direction of the moon." ),
	TSPROPVECTOR_Render_WindSpeedMS						UMETA( DisplayName = "Overall wind speed vector." ),
	TSPROPVECTOR_Render_Origin							UMETA(DisplayName = "Origin as normalized quaternion XYZ at startup."),
	TSPROPVECTOR_Render_OriginLatLongHeadingDeg			UMETA( DisplayName = "Latitude, longitude and heading of origin, at startup."),
	TSPROPVECTOR_SunlightTableTransform					UMETA(DisplayName = "Transform for sunlight altitude table."),
	TSPROPVECTOR_Max									UMETA( Hidden )
};

// Enumeration for the vector-based water properties.
UENUM(BlueprintType)
enum class ETrueSkyWaterPropertiesVector : uint8
{
	TSPROPVECTOR_Water_Location						UMETA(DisplayName = "Location"),
	TSPROPVECTOR_Water_Dimension					UMETA(DisplayName = "Dimension"),
	TSPROPVECTOR_Water_Scattering					UMETA(DisplayName = "Scattering"),
	TSPROPVECTOR_Water_Absorption					UMETA(DisplayName = "Absorption"),
	TSPROPVECTOR_Water_ShoreDepthTextureLocation	UMETA(DisplayName = "Shore Depth Texture Location"),
	TSPROPVECTOR_Water_Max						UMETA(Hidden)
};