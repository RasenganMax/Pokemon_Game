// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Misc/Variant.h"
#include "UObject/ObjectMacros.h"

#include "Engine/Texture2D.h"
#include "Engine/Light.h"
#include "Engine/DirectionalLight.h"
#include "Math/Vector.h"
#include "Math/TransformNonVectorized.h"

#include "ITrueSkyPlugin.h"
#include "TrueSkySequencePropertiesFloat.h"
#include "TrueSkySequencePropertiesInt.h"
#include "TrueSkyPropertiesFloat.h"
#include "TrueSkyPropertiesInt.h"
#include "TrueSkySequenceAsset.h"

#include "TrueSkySequenceActor.generated.h"

UENUM()
enum class EInterpolationMode : uint8
{
	FixedNumber 	= 0x0,
	GameTime	= 0x1,
	RealTime	= 0x2
};

UENUM()
enum class ETrueSkyLightUnits : uint8
{
	Radiometric = 0x0,
	Photometric = 0x1
};

UENUM()
enum class EIntegrationScheme : uint8
{
	Grid	= 0x0,
	Fixed	= 0x1,
	VariableGrid	= 0x2
};

UENUM()
enum class PrecipitationOptionsEnum : uint8
{
	None			= 0x0,
	VelocityStreaks	= 0x1,
	SimulationTime	= 0x2,
	Paused			= 0x4
};

UENUM()
enum class EWaterResolution : uint8
{
	FullSize = 0x0,
	HalfSize = 0x1
};

UENUM( )
enum class ETrueSkyVersion : uint8
{
	Version4_1 	= 0x0		UMETA( DisplayName = "trueSKY Version 4.1" ),
	Version4_1a = 0x1		UMETA( DisplayName = "trueSKY Version 4.1a" ),
	Version4_2 	= 0x2		UMETA( DisplayName = "trueSKY Version 4.2" ),
	Version4_2a = 0x3		UMETA( DisplayName = "trueSKY Version 4.2a" ),
	Version5_0 	= 0x4		UMETA( DisplayName = "trueSKY Version 5.0" ),
};

USTRUCT(BlueprintType)
struct FMoon
{
	GENERATED_BODY()
	FMoon()
	{
		Texture=nullptr;
		LongitudeOfAscendingNode=125.1228;
		LongitudeOfAscendingNodeRate=-0.0529538083;
		Inclination=5.1454;
		ArgumentOfPericentre=318.0634;
		ArgumentOfPericentreRate=0.1643573223;
		MeanDistance=60.2666;
		Eccentricity=0.054900;
		MeanAnomaly=115.3654;
		MeanAnomalyRate=13.0649929509;
		RadiusArcMinutes=16.f;
		Render=true;
	}
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="General")
	FString Name;
	
	/** Texture to render, optional.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Visual")
	UTexture2D* Texture;
	
	/** Colour to render.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Visual")
	FLinearColor Color;

	/** Optional 3D mesh to draw.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Visual")
	UStaticMesh *StaticMesh;

	/** Material to use for mesh.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Visual")
	UMaterialInterface *MeshMaterial;
	 
	UPROPERTY(EditAnywhere,Category="Orbit", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	double LongitudeOfAscendingNode=125.1228;
	UPROPERTY(EditAnywhere,Category="Orbit", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	double LongitudeOfAscendingNodeRate=-0.0529538083;
	UPROPERTY(EditAnywhere,Category="Orbit",	meta = (ClampMin = "0.0", ClampMax = "360.0"))
	double Inclination=5.1454;
	UPROPERTY(EditAnywhere,Category="Orbit",	meta = (ClampMin = "0.0", ClampMax = "360.0"))
	double ArgumentOfPericentre=318.0634;
	UPROPERTY(EditAnywhere,Category="Orbit",	meta = (ClampMin = "0.0", UIMax = "10.0"))
	double ArgumentOfPericentreRate=0.1643573223;
	UPROPERTY(EditAnywhere,Category="Orbit",	meta = (ClampMin = "0.0", UIMax = "100.0"))
	double MeanDistance=60.2666;
	UPROPERTY(EditAnywhere,Category="Orbit",	meta = (ClampMin = "0.0", UIMax = "1.0"))
	double Eccentricity=0.054900;
	UPROPERTY(EditAnywhere,Category="Orbit",	meta = (ClampMin = "0.0", ClampMax = "360.0"))
	double MeanAnomaly=115.3654;
	UPROPERTY(EditAnywhere,Category="Orbit",	meta = (ClampMin = "0.0", UIMax = "10.0"))
	double MeanAnomalyRate=13.0649929509;
	UPROPERTY(EditAnywhere,Category="Shape",	meta=(ClampMin="0", UIMax="1000"))
	float RadiusArcMinutes=16.f;

	// For tracking
	UStaticMeshComponent *CurrentMeshComponent=nullptr;
	FVector Direction;
	FQuat Orientation;
	bool Render = true;
};
/*
example
 enum class ETestType : uint8
 {
     None                = 0            UMETA(DisplayName="None"),
     Left                = 0x1        UMETA(DisplayName=" Left"),
     Right                = 0x2        UMETA(DisplayName=" Right"),
 };
 
 ENUM_CLASS_FLAGS(ETestType);

  UPROPERTY()
 TEnumAsByte<EnumName> VarName;
*/

// Declarations.
class ALight;
class UDirectionalLightComponent;

class UTrueSkySequenceAsset;

UCLASS(Blueprintable, HideCategories = (Actor,Cooking, Collision, Rendering, Input, LOD))
class TRUESKYPLUGIN_API ATrueSkySequenceActor : public AActor
{
	GENERATED_BODY( )

protected:
	~ATrueSkySequenceActor( );

public:
	ATrueSkySequenceActor( const class FObjectInitializer& ObjectInitializer );
	//List of trueSKY Properties, ordered by their order on the Actor. Try to keep new values to their sections.

	//General Settings

	/** What is the current active sequence for this actor? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TrueSky)
	UTrueSkySequenceAsset* ActiveSequence;

	/** Tells trueSKY how many metres there are in a single UE4 distance unit. Typically 0.01 (1cm per unit).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSky, meta = (ClampMin = "0.001", ClampMax = "1000.0"))
	float MetresPerUnit;

	/** If trueSKY is visible in the scene*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSky)
	bool Visible;

	/**Should trueSKY share buffer for VR*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSky)
	bool ShareBuffersForVR;

	/** For debugging only, otherwise leave as -1.0*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = TrueSky)
	float FixedDeltaTime;

	/** When multiple trueSKY Sequence Actors are present in a level, the active one is selected using this checkbox.*/
	UPROPERTY(AdvancedDisplay, BlueprintReadWrite, EditAnywhere, Category = TrueSky)
	bool ActiveInEditor;

	/** Parameters used by trueSKY for Lighting function*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = TrueSky)
	UMaterialParameterCollection* TrueSkyMaterialParameterCollection;

	//Time Settings

	/**Current time. Uses decimals, Length of a day = Time Scale)*/
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = Time)
	float Time;
	/**Speed at which time progresses (1 = real time, 60 = 1 minute per second, 3600 = 1 hour per second)*/
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = Time, meta = (UIMin = "0", UIMax = "3600", DisplayName = "Progression Scale"))
	float TimeProgressionScale;
	/**Scale used for time. A value of 1 means a day is represented by 0-1. 24 would calculate days from 0-24. */
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = Time ,meta= (ClampMin = "1", UIMax = "1440"))
	float TimeUnits;

	/**Scene will jump at the end of the loop. Useful for testing*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = Time)
	bool Loop;
	/**Start time of the loop*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = Time)
	float LoopStart;
	/**End time of the loop*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = Time)
	float LoopEnd;

	/**Set Time hours to the current time in the Scene.*/
	/*UFUNCTION(CallInEditor, Category =Time)
	void GetCurrentTime();
	*/

	//Cloud Settings

	/** The largest cubemap resolution to be used for cloud rendering. Typically 1/4 of the screen width, larger values are more expensive in time and memory.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clouds, meta = (ClampMin = "64", ClampMax = "4096", DisplayName = "Maximum Cloud Resolution"))
	int32 MaximumResolution;

	/** Global wind speed in metres per second. WIll only adjust the churn on the clouds. Cloud movement should be done through the Cloud Window*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "-10000", ClampMax = "10000.0"))
	FVector			WindSpeedMS;

	//Rendering

	/** Maximum distance to render clouds.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "100.0", ClampMax = "1000.0"))
	float			MaxCloudDistanceKm;


	/** Raytracing integration scheme. Grid is more accurate, but slower.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (EditCondition = "bSimulVersion4_1"))
	EIntegrationScheme IntegrationScheme;

	/**Grid Clouds are rendered on. Decrease for higher quality. High performance cost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (EditCondition = "bGridRendering4_2", ClampMin = "0.01", ClampMax = "10.0"))
	float			RenderGridXKm;

	/**Grid Clouds are rendered on. Decrease for higher quality. High performance cost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (EditCondition = "bGridRendering4_2", ClampMin = "0.01", ClampMax = "10.0"))
	float			RenderGridZKm;

	/** The number of raytracing steps to take when rendering clouds, larger values are slower to render.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "20", ClampMax = "500"))
	int32			DefaultCloudSteps;

	/** Tells trueSKY how to spread the cost of rendering over frames. For 1, all pixels are drawn every frame, for amortization 2, it's 2x2, etc.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (ClampMin = "1", ClampMax = "8"))
	int32			Amortization;

	/** A heuristic distance to discard near depths from depth interpolation, improving accuracy of upscaling.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float			CloudThresholdDistanceKm;

	/** The size, in pixels, of the sampling area in the full-resolution depth buffer, which is used to find near/far depths.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (EditCondition = "bSimulVersion4_1", ClampMin = "0.0", ClampMax = "4.0"))
	float DepthSamplingPixelRange;

	/** The alpha for temporal blending of the solid depth buffer into the buffer used for cloud rendering. If 1.0, update is instant.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float DepthTemporalAlpha;

	/** Tells trueSKY whether to blend clouds with scenery, or to draw them in front/behind depending on altitude.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Rendering")
	bool DepthBlending;

	/** Apply a colour tint to all clouds. Must be using Variable Grid Integration Scheme.*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = Clouds, meta=(EditCondition = "bSimulVersion4_2"))
	FLinearColor CloudTint;


	//Noise


	/** Persistence of edge noise texture.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Noise", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.0", ClampMax = "1.0"))
	float			EdgeNoisePersistence;

	/** Frequency of edge noise texture.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Noise", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "4", ClampMax = "16"))
	int				EdgeNoiseFrequency;

	/** Size of edge noise texture.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Noise", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "8", ClampMax = "256"))
	int				EdgeNoiseTextureSize;

	/** Wavelength of edge noise effect.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Noise", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.1", ClampMax = "100.0"))
	float			EdgeNoiseWavelengthKm;

	/** Wavelength of cell noise effect.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Noise", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.1", ClampMax = "100.0"))
	float			CellNoiseWavelengthKm;

	/** Size of the 3D cell-noise texture used to generate clouds. Larger values use more GPU memory.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Noise", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "16", ClampMax = "256"))
	int32			CellNoiseTextureSize;

	/** Strength of edge noise effect.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Noise", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.1", ClampMax = "10.0"))
	float			MaxFractalAmplitudeKm;

	//Cloud Lighting

	/** Effect of direct light on the clouds.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Cloud Lighting", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.0", ClampMax = "5.0"))
	float			DirectLight;

	/** Effect of indirect light on the clouds.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Cloud Lighting", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.0", ClampMax = "5.0"))
	float			IndirectLight;

	/** Effect of ambient light on the clouds.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Cloud Lighting", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.0", ClampMax = "5.0"))
	float			AmbientLight;

	/** Direct light extinction per kilometre.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Cloud Lighting", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.0", ClampMax = "100.0"))
	float			Extinction;

	/** Asymmetry of direct light.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Cloud Lighting", meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.0", ClampMax = "0.999"))
	float			MieAsymmetry;



	//Sky Lighting

	/** Assign a directional light to this to update it in real time.*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Lighting)
	ALight *Light;

	/** If trueSKY should control the direction light's rotation*/ 
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category=Lighting)
	bool DriveLightDirection;


	/** Starlight is the light applied when the Sun and Moon are below the Horizon. This will override the moon's colour if it is brighter.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Stars", meta=(ClampMin = "0.0", UIMax = "1.0"))
	float StarLightIntensity;

	/** Should StarLight cast shadows*/ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Stars")
	bool StarLightCastsShadows;
	
	/** The colour of the starlight*/ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Stars")
	FLinearColor StarLightColor;
			
	/**Manual input can allow values above 10*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = Lighting, meta=(UIMin = "0.0", UIMax = "10.0"))
	float SkyMultiplier;

	/**Manual input can allow values above 10*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Lighting,meta=(UIMin = "0.0", UIMax = "10.0"))
	float SunMultiplier;

	/**Manual input can allow values above 10*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Lighting,meta=(UIMin = "0.0", UIMax = "10.0"))
	float MoonMultiplier;
	
/** A multiplier for brightness of the trueSKY environment, 1.0 by default.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float Brightness;

	/** A gamma correction factor, 1.0 is correct for true physically-based rendering (PBR).*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float Gamma;
	
	/** If trueSKY should use Photometric or Radiometric Units. Be aware Photometric units need an understanding of Unreal's Exposure Settings to use correctly. When using Photmetric, Reset the Irradiance Values and set Brightness Power to 1.*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = Lighting)
	ETrueSkyLightUnits TrueSkyLightUnits;
	
	/** If lighting should vary with altitude.*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = Lighting, meta = (EditCondition = "bSimulVersion4_2&&bAltitudeDirectionalLight"))
	bool AltitudeLighting;

	/** The render texture to fill in with the cloud shadow.*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, Category = Lighting, BlueprintReadWrite, meta = (EditCondition = "bSimulVersion4_2&&bAltitudeDirectionalLight"))
	UTextureRenderTarget2D* AltitudeLightRT;


	//Atmospherics

	/** The maximum sun brightness to be rendered by trueSKY, in radiance units.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Atmospherics, meta = (ClampMin = "0.0", UIMax = "1000000"))
	float MaxSunRadiance;

	/** Whether to adjust the sun radius with maximum radiance to keep total irradiance constant.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Atmospherics)
	bool AdjustSunRadius;

	/** Tells trueSKY how to spread the cost of rendering atmospherics over frames.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Atmospherics, meta = (ClampMin = "1", ClampMax = "8"))
	int32 AtmosphericsAmortization;


	/** The smallest size stars can be drawn. If zero they are drawn as points, otherwise as quads. Use this to compensate for artifacts caused by antialiasing.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category="Lighting|Stars", meta = (EditCondition = "bSimulVersion4_1", ClampMin = "0.0", ClampMax = "20.0"))
	float MinimumStarPixelSize;

	/** The texture to draw for the cosmic background - e.g. the Milky Way.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Atmospherics)
	UTexture2D* CosmicBackgroundTexture;

	/** Default latitude of the origin in degrees.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Atmospherics, meta = (EditCondition = "bSimulVersion4_2", ClampMin = "-90.0", ClampMax = "90.0"))
	float			OriginLatitude;

	/** Default longitude of the origin in degrees.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Atmospherics, meta = (EditCondition = "bSimulVersion4_2", ClampMin = "-180.0", ClampMax = "180.0"))
	float			OriginLongitude;

	/** Default orientation of the origin Y axis in degrees clockwise from North.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Atmospherics, meta = (EditCondition = "bSimulVersion4_2", ClampMin = "-180.0", ClampMax = "180.0"))
	float			OriginHeading;


	//Moons

/**Array of Moons for multi-moon systems*/
	UPROPERTY(EditAnywhere, Category = Moons)
	TArray<FMoon> Moons;
	UPROPERTY(EditAnywhere, Category = Moons)
	uint8 MoonLightingChannel;

	/** A directional light to illuminate moon meshes.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Moons)
	ALight *LightForMoons;
	
	/** The texture to draw for the moon.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Atmospherics, meta = (EditCondition = "!bSimulVersion4_2"))
	UTexture2D* MoonTexture;

	//Interpolation

	/** The method to use for interpolation. Used from trueSKY 4.1 onwards.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Interpolation)
	EInterpolationMode InterpolationMode;

	/** The time for real time interpolation in seconds. Used from trueSKY 4.1 onwards.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Interpolation, meta = (ClampMin = "0", UIMax = "3600", EditCondition = "bInterpolationRealTime"))
	float InterpolationTimeSeconds;

	/** The time for game time interpolation in hours. Used from trueSKY 4.1 onwards.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Interpolation, meta = (ClampMin = "0", UIMax = "24", EditCondition = "bInterpolationGameTime"))
	float InterpolationTimeHours;

	/** The number of subdivisions created for updates*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Interpolation, meta = (ClampMin = "4", ClampMax = "16"))
	int32 InterpolationSubdivisions;

	/** Number of mipmaps for cloud volume (trueSKY 4.2 and above).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Interpolation, meta = (EditCondition = "bSimulVersion4_2", ClampMin = "1", ClampMax = "4"))
	int32			MaxVolumeMips;

	/** High-detail update area (trueSKY 4.2 and above).*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Interpolation, meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.0", ClampMax = "1.0"))
	float			HighDetailProportion;

	/** Medium-detail update area (trueSKY 4.2 and above).*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Interpolation, meta = (EditCondition = "bSimulVersion4_2", ClampMin = "0.0", ClampMax = "1.0"))
	float			MediumDetailProportion;

	/** By default, changes to sun direction or keyframe properties cause instant updates to the sky. This may not be wanted for performance reasons.*/
	UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, Category = Interpolation)
	bool InstantUpdate;


	//Shadows


	/** Range of cloud shadows.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Shadows, meta = (ClampMin = "10.0", ClampMax = "200.0"))
	float			CloudShadowRangeKm;

	/** Size of square cloud shadow texture.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shadows, meta = (ClampMin = "32", UIMax = "2048", ClampMax = "4096"))
	int				ShadowTextureSize;

	/** Strength of cloud shadow.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Shadows, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float			ShadowStrength;

	/** Strength of cloud shadow.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = Shadows, meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float			ShadowEdgeSmoothness;

	/** The render texture to fill in with the cloud shadow.*/
	UPROPERTY(EditAnywhere, Category=Shadows, BlueprintReadWrite)
	UTextureRenderTarget2D* CloudShadowRT;

	/** Strength of crepuscular rays (godrays).*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category=Shadows, meta = (ClampMin = "0.0", UIMax = "1", ClampMax = "100.0"))
	float			CrepuscularRayStrength;

	/** Grid size for crepuscular rays.*/
	UPROPERTY(EditAnywhere, Category=Shadows, meta = (ClampMin = "8", ClampMax = "256"))
	int32 CrepuscularGrid[3];





	//Precipitation
	/** Maximum number of particles to render.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "1000", ClampMax = "1000000"))
	int				MaxPrecipitationParticles;

	/** Radius over which to draw precipitation.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float			PrecipitationRadiusMetres;

	/** Speed in metres per second.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float			RainFallSpeedMS;

	/** Raindrop size.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.1", UIMax = "10.0"))
	float			RainDropSizeMm;

	/** Speed in metres per second.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float			SnowFallSpeedMS;

	/** Raindrop size.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.1", UIMax = "50.0"))
	float			SnowFlakeSizeMm;

	/** Strength of wind effect on rain/snow.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float			PrecipitationWindEffect;

	/** Strength of waver of rain/snow.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float			PrecipitationWaver;

	/** Timescale in seconds of waver of rain/snow.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float			PrecipitationWaverTimescaleS;

	/** Thickness of cloud required to produce rain/snow.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.01", ClampMax = "5.0"))
	float			PrecipitationThresholdKm;


	/** If set, trueSKY will use this cubemap to light the rain, otherwise a TrueSkyLight will be used.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation")
	UTexture* RainCubemap;

	/** If RainDrops should have a streak due to velocity*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation")
	bool VelocityStreaks;

	/** If Precipitation should use Simulation Time*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation")
	bool SimulationTime;

	/** Draw no rain nearer than this distance in Unreal units.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.01", UIMax = "10.0"))
	float RainNearThreshold;

	/** Assign a SceneCapture2D actor to mask rain from covered areas.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation")
	class ASceneCapture2D* RainMaskSceneCapture2D;

	/** Width in Unreal units of the rain mask.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "100", UIMax = "100000.0"))
	float RainMaskWidth;

	/** Height in Unreal units of the rain mask volume.*/
	UPROPERTY(EditAnywhere, interp, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "100", UIMax = "100000.0"))
	float RainDepthExtent;

	/** Render texture for transparent effects. THis should automatically be applied as a post process material.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation")
	UTextureRenderTarget2D* TranslucentRT;

	//Storms
	/** Sounds to play when a storm produces thunder.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storms")
	TArray <  USoundBase*> ThunderSounds;

	/** Attenuation of the audio.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storms")
	class USoundAttenuation* ThunderAttenuation;
	
	//Rainbows
	/** Whether the rainbow follows the antisolar/antilunar point, or is manually set.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainbow")
		bool AutomaticRainbowPosition;

	/** If the automatic positioning is disabled, this sets the elevation of the rainbow.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainbow"  ,meta = (ClampMin = "-90", UIMax = "0"))
		float RainbowElevation;

	/** If the automatic positioning is disabled, this sets the azimuth of the rainbow.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainbow" ,meta = (ClampMin = "0", UIMax = "360.0"))
		float RainbowAzimuth;

	/** Controls the overall brightness of the rainbow.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainbow", meta = (UI = "0", UIMax = "100"))
		float RainbowIntensity;

	/** The point at which the rainbow intersects the terrain.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainbow", meta = (ClampMin = "0", UIMax = "1"))
		float RainbowDepthPoint;

	/** Whether trueSKY should generate rainbows regardless of light occlusion.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainbow")
		bool AllowOccludedRainbow;

	/** Whether trueSKY should generate rainbows using the moon's light.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rainbow")
		bool AllowLunarRainbow;

	//Water

	/** Enable water rendering*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water)
	bool RenderWater;

	/** Does the water use real time or game time for calculating the surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water)
	bool UseWaterGameTime;

	/** Whether to render the water at full resolution or half resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water)
	EWaterResolution WaterResolution;

	/** Enable Screen Space reflections for the water */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water)
	bool EnableReflection;

	/** Whether to render the water at full resolution or half resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water)
	EWaterResolution ReflectionResolution;

	/** The number of steps that the reflection will sample. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "10", ClampMax = "100"))
	int ReflectionSteps;

	/** The on screen pixel step for how fine a resolution the Reflection will be. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Water, meta = (ClampMin = "1", ClampMax = "10"))
	int ReflectionPixelStep;


	//Textures

	/** The render texture to fill in with atmospheric inscatter values.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSkyTextures)
	UTextureRenderTarget2D* InscatterRT;

	/** The render texture to fill in with atmospheric loss values.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSkyTextures)
	UTextureRenderTarget2D* LossRT;

	/** The render texture to fill in with atmospheric loss values.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSkyTextures)
	UTextureRenderTarget2D* CloudVisibilityRT;

	/** The render texture for debugging overlays.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TrueSkyTextures)
	UTextureRenderTarget2D* OverlayRT;

	//Functions

	/** Query the trueSKY version outside the plugin. */
	UFUNCTION( BlueprintCallable, Category = "TrueSky|Debug" )
	static ETrueSkyVersion GetTrueSkyVersion( );

	/** Returns the spectral radiance as a multiplier for colour values output by trueSKY.
		For example, a pixel value of 1.0, with a reference radiance of 2.0, would represent 2 watts per square metre per steradian. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TrueSky|Lighting")
	static float GetReferenceSpectralRadiance();

	UFUNCTION(BlueprintCallable,Category = TrueSky)
	static int32 TriggerAction(FString name);

	/** Get the c */
	UFUNCTION(BlueprintCallable,Category = "TrueSky|Storms")
	static int32 GetLightning(FVector &start,FVector &end,float &magnitude,FVector &colour);

	/** Spawn Lighting at the given position. */
	UFUNCTION( BlueprintCallable,Category = "TrueSky|Storms")
	static int32 SpawnLightning(FVector startPosition, FVector EndPosition,float magnitude,FVector colour);

	/** Set the currently active Sequence Actor. Only one Sequence Actor is recommended. */
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Debug")
	void SetActiveSequenceActor(ATrueSkySequenceActor* newActor);

	void Destroyed() override;
	void BeginDestroy() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	
	
	/** Free all allocated views in trueSKY. This frees up GPU memory.	*/
	UFUNCTION(BlueprintCallable, Category="TrueSky|Debug")
	static void FreeViews();
	
	UFUNCTION(BlueprintCallable, Category="TrueSky|Debug")
	static FString GetProfilingText(int32 cpu_level,int32 gpu_level);

	UFUNCTION( Exec )
	static FString TrueSkyProfileFrame(int32 arg1=5,int32 arg2=6);
	
	/** Returns a string from trueSKY. Valid input is "memory" or "profiling".*/
	UFUNCTION(BlueprintCallable, Category="TrueSky|Debug")
	static FString GetText(FString name);

	/** Sets the time value for trueSKY. By default, 0=midnight, 0.5=midday, 1.0= the following midnight, etc.*/
	UFUNCTION(BlueprintCallable, Category="TrueSky|Time")
	void SetTime( float value );
	
	/** Returns the time value for trueSKY. By default, 0=midnight, 0.5=midday, 1.0= the following midnight, etc.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TrueSky|Time")
	static float GetTime();

	/** Returns the named floating-point property.*/
	static float GetFloat(FString name );
	
	/** Returns the named floating-point property.*/
	//UFUNCTION(BlueprintCallable, Category=TrueSky)
	float GetFloatAtPosition(FString name,FVector pos);

	/** Set the named floating-point property.*/
	static void SetFloat(FString name, float value );
	
	/** Returns the named integer property.*/
	static int32 GetInt(FString name ) ;
	
	static void SetInt(FString name, int32 value);
	
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Debug", DisplayName =  "Set Debug Bool")
	static void SetBool(FString name, bool value );

	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetInteger( ETrueSkyPropertiesInt Property, int32 Value );
	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static int32 GetInteger( ETrueSkyPropertiesInt Property );

	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetFloatProperty( ETrueSkyPropertiesFloat Property, float Value );
	
	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetRenderingFloat( ETrueSkyRenderingProperties Property, float Value  );
	
	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetRenderingInt( ETrueSkyRenderingProperties Property, int32 Value  );

	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetRenderingVector( ETrueSkyPropertiesVector Property, FVector Value );

	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static float GetFloatProperty( ETrueSkyPropertiesFloat Property );

	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static void SetVectorProperty( ETrueSkyPropertiesVector Property, FVector Value );
	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static FVector GetVectorProperty( ETrueSkyPropertiesVector Property );

	//UFUNCTION(BlueprintCallable, Category="TrueSky|Properties")
	static float GetFloatPropertyAtPosition( ETrueSkyPositionalPropertiesFloat Property,FVector pos,int32 queryId);

	//UFUNCTION(BlueprintCallable, Category = "TrueSky|Tools")
	static float GetPrecipitationAtPosition(FVector pos, int32 queryId);

	//UFUNCTION(BlueprintCallable, Category = "TrueSky|Tools")
	static float GetCloudinessAtPosition(FVector pos, int32 queryId);

	/** Returns the named keyframe property for the keyframe identified.*/
	static float GetKeyframeFloat(int32 keyframeUid,FString name );
	
	/** Set the named keyframe property for the keyframe identified.*/
	static void SetKeyframeFloat(int32 keyframeUid,FString name, float value );
	
	/** Returns the named integer keyframe property for the keyframe identified.*/
	static int32 GetKeyframeInt(int32 keyframeUid,FString name );
	
	/** Set the named integer keyframe property for the keyframe identified.*/
	static void SetKeyframeInt(int32 keyframeUid,FString name, int32 value );
	
	/** Returns the named keyframer property for the keyframe identified.*/
	static float GetKeyframerFloat(int32 keyframeUid, FString name);

	/** Set the named keyframe property for the keyframe identified.*/
	static void SetKeyframerFloat(int32 keyframeUid, FString name, float value);

	/** Returns the named integer keyframe property for the keyframe identified.*/
	static int32 GetKeyframerInt(int32 keyframeUid, FString name);

	/** Set the named integer keyframe property for the keyframe identified.*/
	static void SetKeyframerInt(int32 keyframeUid, FString name, int32 value);
	
	/** Returns the calculated sun direction.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= "TrueSky|Sky|Get")
	static FRotator GetSunRotation();

	/** Returns the calculated moon direction.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= "TrueSky|Sky|Get")
	static FRotator GetMoonRotation();
	
	/** Override the sun direction.*/
	//UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetSunRotation(FRotator r);
	
	/** Override the moon direction.*/
	//UFUNCTION(BlueprintCallable, Category=TrueSky)
	static void SetMoonRotation(FRotator r);
	
	/** Returns the amount of cloud at the given position.*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Tools")
	static float CloudLineTest(int32 queryId,FVector StartPos,FVector EndPos);

	/** Returns the amount of cloud at the given position.*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Tools")
	static float CloudPointTest(int32 QueryId,FVector pos);

	/** Returns the cloud shadow at the given position, from 0 to 1.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= "TrueSky|Clouds|Shadows")
	static float GetCloudShadowAtPosition(int32 QueryId,FVector pos);

	/** Returns the rainfall at the given position, from 0 to 1.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TrueSky|Tools")
	static float GetPrecipitationStrengthAtPosition(int32 QueryId, FVector pos);

	/** Returns the Cloudiness at the given position, from 0 to 1.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TrueSky|Tools")
	static float GetCloudDensityAtPosition(int32 QueryId, FVector pos);

	/** Returns the calculated sun colour in irradiance units, divided by intensity.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= "TrueSky|Sky|Get")
	static FLinearColor GetSunColor();
	
	/** Returns the Sequence Actor singleton.*/
	UFUNCTION(BlueprintCallable,BlueprintPure,Category="TrueSky|Tools")
	static float GetMetresPerUnit();

	/** Returns the calculated moon intensity in irradiance units.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= "TrueSky|Sky|Get")
	static float GetSunIntensity();
	
	/** Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight actor (sunset occurs later at higher altitudes).
		Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	//UFUNCTION(BlueprintCallable, Category=TrueSky)
	static bool UpdateSunlight(class ADirectionalLight* DirectionalLight,float multiplier=1.0f,bool include_shadow=false,bool apply_rotation=true);
	
	/** Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight.
	Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	//UFUNCTION(BlueprintCallable, Category=TrueSky)
	static bool UpdateMoonlight(class ADirectionalLight* DirectionalLight,float multiplier=1.0f,bool include_shadow=false,bool apply_rotation=true);
	
	/** Update a single light with either sunlight or moonlight.
	Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight.
	Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	//UFUNCTION(BlueprintCallable, Category = TrueSky)
	static bool UpdateLight(ADirectionalLight* DirectionalLight, float sun_multiplier , float moon_multiplier , bool include_shadow , bool apply_rotation ,bool light_in_space
	, bool StarLightCastsShadows, float StarLightIntensity, FLinearColor StarLightColor);

	/** Update a single light with either sunlight or moonlight.
	Updates the colour and intensity of the light. This depends on the time of day, and also the altitude of the DirectionalLight.
	Returns true if the light was updated. Due to GPU latency, the update will not occur for the first few frames.*/
	static bool UpdateLightComponent(UDirectionalLightComponent *dlc, float sun_multiplier, float moon_multiplier, bool include_shadow, bool apply_rotation,bool light_in_space
		, bool StarLightCastsShadows, float StarLightIntensity, FLinearColor StarLightColor);

	/** Render the full sky cubemap to the specified cubemap render target.*/
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Tools")
	static void RenderToCubemap(class UTextureRenderTargetCube *RenderTargetCube, int32 cubemap_resolution);




	
	






	/** Returns the calculated moon intensity in irradiance units.*/
	UFUNCTION(BlueprintCallable, BlueprintPure,Category="TrueSky|Sky|Get")
	static float GetMoonIntensity();

	/** Returns the calculated moon colour in irradiance units, divided by intensity.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TrueSky|Sky|Get")
	static FLinearColor GetMoonColor();

	/** Illuminate the clouds with the specified values from position pos.*/
	UFUNCTION(BlueprintCallable, Category="TrueSky|Lighting")
	static void SetPointLightSource(int32 id,FLinearColor lightColour,float Intensity,FVector pos,float minRadius,float maxRadius);
	
	/** Illuminate the clouds with the specified point light.*/
	UFUNCTION(BlueprintCallable, Category="TrueSky|Lighting")
	static void SetPointLight(APointLight *source);


	/** Get an identifier for the next sky keyframe that can be altered without requiring a recalculation of the tables.*/
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sky|Keyframe|UID", DisplayName="Get Next Modifiable Sky Keyframe UID")
	static int32 GetNextModifiableSkyKeyframe();
	
	/** Get an identifier for the next cloud keyframe that can be altered without requiring a recalculation of the 3D textures.*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Clouds|Keyframe|UID", DisplayName = "Get Next Modifiable Cloud Keyframe UID")
	static int32 GetNextModifiableCloudKeyframe(int32 layer);

	/** Get an identifier for the cloud keyframe at the specified index. Returns zero if there is none at that index (e.g. you have gone past the end of the list).*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Clouds|Keyframe|UID", DisplayName = "Get Cloud Keyframe UID by Index")
	static int32 GetCloudKeyframeByIndex(int32 layer,int32 index);

	/** Get an identifier for the next cloud keyframe at or after the specified time.*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Clouds|Keyframe|UID", DisplayName = "Get Next Cloud Keyframe UID After Set Time")
	static int32 GetNextCloudKeyframeAfterTime(int32 layer,float t);

	/** Get an identifier for the last cloud keyframe before the specified time.*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Clouds|Keyframe|UID", DisplayName = "Get Previous Cloud Keyframe UID Before Set Time")
	static int32 GetPreviousCloudKeyframeBeforeTime(int32 layer,float t);
	
	/** Get an identifier for the sky keyframe at the specified index. Returns zero if there is none at that index (e.g. you have gone past the end of the list).*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Sky|Keyframe|UID", DisplayName = "Get Sky Keyframe UID by Index")
	static int32 GetSkyKeyframeByIndex(int32 index);

	/** Get an identifier for the next cloud keyframe at or after the specified time.*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Sky|Keyframe|UID", DisplayName = "Get Next Sky Keyframe UID After Set Time")
	static int32 GetNextSkyKeyframeAfterTime(float t);

	/** Get an identifier for the last sky keyframe before the specified time.*/
	UFUNCTION(BlueprintCallable, Category= "TrueSky|Sky|Keyframe|UID", DisplayName = "Get Previous Sky Keyframe UID Before Set Time")
	static int32 GetPreviousSkyKeyframeBeforeTime(float t);

	/** Get an indentifier for the interpolated cloud keyframe. Interpolated Keyframes are Read Only */
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Clouds|Keyframe|UID", DisplayName = "Get Interpolated Cloud Keyframe UID (Read Only)")
	static int32 GetInterpolatedCloudKeyframe(int32 layerUID);

	/** Get an indentifier for the interpolated sky keyframe. Interpolated Keyframes are Read Only */
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Sky|Keyframe|UID", DisplayName = "Get Interpolated Sky Keyframe UID (Read Only)")
	static int32 GetInterpolatedSkyKeyframe();
		


	UFUNCTION(BlueprintCallable, Category="TrueSky|Depreciated|Sky|Layer|Set", Meta = (DeprecatedFunction))
	static void SetSkyLayerInt( ETrueSkySkySequenceLayerPropertiesInt Property, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sky|Keyframe|Set", Meta = (DeprecatedFunction))
	static void SetSkyKeyframeInt( ETrueSkySkySequenceKeyframePropertiesInt Property, int32 KeyframeUID, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Clouds|Layer|Set", Meta = (DeprecatedFunction))
	static void SetCloudLayerInt( ETrueSkyCloudSequenceLayerPropertiesInt Property, int32 layerID, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Clouds|Keyframe|Set", Meta = (DeprecatedFunction))
	static void SetCloudKeyframeInt( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32 KeyframeUID, int32 Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sky|Layer|Get", Meta = (DeprecatedFunction))
	static int32 GetSkyLayerInt( ETrueSkySkySequenceLayerPropertiesInt Property );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sky|Keyframe|Get", Meta = (DeprecatedFunction))
	static int32 GetSkyKeyframeInt( ETrueSkySkySequenceKeyframePropertiesInt Property, int32 KeyframeUID );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Clouds|Layer|Get", Meta = (DeprecatedFunction))
	static int32 GetCloudLayerInt( ETrueSkyCloudSequenceLayerPropertiesInt Property, int32 layerID );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Clouds|Keyframe|Get", Meta = (DeprecatedFunction))
	static int32 GetCloudKeyframeInt( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32 KeyframeUID );

	UFUNCTION(BlueprintCallable, Category="TrueSky|Sky|Layer|Set", Meta = (DeprecatedFunction))
	static void SetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat Property, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sky|Keyframe|Set", Meta = (DeprecatedFunction))
	static void SetSkyKeyframeFloat( ETrueSkySkySequenceKeyframePropertiesFloat Property, int32 KeyframeUID, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Clouds|Layer|Set", Meta = (DeprecatedFunction))
	static void SetCloudLayerFloat( ETrueSkyCloudSequenceLayerPropertiesFloat Property, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Clouds|Keyframe|Set", Meta = (DeprecatedFunction))
	static void SetCloudKeyframeFloat( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, int32 KeyframeUID, float Value );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sky|Layer|Get", Meta = (DeprecatedFunction))
	static float GetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat Property );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Sky|Keyframe|Get", Meta = (DeprecatedFunction))
	static float GetSkyKeyframeFloat( ETrueSkySkySequenceKeyframePropertiesFloat Property, int32 KeyframeUID );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Clouds|Layer|Get", Meta = (DeprecatedFunction))
	static float GetCloudLayerFloat( ETrueSkyCloudSequenceLayerPropertiesFloat Property, int32 LayerUID );
	UFUNCTION(BlueprintCallable, Category="TrueSky|Clouds|Keyframe|Get", Meta = (DeprecatedFunction))
	static float GetCloudKeyframeFloat( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, int32 KeyframeUID );

	UFUNCTION(BlueprintCallable, Category = "TrueSky|Sky|Layer|Get")
	static void GetSkyLayerValue(ETrueSkySkyLayerProperties Property, int& returnInt, float& returnFloat);
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Sky|Layer|Set")
	static void SetSkyLayerValue(ETrueSkySkyLayerProperties Property, int intValue, float floatValue);
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Sky|Keyframe|Get")
	static void GetSkyKeyframeValue(ETrueSkySkyKeyframeProperties Property, int32 UID, int& returnInt, float& returnFloat);
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Sky|Keyframe|Set")
	static void SetSkyKeyframeValue(ETrueSkySkyKeyframeProperties Property, int32 UID, int intValue, float floatValue);
	
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Clouds|Layer")
	static void GetCloudLayerValue(ETrueSkyCloudLayerProperties Property, int32 layerUID, int& returnInt, float& returnFloat);
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Clouds|Layer")
	static void SetCloudLayerValue(ETrueSkyCloudLayerProperties Property, int32 layerUID, int intValue, float floatValue);
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Clouds|Keyframe")
	static void GetCloudKeyframeValue(ETrueSkyCloudKeyframeProperties Property, int32 UID, int& returnInt, float& returnFloat);
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Clouds|Keyframe")
	static void SetCloudKeyframeValue(ETrueSkyCloudKeyframeProperties Property, int32 UID, int intValue, float floatValue);



	// Properties to hide irrelevant data based on certain settings.
	UPROPERTY(Transient)
	bool bSimulVersion4_1;
	UPROPERTY(Transient)
	bool bInterpolationFixed;
	UPROPERTY(Transient)
	bool bInterpolationGameTime;
	UPROPERTY(Transient)
	bool bInterpolationRealTime;
	UPROPERTY(Transient)
	bool bSimulVersion4_2;
	UPROPERTY( Transient )
	bool bGridRendering4_2;
	UPROPERTY(Transient)
	bool bAltitudeDirectionalLight;
private:
	/** The sound to play when it is raining.*/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sound, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	//class UAudioComponent* AudioComponent;

public:
	// Map indices for the various properties.
	static int32 TSSkyLayerPropertiesFloatIdx;
	static int32 TSSkyKeyframePropertiesFloatIdx;
	static int32 TSCloudLayerPropertiesFloatIdx;
	static int32 TSCloudKeyframePropertiesFloatIdx;
	static int32 TSSkyLayerPropertiesIntIdx;
	static int32 TSSkyKeyframePropertiesIntIdx;
	static int32 TSCloudLayerPropertiesIntIdx;	
	static int32 TSCloudKeyframePropertiesIntIdx;

	static int32 TSPropertiesFloatIdx;
	static int32 TSPropertiesIntIdx;
	static int32 TSPositionalPropertiesFloatIdx;
	static int32 TSPropertiesVectorIdx;
	static int32 TSRenderingPropertiesIdx;
	static int32 TSActorPropertiesIdx;
public:
	// Enumeration names.
	typedef TMap< int64, TrueSkyPropertyFloatData > TrueSkyPropertyFloatMap;
	static TArray< TrueSkyPropertyFloatMap > TrueSkyPropertyFloatMaps;
	typedef TMap< int64, TrueSkyPropertyIntData > TrueSkyPropertyIntMap;
	static TArray< TrueSkyPropertyIntMap > TrueSkyPropertyIntMaps;
	typedef TMap< int64, TrueSkyPropertyVectorData > TrueSkyPropertyVectorMap;
	static TArray< TrueSkyPropertyVectorMap > TrueSkyPropertyVectorMaps;

	static void InitializeTrueSkyPropertyMap( );
protected:
	void UpdateEnabledProperties();
	void ApplyRenderingOptions();
	// Property setup methods.
	static void PopulatePropertyFloatMap( UEnum* EnumerationType, TrueSkyPropertyFloatMap& EnumerationMap );
	static void PopulatePropertyIntMap( UEnum* EnumerationType, TrueSkyPropertyIntMap& EnumerationMap );
	static void PopulatePropertyVectorMap( UEnum* EnumerationType, TrueSkyPropertyVectorMap& EnumerationMap );

	inline void SetFloatPropertyData( int32 EnumType, TrueSkyEnum EnumProperty, float PropertyMin, float PropertyMax )
	{
		TrueSkyPropertyFloatData* data = TrueSkyPropertyFloatMaps[EnumType].Find( EnumProperty );
	}

	inline void SetIntPropertyData( int32 EnumType, TrueSkyEnum EnumProperty, int32 PropertyMin, int32 PropertyMax )
	{
		TrueSkyPropertyIntData* data = TrueSkyPropertyIntMaps[EnumType].Find( EnumProperty );
		
	}
	// Caching modules
	static ITrueSkyPlugin *CachedTrueSkyPlugin;
	static const ITrueSkyPlugin::LightingQueryResult &GetLightingAtPosition(int32 uid, FVector pos);
public:
	static ITrueSkyPlugin *GetTrueSkyPlugin();

public:
	//static float _Time;

	void PlayThunder(FVector pos);

public:
	void CleanQueries();
	void ClearRenderTargets();
	virtual void PostRegisterAllComponents() override;
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PostInitializeComponents() override;
	bool ShouldTickIfViewportsOnly() const override 
	{
		return true;
	}
	void TickActor( float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction ) override;
#if WITH_EDITOR
	virtual bool CanEditChange(const UProperty* InProperty)const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void EditorApplyTranslation(const FVector & DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
#endif
	virtual FBox GetComponentsBoundingBox(bool bNonColliding = false) const override;
protected:
	bool IsActiveActor();
	void TransferPropertiesToProxy();
	void UpdateVolumes();
	void UpdateTime();

	int latest_thunderbolt_id;
	bool bPlaying;
	void FillMaterialParameters();
	static bool bSetTime;

protected:
	FStringAssetReference MoonTextureAssetRef;
	FStringAssetReference MilkyWayAssetRef;
	FStringAssetReference TrueSkyInscatterRTAssetRef;
	FStringAssetReference TrueSkyLossRTAssetRef;
	FStringAssetReference CloudShadowRTAssetRef;
	FStringAssetReference AltitudeLightRTAssetRef;
	FStringAssetReference CloudVisibilityRTAssetRef;
	FStringAssetReference OverlayRTAssetRef;
	FStringAssetReference TranslucentRTAssetRef;
	FStringAssetReference MaterialParameterCollectionAssetRef;
public:
	static const FString kDefaultMoon;
	static const FString kDefaultMilkyWay;
	static const FString kDefaultTrueSkyInscatterRT;
	static const FString kDefaultTrueSkyLossRT;
	static const FString kDefaultCloudShadowRT;
	static const FString kDefaultAltitudeLightRT;
	static const FString kDefaultCloudVisibilityRT;
	static const FString kDefaultOverlayRT;
	static const FString kDefaultTranslucentRT;

	static const FString kDefaultMaterialParameterCollection;

	static const FName kMPC_MaxAtmosphericDistanceName;
	static const FName kMPC_CloudShadowOrigin;
	static const FName kMPC_CloudShadowScale;
	static const FName kMPC_CloudShadowOffset;
	static const FName kMPC_LocalWetness;
	static const FName kMPC_ShadowStrength;
	static const FName kMPC_ShadowEdgeSmoothness;
	static const FName kMPC_SunlightTableTransform;
	static const FName kMPC_CloudTint;
	
	void SetDefaultTextures();
	void SetDefaultValues();

public:
	bool initializeDefaults;
	float wetness;

public:
	/** Returns AudioComponent subobject **/
	//class UAudioComponent* GetRainAudioComponent() const;
	class UAudioComponent* GetThunderAudioComponent() const;
	bool IsAnyActorActive() ;
	protected:
	class UTextureRenderTargetCube *CaptureTargetCube;

private:
	bool isRemoved = false; //Whether the sequence actor has already been removed.

	/* Perform operations when the actor is removed.
	* The problem is that Unreal doesn't seem to call the destructor if the actor is deleted after being loaded in,
	* and doesn't call Destroyed() when unloading the level.
	*/
	void Remove();
	unsigned MoonsChecksum;
	unsigned MoonsMeshesChecksum;
	ATrueSkySequenceActor* CurrentActiveActor = NULL;

	TArray<UStaticMeshComponent*> moonMeshes;
	/*
		For any moon that has a static mesh pointer, we position it correctly.
	*/
	void UpdateMoonPositions();
	/*
		For any moon that has a static mesh pointer, we make sure there's a runtime subobject component.
	*/
	void UpdateMoonMeshes();
};
