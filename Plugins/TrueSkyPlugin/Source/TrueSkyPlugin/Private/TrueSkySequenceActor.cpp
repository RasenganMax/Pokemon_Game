// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkySequenceActor.h"

#include "TrueSkyPluginPrivatePCH.h"

#include "Engine/Engine.h"
#include "Engine/Classes/Kismet/KismetRenderingLibrary.h" //ClearRenderTarget2D

#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Light.h"
#include "Engine/PointLight.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetCube.h"
#include "GameFramework/Actor.h"
#include "ActorCrossThreadProperties.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Logging/LogMacros.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DirectionalLightComponent.h"
#include "TrueSkyLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "TimerManager.h"
#include "TrueSkyComponent.h"
#include "TrueSkySequenceAsset.h"
#include <atomic>
#include <map>
#include <algorithm>

static std::atomic<int> actors(0);

static const float PI_F=3.1415926536f;
using namespace simul;
#ifndef UE_LOG4_ONCE
#define UE_LOG4_ONCE(tsa_a,tsa_b,tsa_c,d) {static bool done=false; if(!done) {UE_LOG(tsa_a,tsa_b,tsa_c,d);done=true;}}
#endif

#ifndef UE_LOG_ONCE
#define UE_LOG_ONCE(a,b,c,...) {static bool done=false; if(!done) {UE_LOG(a,b,c, TEXT(""), ##__VA_ARGS__);done=true;}}
#endif


using namespace simul::unreal;
static float m1=1.0f,m2=-1.0f,m3=0,m4=0;
static float c1=0.0f,c2=90.0f,c3=0.0f;
static float yo=180.0;
static float tsa_a=-1.0f,tsa_b=-1.0f,tsa_c=-1.0f;
static FVector tsa_dir, tsa_u;
static FTransform worldToSky;

bool ATrueSkySequenceActor::bSetTime=false;
// These will get overwritten by the proper logic, but likely will result in the same values.
int32 ATrueSkySequenceActor::TSSkyLayerPropertiesFloatIdx = 0;
int32 ATrueSkySequenceActor::TSSkyKeyframePropertiesFloatIdx = 1;
int32 ATrueSkySequenceActor::TSCloudLayerPropertiesFloatIdx = 2;
int32 ATrueSkySequenceActor::TSCloudKeyframePropertiesFloatIdx = 3;
int32 ATrueSkySequenceActor::TSSkyLayerPropertiesIntIdx = 0;
int32 ATrueSkySequenceActor::TSSkyKeyframePropertiesIntIdx = 1;
int32 ATrueSkySequenceActor::TSCloudLayerPropertiesIntIdx = 2;	
int32 ATrueSkySequenceActor::TSCloudKeyframePropertiesIntIdx = 3;

int32 ATrueSkySequenceActor::TSPropertiesIntIdx=4;
int32 ATrueSkySequenceActor::TSPropertiesFloatIdx=4;
int32 ATrueSkySequenceActor::TSPositionalPropertiesFloatIdx=5;
int32 ATrueSkySequenceActor::TSPropertiesVectorIdx=0;
int32 ATrueSkySequenceActor::TSRenderingPropertiesIdx=6;

TArray< ATrueSkySequenceActor::TrueSkyPropertyIntMap > ATrueSkySequenceActor::TrueSkyPropertyIntMaps;
TArray< ATrueSkySequenceActor::TrueSkyPropertyFloatMap > ATrueSkySequenceActor::TrueSkyPropertyFloatMaps;
TArray< ATrueSkySequenceActor::TrueSkyPropertyVectorMap > ATrueSkySequenceActor::TrueSkyPropertyVectorMaps;

const FString ATrueSkySequenceActor::kDefaultMoon = "Texture2D'/TrueSkyPlugin/Moon.Moon'";
const FString ATrueSkySequenceActor::kDefaultMilkyWay = "Texture2D'/TrueSkyPlugin/MilkyWay.MilkyWay'";
const FString ATrueSkySequenceActor::kDefaultTrueSkyInscatterRT = "TextureRenderTarget2D'/TrueSkyPlugin/trueSKYInscatterRT.trueSKYInscatterRT'";
const FString ATrueSkySequenceActor::kDefaultTrueSkyLossRT = "TextureRenderTarget2D'/TrueSkyPlugin/trueSKYLossRT.trueSKYLossRT'";
const FString ATrueSkySequenceActor::kDefaultCloudShadowRT = "TextureRenderTarget2D'/TrueSkyPlugin/cloudShadowRT.cloudShadowRT'";
const FString ATrueSkySequenceActor::kDefaultAltitudeLightRT = "TextureRenderTarget2D'/TrueSkyPlugin/altitudeLightRT.altitudeLightRT'";
const FString ATrueSkySequenceActor::kDefaultCloudVisibilityRT = "TextureRenderTarget2D'/TrueSkyPlugin/cloudVisibilityRT.cloudVisibilityRT'";
const FString ATrueSkySequenceActor::kDefaultOverlayRT = "TextureRenderTarget2D'/TrueSkyPlugin/Overlay/TrueSkyOverlayRT.TrueSkyOverlayRT'";
const FString ATrueSkySequenceActor::kDefaultTranslucentRT = "TextureRenderTarget2D'/TrueSkyPlugin/Translucent/TranslucentRT.TranslucentRT'";
const FString ATrueSkySequenceActor::kDefaultMaterialParameterCollection = "MaterialParameterCollection'/TrueSkyPlugin/trueSkyMaterialParameters.trueSkyMaterialParameters'";

const FName ATrueSkySequenceActor::kMPC_MaxAtmosphericDistanceName = TEXT( "MaxAtmosphericDistance" );
const FName ATrueSkySequenceActor::kMPC_CloudShadowOrigin = TEXT( "CloudShadowOrigin" );
const FName ATrueSkySequenceActor::kMPC_CloudShadowScale = TEXT( "CloudShadowScale" );
const FName ATrueSkySequenceActor::kMPC_CloudShadowOffset = TEXT( "CloudShadowOffset");
const FName ATrueSkySequenceActor::kMPC_LocalWetness = TEXT( "LocalWetness" );
const FName ATrueSkySequenceActor::kMPC_ShadowStrength = TEXT( "ShadowStrength" );
const FName ATrueSkySequenceActor::kMPC_SunlightTableTransform = TEXT("SunlightTableTransform");


simul::QueryMap truesky_queries;

FTextureRHIRef simul::GetTextureRHIRef(UTexture *t)
{
	if (!t)
		return nullptr;
	if (!t->Resource)
		return nullptr;
	return t->Resource->TextureRHI;
}

FTextureRHIRef simul::GetTextureRHIRef(UTextureRenderTargetCube *t)
{
	if (!t)
		return nullptr;
	if (!t->Resource)
		return nullptr;
	return t->Resource->TextureRHI;
}


ATrueSkySequenceActor::ATrueSkySequenceActor(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	,ActiveSequence(nullptr) 
	,MetresPerUnit(0.01f)
	,Visible(true)
	,ShareBuffersForVR(true)
	,FixedDeltaTime(-1.0f)
	,ActiveInEditor(true)
	,TrueSkyMaterialParameterCollection(nullptr)

	//Time
	,ControlTimeThroughActor(false)
	,TimeHours(12.0f)
	,TimeFactor(100.0f)
	,Loop(false)
	,LoopStart(11.0f)
	,LoopEnd(13.0f)

	//Clouds
	,MaximumResolution(256)	
	,WindSpeedMS(0.f, 0.f, 0.f)

	//Cloud Rendering
	,MaxCloudDistanceKm(300.0f)
	, IntegrationScheme(EIntegrationScheme::Grid)
	,RenderGridXKm(0.4f)
	,RenderGridZKm(0.4f)
	,DefaultCloudSteps(200)
	,Amortization(4)
	,CloudThresholdDistanceKm(0.0f)
	,DepthSamplingPixelRange(1.5f)
	,DepthTemporalAlpha(0.1f)
	,DepthBlending(true)

	//Cloud Noise
	, EdgeNoisePersistence(0.63f)
	, EdgeNoiseFrequency(4)
	, EdgeNoiseTextureSize(32)
	, EdgeNoiseWavelengthKm(2.5f)
	, CellNoiseWavelengthKm(8.7f)
	, CellNoiseTextureSize(64)
	, MaxFractalAmplitudeKm(3.0f)

	//Cloud Lighting
	, DirectLight(1.f)		
	, IndirectLight(0.6f)
	, AmbientLight(1.0f)
	, Extinction(4.00f)
	, MieAsymmetry(0.87f)

	//Lighting
	, Light(nullptr)			
	, DriveLightDirection(true)
	, TrueSkyLightUnits(ETrueSkyLightUnits::Radiometric)
	, SkyMultiplier(1.0f)
	, SunMultiplier(1.0f)
	, MoonMultiplier(1.0f)
	, Brightness(1.0f)
	, Gamma(1.0f)
#if SIMUL_ALTITUDE_DIRECTIONAL_LIGHT
	,AltitudeLighting(true)
#else
	,AltitudeLighting(false)
#endif
	, AltitudeLightRT(nullptr)

	//Atmospherics
	,MaxSunRadiance(15000.0f)
	,AdjustSunRadius(false)
	, AtmosphericsAmortization(4)
	, MinimumStarPixelSize(1.5f)
	, MoonTexture(nullptr)
	, CosmicBackgroundTexture(nullptr)
	, OriginLatitude(0.0f)
	, OriginLongitude(0.0f)
	, OriginHeading(0.0f)

	//Interpolation
	,InterpolationMode(EInterpolationMode::GameTime) 
	,InterpolationTimeSeconds(10.0f)
	,InterpolationTimeHours(0.2f)
	,InterpolationSubdivisions(8)
	,MaxVolumeMips(4)
	,HighDetailProportion(0.2f)
	,MediumDetailProportion(0.35f)
	,InstantUpdate(true)

	//Shadows
	,CloudShadowRangeKm(80.0f)		
	,ShadowTextureSize(512)
	,ShadowStrength(0.8f)
	,CloudShadowRT(nullptr)
	,CrepuscularRayStrength(1.0f)

	//Precipitation
	,MaxPrecipitationParticles(216000)
	,PrecipitationRadiusMetres(6.f)
	,RainFallSpeedMS(6.0f)
	,RainDropSizeMm(2.5f)
	,SnowFallSpeedMS(0.5f)
	,SnowFlakeSizeMm(10.0f)
	,PrecipitationWindEffect(0.25f)
	,PrecipitationWaver(0.7f)
	,PrecipitationWaverTimescaleS(3.0f)
	,PrecipitationThresholdKm(0.2f)
	,RainCubemap(nullptr)
	,VelocityStreaks(true)
	,SimulationTime(false)
	,RainNearThreshold(30.0f)
	,RainMaskSceneCapture2D(nullptr)
	,RainMaskWidth(1000.0f)
	,RainDepthExtent(2500.0f)
	,TranslucentRT(nullptr)

	//Rainbows
	,AutomaticRainbowPosition(true)
	,RainbowElevation(0.0f)
	,RainbowAzimuth(0.0f)
	,RainbowIntensity(1.0f)
	,RainbowDepthPoint(1.0f)
	,AllowOccludedRainbow(false)
	,AllowLunarRainbow(true)

	,ThunderAttenuation(nullptr)

	//Water
	,RenderWater(false)		
	,UseWaterGameTime(false)
	,WaterResolution(EWaterResolution::FullSize)
	,EnableReflection(true)
	,ReflectionResolution(EWaterResolution::FullSize)
	,ReflectionSteps(100)
	,ReflectionPixelStep(2)

	//Textures
	,InscatterRT(nullptr)		
	,LossRT(nullptr)
	,CloudVisibilityRT(nullptr)
	,OverlayRT(nullptr)


	,bSimulVersion4_1(false)
	,bInterpolationFixed( true )
	,bInterpolationGameTime( false )
	,bInterpolationRealTime( false )
	,bSimulVersion4_2(false)
	,bGridRendering4_2(false)
	,latest_thunderbolt_id(-1)
	,bPlaying(false)
	,initializeDefaults(false)
	,wetness(0.0f)
	,CaptureTargetCube(nullptr)
	,MoonsChecksum(1)
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	PrimaryActorTick.bTickEvenWhenPaused	=true;
	PrimaryActorTick.bCanEverTick			=true;
	PrimaryActorTick.bStartWithTickEnabled	=true;
	SetTickGroup( TG_DuringPhysics);
	SetActorTickEnabled(true);
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	++actors;
	// we must have TWO actors in order for one to be in tsa_a scene, because there's also tsa_a DEFAULT object!
	if(A&&actors>=2)
		A->Destroyed=false;
	latest_thunderbolt_id=0;
	CrepuscularGrid[0]=64;
	CrepuscularGrid[1]=32;
	CrepuscularGrid[2]=32;

	// Setup the asset references for the default textures/RTs.
	MoonTextureAssetRef = kDefaultMoon;
	MilkyWayAssetRef = kDefaultMilkyWay;
	TrueSkyInscatterRTAssetRef = kDefaultTrueSkyInscatterRT;
	
	TrueSkyLossRTAssetRef = kDefaultTrueSkyLossRT;
	CloudShadowRTAssetRef = kDefaultCloudShadowRT;
	AltitudeLightRTAssetRef = kDefaultAltitudeLightRT;
	CloudVisibilityRTAssetRef = kDefaultCloudVisibilityRT;
	OverlayRTAssetRef = kDefaultOverlayRT; 
	TranslucentRTAssetRef = kDefaultTranslucentRT;
	MaterialParameterCollectionAssetRef = kDefaultMaterialParameterCollection;
	//if (!FPaths::FileExists(kDefaultTrueSkyInscatterRT))
	//If not found, we could reassign them to the default engine directories opposed to the game directory.
	
}

void ATrueSkySequenceActor::UpdateEnabledProperties()
{
	bInterpolationFixed = ( InterpolationMode == EInterpolationMode::FixedNumber );
	bInterpolationGameTime = ( InterpolationMode == EInterpolationMode::GameTime );
	bInterpolationRealTime = ( InterpolationMode == EInterpolationMode::RealTime );

	bGridRendering4_2 = bSimulVersion4_2 && ( IntegrationScheme==EIntegrationScheme::Grid );
}

#define SET_RENDERING_FLOAT(name) SetRenderingFloat(ETrueSkyRenderingProperties::TSPROPFLOAT_Render_##name,name);
#define SET_RENDERING_INT(name) SetRenderingInt(ETrueSkyRenderingProperties::TSPROPINT_Render_##name,name);
#define SET_RENDERING_VECTOR(name) SetRenderingVector(ETrueSkyPropertiesVector::TSPROPVECTOR_Render_##name,name);

void ATrueSkySequenceActor::ApplyRenderingOptions()
{
	SET_RENDERING_FLOAT(HighDetailProportion);
	SET_RENDERING_FLOAT(MediumDetailProportion);
	SET_RENDERING_FLOAT(CloudShadowRangeKm); 
	SET_RENDERING_FLOAT(CrepuscularRayStrength);
	SET_RENDERING_FLOAT(PrecipitationRadiusMetres);
	SET_RENDERING_FLOAT(RainFallSpeedMS);
	SET_RENDERING_FLOAT(RainDropSizeMm);
	SET_RENDERING_FLOAT(SnowFallSpeedMS);
	SET_RENDERING_FLOAT(SnowFlakeSizeMm);
	SET_RENDERING_FLOAT(PrecipitationWindEffect);
	SET_RENDERING_FLOAT(PrecipitationWaver);
	SET_RENDERING_FLOAT(PrecipitationWaverTimescaleS);
	SET_RENDERING_FLOAT(PrecipitationThresholdKm);
	SET_RENDERING_FLOAT(EdgeNoisePersistence);
	SET_RENDERING_FLOAT(EdgeNoiseWavelengthKm);
	SET_RENDERING_FLOAT(CellNoiseWavelengthKm);
	SET_RENDERING_FLOAT(MaxFractalAmplitudeKm);
	SET_RENDERING_FLOAT(MaxCloudDistanceKm);
	SET_RENDERING_FLOAT(RenderGridXKm);
	SET_RENDERING_FLOAT(RenderGridZKm);
	SET_RENDERING_FLOAT(CloudThresholdDistanceKm);
	SET_RENDERING_FLOAT(DirectLight);
	SET_RENDERING_FLOAT(IndirectLight);
	SET_RENDERING_FLOAT(AmbientLight);
	SET_RENDERING_FLOAT(Extinction);
	SET_RENDERING_FLOAT(MieAsymmetry);
	SET_RENDERING_INT(MaxPrecipitationParticles);
	SET_RENDERING_INT(EdgeNoiseFrequency);
	SET_RENDERING_INT(EdgeNoiseTextureSize);
	SET_RENDERING_VECTOR(WindSpeedMS);

	FVector OriginLatLongHeadingDeg(OriginLatitude, OriginLongitude, OriginHeading);
	SET_RENDERING_VECTOR(OriginLatLongHeadingDeg);

	SET_RENDERING_INT(AutomaticRainbowPosition);
	SET_RENDERING_FLOAT(RainbowElevation);
	SET_RENDERING_FLOAT(RainbowAzimuth);
	SET_RENDERING_FLOAT(RainbowIntensity);
	SET_RENDERING_FLOAT(RainbowDepthPoint);
	SET_RENDERING_INT(AllowOccludedRainbow);
	SET_RENDERING_INT(AllowLunarRainbow);
}

ATrueSkySequenceActor::~ATrueSkySequenceActor()
{
	Remove();
}

ETrueSkyVersion ATrueSkySequenceActor::GetTrueSkyVersion( )
{
	if( ( GetTrueSkyPlugin( )->GetVersion( ) ) >= SIMUL_4_2 )
	{
		return ETrueSkyVersion::Version4_2;
	}

	//	NOTE (trent, 6/26/18): There's no 4.1 test as far as I can tell; so ::bSimulVersion4_1 is 4.1a.
	return ETrueSkyVersion::Version4_1a;
}

float ATrueSkySequenceActor::GetReferenceSpectralRadiance()
{
	float r	=GetTrueSkyPlugin()->GetRenderFloat("ReferenceSpectralRadiance");
	return r;
}

int32 ATrueSkySequenceActor::TriggerAction(FString name)
{
	int32 r	=GetTrueSkyPlugin()->TriggerAction(name);
	return r;
}

UFUNCTION( BlueprintCallable,Category = TrueSky)
int32 ATrueSkySequenceActor::SpawnLightning(FVector startPosition,FVector EndPosition,float magnitude,FVector colour)
{
	return GetTrueSkyPlugin()->SpawnLightning(startPosition, EndPosition,magnitude,colour);
}

int32 ATrueSkySequenceActor::GetLightning(FVector &start,FVector &end,float &magnitude,FVector &colour)
{
    ActorCrossThreadProperties* A = GetActorCrossThreadProperties();
    int i = 0;
	{
        simul::LightningBolt* L = &(A->lightningBolts[i]);
		if(L && L->id != 0)
		{
			start       = FVector(L->pos[0],L->pos[1],L->pos[2]);
			end         = FVector(L->endpos[0],L->endpos[1],L->endpos[2]);
			magnitude   = L->brightness;
			colour      = FVector(L->colour[0],L->colour[1],L->colour[2]);
			return L->id;
		}
		else
		{
			magnitude = 0.0f;
	    }
	}
	return 0;
}

void ATrueSkySequenceActor::PostRegisterAllComponents()
{
	UWorld *world = GetWorld();
	// If any other Sequence Actor is already ActiveInEditor, make this new one Inactive.
	if (ActiveInEditor&&world!=nullptr&&world->WorldType == EWorldType::Editor)
	{
		bool in_list = false;
		for (TActorIterator<ATrueSkySequenceActor> Itr(world); Itr; ++Itr)
		{
			ATrueSkySequenceActor* SequenceActor = *Itr;
			if (SequenceActor == this)
			{
				in_list = true;
				continue;
			}
			if (SequenceActor->ActiveInEditor)
			{
				ActiveInEditor = false;
				break;
			}
		}

		if(in_list)
		{
			TransferProperties();
		}
	}
}

void ATrueSkySequenceActor::PostInitProperties()
{
    Super::PostInitProperties();
	// We need to ensure this first, or we may get bad data in the transform:
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	UpdateEnabledProperties();

	// NOTE: Although this is called "PostInitProperties", it won't actually have the initial properties in the Actor instance.
	// those are set... somewhere... later.
	// So we MUST NOT call TransferProperties here, or else we will get GPU memory etc allocated with the wrong values.
	//TransferProperties();
	UpdateVolumes();// no use
}
void ATrueSkySequenceActor::UpdateTime()
{
	if (!ControlTimeThroughActor)
		return;

	if (Loop)
	{
		if (TimeHours > LoopEnd)
			TimeHours = LoopStart;
		else if (TimeHours < LoopStart)
			TimeHours = LoopStart;
	}
	float timeUpdate = (TimeFactor / 3600) * GetWorld()->GetDeltaSeconds();
	TimeHours += timeUpdate;
	SetTime((TimeHours)/24);

}

void ATrueSkySequenceActor::GetCurrentTime()
{
	Time = GetTime();
	TimeHours = Time * 24.0f;
}

void ATrueSkySequenceActor::UpdateVolumes()
{
	if (!IsInGameThread() && !ITrueSkyPlugin::IsAvailable())
		return;
	if (!ITrueSkyPlugin::IsAvailable())
		return;

	//TArray< USceneComponent * > comps;
	TArray<UActorComponent *> comps;
	this->GetComponents(comps);

	GetTrueSkyPlugin()->ClearCloudVolumes();
	for(int i=0;i<comps.Num();i++)
	{
		UActorComponent *a=comps[i];
		if(!a)
			continue;
	}
}

void ATrueSkySequenceActor::SetDefaultTextures()
{
#define DEFAULT_PROPERTY(type,propname,filename) \
	if(propname==nullptr)\
	{\
		propname = LoadObject< type >( nullptr, *( filename.ToString( ) ) );\
	}

	DEFAULT_PROPERTY( UTexture2D, MoonTexture, MoonTextureAssetRef )
	DEFAULT_PROPERTY( UTexture2D, CosmicBackgroundTexture, MilkyWayAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, InscatterRT, TrueSkyInscatterRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, LossRT, TrueSkyLossRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, CloudShadowRT, CloudShadowRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, AltitudeLightRT, AltitudeLightRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, CloudVisibilityRT, CloudVisibilityRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, OverlayRT, OverlayRTAssetRef )
	DEFAULT_PROPERTY( UTextureRenderTarget2D, TranslucentRT, TranslucentRTAssetRef)
		
#undef DEFAULT_PROPERTY
}

void ATrueSkySequenceActor::PostLoad()
{
    Super::PostLoad();
	truesky_queries.Reset();
	// Now, apparently, we have good data.
	// We need to ensure this first, or we may get bad data in the transform:
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	if(initializeDefaults)
	{
		SetDefaultTextures();
		initializeDefaults=false;
	}
	latest_thunderbolt_id=0;
	bSetTime=false;
	ApplyRenderingOptions();
}

void ATrueSkySequenceActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();
	// We need to ensure this first, or we may get bad data in the transform:
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	latest_thunderbolt_id=0;
	if(initializeDefaults)
	{
		SetDefaultTextures();
		initializeDefaults=false;
	}
	TransferProperties();
}

void ATrueSkySequenceActor::BeginDestroy()
{
	Remove();

	AActor::BeginDestroy();
}


void ATrueSkySequenceActor::Destroyed()
{
	AActor::Destroyed();
}

void ATrueSkySequenceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	latest_thunderbolt_id=0;

	Remove();
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(A)
	{
		A->Reset = true;
	}
}

void ATrueSkySequenceActor::SetActiveSequenceActor(ATrueSkySequenceActor* newActor)
{
	CurrentActiveActor = newActor;
	//actorSwitched = true;
}

void ATrueSkySequenceActor::FreeViews()
{
	GetTrueSkyPlugin()->TriggerAction("FreeViews");
}

FString ATrueSkySequenceActor::GetProfilingText(int32 c,int32 g)
{
	GetTrueSkyPlugin()->SetRenderInt("maxCpuProfileLevel",c);
	GetTrueSkyPlugin()->SetRenderInt("maxGpuProfileLevel",g);
	return GetTrueSkyPlugin()->GetRenderString("profiling");
}

FString ATrueSkySequenceActor::TrueSkyProfileFrame(int32 arg1,int32 arg2)
{
	return( GetProfilingText( arg1, arg2 ) );
}

FString ATrueSkySequenceActor::GetText(FString str)
{
	return GetTrueSkyPlugin()->GetRenderString(str);
}

void ATrueSkySequenceActor::SetTime( float value )
{
	Time=value;
	bSetTime=true;
	// Force the actor to transfer properties:
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(A)
		A->Playing=true;
}

float ATrueSkySequenceActor::GetTime( )
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	return A->Time;
}

void ATrueSkySequenceActor::SetInteger( ETrueSkyPropertiesInt Property, int32 Value )
{
	if(TrueSkyPropertyIntMaps.Num())
	{
		auto p=TrueSkyPropertyIntMaps[TSPropertiesIntIdx].Find( ( int64 )Property );
		if(p->TrueSkyEnum>0)
		{
			ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
			if(A&&A->SetInts.Num()<100)
			{
				TPair<TrueSkyEnum,int32> t;
				t.Key=p->TrueSkyEnum;
				t.Value=Value;
				A->CriticalSection.Lock();
				A->SetInts.Add(t);
				A->CriticalSection.Unlock();
			}
		}
	}
	SetInt( TrueSkyPropertyIntMaps[TSPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ),Value);
}

int32 ATrueSkySequenceActor::GetInteger( ETrueSkyPropertiesInt Property )
{
	return( GetInt( TrueSkyPropertyIntMaps[TSPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}

void ATrueSkySequenceActor::SetFloatProperty( ETrueSkyPropertiesFloat Property, float Value )
{
	if(TrueSkyPropertyFloatMaps.Num())
	{
		auto p=TrueSkyPropertyFloatMaps[TSPropertiesFloatIdx].Find( ( int64 )Property );
		if(p->TrueSkyEnum>0)
		{
			ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
			if(A&&A->SetFloats.Num()<100)
			{
				TPair<TrueSkyEnum,float> t;
				t.Key=p->TrueSkyEnum;
				t.Value=Value;
				A->CriticalSection.Lock();
				A->SetFloats.Add(t);
				A->CriticalSection.Unlock();
			}
		}
	}
}

void ATrueSkySequenceActor::SetRenderingFloat( ETrueSkyRenderingProperties Property, float Value )
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if (TSRenderingPropertiesIdx>=0&&TSRenderingPropertiesIdx<TrueSkyPropertyFloatMaps.Num())
	{
		auto p = TrueSkyPropertyFloatMaps[TSRenderingPropertiesIdx].Find((int64)Property);
		if (p&&p->TrueSkyEnum > 0)
		{
			if (A&&A->SetRenderingValues.Num() < 100)
			{
				TPair<TrueSkyEnum, VariantPass> t;
				t.Key = p->TrueSkyEnum;
				t.Value.floatVal = Value;
				A->CriticalSection.Lock();
				A->SetRenderingValues.Add(t);
				A->CriticalSection.Unlock();
			}
		}
	}
}

void ATrueSkySequenceActor::SetRenderingInt( ETrueSkyRenderingProperties Property, int32 Value )
{
	if (TSRenderingPropertiesIdx >= 0 && TSRenderingPropertiesIdx<TrueSkyPropertyFloatMaps.Num())
	{
		auto p = TrueSkyPropertyFloatMaps[TSRenderingPropertiesIdx].Find((int64)Property);
		if (p&&p->TrueSkyEnum > 0)
		{
			ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
			if (A&&A->SetRenderingValues.Num() < 100)
			{
				TPair<TrueSkyEnum, VariantPass> t;
				t.Key = p->TrueSkyEnum;
				t.Value.intVal = Value;
				A->CriticalSection.Lock();
				A->SetRenderingValues.Add(t);
				A->CriticalSection.Unlock();
			}
		}
	}
}

void ATrueSkySequenceActor::SetRenderingVector( ETrueSkyPropertiesVector Property, FVector Value )
{
	// Essentially duplicates SetVectorProperty for now...
	if (TSPropertiesVectorIdx >= 0 && TSPropertiesVectorIdx <TrueSkyPropertyVectorMaps.Num())
	{
		auto p = TrueSkyPropertyVectorMaps[TSPropertiesVectorIdx].Find((int64)Property);
		if (p&&p->TrueSkyEnum > 0)
		{
			ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
			if (A&&A->SetVectors.Num() < 100)
			{
				TPair<TrueSkyEnum, vec3> t;
				t.Key = p->TrueSkyEnum;
				t.Value.x = Value.X;
				t.Value.y = Value.Y;
				t.Value.z = Value.Z;
				A->CriticalSection.Lock();
				A->SetVectors.Add(t);
				A->CriticalSection.Unlock();
			}
		}
	}
}

float ATrueSkySequenceActor::GetFloatProperty( ETrueSkyPropertiesFloat Property )
{
	if(TrueSkyPropertyFloatMaps.Num())
	{
		return( GetFloat( TrueSkyPropertyFloatMaps[TSPropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
	}
	return 0.0f;
}

void ATrueSkySequenceActor::SetVectorProperty( ETrueSkyPropertiesVector Property, FVector Value )
{
	if(TrueSkyPropertyVectorMaps.Num())
	{
		auto p=TrueSkyPropertyVectorMaps[TSPropertiesVectorIdx].Find( ( int64 )Property );
		if(p&&p->TrueSkyEnum>0)
		{
			 ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
			 if(A&&A->SetVectors.Num()<100)
			 {
				 TPair<TrueSkyEnum,vec3> t;
				 t.Key = p->TrueSkyEnum;
				 t.Value.x=Value.X;
				 t.Value.y=Value.Y;
				 t.Value.z=Value.Z;
				 A->CriticalSection.Lock();
				 A->SetVectors.Add(t);
				 A->CriticalSection.Unlock();
			 }
		}
	}
}

FVector ATrueSkySequenceActor::GetVectorProperty( ETrueSkyPropertiesVector Property )
{
	if(TrueSkyPropertyVectorMaps.Num())
	{
		auto p=TrueSkyPropertyVectorMaps[TSPropertiesVectorIdx].Find( ( int64 )Property );
		
		if(p&&p->TrueSkyEnum>0)
		{
			return GetTrueSkyPlugin()->GetVector(p->TrueSkyEnum);
		}
		else
		{
			UE_LOG4_ONCE(TrueSky,Error,TEXT("No such vector property %d"), ( int64 )Property);
		}
	}
	return FVector(0,0,0);
}

float ATrueSkySequenceActor::GetFloatPropertyAtPosition( ETrueSkyPositionalPropertiesFloat Property,FVector pos,int32 queryId)
{
	if(TrueSkyPropertyFloatMaps.Num())
	{
		auto p=TrueSkyPropertyFloatMaps[TSPositionalPropertiesFloatIdx].Find( ( int64 )Property );
		if(p->TrueSkyEnum>0)
		{
			if(!truesky_queries.Contains(queryId))
				truesky_queries.Add(queryId);
			auto &q=truesky_queries[queryId];
			q.uid=queryId;
			q.Pos=pos;
			q.LastFrame=GFrameNumber;
			q.Enum=p->TrueSkyEnum;
			if(q.Valid)
				return q.Float;
		}
		return GetTrueSkyPlugin()->GetRenderFloatAtPosition( p->PropertyName.ToString(),pos);
	}
	return 0.0f;
}

float ATrueSkySequenceActor::GetPrecipitationAtPosition(FVector pos, int32 queryId)
{
	return GetFloatPropertyAtPosition(ETrueSkyPositionalPropertiesFloat::TSPROPFLOAT_Precipitation, pos, queryId);
}


float ATrueSkySequenceActor::GetCloudinessAtPosition(FVector pos, int32 queryId)
{
	return GetFloatPropertyAtPosition(ETrueSkyPositionalPropertiesFloat::TSPROPFLOAT_Cloudiness, pos, queryId);
}

void ATrueSkySequenceActor::CleanQueries()
{
	for(auto q:truesky_queries)
	{
		if(GFrameNumber-q.Value.LastFrame>100)
		{
			truesky_queries.Remove(q.Key);
			break;
		}
	}
}

float ATrueSkySequenceActor::GetFloat(FString name )
{
	if(!name.Compare("time",ESearchCase::IgnoreCase))
	{
		UE_LOG_ONCE( TrueSky, Warning, TEXT( "Time should no longer be retrieved using GetFloat." ) );
		ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
		return A->Time;
	}
	else
		return GetTrueSkyPlugin()->GetRenderFloat(name);
}

void ATrueSkySequenceActor::GetSkyLayerValue(ETrueSkySkyLayerProperties Property, int& returnInt, float& returnFloat)
{
	returnInt = -1;
	returnFloat = -1.f;
	int maxInt = (int)ETrueSkySkyLayerProperties::TSPROP_MaxInt;
	
	if ((int)Property <= maxInt)
		returnInt = (GetKeyframerInt(0,TrueSkyPropertyIntMaps[TSSkyLayerPropertiesIntIdx].Find((int)Property)->PropertyName.ToString()));
	else
		returnFloat = (GetKeyframerFloat(0, TrueSkyPropertyFloatMaps[TSSkyLayerPropertiesFloatIdx].Find((int)Property - maxInt - 1)->PropertyName.ToString()));
}

void ATrueSkySequenceActor::SetSkyLayerValue(ETrueSkySkyLayerProperties Property, int intValue, float floatValue)
{
	int maxInt = (int)ETrueSkySkyLayerProperties::TSPROP_MaxInt;

	if ((int)Property <= maxInt)
		SetKeyframerInt(0,TrueSkyPropertyIntMaps[TSSkyLayerPropertiesIntIdx].Find((int)Property)->PropertyName.ToString(), intValue);
	else
		SetKeyframerFloat(0,TrueSkyPropertyFloatMaps[TSSkyLayerPropertiesFloatIdx].Find((int)Property - maxInt - 1)->PropertyName.ToString(), floatValue);

}

void ATrueSkySequenceActor::GetSkyKeyframeValue(ETrueSkySkyKeyframeProperties Property, int32 UID, int& returnInt, float& returnFloat)
{
	returnInt = -1;
	returnFloat = -1.f;
	int maxInt = (int)ETrueSkySkyKeyframeProperties::TSPROP_MaxInt;

	if ((int)Property <= maxInt)
		returnInt = (GetKeyframeInt(UID, TrueSkyPropertyIntMaps[TSSkyKeyframePropertiesIntIdx].Find((int)Property)->PropertyName.ToString()));
	else
		returnFloat = (GetKeyframeFloat(UID, TrueSkyPropertyFloatMaps[TSSkyKeyframePropertiesFloatIdx].Find((int)Property - maxInt - 1)->PropertyName.ToString()));
}

void ATrueSkySequenceActor::SetSkyKeyframeValue(ETrueSkySkyKeyframeProperties Property, int32 UID, int intValue, float floatValue)
{
	int maxInt = (int)ETrueSkySkyKeyframeProperties::TSPROP_MaxInt;

	if ((int)Property <= maxInt)
		SetKeyframeInt(UID, TrueSkyPropertyIntMaps[TSSkyKeyframePropertiesIntIdx].Find((int)Property)->PropertyName.ToString(), intValue);
	else
		SetKeyframeFloat(UID, TrueSkyPropertyFloatMaps[TSSkyKeyframePropertiesFloatIdx].Find((int)Property - maxInt - 1)->PropertyName.ToString(), floatValue);
}

void ATrueSkySequenceActor::GetCloudLayerValue(ETrueSkyCloudLayerProperties Property, int32 layerUID, int& returnInt, float& returnFloat)
{
	returnInt = -1;
	returnFloat = -1.f;
	int maxInt = (int)ETrueSkyCloudLayerProperties::TSPROP_MaxInt;

	if ((int)Property <= maxInt)
		returnInt = (GetKeyframerInt(layerUID, TrueSkyPropertyFloatMaps[TSCloudLayerPropertiesIntIdx].Find((int64)Property)->PropertyName.ToString()));
	else
		returnFloat = (GetKeyframerFloat(layerUID, TrueSkyPropertyFloatMaps[TSCloudLayerPropertiesFloatIdx].Find((int64)Property - maxInt - 1)->PropertyName.ToString()));

}

void ATrueSkySequenceActor::SetCloudLayerValue(ETrueSkyCloudLayerProperties Property, int32 layerUID, int intValue, float floatValue)
{
	int maxInt = (int)ETrueSkyCloudLayerProperties::TSPROP_MaxInt;

	if ((int)Property <= maxInt)
		SetKeyframerInt(layerUID, TrueSkyPropertyIntMaps[TSCloudLayerPropertiesIntIdx].Find((int)Property)->PropertyName.ToString(), intValue);
	else
		SetKeyframerFloat(layerUID, TrueSkyPropertyFloatMaps[TSCloudLayerPropertiesFloatIdx].Find((int)Property - maxInt - 1)->PropertyName.ToString(), floatValue);

}

void ATrueSkySequenceActor::GetCloudKeyframeValue(ETrueSkyCloudKeyframeProperties Property, int32 UID, int& returnInt, float& returnFloat)
{
	returnInt = -1;
	returnFloat = -1.f;
	int maxInt = (int)ETrueSkyCloudKeyframeProperties::TSPROP_MaxInt;

	if ((int)Property <= maxInt)
		returnInt = (GetKeyframeInt(UID,TrueSkyPropertyIntMaps[TSCloudKeyframePropertiesIntIdx].Find((int)Property)->PropertyName.ToString()));
	else
		returnFloat = (GetKeyframeFloat(UID,TrueSkyPropertyFloatMaps[TSCloudKeyframePropertiesFloatIdx].Find((int)Property - maxInt - 1)->PropertyName.ToString()));
}

void ATrueSkySequenceActor::SetCloudKeyframeValue(ETrueSkyCloudKeyframeProperties Property, int32 UID, int intValue, float floatValue)
{
	int maxInt = (int)ETrueSkyCloudKeyframeProperties::TSPROP_MaxInt;

	if ((int)Property <= maxInt)
		SetKeyframeInt(UID, TrueSkyPropertyIntMaps[TSCloudKeyframePropertiesIntIdx].Find((int)Property)->PropertyName.ToString(), intValue);
	else
		SetKeyframeFloat(UID,TrueSkyPropertyFloatMaps[TSCloudKeyframePropertiesFloatIdx].Find((int)Property - maxInt - 1)->PropertyName.ToString(), floatValue);

}



float ATrueSkySequenceActor::GetFloatAtPosition(FString name,FVector pos)
{
	return GetTrueSkyPlugin()->GetRenderFloatAtPosition(name,pos);
}

void ATrueSkySequenceActor::SetFloat(FString name, float value )
{
	if(!name.Compare("time",ESearchCase::IgnoreCase))
	{
		UE_LOG_ONCE( TrueSky, Warning, TEXT( "Time should no longer be set using SetFloat." ) );
	}
	else
		GetTrueSkyPlugin()->SetRenderFloat(name,value);
}

int32 ATrueSkySequenceActor::GetInt(FString name ) 
{
	return GetTrueSkyPlugin()->GetRenderInt(name);
}

void ATrueSkySequenceActor::SetInt(FString name, int32 value )
{
	GetTrueSkyPlugin()->SetRenderInt(name,value);
}

void ATrueSkySequenceActor::SetKeyframerInt(int32 layerID, FString name, int32 value)
{
	GetTrueSkyPlugin()->SetKeyframerInt(layerID, name, value);
}

void ATrueSkySequenceActor::SetKeyframerFloat(int32 layerID, FString name, float value)
{
	GetTrueSkyPlugin()->SetKeyframerFloat(layerID, name, value);
}

int32 ATrueSkySequenceActor::GetKeyframerInt(int32 layerID, FString name)
{
	return GetTrueSkyPlugin()->GetKeyframerInt(layerID, name);
}

float ATrueSkySequenceActor::GetKeyframerFloat(int32 layerID, FString name)
{
	return GetTrueSkyPlugin()->GetKeyframerFloat(layerID, name);
}

void ATrueSkySequenceActor::SetBool(FString name, bool value )
{
	GetTrueSkyPlugin()->SetRenderBool(name,value);
}

float ATrueSkySequenceActor::GetKeyframeFloat(int32 keyframeUid,FString name ) 
{
	return GetTrueSkyPlugin()->GetKeyframeFloat(keyframeUid,name);
}

int ATrueSkySequenceActor::GetKeyframeInt(int32 keyframeUid, FString name)
{
	return GetTrueSkyPlugin()->GetKeyframeInt(keyframeUid, name);
}

void ATrueSkySequenceActor::SetKeyframeFloat(int32 keyframeUid,FString name, float value )
{
	GetTrueSkyPlugin()->SetKeyframeFloat(keyframeUid,name,value);
}

void ATrueSkySequenceActor::SetKeyframeInt(int32 keyframeUid,FString name, int32 value )
{
	GetTrueSkyPlugin()->SetKeyframeInt(keyframeUid,name,value);
}

int32 ATrueSkySequenceActor::GetNextModifiableSkyKeyframe() 
{
	return GetTrueSkyPlugin()->GetRenderInt("NextModifiableSkyKeyframe");
}
	
int32 ATrueSkySequenceActor::GetNextModifiableCloudKeyframe(int32 layer) 
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer));
	return GetTrueSkyPlugin()->GetRenderInt("NextModifiableCloudKeyframe",arr);
}

int32 ATrueSkySequenceActor::GetCloudKeyframeByIndex(int32 layer,int32 index)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer));
	arr.Push(FVariant(index));
	return GetTrueSkyPlugin()->GetRenderInt("clouds:KeyframeByIndex",arr);
}

int32 ATrueSkySequenceActor::GetNextCloudKeyframeAfterTime(int32 layer,float t)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer));
	arr.Push(FVariant(t));
	return GetTrueSkyPlugin()->GetRenderInt("clouds:NextKeyframeAfterTime",arr);
}

int32 ATrueSkySequenceActor::GetPreviousCloudKeyframeBeforeTime(int32 layer,float t)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer));
	arr.Push(FVariant(t));
	return GetTrueSkyPlugin()->GetRenderInt("clouds:PreviousKeyframeBeforeTime",arr);
}
	
int32 ATrueSkySequenceActor::GetInterpolatedCloudKeyframe(int32 layer)
{
	TArray<FVariant> arr;
	arr.Push(FVariant(layer)); 
	return GetTrueSkyPlugin()->GetRenderInt("InterpolatedCloudKeyframe", arr);
}

int32 ATrueSkySequenceActor::GetInterpolatedSkyKeyframe()
{ 
	return GetTrueSkyPlugin()->GetRenderInt("InterpolatedSkyKeyframe");
}

int32 ATrueSkySequenceActor::GetSkyKeyframeByIndex(int32 index)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(index));
	return GetTrueSkyPlugin()->GetRenderInt("sky:KeyframeByIndex", arr);
}

int32 ATrueSkySequenceActor::GetNextSkyKeyframeAfterTime(float t)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(t));
	return GetTrueSkyPlugin()->GetRenderInt("sky:NextKeyframeAfterTime",arr);
}

int32 ATrueSkySequenceActor::GetPreviousSkyKeyframeBeforeTime(float t)  
{
	TArray<FVariant> arr;
	arr.Push(FVariant(t));
	return GetTrueSkyPlugin()->GetRenderInt("sky:PreviousKeyframeBeforeTime",arr);
}
//Deprecated
void ATrueSkySequenceActor::SetSkyLayerInt( ETrueSkySkySequenceLayerPropertiesInt Property, int32 Value )
{
	SetInt( TrueSkyPropertyIntMaps[TSSkyLayerPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}
//Deprecated
void ATrueSkySequenceActor::SetSkyKeyframeInt( ETrueSkySkySequenceKeyframePropertiesInt Property, int32 KeyframeUID, int32 Value )
{
	SetKeyframeInt( KeyframeUID, TrueSkyPropertyIntMaps[TSSkyKeyframePropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}
//Deprecated
void ATrueSkySequenceActor::SetCloudLayerInt( ETrueSkyCloudSequenceLayerPropertiesInt Property, int32 layerID, int32 Value )
{
	SetKeyframerInt(layerID, TrueSkyPropertyIntMaps[TSCloudLayerPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}
//Deprecated
void ATrueSkySequenceActor::SetCloudKeyframeInt( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32 KeyframeUID, int32 Value )
{
	SetKeyframeInt( KeyframeUID, TrueSkyPropertyIntMaps[TSCloudKeyframePropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}
//Deprecated
int32 ATrueSkySequenceActor::GetSkyLayerInt( ETrueSkySkySequenceLayerPropertiesInt Property )
{
	return( GetInt( TrueSkyPropertyIntMaps[TSSkyLayerPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}
//Deprecated
int32 ATrueSkySequenceActor::GetSkyKeyframeInt( ETrueSkySkySequenceKeyframePropertiesInt Property, int32 KeyframeUID )
{
	return( GetKeyframeInt( KeyframeUID, TrueSkyPropertyIntMaps[TSSkyKeyframePropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}
//Deprecated
int32 ATrueSkySequenceActor::GetCloudLayerInt( ETrueSkyCloudSequenceLayerPropertiesInt Property, int32 layerID )
{
	return( GetKeyframerInt(layerID, TrueSkyPropertyIntMaps[TSCloudLayerPropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}
//Deprecated
int32 ATrueSkySequenceActor::GetCloudKeyframeInt( ETrueSkyCloudSequenceKeyframePropertiesInt Property, int32 KeyframeUID )
{
	return( GetKeyframeInt( KeyframeUID, TrueSkyPropertyIntMaps[TSCloudKeyframePropertiesIntIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}
//Deprecated
void ATrueSkySequenceActor::SetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat Property, float Value )
{
	SetFloat( TrueSkyPropertyFloatMaps[TSSkyLayerPropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}
//Deprecated
void ATrueSkySequenceActor::SetSkyKeyframeFloat( ETrueSkySkySequenceKeyframePropertiesFloat Property, int32 KeyframeUID, float Value )
{
	SetKeyframeFloat( KeyframeUID, TrueSkyPropertyFloatMaps[TSSkyKeyframePropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}
//Deprecated
void ATrueSkySequenceActor::SetCloudLayerFloat( ETrueSkyCloudSequenceLayerPropertiesFloat Property, float Value )
{
	SetFloat( TrueSkyPropertyFloatMaps[TSCloudLayerPropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}
//Deprecated
void ATrueSkySequenceActor::SetCloudKeyframeFloat( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, int32 KeyframeUID, float Value )
{
	SetKeyframeFloat( KeyframeUID, TrueSkyPropertyFloatMaps[TSCloudKeyframePropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ), Value );
}
//Deprecated
float ATrueSkySequenceActor::GetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat Property )
{
	if(TrueSkyPropertyFloatMaps.Num())
	{
		TrueSkyPropertyFloatData *p= TrueSkyPropertyFloatMaps[TSSkyLayerPropertiesFloatIdx].Find( ( int64 )Property );
		if(p->TrueSkyEnum>0)
		{
			return GetTrueSkyPlugin()->GetFloat(p->TrueSkyEnum);
		}
		return( GetFloat(p->PropertyName.ToString( ) ) );
	}
	return 0.0f;
}
//Deprecated
float ATrueSkySequenceActor::GetSkyKeyframeFloat( ETrueSkySkySequenceKeyframePropertiesFloat Property, int32 KeyframeUID )
{
	return( GetKeyframeFloat( KeyframeUID, TrueSkyPropertyFloatMaps[TSSkyKeyframePropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ) ) );
}
//Deprecated
float ATrueSkySequenceActor::GetCloudLayerFloat( ETrueSkyCloudSequenceLayerPropertiesFloat Property, int32 layerUID)
{
	return( GetKeyframerFloat(layerUID, TrueSkyPropertyFloatMaps[TSCloudLayerPropertiesFloatIdx].Find( ( int64 )Property )->PropertyName.ToString( ))  );
}
//Deprecated
float ATrueSkySequenceActor::GetCloudKeyframeFloat( ETrueSkyCloudSequenceKeyframePropertiesFloat Property, int32 KeyframeUID )
{
	return GetKeyframeFloat(KeyframeUID, TrueSkyPropertyFloatMaps[TSCloudKeyframePropertiesFloatIdx].Find((int64)Property)->PropertyName.ToString());
}

FRotator GetRotatorFromAzimuthElevation(FTransform Transform, float azimuth, float elevation)
{
	FRotator rotation(c1 + m1 * elevation, c2 + m2 * azimuth, c3 + m3 * elevation + m4 * azimuth);
	FVector dir = rotation.Vector();
	FVector d_u = TrueSkyToUEDirection(Transform, dir);
	float p = asin(tsa_c*d_u.Z)*180.f / PI_F;
	float y = atan2(tsa_a*d_u.Y, tsa_b*d_u.X)*180.f / PI_F;
	return FRotator(p, y, 0);
}

FRotator ATrueSkySequenceActor::GetSunRotation() 
{
	float azimuth	=GetTrueSkyPlugin()->GetRenderFloat("sky:SunAzimuthDegrees");		// Azimuth in compass degrees from north.
	float elevation	=GetTrueSkyPlugin()->GetRenderFloat("sky:SunElevationDegrees");

	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();

	return GetRotatorFromAzimuthElevation(A->Transform, azimuth, elevation);
}

FRotator ATrueSkySequenceActor::GetMoonRotation() 
{
	float azimuth	=GetTrueSkyPlugin()->GetRenderFloat("sky:MoonAzimuthDegrees");
	float elevation	=GetTrueSkyPlugin()->GetRenderFloat("sky:MoonElevationDegrees");

	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();

	return GetRotatorFromAzimuthElevation(A->Transform, azimuth, elevation);
}

void ATrueSkySequenceActor::SetSunRotation(FRotator r)
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	FVector dir		=UEToTrueSkyDirection(A->Transform,r.Vector());
	SetVectorProperty( ETrueSkyPropertiesVector::TSPROPVECTOR_SunDirection, -dir );
}

void ATrueSkySequenceActor::SetMoonRotation(FRotator r)
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	FVector dir=UEToTrueSkyDirection(A->Transform,r.Vector());
	SetVectorProperty( ETrueSkyPropertiesVector::TSPROPVECTOR_MoonDirection, -dir );
}

float ATrueSkySequenceActor::CloudLineTest(int32 queryId,FVector StartPos,FVector EndPos) 
{
	return GetTrueSkyPlugin()->CloudLineTest(queryId,StartPos,EndPos);
}

float ATrueSkySequenceActor::CloudPointTest(int32 queryId,FVector pos) 
{
	return GetTrueSkyPlugin()->GetCloudinessAtPosition(queryId,pos);
}

float ATrueSkySequenceActor::GetCloudShadowAtPosition(int32 queryId,FVector pos) 
{
	ITrueSkyPlugin::VolumeQueryResult res= GetTrueSkyPlugin()->GetStateAtPosition(queryId,pos);
	return (1 - res.direct_light);
}

float ATrueSkySequenceActor::GetPrecipitationStrengthAtPosition(int32 queryId,FVector pos)
{
	ITrueSkyPlugin::VolumeQueryResult res= GetTrueSkyPlugin()->GetStateAtPosition(queryId,pos);
	return res.precipitation;
}

float ATrueSkySequenceActor::GetCloudDensityAtPosition(int32 queryId, FVector pos)
{
	ITrueSkyPlugin::VolumeQueryResult res = GetTrueSkyPlugin()->GetStateAtPosition(queryId, pos);
	return res.density;
}

FLinearColor ATrueSkySequenceActor::GetSunColor() 
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	FLinearColor col;
	if(!A)
		return col;
	FVector pos=A->Transform.GetTranslation();
	ITrueSkyPlugin::LightingQueryResult	res=GetLightingAtPosition(9813475, pos);
	if(res.valid)
	{
		col.R=res.sunlight[0];
		col.G=res.sunlight[1];
		col.B=res.sunlight[2];
		float m=std::max(std::max(std::max(col.R,col.G),col.B),1.0f);
		col/=m;
	}
	return col;
}

FLinearColor ATrueSkySequenceActor::GetMoonColor() 
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	FLinearColor col;
	if(!A)
		return col;
	FVector pos=A->Transform.GetTranslation();
	ITrueSkyPlugin::LightingQueryResult	res=GetLightingAtPosition(9813475, pos);
	if(res.valid)
	{
		col.R=res.moonlight[0];
		col.G=res.moonlight[1];
		col.B=res.moonlight[2];
		float m=std::max(std::max(std::max(col.R,col.G),col.B),1.0f);
		col/=m;
	}
	return col;
}

float ATrueSkySequenceActor::GetSunIntensity() 
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	float i=1.0f;
	if(!A)
		return i;
	FVector pos=A->Transform.GetTranslation();
	ITrueSkyPlugin::LightingQueryResult	res=GetLightingAtPosition(9813475, pos);
	if(res.valid)
	{
		FLinearColor col;
		col.R=res.sunlight[0];
		col.G=res.sunlight[1];
		col.B=res.sunlight[2];
		float m=std::max(std::max(std::max(col.R,col.G),col.B),1.0f);
		i=m;
	}
	return i;
}

float ATrueSkySequenceActor::GetMoonIntensity() 
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	float i=1.0f;
	if(!A)
		return i;
	FVector pos=A->Transform.GetTranslation();
	ITrueSkyPlugin::LightingQueryResult	res=GetLightingAtPosition(9813475, pos);
	if(res.valid)
	{
		FLinearColor col;
		col.R=res.moonlight[0];
		col.G=res.moonlight[1];
		col.B=res.moonlight[2];
		float m=std::max(std::max(std::max(col.R,col.G),col.B),1.0f);
		i=m;
	}
	return i;
}

bool ATrueSkySequenceActor::UpdateLight(ADirectionalLight* DirectionalLight,float sun_multiplier,float moon_multiplier,bool include_shadow,bool apply_rotation, bool light_in_space)
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	if(!A)
		return false;
	if(!DirectionalLight)
		return false;
	UDirectionalLightComponent* DirectionalLightComponent = CastChecked<UDirectionalLightComponent>(DirectionalLight->GetLightComponent());
	if(!DirectionalLightComponent)
		return false;
	return( UpdateLightComponent(DirectionalLightComponent, sun_multiplier,moon_multiplier, include_shadow, apply_rotation, light_in_space) );
}

const ITrueSkyPlugin::LightingQueryResult &ATrueSkySequenceActor::GetLightingAtPosition(int32 uid,FVector ue_pos)
{
	static ITrueSkyPlugin::LightingQueryResult empty;
	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
	A->CriticalSection.Lock();
	FVector ts_pos = UEToTrueSkyPosition(A->Transform, A->MetresPerUnit, ue_pos);
	ITrueSkyPlugin::LightingQueryResult &res = A->LightingQueries.FindOrAdd(uid);

	for (int i = 0; i < 3; i++)
		res.pos[i] = ts_pos[i];
	A->CriticalSection.Unlock();
	return res;
}


bool ATrueSkySequenceActor::UpdateLightComponent(UDirectionalLightComponent* DirectionalLightComponent,float sun_multiplier,float moon_multiplier,bool include_shadow,bool apply_rotation,bool light_in_space)
{
	ActorCrossThreadProperties* A = GetActorCrossThreadProperties();
	if (!A || !IsValid(DirectionalLightComponent))
		return false;
	FVector pos(0,0,0);
	pos=DirectionalLightComponent->GetComponentTransform().GetTranslation();
	unsigned uid=(unsigned)((int64)DirectionalLightComponent);
	ITrueSkyPlugin::LightingQueryResult	res=GetLightingAtPosition(uid, pos);

	if (light_in_space && !(GetTrueSkyPlugin()->GetVersion() == SIMUL_4_1))
	{
		res.sunlight[0] = res.sunlight_in_space[0];
		res.sunlight[1] = res.sunlight_in_space[1];
		res.sunlight[2] = res.sunlight_in_space[2];
	}
	if (res.valid)
	{
		//move to a new function? This is checking if the units are photometric, the Units Unreal wants, then we are applying the calculations to the value. Need
		//access to individual wavelengths for correct Value. If the values are Radiometric, then we can apply the brightness power multiplier in the functions below
		if (A->TrueSkyLightUnits == (int)ETrueSkyLightUnits::Photometric) //If we want to convert into Lux
		{
			for (int i = 0; i < 3; i++)
			{
				//ideally we want to /wavelength (nm), * 683 * Photopic/Scotpic Luminous Efficacy. We currently store the wavelengths on the Sequencer.
				//or we can have the estimate instead of having photopic and scotopic values (Lumens * 0.0079 = W/m2), (W/m2 * 127 = Lumens). This does not take into account wavelengths
				const float PhotometricUnitConversion = 127.f * 1000.f; //the 1000 here is to make the value somewhat similar. We are using nm, and for the Lux we want to be in meters, but a 10^9 change is much too large. 
				res.sunlight[i] *= PhotometricUnitConversion;
				res.moonlight[i] *= PhotometricUnitConversion;// I think we may actually be using MicroMeters. Regardless, this conversion will allow people to have values around the 100k mark. 
				res.ambient[i] *= PhotometricUnitConversion;	//Brightness Power and Irradiance need to be set to their default values to use this.	

				//res.ambient[i] = currentWatts/m2 * 683.f * (efficacy, 555nm = 1.0) http://hyperphysics.phy-astr.gsu.edu/hbase/vision/efficacy.html#c1
			}
		}
		
		FLinearColor linearColour;
		linearColour.R=res.sunlight[0];
		linearColour.G=res.sunlight[1];
		linearColour.B=res.sunlight[2];
		linearColour*=sun_multiplier;
		if(include_shadow)
			linearColour*=res.sunlight[3];
		float m=std::max(std::max(linearColour.R,linearColour.G),linearColour.B);
		float l=std::max(m,1.0f);
		float azimuth	=0.0f;	// Azimuth in compass degrees from north.
		float elevation	=0.0f;
		if(m>0.0f)
		{
			linearColour/=l;
			DirectionalLightComponent->SetCastShadows(true);
			DirectionalLightComponent->SetIntensity(l);
			FVector SunDirection=GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_SunDirection);
			azimuth				=180.0f/3.1415926536f*atan2(SunDirection.X,SunDirection.Y);
			elevation			=180.0f/3.1415926536f*asin(SunDirection.Z);
		}
		else
		{
			linearColour.R=res.moonlight[0];
			linearColour.G=res.moonlight[1];
			linearColour.B=res.moonlight[2];
			linearColour*=moon_multiplier;
			if(include_shadow)
				linearColour*=res.moonlight[3];
			m=std::max(std::max(linearColour.R,linearColour.G),linearColour.B);
			l=std::max(m,1.0f);
			if(m>0.0f)
			{
				linearColour/=l;
				DirectionalLightComponent->SetCastShadows(true);
				DirectionalLightComponent->SetIntensity(l);
				FVector MoonDirection=GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_MoonDirection);
				azimuth				=180.0f/3.1415926536f*atan2(MoonDirection.X,MoonDirection.Y);
				elevation			=180.0f/3.1415926536f*asin(MoonDirection.Z);
			}
			else
			{
				DirectionalLightComponent->SetCastShadows(false);
				DirectionalLightComponent->SetIntensity(0.0f);
			}
		}
		DirectionalLightComponent->SetLightColor(linearColour,false);
		if(apply_rotation)
		{
			DirectionalLightComponent->SetUsingAbsoluteRotation(true);
			DirectionalLightComponent->SetWorldRotation(GetRotatorFromAzimuthElevation(A->Transform, azimuth, elevation));
		}
		return true;
	}
	return false;
}
	
bool ATrueSkySequenceActor::UpdateSunlight(ADirectionalLight *DirectionalLight,float multiplier,bool include_shadow,bool apply_rotation)
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	if(!A)
		return false;
	if(!DirectionalLight)
		return false;
	UDirectionalLightComponent* DirectionalLightComponent = CastChecked<UDirectionalLightComponent>(DirectionalLight->GetLightComponent());
	if(!DirectionalLightComponent)
		return false;
	FVector pos=A->Transform.GetTranslation();
	unsigned uid=(unsigned)((int64)DirectionalLight);
	ITrueSkyPlugin::LightingQueryResult	res=GetLightingAtPosition(uid, pos);
	if(res.valid)
	{
		FLinearColor linearColour;
		linearColour.R=res.sunlight[0];
		linearColour.G=res.sunlight[1];
		linearColour.B=res.sunlight[2];
		linearColour*=multiplier;
		if(include_shadow)
			linearColour*=res.sunlight[3];
		float m=std::max(std::max(linearColour.R,linearColour.G),linearColour.B);
		float l=std::max(m,1.0f);
		if(m>0.0f)
		{
			linearColour/=l;
			DirectionalLightComponent->SetCastShadows(true);
			DirectionalLightComponent->SetIntensity(l);
		}
		else
		{
			DirectionalLightComponent->SetCastShadows(false);
			DirectionalLightComponent->SetIntensity(0.0f);
		}
		DirectionalLightComponent->SetLightColor(linearColour,false);
		if(apply_rotation)
		{
			float azimuth	=GetTrueSkyPlugin()->GetRenderFloat("sky:SunAzimuthDegrees");		// Azimuth in compass degrees from north.
			float elevation	=GetTrueSkyPlugin()->GetRenderFloat("sky:SunElevationDegrees");
			DirectionalLight->SetActorRotation(GetRotatorFromAzimuthElevation(A->Transform, azimuth, elevation));
		}
		return true;
	}
	return false;
}
	
bool ATrueSkySequenceActor::UpdateMoonlight(ADirectionalLight*DirectionalLight,float multiplier,bool include_shadow,bool apply_rotation)
{
	ActorCrossThreadProperties *A=GetActorCrossThreadProperties();
	if(!A)
		return false;
	if(!DirectionalLight)
		return false;
	UDirectionalLightComponent* DirectionalLightComponent = CastChecked<UDirectionalLightComponent>(DirectionalLight->GetLightComponent());
	if(!DirectionalLightComponent)
		return false;
	FVector pos=A->Transform.GetTranslation();
	unsigned uid=(unsigned)((int64)DirectionalLight);
	ITrueSkyPlugin::LightingQueryResult	res=GetLightingAtPosition(uid, pos);
	if(res.valid)
	{
		FLinearColor linearColour;
		linearColour.R=res.moonlight[0];
		linearColour.G=res.moonlight[1];
		linearColour.B=res.moonlight[2];
		linearColour*=multiplier;
		if(include_shadow)
			linearColour*=res.moonlight[3];
		float m=std::max(std::max(linearColour.R,linearColour.G),linearColour.B);
		float l=std::max(m,1.0f);
		if(m>0.0f)
		{
			linearColour/=l;
			DirectionalLightComponent->SetCastShadows(true);
			DirectionalLightComponent->SetIntensity(l);
		}
		else
		{
			DirectionalLightComponent->SetCastShadows(false);
			DirectionalLightComponent->SetIntensity(0.0f);
		}
		DirectionalLightComponent->SetLightColor(linearColour,false);
		if(apply_rotation)
		{
			float azimuth	=GetTrueSkyPlugin()->GetRenderFloat("sky:MoonAzimuthDegrees");		// Azimuth in compass degrees from north.
			float elevation	=GetTrueSkyPlugin()->GetRenderFloat("sky:MoonElevationDegrees");
			DirectionalLight->SetActorRotation(GetRotatorFromAzimuthElevation(A->Transform, azimuth, elevation));
		}
		return true;
	}
	return false;
}

void ATrueSkySequenceActor::SetPointLightSource(int id,FLinearColor lc,float Intensity,FVector pos,float min_radius,float max_radius) 
{
	GetTrueSkyPlugin()->SetPointLight(id,lc*Intensity,pos, min_radius, max_radius);
}

void ATrueSkySequenceActor::SetPointLight(APointLight *source) 
{
	int id=(int)(int64_t)(source);
	FLinearColor lc=source->PointLightComponent->LightColor;
	lc*=source->PointLightComponent->Intensity;
	FVector pos=source->GetActorLocation();
	float min_radius=source->PointLightComponent->SourceRadius;
	float max_radius=source->PointLightComponent->AttenuationRadius;
	GetTrueSkyPlugin()->SetPointLight(id,lc,pos, min_radius, max_radius);
}

// Adapted from the function in SceneCaptureRendering.cpp.
void BuildProjectionMatrix(float InOrthoWidth,float FarPlane,FMatrix &ProjectionMatrix)
{
	const float OrthoSize = InOrthoWidth / 2.0f;

	const float NearPlane = 0;

	const float ZScale = 1.0f / (FarPlane - NearPlane);
	const float ZOffset = -NearPlane;

	ProjectionMatrix = FReversedZOrthoMatrix(
		OrthoSize,
		OrthoSize,
		ZScale,
		ZOffset
	);
}

void AdaptOrthoProjectionMatrix(FMatrix &projMatrix, float metresPerUnit)
{
	projMatrix.M[0][0]	/= metresPerUnit;
	projMatrix.M[1][1]	/= metresPerUnit;
	projMatrix.M[2][2]	*= -1.0f/metresPerUnit;
	projMatrix.M[3][2]	= (1.0-projMatrix.M[3][2])/metresPerUnit;
}


void CaptureComponentToViewMatrix(FMatrix &viewMatrix,float metresPerUnit,const FMatrix &worldToSkyMatrix)
{
	FMatrix u=worldToSkyMatrix*viewMatrix;
	u.M[3][0]	*= metresPerUnit;
	u.M[3][1]	*= metresPerUnit;
	u.M[3][2]	*= metresPerUnit;
	// A camera in UE has X forward, Y right and Z up. This is no good for a view matrix, which needs to have Z forward
	// or backwards.
	static float U=0.f,V=90.f,W=0.f;	// pitch, yaw, roll.
	FRotator rot(U,V,W);
	FMatrix v;
	
	FRotationMatrix RotMatrix(rot);
	FMatrix r	=RotMatrix.GetTransposed();
	v			=r.operator*(u);
	
	{
		v.M[0][0]*=-1.f;
		v.M[0][1]*=-1.f;
		v.M[0][3]*=-1.f;
	}
	{
		v.M[1][2]*=-1.f;
		v.M[2][2]*=-1.f;
		v.M[3][2]*=-1.f;
	}
	viewMatrix.M[0][2]=v.M[0][0];
	viewMatrix.M[1][2]=v.M[1][0];
	viewMatrix.M[2][2]=v.M[2][0];
	viewMatrix.M[3][2]=v.M[3][0];

	viewMatrix.M[0][0]=v.M[0][1];
	viewMatrix.M[1][0]=v.M[1][1];
	viewMatrix.M[2][0]=v.M[2][1];
	viewMatrix.M[3][0]=v.M[3][1];

	viewMatrix.M[0][1]=v.M[0][2];
	viewMatrix.M[1][1]=v.M[1][2];
	viewMatrix.M[2][1]=v.M[2][2];
	viewMatrix.M[3][1]=v.M[3][2];

	viewMatrix.M[0][3]=v.M[0][3];
	viewMatrix.M[1][3]=v.M[1][3];
	viewMatrix.M[2][3]=v.M[2][3];
	viewMatrix.M[3][3]=v.M[3][3];
}


extern void AdaptViewMatrix(FMatrix &viewMatrix,float metresPerUnit,const FMatrix &worldToSkyMatrix);

void ATrueSkySequenceActor::TransferProperties()
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(!A)
		return;
	//A->RainDepthRT					=nullptr;
	if (!IsActiveActor())
	{
		if(!IsAnyActorActive())
		{
			A->Playing=false;
			A->Visible					=false;
			A->activeSequence			=nullptr;
		}
		return;
	}
	SimulVersion simulVersion		=GetTrueSkyPlugin()->GetVersion();
	bSimulVersion4_1				=(simulVersion>SIMUL_4_0);
	bSimulVersion4_2				=(simulVersion>=SIMUL_4_2);
	AltitudeLighting				&= bSimulVersion4_2;
	A->InterpolationMode			=(uint8_t)InterpolationMode;
	A->InstantUpdate				=InstantUpdate;
	A->InterpolationTimeSeconds		=InterpolationTimeSeconds;
	A->InterpolationTimeHours		=InterpolationTimeHours;
	A->InterpolationSubdivisions	=InterpolationSubdivisions;
	A->RainNearThreshold			=RainNearThreshold;
	A->RenderWater					=RenderWater;
	A->WaterFullResolution			=(WaterResolution == EWaterResolution::FullSize);
	A->EnableReflection				=EnableReflection;
	A->ReflectionFullResolution		=(ReflectionResolution == EWaterResolution::FullSize);
	A->ReflectionSteps				=ReflectionSteps;
	A->ReflectionPixelStep			=ReflectionPixelStep;
	A->WaterUseGameTime				=UseWaterGameTime;
	A->Rotation						=GetActorRotation().Yaw;
	A->WaterOffset					=-(GetActorLocation());
	A->ShareBuffersForVR			=ShareBuffersForVR;
	A->Destroyed					=false;
	A->Visible						=Visible;
	A->MetresPerUnit				=MetresPerUnit;
	A->Brightness					=Brightness;
	A->Gamma						=Gamma;
	UWorld *world					=GetWorld();
	A->GridRendering				=(IntegrationScheme==EIntegrationScheme::Grid);
	
	if(A->activeSequence.Get()!=ActiveSequence)
	{
		A->activeSequence				=( UTrueSkySequenceAsset *)ActiveSequence;
		if (!world|| world->WorldType == EWorldType::Editor)
			A->Reset=true;
	}
	if(!world || world->WorldType == EWorldType::Editor)
	{
		if(A->activeSequence.Get() == ActiveSequence)
		{
			A->Playing = false;
		}
	}

	//Clear weather effects; if there is no active sequence, or the active sequence is empty.
	if(!ActiveSequence || ActiveSequence->SequenceText.Num() == 0)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, TranslucentRT);
	}

	A->Playing				=bPlaying;
	A->PrecipitationOptions			=(VelocityStreaks?(uint8_t)PrecipitationOptionsEnum::VelocityStreaks:0)
		|(SimulationTime?(uint8_t)PrecipitationOptionsEnum::SimulationTime:0)
		|(!bPlaying?(uint8_t)PrecipitationOptionsEnum::Paused:0);
	A->CloudThresholdDistanceKm		=CloudThresholdDistanceKm;
	A->MaximumResolution			=MaximumResolution;
	A->DepthSamplingPixelRange		=DepthSamplingPixelRange;
	A->DepthTemporalAlpha			=DepthTemporalAlpha;
	A->CloudShadowTextureSize		=ShadowTextureSize;
	A->Amortization					=Amortization;
	A->AtmosphericsAmortization		=AtmosphericsAmortization;
	A->MoonTexture					=GetTextureRHIRef(MoonTexture);
	A->CosmicBackgroundTexture		=GetTextureRHIRef(CosmicBackgroundTexture);
	A->LossRT						=GetTextureRHIRef(LossRT);
	A->CloudVisibilityRT			=GetTextureRHIRef(CloudVisibilityRT);
	A->OverlayRT					=GetTextureRHIRef(OverlayRT);
	A->TranslucentRT				=GetTextureRHIRef(TranslucentRT);
	A->CloudShadowRT				=GetTextureRHIRef(CloudShadowRT);
	A->AltitudeLightRT				=GetTextureRHIRef(AltitudeLightRT);
	A->InscatterRT					=GetTextureRHIRef(InscatterRT);
	A->RainCubemap					=GetTextureRHIRef(RainCubemap);

	if(MoonsChecksum!=A->MoonsChecksum)
	{
		A->Moons.Empty(Moons.Num());
		for(int i=0;i<Moons.Num();i++)
		{
			FMoon &moon=Moons[i];
			// use id=index+1
			auto &emp=A->Moons.FindOrAdd(i+1);
			emp.Name=moon.Name;
			emp.orbit={moon.LongitudeOfAscendingNode
						,moon.LongitudeOfAscendingNodeRate
						,moon.Inclination
						,moon.ArgumentOfPericentre
						,moon.ArgumentOfPericentreRate
						,moon.MeanDistance
						,moon.Eccentricity
						,moon.MeanAnomaly
						,moon.MeanAnomalyRate};
			emp.RadiusArcMinutes=moon.RadiusArcMinutes;
			emp.Texture=GetTextureRHIRef(moon.Texture);
		}
		A->MoonsChecksum=MoonsChecksum;
	}
	if(A->GetTime)
	{
		Time=A->Time;
		A->GetTime=false;
	}
	else if(Time!=A->Time||bSetTime)
		if(ControlTimeThroughActor)
		{
			A->Time							=Time;
			A->SetTime						=true;
		}
	A->MaxSunRadiance				=MaxSunRadiance;
	A->AdjustSunRadius				=AdjustSunRadius;
	// Note: This seems to be necessary, it's not clear why:
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	A->Transform					=GetTransform();
	A->DepthBlending				=DepthBlending;
	A->MinimumStarPixelSize			=MinimumStarPixelSize;
	A->GodraysGrid[0]				=CrepuscularGrid[0];
	A->GodraysGrid[1]				=CrepuscularGrid[1];
	A->GodraysGrid[2]				=CrepuscularGrid[2];
	A->TrueSkyLightUnits			=(uint8_t)TrueSkyLightUnits;
	A->SkyMultiplier				=SkyMultiplier;
	A->WorleyNoiseTextureSize		=CellNoiseTextureSize;
	A->DefaultCloudSteps			=DefaultCloudSteps;

	SetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_Render_WindSpeedMS,WindSpeedMS);
	FVector latlongdeg(OriginLatitude, OriginLongitude, OriginHeading);
	SetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_Render_OriginLatLongHeadingDeg, latlongdeg);
	for(int i=0;i<4;i++)
	{
		simul::LightningBolt *L=&(A->lightningBolts[i]);
		if(L&&L->id!=0&&L->id!=latest_thunderbolt_id)
		{
			FVector pos(L->endpos[0],L->endpos[1],L->endpos[2]);
			float magnitude=L->brightness;
			FVector colour(L->colour[0],L->colour[1],L->colour[2]);
			latest_thunderbolt_id=L->id;
			if(ThunderSounds.Num())
			{
				for( FConstPlayerControllerIterator Iterator = world->GetPlayerControllerIterator(); Iterator; ++Iterator )
				{
					FVector listenPos,frontDir,rightDir;
					Iterator->Get()->GetAudioListenerPosition(listenPos,frontDir,rightDir);
					FVector offset		=listenPos-pos;
				
					FTimerHandle UnusedHandle;
					float dist			=offset.Size()*A->MetresPerUnit;
					static float vsound	=3400.29f;
					float delaySeconds	=dist/vsound;
					FTimerDelegate soundDelegate = FTimerDelegate::CreateUObject( this, &ATrueSkySequenceActor::PlayThunder, pos );
					GetWorldTimerManager().SetTimer(UnusedHandle,soundDelegate,delaySeconds,false);
				}
			}
		}
	}
}

void ATrueSkySequenceActor::PlayThunder(FVector pos)
{
	USoundBase *ThunderSound=ThunderSounds[FMath::Rand()%ThunderSounds.Num()];
	if(ThunderSound)
		UGameplayStatics::PlaySoundAtLocation(this,ThunderSound,pos,FMath::RandRange(0.5f,1.5f),FMath::RandRange(0.7f,1.3f),0.0f,ThunderAttenuation);
}


float ATrueSkySequenceActor::GetMetresPerUnit()
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	return A->MetresPerUnit;
}

bool ATrueSkySequenceActor::IsActiveActor()
{
	UWorld *world = GetWorld();
	if(HasAnyFlags(RF_Transient|RF_ClassDefaultObject|RF_ArchetypeObject))
		return false;
	// WorldType is often not yet initialized here, even though we're calling from PostLoad.
	if (!world||world->WorldType == EWorldType::Editor||world->WorldType==EWorldType::Inactive)
		return ActiveInEditor;
	
	return true;
}

bool ATrueSkySequenceActor::IsAnyActorActive() 
{
	if(!GetOuter())
		return false;
	ULevel* Level=Cast<ULevel>(GetOuter());
	if(!Level)
		return false;
	if(Level->OwningWorld == nullptr)
		return false;
	
	return true;
}

void ATrueSkySequenceActor::Remove()
{
	//This function only needs to be called once, but Destroyed() and the destuctor may both be called.
	if(isRemoved)
	{
		return;
	}
	isRemoved = true;

	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
	if(A)
	{
		actors--;
		if(actors <= 1)
		{
			A->Destroyed = true;
		}

		//Hide trueSKY, if this function was called from the world changing, and there is no longer a sequence actor assigned to the world.
		//Unreal seems to call the destructor when a level is deleted, even if the level was never loaded, so the crossthread check is necessary.
		if(A->Destroyed)
		{
			A->Visible = false;
			A->activeSequence = nullptr;
		}
	}
}

FBox ATrueSkySequenceActor::GetComponentsBoundingBox(bool bNonColliding) const
{
	FBox Box(EForceInit::ForceInit);

	for (const UActorComponent* ActorComponent : GetComponents())
	{
		const UBoxComponent* PrimComp = Cast<const UBoxComponent>(ActorComponent);
		if (PrimComp)
		{
			// Only use collidable components to find collision bounding box.
			if (PrimComp->IsRegistered() && (bNonColliding || PrimComp->IsCollisionEnabled()))
			{
				Box += PrimComp->Bounds.GetBox();
			}
		}
	}

	return Box;
}

void ATrueSkySequenceActor::FillMaterialParameters( )
{
	if (!ITrueSkyPlugin::IsAvailable()||ITrueSkyPlugin::Get().GetVersion()==ToSimulVersion(0,0,0))
		return;
	FVector CloudShadowOriginKm(0, 0, 0);
	FVector CloudShadowScaleKm(0, 0, 0);
	FVector CloudShadowOffset(0, 0, 0);
	FVector SunlightTableTransform(0, 0, 0);

	// Warning: don't delete the trueSkyMaterialParameterCollection in-game. But why would you ever do that?
	if(!TrueSkyMaterialParameterCollection)
	{
		TrueSkyMaterialParameterCollection = LoadObject< UMaterialParameterCollection >( nullptr, *( MaterialParameterCollectionAssetRef.ToString( ) ) );
	}

	if( !IsValid( TrueSkyMaterialParameterCollection ) )
	{
		UE_LOG( TrueSky, Error, TEXT( "[trueSKY] ERROR: Material parameter collection was unable to load." ) );
		return;
	}

	if( TrueSkyPropertyFloatMaps.Num( ) )
	{
		static float mix = 0.05;
		ActorCrossThreadProperties *A = GetActorCrossThreadProperties( );

		float MaxAtmosphericDistanceKm = GetSkyLayerFloat( ETrueSkySkySequenceLayerPropertiesFloat::TSPROPFLOAT_MaxDistanceKm );
		CloudShadowOriginKm = GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_CloudShadowOriginKm);
		CloudShadowScaleKm = GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_CloudShadowScaleKm);
		SunlightTableTransform = GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_SunlightTableTransform);
		FVector CloudShadowScale;
		FVector CloudShadowOrigin;
		if (bSimulVersion4_2)
		{
			CloudShadowOrigin = TrueSkyToUEPosition(A->Transform, A->MetresPerUnit, CloudShadowOriginKm);
			CloudShadowScale = (1000.0f * (CloudShadowRangeKm / 80.0)) / A->MetresPerUnit*TrueSkyToUEDirection(A->Transform, CloudShadowScaleKm);

			FVector sunDir = GetVectorProperty(ETrueSkyPropertiesVector::TSPROPVECTOR_SunDirection);
			float sampleHeight = GetFloatProperty(ETrueSkyPropertiesFloat::TSPROPFLOAT_CloudShadowHeight);

			FVector2D normalisedSunDir = FVector2D(sunDir.X, sunDir.Y);
			normalisedSunDir.Normalize();

			float tempLength = sqrt(pow(sunDir.X, 2) + pow(sunDir.Y, 2));
			float angle = tempLength / sunDir.Z;

			float groundLength = sampleHeight * angle * (4.0 / CloudShadowRangeKm);

			CloudShadowOffset.X = groundLength * normalisedSunDir.X;
			CloudShadowOffset.Y = groundLength * normalisedSunDir.Y;

		}
		else
		{
			CloudShadowOrigin = TrueSkyToUEPosition(A->Transform, A->MetresPerUnit, (1000.0f * ((CloudShadowScaleKm/2.0) + CloudShadowOriginKm)));
			CloudShadowScale = (1000.0f / 8.f) / A->MetresPerUnit*TrueSkyToUEDirection(A->Transform, CloudShadowScaleKm);
		}
		// X is km. convert to units by multiplying by 1000 and dividing by metresPerUnit
		SunlightTableTransform *= 1000.0f/A->MetresPerUnit;
		// Y unitless. The formula is (y+alt*x)=texc;
		/*FVector pos = A->Transform.GetTranslation();
		wetness *= 1.0 - mix;
		wetness += mix*GetFloatPropertyAtPosition( ETrueSkyPositionalPropertiesFloat::TSPROPFLOAT_Precipitation, pos, 9183759 );*/

		UKismetMaterialLibrary::SetScalarParameterValue( this, TrueSkyMaterialParameterCollection, kMPC_MaxAtmosphericDistanceName, MaxAtmosphericDistanceKm*1000.0f/A->MetresPerUnit );
		UKismetMaterialLibrary::SetVectorParameterValue( this, TrueSkyMaterialParameterCollection, kMPC_CloudShadowOrigin, CloudShadowOrigin );
		UKismetMaterialLibrary::SetVectorParameterValue( this, TrueSkyMaterialParameterCollection, kMPC_CloudShadowScale, CloudShadowScale );
		UKismetMaterialLibrary::SetVectorParameterValue( this, TrueSkyMaterialParameterCollection, kMPC_CloudShadowOffset, CloudShadowOffset );
		//UKismetMaterialLibrary::SetScalarParameterValue( this, TrueSkyMaterialParameterCollection, kMPC_LocalWetness, wetness );
		UKismetMaterialLibrary::SetScalarParameterValue( this, TrueSkyMaterialParameterCollection, kMPC_ShadowStrength, ShadowStrength ); 
		UKismetMaterialLibrary::SetVectorParameterValue( this, TrueSkyMaterialParameterCollection, kMPC_SunlightTableTransform, SunlightTableTransform );
		
	}
}

void ATrueSkySequenceActor::TickActor(float DeltaTime,enum ELevelTick TickType,FActorTickFunction& ThisTickFunction)
{
	if(initializeDefaults)
	{
		SetDefaultTextures();
		ApplyRenderingOptions();
		initializeDefaults=false;
	}
	CleanQueries();
	
	if( IsValid( Light ) )
	{
		/*if(!DriveLightDirection)
			SetSunRotation(DirectionalLight->GetTransform().Rotator());*/
		
#if SIMUL_ALTITUDE_DIRECTIONAL_LIGHT
			if (DirectionalLight->bColorLightFunction != AltitudeLighting)
			{
				DirectionalLight->bColorLightFunction = AltitudeLighting;
				DirectionalLight->InvalidateLightingCache();
			}
#endif
			UpdateLight((ADirectionalLight*)Light,SunMultiplier,MoonMultiplier,false,DriveLightDirection, AltitudeLighting);
	}
	UWorld *world = GetWorld();
	FillMaterialParameters();
	if(RainMaskSceneCapture2D)
	{
		ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
		static bool cc=true;
		USceneCaptureComponent2D *sc=(USceneCaptureComponent2D*)RainMaskSceneCapture2D->GetCaptureComponent2D();
		if(sc->IsActive())
		{
			sc->bUseCustomProjectionMatrix=cc;
			float ZHeight = RainDepthExtent;
			sc->OrthoWidth=RainMaskWidth;
			FMatrix proj;
			BuildProjectionMatrix(sc->OrthoWidth, ZHeight,proj);
			FMatrix ViewMat=sc->GetComponentToWorld().ToInverseMatrixWithScale();
			CaptureComponentToViewMatrix(ViewMat,MetresPerUnit,A->Transform.ToMatrixWithScale());
			sc->CustomProjectionMatrix=proj;
			AdaptOrthoProjectionMatrix(proj,MetresPerUnit);
			// Problem, UE doesn't fill in depth according to the projection matrix, it does it in cm! Or Unreal Units.
			A->RainDepthTextureScale=1.0f/ZHeight;
			A->RainDepthMatrix	=ViewMat*proj;
			A->RainProjMatrix	=proj;
			A->RainDepthRT		=GetTextureRHIRef(sc->TextureTarget);
		}
	}
	
	// We DO NOT accept ticks from actors in Editor worlds.
	// For some reason, actors start ticking when you Play in Editor, the ones in the Editor and well as the ones in the PIE world.
	// This results in duplicate actors competing.
	if (!world || world->WorldType == EWorldType::Editor)
	{
		ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
		// But we do transfer the properties.
		if(A&&A->Playing || ControlTimeThroughActor)
		{
			A->Playing=bPlaying=false;
			UpdateTime();
			TransferProperties();
		}
		return;
	}
	// Find out which trueSKY actor should be active. We should only do this once per frame, so this is inefficient at present.
	
	if (CurrentActiveActor == nullptr)// || worldCurrentActiveSequenceActors.find(world) != worldCurrentActiveSequenceActors.end())
	{
		for (TActorIterator<ATrueSkySequenceActor> Itr(world); Itr; ++Itr)
		{
			CurrentActiveActor = *Itr;
			break;
		}
		//worldCurrentActiveSequenceActors[world] = CurrentActiveActor;
	}

	if (!IsActiveActor())
	{
		if(!IsAnyActorActive())
		{
			ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
			if(A)
			{
				A->Playing					=false;
				A->Visible					=false;
				A->activeSequence			=nullptr;
			}
		}
	}
	bPlaying=!world->IsPaused();
	UpdateTime();
	TransferProperties();
	UpdateVolumes();
}

#if WITH_EDITOR

bool ATrueSkySequenceActor::CanEditChange(const UProperty* InProperty)const
{
	if( InProperty )
	{
		FString PropertyName = InProperty->GetName( );
		if( !ITrueSkyPlugin::Get( ).GetWaterRenderingEnabled( ) )
		{
			if( PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, RenderWater ) ||
				PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, WaterResolution) ||
				PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, EnableReflection) ||
				PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, ReflectionResolution) ||
				PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, ReflectionSteps) ||
				PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, ReflectionPixelStep))
				return false;
		}
		if (!RenderWater && (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, WaterResolution) ||
			PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, EnableReflection) ||
			PropertyName == GET_MEMBER_NAME_STRING_CHECKED(ATrueSkySequenceActor, ReflectionResolution)))
			return false;
	}
	
	return( Super::CanEditChange(InProperty) );
}

void ATrueSkySequenceActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if(RootComponent)
		RootComponent->UpdateComponentToWorld();
	MoonsChecksum++;
	UpdateEnabledProperties();
	ApplyRenderingOptions();
	TransferProperties();
	UpdateVolumes();
	// If this is the active Actor, deactivate the others:
	if (ActiveInEditor)
	{
		UWorld *world = GetWorld();
		if (!world||world->WorldType!= EWorldType::Editor)
		{
			return;
		}
		for (TActorIterator<ATrueSkySequenceActor> Itr(world); Itr; ++Itr)
		{
			ATrueSkySequenceActor* SequenceActor = *Itr;
			if (SequenceActor == this)
				continue;
			SequenceActor->ActiveInEditor = false;
		}
		// Force instant, not gradual change, if the change was from editing:
		GetTrueSkyPlugin()->TriggerAction("Reset");
	}
}

void ATrueSkySequenceActor::EditorApplyTranslation(const FVector & DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	Super::EditorApplyTranslation(DeltaTranslation, bAltDown, bShiftDown, bCtrlDown);
	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
	A->alteredPosition = true;
	TransferProperties();
}
#endif

// Cached Modules
ITrueSkyPlugin *ATrueSkySequenceActor::CachedTrueSkyPlugin = nullptr;

ITrueSkyPlugin *ATrueSkySequenceActor::GetTrueSkyPlugin()
{
	if (!ATrueSkySequenceActor::CachedTrueSkyPlugin)
	{
		ATrueSkySequenceActor::CachedTrueSkyPlugin = &ITrueSkyPlugin::Get();
	}
	
	return ATrueSkySequenceActor::CachedTrueSkyPlugin;
}

void ATrueSkySequenceActor::PopulatePropertyFloatMap( UEnum* EnumerationType, TrueSkyPropertyFloatMap& EnumerationMap)
{
	// Enumerate through all of the values.
	for( int i = 0; i < EnumerationType->GetMaxEnumValue( ); ++i )
	{
		FString prefix;
		FString keyName = EnumerationType->GetNameByIndex( i ).ToString( );

		TArray< FString > parsedKeyName;
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check( ParsedElements > 0 );
		if(ParsedElements>2)
			prefix=parsedKeyName[ParsedElements-2].ToLower()+":";
		FString nameString=prefix+parsedKeyName[ParsedElements-1];

		int64 enum_int64=GetTrueSkyPlugin()->GetEnum(nameString);
#ifdef UE_LOG_SIMUL_INTERNAL
		if( !enum_int64 )
			UE_LOG_SIMUL_INTERNAL( TrueSky, Warning, TEXT( "No enum for Float: %s"), *nameString );
#endif	// UE_LOG_SIMUL_INTERNAL
		
		// The property name is the final key.
		EnumerationMap.Add( EnumerationType->GetValueByIndex( i ), TrueSkyPropertyFloatData( FName( *nameString ), 0.0f, 1.0f, enum_int64 ) );
	}
}

void ATrueSkySequenceActor::PopulatePropertyIntMap( UEnum* EnumerationType, TrueSkyPropertyIntMap& EnumerationMap )
{
	// Enumerate through all of the values.
	for( int i = 0; i < EnumerationType->GetMaxEnumValue( ); ++i )
	{
		FString prefix;
		FString keyName = EnumerationType->GetNameByIndex( i ).ToString( );

		TArray< FString > parsedKeyName;
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check( ParsedElements > 0 );
		if(ParsedElements>2)
			prefix=parsedKeyName[ParsedElements-2].ToLower()+":";
		FString nameString=prefix+parsedKeyName[ParsedElements-1];
		// UL End

		int64 enum_int64=GetTrueSkyPlugin()->GetEnum(nameString);
#ifdef UE_LOG_SIMUL_INTERNAL
		if( !enum_int64 )
			UE_LOG_SIMUL_INTERNAL( TrueSky, Warning, TEXT( "No enum for Int: %s" ), *nameString );
#endif	// UE_LOG_SIMUL_INTERNAL

//		UE_LOG( TrueSky, Warning, TEXT( "Registering: %s"), *parsedKeyName[2] );

		// The property name is the final key.
		EnumerationMap.Add( EnumerationType->GetValueByIndex( i ), TrueSkyPropertyIntData( FName( *nameString),enum_int64 ) );
	}
}

void ATrueSkySequenceActor::PopulatePropertyVectorMap( UEnum* EnumerationType, TrueSkyPropertyVectorMap& EnumerationMap )
{
	// Enumerate through all of the values.
	for( int i = 0; i < EnumerationType->GetMaxEnumValue( ); ++i )
	{
		FString keyName = EnumerationType->GetNameByIndex( i ).ToString( );

		TArray< FString > parsedKeyName;
		// UL Begin - @jeff.sult: fix crash in shipping build
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check( ParsedElements > 0 );
		FString prefix;
		if(ParsedElements>2)
			prefix=parsedKeyName[ParsedElements-2].ToLower()+":";
		FString nameString=prefix+parsedKeyName[ParsedElements-1];
		// UL End

		int64 enum_int64 = ITrueSkyPlugin::Get( ).GetEnum( nameString );
#ifdef UE_LOG_SIMUL_INTERNAL
		if( !enum_int64 )
			UE_LOG_SIMUL_INTERNAL( TrueSky, Warning, TEXT( "No enum for Vector: %s" ), *nameString );
#endif	// UE_LOG_SIMUL_INTERNAL

		//		UE_LOG( TrueSky, Warning, TEXT( "Registering: %s"), *parsedKeyName[2] );

		// The property name is the final key.
		if(enum_int64!=0)
			EnumerationMap.Add( EnumerationType->GetValueByIndex( i ), TrueSkyPropertyVectorData( FName( *nameString), enum_int64 ) );
	}
}

void ATrueSkySequenceActor::InitializeTrueSkyPropertyMap( )
{
	if(TrueSkyPropertyFloatMaps.Num())
		return;
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	A->CriticalSection.Lock();
	TrueSkyPropertyFloatMaps.Empty( );
	TrueSkyPropertyIntMaps.Empty( );
	TrueSkyPropertyVectorMaps.Empty( );

	// Setup the enumerations.
	TArray< FName > enumerationNames = {
		TEXT( "ETrueSkySkySequenceLayerPropertiesFloat" ),
		TEXT( "ETrueSkySkySequenceKeyframePropertiesFloat" ),
		TEXT( "ETrueSkyCloudSequenceLayerPropertiesFloat" ),
		TEXT( "ETrueSkyCloudSequenceKeyframePropertiesFloat" ),
		TEXT( "ETrueSkySkySequenceLayerPropertiesInt" ),
		TEXT( "ETrueSkySkySequenceKeyframePropertiesInt" ),
		TEXT( "ETrueSkyCloudSequenceLayerPropertiesInt" ),
		TEXT( "ETrueSkyCloudSequenceKeyframePropertiesInt" ),
		TEXT( "ETrueSkyPropertiesFloat" ),
		TEXT( "ETrueSkyPropertiesInt" ),
		TEXT( "ETrueSkyPositionalPropertiesFloat" ),
		TEXT( "ETrueSkyPropertiesVector" ),
		TEXT( "ETrueSkyRenderingProperties" )
	};

	TArray< int32* > enumerationIndices = {
		&TSSkyLayerPropertiesFloatIdx,
		&TSSkyKeyframePropertiesFloatIdx,
		&TSCloudLayerPropertiesFloatIdx,
		&TSCloudKeyframePropertiesFloatIdx,
		&TSSkyLayerPropertiesIntIdx,
		&TSSkyKeyframePropertiesIntIdx,
		&TSCloudLayerPropertiesIntIdx,
		&TSCloudKeyframePropertiesIntIdx,
		&TSPropertiesFloatIdx,
		&TSPropertiesIntIdx,
		&TSPositionalPropertiesFloatIdx,
		&TSPropertiesVectorIdx,
		&TSRenderingPropertiesIdx,
	};

	// Populate the full enumeration property map.
	for( int i = 0; i < enumerationNames.Num( ); ++i )
	{
		FName enumerator = enumerationNames[i];
		FString prefix="";
		UEnum* pEnumeration = FindObject< UEnum >( ANY_PACKAGE, *enumerator.ToString( ), true );
		if(!pEnumeration)
		{
			UE_LOG(TrueSky, Display, TEXT("Enum not found: %s"), *enumerator.ToString( ));
			continue;
		}
		/*if(enumerator==TEXT( "ETrueSkySkySequenceLayerPropertiesFloat" )
			||enumerator==TEXT( "ETrueSkySkySequenceLayerPropertiesInt"))
			prefix="sky:";
		else if(enumerator==TEXT( "ETrueSkyCloudSequenceLayerPropertiesFloat" )
			||enumerator==TEXT( "ETrueSkyCloudSequenceLayerPropertiesInt"))
			prefix="clouds:";*/
		if(enumerator==TEXT( "ETrueSkyRenderingProperties"))
			prefix="render:";
		if( enumerator.ToString( ).Contains( TEXT( "Vector" ) ) )
		{
			// Float-based map population.
			int32 idx = TrueSkyPropertyVectorMaps.Add( TrueSkyPropertyVectorMap( ) );
			*enumerationIndices[i] = idx;

			PopulatePropertyVectorMap( pEnumeration, TrueSkyPropertyVectorMaps[idx]);
		}
		else if( enumerator.ToString( ).Contains( TEXT( "Int" ) ) )
		{
			// Int-based map population.
			int32 idx = TrueSkyPropertyIntMaps.Add( TrueSkyPropertyIntMap( ) );
			*enumerationIndices[i] = idx;

			PopulatePropertyIntMap( pEnumeration, TrueSkyPropertyIntMaps[idx]);
		}
		else
		{
			// Variant-based map population.
			int32 idx = TrueSkyPropertyFloatMaps.Add( TrueSkyPropertyFloatMap( ) );
			*enumerationIndices[i] = idx;
			PopulatePropertyFloatMap( pEnumeration, TrueSkyPropertyFloatMaps[idx]);
		}
	}
	auto p=ATrueSkySequenceActor::TrueSkyPropertyFloatMaps[ATrueSkySequenceActor::TSPropertiesFloatIdx].Find( ( int64 )ETrueSkyPropertiesFloat::TSPROPFLOAT_DepthSamplingPixelRange );

	A->CriticalSection.Unlock();
}

void ATrueSkySequenceActor::RenderToCubemap(UTextureRenderTargetCube *RenderTargetCube, int32 cubemap_resolution)
{
	ActorCrossThreadProperties *A	=GetActorCrossThreadProperties();
	if(A)
	{
		A->CaptureCubemapRT=GetTextureRHIRef(RenderTargetCube);
		A->CaptureCubemapResolution = FMath::Clamp(cubemap_resolution, 64, 2048);
	}
}

static void ShowTrueSkyMemory(const TArray<FString>& Args, UWorld* World)
{
	FString str = ATrueSkySequenceActor::GetText("memory");
	GEngine->AddOnScreenDebugMessage(1, 5.f, FColor(255, 255, 0, 255), str);
	UE_LOG(TrueSky, Display, TEXT("%s"), *str);
}

static FAutoConsoleCommandWithWorldAndArgs GTrueSkyMemory(
	TEXT("r.TrueSky.Memory"),
	TEXT("Show how much GPU memory trueSKY is using."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowTrueSkyMemory)
);

static void ShowCloudCrossSections(const TArray<FString>& Args, UWorld* World)
{
	bool value = false;
	if(Args.Num()>0)
		value=(FCString::Atoi(*Args[0]))!=0;
	ATrueSkySequenceActor::SetBool("ShowCloudCrossSections",value);
}

static FAutoConsoleCommandWithWorldAndArgs GTrueSkyCrossSections(
	TEXT("r.TrueSky.CloudCrossSections"),
	TEXT("Show the cross-sections of the cloud volumes."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowCloudCrossSections)
);


static void ShowCompositing(const TArray<FString>& Args, UWorld* World)
{
	bool value = false;
	if (Args.Num()>0)
		value = (FCString::Atoi(*Args[0])) != 0;
	ATrueSkySequenceActor::SetBool("ShowCompositing", value);
}

static FAutoConsoleCommandWithWorldAndArgs GTrueSkyCompositing(
	TEXT("r.TrueSky.Compositing"),
	TEXT("Show the compositing overlay for trueSKY."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowCompositing)
);

static void ShowProfiling(const TArray<FString>& Args, UWorld* World)
{
	bool value = false;
	if (Args.Num()>0)
		value = (FCString::Atoi(*Args[0])) != 0;
	ATrueSkySequenceActor::SetBool("onscreenprofiling", value);
}

static FAutoConsoleCommandWithWorldAndArgs GTrueSkyProfiling(
	TEXT("r.TrueSky.Profiling"),
	TEXT("Show the profiling for trueSKY."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowProfiling)
);

static void SetDiagnosticMode(const TArray<FString>& Args, UWorld* World)
{
	bool value = false;
	if (Args.Num()>0)
		value = (FCString::Atoi(*Args[0])) != 0;
	ATrueSkySequenceActor::SetBool("diagnosticmode", value);
}

static FAutoConsoleCommandWithWorldAndArgs GDiagnosticMode(
	TEXT("r.TrueSky.DiagnosticMode"),
	TEXT("Activate output diagnostic debug messages for trueSKY."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetDiagnosticMode)
);

#define TRUESKY_TOGGLE(name)\
static void name(const TArray<FString>& Args, UWorld* World)\
{\
	bool value = false;\
	if (Args.Num()>0)\
		value = (FCString::Atoi(*Args[0])) != 0;\
	ITrueSkyPlugin::Get().Set##name ## Enabled(value);\
}\
\
static FAutoConsoleCommandWithWorldAndArgs GTrueSky##name(\
	TEXT("r.TrueSky."#name),\
	TEXT("Toggle "#name" for trueSKY."),\
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&name)\
);\


TRUESKY_TOGGLE(PostOpaque)
TRUESKY_TOGGLE(PostTranslucent)
TRUESKY_TOGGLE(Overlays)