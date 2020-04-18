// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Engine/Texture2D.h"
#include "Math/Vector.h"
#include "Runtime/Core/Public/Math/Quat.h"
#include "Math/TransformNonVectorized.h"
#include "Misc/Variant.h"

typedef int64 TrueSkyEnum;
namespace simul
{
	namespace crossplatform
	{
		class PlatformRendererInterface;
		class DisplaySurfaceManagerInterface ;
	}
	namespace unreal
	{
		struct vec3
		{
			float x,y,z;
		};
		struct vec4
		{
			float x, y, z, w;
		};
		struct VariantPass
		{
			VariantPass():intVal(0)
			{}
			VariantPass(float f):floatVal(f)
			{}
			VariantPass(int32 f):intVal(f)
			{}
			VariantPass(double f):doubleVal(f)
			{}
			VariantPass(vec3 val)
			{
				val.x = vec3Val[0];
				val.y = vec3Val[1];
				val.z = vec3Val[2];
			}
			union
			{
				float floatVal;
				int intVal;
				double doubleVal;
				float vec3Val[3];
			};
		};
		/// This allows the UI to use the rendering API.
		struct RenderingInterface
		{
			simul::crossplatform::PlatformRendererInterface *platformRendererInterface;
			simul::crossplatform::DisplaySurfaceManagerInterface *displaySurfaceManagerInterface;
			void *cloudRenderer;
			void *cloudWindowRenderFunction;
		};
		extern FVector UEToTrueSkyPosition(const FTransform &tr,float MetresPerUnit,FVector ue_pos) ;
		extern FVector TrueSkyToUEPosition(const FTransform &tr,float MetresPerUnit,FVector ts_pos) ;
		extern FVector UEToTrueSkyDirection(const FTransform &tr,FVector ue_dir) ;
		extern FVector TrueSkyToUEDirection(const FTransform &tr,FVector ts_dir) ;
	}
}


// Definitions for Simul version number:
#define MAKE_SIMUL_VERSION(major,minor,build) ((major<<24)+(minor<<16)+build)

typedef unsigned SimulVersion;

static const SimulVersion SIMUL_4_0=MAKE_SIMUL_VERSION(4,0,0);
static const SimulVersion SIMUL_4_1=MAKE_SIMUL_VERSION(4,1,0);
static const SimulVersion SIMUL_4_2=MAKE_SIMUL_VERSION(4,2,0);
static const SimulVersion SIMUL_4_3=MAKE_SIMUL_VERSION(4,3,0);
static const SimulVersion SIMUL_4_4=MAKE_SIMUL_VERSION(4,4,0);
static const SimulVersion SIMUL_5_0=MAKE_SIMUL_VERSION(5,0,0);

inline SimulVersion ToSimulVersion(int major,int minor,int build=0)
{
	return MAKE_SIMUL_VERSION(major,minor,build);
}

class ATrueSkySequenceActor;

//! Type of water masking object
enum maskObjectType
{
	plane,
	cube,
	custom
};

/*
*	Data Structures for trueSKY Property Data.
*/
struct TrueSkyPropertyFloatData
{
	TrueSkyPropertyFloatData(FName Name, float Min, float Max, int64 en) : PropertyName(Name), TrueSkyEnum(en), PropertyValue(0.0f), Initialized(false)
	{
	}

	FName PropertyName;
	int64 TrueSkyEnum;
	float PropertyValue;
	bool Initialized;
};
struct TrueSkyPropertyIntData
{
	TrueSkyPropertyIntData(FName Name, int64 en) : PropertyName(Name), TrueSkyEnum(en), PropertyValue(-2), Initialized(false)
	{
	}

	FName PropertyName;
	int64 TrueSkyEnum;
	int32 PropertyValue;
	bool Initialized;
};
struct TrueSkyPropertyVectorData
{
	TrueSkyPropertyVectorData(FName Name, int64 en) : PropertyName(Name), TrueSkyEnum(en), PropertyValue(0.0f), Initialized(false)
	{	}

	FName PropertyName;
	int64 TrueSkyEnum;
	FVector PropertyValue;
	bool Initialized;
};
/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules 
 * within this plugin.
 */
class ITrueSkyPlugin : public IModuleInterface
{
public:
	struct ColourTableRequest
	{
		ColourTableRequest():uid(0),x(0),y(0),z(0),valid(false),data(NULL)
		{
		}
		unsigned uid;
		int x,y,z;	// sizes of the table
		bool valid;
		float *data;
	};
	//! The result struct for a point or volume query.
	struct VolumeQueryResult
	{
		float pos_m[4];
		int valid;
		float density;
		float direct_light;
		float indirect_light;
		float ambient_light;
		float precipitation;
		float rain_to_snow;
		float padding;
	};
	//! The result struct for a light query.
	struct LightingQueryResult
	{
		float pos[4];
		float sunlight[4];		// we use vec4's here to avoid padding.
		float moonlight[4];		// The 4th number will be a shadow factor from 0 to 1.0.
		float ambient[4];
		float sunlight_in_space[3];
		int valid;
	};
	//! The result struct for a line query.
	struct LineQueryResult
	{
		float pos1[4];
		float pos2[4];
		float padding[3];
		int valid;
		float density;
		float visibility;
		float optical_thickness_km;
		float first_contact_km;
	};
	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline ITrueSkyPlugin& Get()
	{
		return FModuleManager::GetModuleChecked< ITrueSkyPlugin >( "TrueSkyPlugin" );
	}
	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline ITrueSkyPlugin& EnsureLoaded()
	{
		return FModuleManager::LoadModuleChecked< ITrueSkyPlugin >( "TrueSkyPlugin" );
	}


	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "TrueSkyPlugin" );
	}
	virtual void	SetPointLight(int id,FLinearColor c,FVector pos,float min_radius,float max_radius)=0;
	virtual float	CloudLineTest(int32 queryId,FVector StartPos,FVector EndPos) =0;
	virtual float	GetCloudinessAtPosition(int32 queryId,FVector pos)=0;
	virtual VolumeQueryResult		GetStateAtPosition(int32 queryId,FVector pos) =0;
	virtual LightingQueryResult		GetLightingAtPosition(int32 queryId,FVector pos)=0;
	virtual void	SetRenderFloat(const FString& name, float value) = 0;
	virtual float	GetRenderFloat(const FString& name) const = 0;
	virtual float	GetRenderFloatAtPosition(const FString& name,FVector Pos) const = 0;
	
	virtual float	GetFloatAtPosition(int64 Enum,FVector pos,int32 uid) const = 0;
	virtual float	GetFloat(int64 Enum) const = 0;
	virtual FVector GetVector(int64 Enum) const=0;
	virtual void	SetVector(int64 Enum,FVector value) const=0;

	virtual void	SetRender(const FString &fname,const TArray<FVariant> &params) = 0;
	virtual void	SetRenderInt(const FString& name, int value) = 0;
	virtual int		GetRenderInt(const FString& name) const = 0;
	virtual int		GetRenderInt(const FString& name,const TArray<FVariant> &params) const = 0;
	
	virtual void	CreateBoundedWaterObject(unsigned int ID, float* dimension, float* location) = 0;
	virtual void	RemoveBoundedWaterObject(unsigned int ID) = 0;
	
	virtual bool	AddWaterProbe(int ID, FVector location, FVector velocity, float radius) = 0;
	virtual void	RemoveWaterProbe(int ID) = 0;
	virtual void	UpdateWaterProbeValues(int ID, FVector location, FVector velocity, float radius, float dEnergy) = 0;
	virtual FVector4 GetWaterProbeValues(int ID) = 0;

	virtual bool	AddWaterBuoyancyObject(int ID, FVector location, FQuat rotation, FVector scale, TArray<FVector> vertices) = 0;
	virtual void	UpdateWaterBuoyancyObjectValues(int ID, FVector location, FQuat rotation, FVector scale) = 0;
	virtual TArray<float> GetWaterBuoyancyObjectResults(int ID, int noOfVertices) = 0;
	virtual void	RemoveWaterBuoyancyObject(int ID) = 0;

	virtual bool	AddWaterMaskObject(int ID, FVector location, FQuat rotation, FVector scale, TArray<FVector> vertices, TArray<uint32> indices, maskObjectType objectType, bool active, bool totalMask) = 0;
	virtual void	UpdateWaterMaskObjectValues(int ID, FVector location, FQuat rotation, FVector scale, maskObjectType objectType, bool active, bool totalMask) = 0;
	virtual void	RemoveWaterMaskObject(int ID) = 0;


	virtual void	SetWaterFloat(const FString &fname, unsigned int ID, float value) = 0;
	virtual void	SetWaterInt(const FString &fname, unsigned int ID, int value) = 0;
	virtual void	SetWaterBool(const FString &fname, unsigned int ID, bool value) = 0;
	virtual void	SetWaterVector(const FString &fname, int ID, const FVector &value) = 0;
	virtual float	GetWaterFloat(const FString &fname, unsigned int ID) const = 0;
	virtual int		GetWaterInt(const FString &fname, unsigned int ID) const = 0;
	virtual bool	GetWaterBool(const FString &fname, unsigned int ID) const = 0;
	virtual FVector	GetWaterVector(const FString &fname, unsigned int ID) const = 0;

	virtual bool	GetWaterRenderingEnabled() = 0;

	virtual void	SetKeyframeFloat(unsigned,const FString& name, float value) = 0;
	virtual float	GetKeyframeFloat(unsigned,const FString& name) const = 0;
	virtual void	SetKeyframeInt(unsigned,const FString& name, int value) = 0;
	virtual int		GetKeyframeInt(unsigned,const FString& name) const = 0;


	virtual void	SetKeyframerFloat(unsigned, const FString& name, float value) = 0;
	virtual float	GetKeyframerFloat(unsigned, const FString& name) const = 0;
	virtual void	SetKeyframerInt(unsigned, const FString& name, int value) = 0;
	virtual int		GetKeyframerInt(unsigned, const FString& name) const = 0;

	virtual void	SetRenderBool(const FString& name, bool value) = 0;
	virtual bool	GetRenderBool(const FString& name) const = 0;
	virtual void	SetRenderString(const FString& name, const FString&  value)=0;
	virtual FString	GetRenderString(const FString& name) const =0;
	virtual bool	TriggerAction(const FString& name) = 0;
	virtual void	SetRenderingEnabled(bool) = 0;
	virtual void	SetPostOpaqueEnabled(bool)=0;
	virtual void	SetPostTranslucentEnabled(bool)=0;
	virtual void	SetOverlaysEnabled(bool)=0;
	
	virtual class	UTrueSkySequenceAsset* GetActiveSequence()=0;
	virtual void*	GetRenderEnvironment()=0;
	virtual void	OnToggleRendering() = 0;

	virtual void	OnUIChangedSequence()=0;
	virtual void	OnUIChangedTime(float)=0;
	
	virtual void	ExportCloudLayer(const FString& filenameUtf8,int index)=0;
	virtual UTexture *GetAtmosphericsTexture()=0;

	
	virtual void ClearCloudVolumes()=0;
	virtual void SetCloudVolume(int,const FTransform &tr,FVector ext)=0;
	
	virtual int32 SpawnLightning(FVector startpos,FVector endpos,float magnitude,FVector colour)=0;

	virtual void RequestColourTable(unsigned uid,int x,int y,int z)=0;
	virtual void ClearColourTableRequests()=0;
	virtual const ColourTableRequest *GetColourTable(unsigned uid) =0;

	typedef void (*FTrueSkyEditorCallback)();
	virtual void SetEditorCallback(FTrueSkyEditorCallback c) =0;

	virtual void InitializeDefaults(ATrueSkySequenceActor *)=0;

	virtual void AdaptViewMatrix(FMatrix &viewMatrix,bool editor_version=false) const=0;

	virtual SimulVersion GetVersion() const=0;

	// If we can get an integer enum for a string, we can use that instead of the string and improve performance.
	virtual int64	GetEnum(const FString &name) const=0;

	virtual simul::unreal::RenderingInterface *GetRenderingInterface()=0;
};

UENUM(BlueprintType)
enum class ETrueSkySkyLayerProperties : uint8
{
	TSPROPINT_Sky_NumAltitudes					UMETA(DisplayName = "Altitude (Int)"),
	TSPROPINT_Sky_NumElevations					UMETA(DisplayName = "Elevation (Int)"),
	TSPROPINT_Sky_NumDistances					UMETA(DisplayName = "Distance (Int)"),
	TSPROPINT_Sky_StartDayNumber				UMETA(DisplayName = "Start Day Number (From 1/1/2000) (Int)"),
	TSPROP_MaxInt								UMETA(Hidden),
	TSPROPFLOAT_Sky_BrightnessPower				UMETA(DisplayName = "Brightness Power"),
	TSPROPFLOAT_Sky_MaxAltitudeKm				UMETA(DisplayName = "Max Altitude (Km)"),
	TSPROPFLOAT_Sky_MaxDistanceKm				UMETA(DisplayName = "Max Distance (Km)"),
	TSPROPFLOAT_Sky_OvercastEffectStrength		UMETA(DisplayName = "Overcast Effect Strength"),
	TSPROPFLOAT_Sky_OzoneStrength				UMETA(DisplayName = "Ozone Strength"),
	TSPROPFLOAT_Sky_Emissivity					UMETA(DisplayName = "Emissivity"),
	TSPROPFLOAT_Sky_SunRadiusArcMinutes			UMETA(DisplayName = "Diameter (Sun)"),
	TSPROPFLOAT_Sky_SunBrightnessLimit			UMETA(DisplayName = "Brightness Limit (Sun)"),
	TSPROPFLOAT_Sky_LatitudeRadians				UMETA(DisplayName = "Latitude"),
	TSPROPFLOAT_Sky_LongitudeRadians			UMETA(DisplayName = "Longitutde"),
	TSPROPFLOAT_Sky_TimezoneHours				UMETA(DisplayName = "Timezone +/-"),
	TSPROPFLOAT_Sky_MoonAlbedo					UMETA(DisplayName = "Albedo (Moon)"),
	TSPROPFLOAT_Sky_MoonRadiusArcMinutes		UMETA(DisplayName = "Diameter (Moon)"),
	TSPROPFLOAT_Sky_MaxStarMagnitude			UMETA(DisplayName = "Max Magnitude (Star)"),
	TSPROPFLOAT_Sky_StarBrightness				UMETA(DisplayName = "Brightness (Stars)"),
	TSPROPFLOAT_Sky_BackgroundBrightness		UMETA(DisplayName = "Brightness (Background)"),
	TSPROPFLOAT_Sky_SunIrradianceRed			UMETA(DisplayName = "Sun Irradiance (Red)"),
	TSPROPFLOAT_Sky_SunIrradianceGreen			UMETA(DisplayName = "Sun Irradiance (Green)"),
	TSPROPFLOAT_Sky_SunIrradianceBlue			UMETA(DisplayName = "Sun Irradiance (Blue)"),
	TSPROPFLOAT_Sky_ColourWavelengthsNmRed		UMETA(DisplayName = "Colour Wavelengths Nm (Red)"),
	TSPROPFLOAT_Sky_ColourWavelengthsNmGreen	UMETA(DisplayName = "Colour Wavelengths Nm (Green)"),
	TSPROPFLOAT_Sky_ColourWavelengthsNmBlue		UMETA(DisplayName = "Colour Wavelengths Nm (Blue)"),

	TSPROPFLOAT_Sky_Max						UMETA(Hidden)

};

UENUM(BlueprintType)
enum class ETrueSkySkyKeyframeProperties : uint8
{

	TSPROPINT_StoreAsColours			UMETA(DisplayName = "Store Keyframe as Colours (Int)"),
	TSPROPINT_NumColourAltitudes		UMETA(DisplayName = "Colour Altitude Layer Count (Int)"),
	TSPROPINT_NumColourElevations		UMETA(DisplayName = "Colour Elevation Layer Count (Int)"),
	TSPROPINT_NumColourDistances		UMETA(DisplayName = "Colour Distance Layer Count (Int)"),
	TSPROPINT_AutomaticSunPosition		UMETA(DisplayName = "Automatic Sun Position (Int)"),
	TSPROPINT_AutomaticMoonPosition		UMETA(DisplayName = "Automatic Moon Position (Int)"),
	TSPROP_MaxInt						UMETA(Hidden),

	TSPROPFLOAT_Daytime					UMETA(DisplayName = "Time"),
	TSPROPFLOAT_Haze					UMETA(DisplayName = "Haze (i.e. Fog)"),
	TSPROPFLOAT_HazeBaseKm				UMETA(DisplayName = "Haze Base (Km)"),
	TSPROPFLOAT_HazeScaleKm				UMETA(DisplayName = "Haze Scale (Km)"),
	TSPROPFLOAT_HazeEccentricity		UMETA(DisplayName = "Haze Eccentricity"),
	TSPROPFLOAT_GroundFog				UMETA(DisplayName = "Ground Fog"),
	TSPROPFLOAT_FogCeilingKm			UMETA(DisplayName = "Fog Ceiling (Km)"),
	TSPROPFLOAT_MieRed					UMETA(DisplayName = "Mie Coefficient (Red)"),
	TSPROPFLOAT_MieGreen				UMETA(DisplayName = "Mie Coefficient (Green)"),
	TSPROPFLOAT_MieBlue					UMETA(DisplayName = "Mie Coefficient (Blue)"),
	TSPROPFLOAT_SunElevation			UMETA(DisplayName = "Sun Elevation"),
	TSPROPFLOAT_SunAzimuth				UMETA(DisplayName = "Sun Azimuth"),
	TSPROPFLOAT_MoonElevation			UMETA(DisplayName = "Moon Elevation"),
	TSPROPFLOAT_MoonAzimuth				UMETA(DisplayName = "Moon Azimuth"),
	TSPROPFLOAT_SeaLevelTemperatureK	UMETA(DisplayName = "Sea Level Deg. K")

};

UENUM(BlueprintType)
enum class ETrueSkyCloudLayerProperties : uint8
{
	TSPROPINT_Clouds_RenderMode					UMETA(DisplayName = "Enable Cirrus Style Clouds (Int)"),
	TSPROPINT_Clouds_DefaultNumSlices			UMETA(DisplayName = "Default Slices (Int)"),
	TSPROPINT_Clouds_GridWidth					UMETA(DisplayName = "Cloud Detail Grid Width (Int)"),
	TSPROPINT_Clouds_GridHeight					UMETA(DisplayName = "Cloud Detail Grid Height (Int)"),
	TSPROPINT_Clouds_NoiseResolution			UMETA(DisplayName = "3D Noise Resolution (Int)"),
	TSPROP_MaxInt								UMETA(Hidden),
	TSPROPFLOAT_NoisePeriod						UMETA(DisplayName = "Noise Period"),
	TSPROPFLOAT_NoisePhase						UMETA(DisplayName = "Noise Phase")

};

UENUM(BlueprintType)
enum class ETrueSkyCloudKeyframeProperties : uint8
{
	TSPROPINT_Octaves					UMETA(DisplayName = "Octaves (Int)"),
	TSPROPINT_RegionalPrecipitation		UMETA(DisplayName = "Regional Precipitation (Int)"),
	TSPROPINT_LockRainToClouds			UMETA(DisplayName = "Lock Rain to Clouds (Int)"),
	TSPROP_MaxInt						UMETA(Hidden),
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

};