// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkySequencePropertiesFloat.generated.h"

// Enumeration for the float-based sky sequencer sky-layer properties.
UENUM( BlueprintType )
enum class ETrueSkySkySequenceLayerPropertiesFloat : uint8
{
	TSPROPFLOAT_BrightnessPower					UMETA( DisplayName = "Brightness Power" ),
	TSPROPFLOAT_MaxAltitudeKm					UMETA( DisplayName = "Max Altitude (Km)" ),
	TSPROPFLOAT_MaxDistanceKm					UMETA( DisplayName = "Max Distance (Km)" ),
	TSPROPFLOAT_OvercastEffectStrength			UMETA( DisplayName = "Overcast Effect Strength" ),
	TSPROPFLOAT_OzoneStrength					UMETA( DisplayName = "Ozone Strength" ),
	TSPROPFLOAT_Emissivity						UMETA( DisplayName = "Emissivity" ),
	TSPROPFLOAT_SunRadiusArcMinutes				UMETA( DisplayName = "Diameter (Sun)" ),
	TSPROPFLOAT_SunBrightnessLimit				UMETA( DisplayName = "Brightness Limit (Sun)" ),
	TSPROPFLOAT_LatitudeRadians					UMETA( DisplayName = "Latitude" ),
	TSPROPFLOAT_LongitudeRadians				UMETA( DisplayName = "Longitutde" ),
	TSPROPFLOAT_TimezoneHours					UMETA( DisplayName = "Timezone +/-" ),
	TSPROPFLOAT_MoonAlbedo						UMETA( DisplayName = "Albedo (Moon)" ),
	TSPROPFLOAT_MoonRadiusArcMinutes			UMETA( DisplayName = "Diameter (Moon)" ),
	TSPROPFLOAT_MaxStarMagnitude				UMETA( DisplayName = "Max Magnitude (Star)" ),
	TSPROPFLOAT_StarBrightness					UMETA( DisplayName = "Brightness (Stars)" ),
	TSPROPFLOAT_BackgroundBrightness			UMETA( DisplayName = "Brightness (Background)" ),
	TSPROPFLOAT_SunIrradianceRed				UMETA( DisplayName = "Sun Irradiance (Red)"),
	TSPROPFLOAT_SunIrradianceGreen				UMETA( DisplayName = "Sun Irradiance (Green)"),
	TSPROPFLOAT_SunIrradianceBlue				UMETA( DisplayName = "Sun Irradiance (Blue)"),
	TSPROPFLOAT_ColourWavelengthsNmRed			UMETA( DisplayName = "Colour Wavelengths Nm (Red)"),
	TSPROPFLOAT_ColourWavelengthsNmGreen		UMETA( DisplayName = "Colour Wavelengths Nm (Green)"),
	TSPROPFLOAT_ColourWavelengthsNmBlue			UMETA( DisplayName = "Colour Wavelengths Nm (Blue)"),

	TSPROPFLOAT_Max								UMETA( Hidden )
};

// Enumeration for the float-based sky sequencer sky-keyframe properties.
UENUM( BlueprintType )
enum class ETrueSkySkySequenceKeyframePropertiesFloat : uint8
{
	TSPROPFLOAT_Daytime						UMETA( DisplayName = "Time" ),
	TSPROPFLOAT_Haze						UMETA( DisplayName = "Haze (i.e. Fog)" ),
	TSPROPFLOAT_HazeBaseKm					UMETA( DisplayName = "Haze Base (Km)" ),
	TSPROPFLOAT_HazeScaleKm					UMETA( DisplayName = "Haze Scale (Km)" ),
	TSPROPFLOAT_HazeEccentricity			UMETA( DisplayName = "Eccentricity" ),
	TSPROPFLOAT_FogCeilingKm				UMETA( DisplayName = "Fog Height (Km)"),
	TSPROPFLOAT_GroundFog					UMETA( DisplayName = "Ground Fog"),
	TSPROPFLOAT_MieRed						UMETA( DisplayName = "Mie Coefficient (Red)" ),
	TSPROPFLOAT_MieGreen					UMETA( DisplayName = "Mie Coefficient (Green)" ),
	TSPROPFLOAT_MieBlue						UMETA( DisplayName = "Mie Coefficient (Blue)" ),
	TSPROPFLOAT_SunElevation				UMETA( DisplayName = "Sun Elevation" ),
	TSPROPFLOAT_SunAzimuth					UMETA( DisplayName = "Sun Azimuth" ),
	TSPROPFLOAT_MoonElevation				UMETA( DisplayName = "Moon Elevation" ),
	TSPROPFLOAT_MoonAzimuth					UMETA( DisplayName = "Moon Azimuth" ),
	TSPROPFLOAT_SeaLevelTemperatureK		UMETA( DisplayName = "Sea Level Deg. K"),

	TSPROPFLOAT_Max							UMETA( Hidden )
};

// Enumeration for the float-based sky sequencer cloud-layer properties.
UENUM( BlueprintType )
enum class ETrueSkyCloudSequenceLayerPropertiesFloat : uint8
{

	TSPROPFLOAT_NoisePeriod					UMETA( DisplayName = "Noise Period" ),
	TSPROPFLOAT_NoisePhase					UMETA( DisplayName = "Noise Phase" ),
	TSPROPFLOAT_Max							UMETA( Hidden )
};

// Enumeration for the float-based sky sequencer cloud-keyframe properties.
UENUM( BlueprintType )
enum class ETrueSkyCloudSequenceKeyframePropertiesFloat : uint8
{
	TSPROPFLOAT_Cloudiness				UMETA(DisplayName = "Cloudiness"),
	TSPROPFLOAT_CloudBase				UMETA(DisplayName = "Cloud Base"),
	TSPROPFLOAT_CloudHeight				UMETA(DisplayName = "Cloud Height"),
	TSPROPFLOAT_CloudWidthKm			UMETA(DisplayName = "Cloud Width (Km)"),
	TSPROPFLOAT_DistributionBaseLayer	UMETA(DisplayName = "Base Layer"),
	TSPROPFLOAT_DistributionTransition	UMETA(DisplayName = "Transition Layer"),
	TSPROPFLOAT_UpperDensity			UMETA(DisplayName = "Upper Layer"),
	TSPROPFLOAT_Persistence				UMETA(DisplayName = "Persistence"),
	TSPROPFLOAT_WorleyNoise				UMETA(DisplayName = "Worley Noise"),
	TSPROPFLOAT_WorleyScale				UMETA(DisplayName = "Worley Scale"),
	TSPROPFLOAT_Diffusivity				UMETA(DisplayName = "Diffusivity"),
	TSPROPFLOAT_FractalWavelength		UMETA(DisplayName = "Fractal Wavelength"),
	TSPROPFLOAT_FractalAmplitude		UMETA(DisplayName = "Fractal Amplitude"),
	TSPROPFLOAT_EdgeWorleyScale			UMETA(DisplayName = "Edge Worley Scale"),
	TSPROPFLOAT_EdgeWorleyNoise			UMETA(DisplayName = "Edge Worley Noise"),
	TSPROPFLOAT_EdgeSharpness			UMETA(DisplayName = "Edge Sharpness"),
	TSPROPFLOAT_BaseNoiseFactor			UMETA(DisplayName = "Base Noise Factor"),
	TSPROPFLOAT_Churn					UMETA(DisplayName = "Churn"),
	TSPROPFLOAT_RainRadiusKm			UMETA(DisplayName = "Rain Radius (Km)"),
	TSPROPFLOAT_Precipitation			UMETA(DisplayName = "Precipitation Strength"),
	TSPROPFLOAT_RainEffectStrength		UMETA(DisplayName = "Rain Strength"),
	TSPROPFLOAT_RainToSnow				UMETA(DisplayName = "Rain to Snow"),
	TSPROPFLOAT_Offsetx					UMETA(DisplayName = "Offset X (Map)"),
	TSPROPFLOAT_Offsety					UMETA(DisplayName = "Offset Y (Map)"),
	TSPROPFLOAT_RainCentreXKm			UMETA(DisplayName = "Region Centre X (Precipitation)"),
	TSPROPFLOAT_RainCentreYKm			UMETA(DisplayName = "Region Centre Y (Precipitation)"),

	TSPROPFLOAT_Max							UMETA( Hidden )
};


// Enumeration for the float-based sky sequencer cloud-keyframe properties.
UENUM(BlueprintType)
enum class ETrueSkyRenderingProperties : uint8
{
	TSPROPFLOAT_Render_HighDetailProportion				UMETA(DisplayName = "For cloud volume update rate."),
	TSPROPFLOAT_Render_MediumDetailProportion			UMETA(DisplayName = "For medium cloud volume update rate."),
	TSPROPFLOAT_Render_CloudShadowStrength				UMETA(DisplayName = "Strength of the cloud shadows."),
	TSPROPFLOAT_Render_CloudShadowRangeKm				UMETA(DisplayName = "Maximum range for the cloud shadows to cover."),
	TSPROPFLOAT_Render_CrepuscularRayStrength			UMETA(DisplayName = "Strength of the godrays."),
	TSPROPFLOAT_Render_PrecipitationRadiusMetres		UMETA(DisplayName = "Size of the particle zone."),
	TSPROPFLOAT_Render_RainFallSpeedMS					UMETA(DisplayName = "Speed of falling rain."),
	TSPROPFLOAT_Render_RainDropSizeMm					UMETA(DisplayName = "Size of a single raindrop."),
	TSPROPFLOAT_Render_SnowFallSpeedMS					UMETA(DisplayName = "Speed of falling snow."),
	TSPROPFLOAT_Render_SnowFlakeSizeMm					UMETA(DisplayName = "Size of a single snowflake.".),
	TSPROPFLOAT_Render_PrecipitationWindEffect			UMETA(DisplayName = "Strength of the effect of wind on precipitation direction."),
	TSPROPFLOAT_Render_PrecipitationWaver				UMETA(DisplayName = "How much precipitation layers waver as they fall."),
	TSPROPFLOAT_Render_PrecipitationWaverTimescaleS		UMETA(DisplayName = "Time scale of wavering in seconds."),
	TSPROPFLOAT_Render_PrecipitationThresholdKm			UMETA(DisplayName = "Precipitation Threshold."),
	TSPROPFLOAT_Render_EdgeNoisePersistence				UMETA(DisplayName = "Edge Noise Persistence."),
	TSPROPFLOAT_Render_EdgeNoiseWavelengthKm			UMETA(DisplayName = "Edge Noise Wavelength."),
	TSPROPFLOAT_Render_CellNoiseWavelengthKm			UMETA(DisplayName = "Wavelength of cell, or Worley noise."),
	TSPROPFLOAT_Render_MaxFractalAmplitudeKm			UMETA(DisplayName = "Fractal Amplitude."),
	TSPROPFLOAT_Render_MaxCloudDistanceKm				UMETA(DisplayName = "Maximum distance to render clouds."),
	TSPROPFLOAT_Render_RenderGridXKm					UMETA(DisplayName = "Minimum grid width for raytracing."),
	TSPROPFLOAT_Render_RenderGridZKm					UMETA(DisplayName = "Minimum grid height for raytracing."),
	TSPROPFLOAT_Render_CloudThresholdDistanceKm			UMETA(DisplayName = "Cloud Threshold Distance Km."),
	TSPROPFLOAT_Render_DirectLight						UMETA(DisplayName = "The amount of direct light to be used for rendering."),
	TSPROPFLOAT_Render_IndirectLight					UMETA(DisplayName = "The amount of indirect or secondary light to be used for rendering."),
	TSPROPFLOAT_Render_AmbientLight						UMETA(DisplayName = "The amount of ambient light to be used for rendering."),
	TSPROPFLOAT_Render_Extinction						UMETA(DisplayName = "The amount of light scattered per metre - larger values produce darker clouds, default 0.05."),
	TSPROPFLOAT_Render_MieAsymmetry						UMETA(DisplayName = "Mie scattering eccentricity."),
	TSPROPINT_Render_DefaultNumSteps					UMETA(DisplayName = "Steps for cloud rendering (trueSKY 4.2 and above)."),
	TSPROPINT_Render_MaxPrecipitationParticles			UMETA(DisplayName = "Max Particles (Precipitation)"),
	TSPROPINT_Render_EdgeNoiseFrequency					UMETA(DisplayName = "Edge Noise Frequency"),
	TSPROPINT_Render_EdgeNoiseTextureSize				UMETA(DisplayName = "Edge Noise Texture Size"),
	TSPROPINT_Render_AutomaticRainbowPosition			UMETA(DisplayName = "Whether the rainbow follows the antisolar/antilunar point, or is manually set."),
	TSPROPFLOAT_Render_RainbowElevation					UMETA(DisplayName = "Manually set the elevation of the rainbow."),
	TSPROPFLOAT_Render_RainbowAzimuth					UMETA(DisplayName = "Manually set the azimuth of the rainbow."),
	TSPROPFLOAT_Render_RainbowIntensity					UMETA(DisplayName = "Overall brightness of the rainbow."),
	TSPROPFLOAT_Render_RainbowDepthPoint				UMETA(DisplayName = "Set the point at which the rainbow intersects the terrain."),
	TSPROPINT_Render_AllowOccludedRainbow				UMETA(DisplayName = "Whether trueSKY should generate rainbows regardless of light occlusion."),
	TSPROPINT_Render_AllowLunarRainbow					UMETA(DisplayName = "Whether trueSKY should generate rainbows using the moon's light."),
	TSPROPFLT_Render_Max								UMETA(Hidden)
};