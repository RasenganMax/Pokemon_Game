// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkyWaterActor.h"

#include "TrueSkyPluginPrivatePCH.h"

#include "UObject/ConstructorHelpers.h"
#include "Model.h"
#include "Containers/Array.h"
#include "EngineUtils.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetCube.h"
#include "Engine/Light.h"

#include "GameFramework/Actor.h"

#include "Math/Vector.h"
#include "Math/TransformNonVectorized.h"

#include "Components/BrushComponent.h"
#include "Components/SceneCaptureComponent2D.h"

#include "WaterActorCrossThreadProperties.h"
#include "ActorCrossThreadProperties.h"

#include <map>
#include <atomic>

using namespace simul;

int32 ATrueSkyWater::TSWaterPropertiesIntIdx = 6;
int32 ATrueSkyWater::TSWaterPropertiesFloatIdx = 7;
int32 ATrueSkyWater::TSWaterPropertiesVectorIdx = 8;

TArray< ATrueSkyWater::TrueSkyWaterPropertyIntMap > ATrueSkyWater::TrueSkyWaterPropertyIntMaps;
TArray< ATrueSkyWater::TrueSkyWaterPropertyFloatMap > ATrueSkyWater::TrueSkyWaterPropertyFloatMaps;
TArray< ATrueSkyWater::TrueSkyWaterPropertyVectorMap > ATrueSkyWater::TrueSkyWaterPropertyVectorMaps;


// Adapted from the function in SceneCaptureRendering.cpp.
extern void BuildProjectionMatrix(float InOrthoWidth, float FarPlane, FMatrix &ProjectionMatrix);

extern void AdaptOrthoProjectionMatrix(FMatrix &projMatrix, float metresPerUnit);

extern void CaptureComponentToViewMatrix(FMatrix &viewMatrix, float metresPerUnit, const FMatrix &worldToSkyMatrix);


using namespace simul;
using namespace unreal;

static std::atomic<int> IDCount(0);

static std::map<UWorld*, ATrueSkyWater *> worldCurrentActiveWaterActors;

ATrueSkyWater::ATrueSkyWater(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Render(true)
	, boundlessOcean(false)
	, BeaufortScale(3.0)
	, WindDirection(0.0)
	, WindDependency(0.95)
	, Scattering(0.17, 0.2, 0.234)
	, Absorption(0.2916, 0.0474, 0.0092)
	, ProfileBufferResolution(2048)
	, AdvancedSurfaceOptions(false)
	, WindSpeed(5.0)
	, WaveAmplitude(1.0)
	, MaxWavelength(50.0)
	, MinWavelength(0.06)
	, EnableWaveGrid(false)
	, WaveGridDistance(128)
	, EnableFoam(true)
	, FoamStrength(0.4f)
	, EnableShoreEffects(false)
	, ShoreDepthSceneCapture2D(nullptr)
	, ShoreDepthWidth(3000.f)
	, ShoreDepthExtent(2500.f)
{
	RootComponent = GetBrushComponent();

	//RootComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	//RootComponent->Mobility = EComponentMobility::Movable;

	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetTickGroup(TG_DuringPhysics);
	SetActorTickEnabled(true);

	bColored = true;
	BrushColor.R = 100;
	BrushColor.G = 100;
	BrushColor.B = 255;
	BrushColor.A = 255;
}

ATrueSkyWater::~ATrueSkyWater()
{
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	if(A)
	{
		if(A->BoundedObjectCreated)
		ITrueSkyPlugin::Get().RemoveBoundedWaterObject(ID);
		A->Destroyed = true;
		A->BoundlessOcean = false;
	}
	//RemoveWaterActorCrossThreadProperties(ID);

	for (auto i = worldCurrentActiveWaterActors.begin(); i != worldCurrentActiveWaterActors.end(); i++)
	{
		if (i->second == this)
		{
			worldCurrentActiveWaterActors.erase(i);
			break;
		}
	}
}

#define SET_WATER_FLOAT(name) SetWaterFloat(ETrueSkyWaterPropertiesFloat::TSPROPFLOAT_Water_##name,name);
#define SET_WATER_INT(name) SetWaterInt(ETrueSkyWaterPropertiesInt::TSPROPINT_Water_##name,name);
#define SET_WATER_VECTOR(name) SetWaterVector(ETrueSkyWaterPropertiesVector::TSPROPVECTOR_Water_##name,name);

void ATrueSkyWater::ApplyRenderingOptions()
{
	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
	if (A->RenderWater)
	{
		SET_WATER_FLOAT(WindDirection)
		SET_WATER_FLOAT(WindDependency)

		SET_WATER_INT(Render)
		SET_WATER_INT(ProfileBufferResolution)
		SET_WATER_INT(EnableWaveGrid)
		SET_WATER_INT(WaveGridDistance)

		//SET_WATER_VECTOR(Location)
		SET_WATER_VECTOR(Scattering)
		SET_WATER_VECTOR(Absorption)

		//if (ShoreDepthSceneCapture2D)
		//	SET_WATER_VECTOR(ShoreDepthTextureLocation)

		if (!boundlessOcean)
		{
			SET_WATER_VECTOR(Dimension)
			SET_WATER_FLOAT(Rotation)
		}

		if (!AdvancedSurfaceOptions)
			SET_WATER_FLOAT(BeaufortScale)
		else
		{
			SET_WATER_FLOAT(WindSpeed)
			SET_WATER_FLOAT(WaveAmplitude)
			SET_WATER_FLOAT(MaxWavelength)
			SET_WATER_FLOAT(MinWavelength)
		}
	}
}

void ATrueSkyWater::SetWaterFloat(ETrueSkyWaterPropertiesFloat Property, float Value)
{
	if (TrueSkyWaterPropertyFloatMaps.Num() != 0)
	{
		auto p = TrueSkyWaterPropertyFloatMaps[0].Find((int64)Property);
		if (p && p->TrueSkyEnum > 0)
		{
			WaterActorCrossThreadProperties* A = GetWaterActorCrossThreadProperties(ID);
			if (A && A->SetWaterUpdateValues.Num() < 100)
			{
				TPair<TrueSkyEnum, VariantPass> t;
				t.Key = p->TrueSkyEnum;
				t.Value.floatVal = Value;
				//A->CriticalSection.Lock();
				A->SetWaterUpdateValues.Add(t);
				A->Updated = true;
				//A->CriticalSection.Unlock();
			}
		}
	}
}

void ATrueSkyWater::SetWaterInt(ETrueSkyWaterPropertiesInt Property, int32 Value)
{
	if (TrueSkyWaterPropertyIntMaps.Num() != 0)
	{
		auto p = TrueSkyWaterPropertyIntMaps[0].Find((int64)Property);
		if (p && p->TrueSkyEnum > 0)
		{
			WaterActorCrossThreadProperties* A = GetWaterActorCrossThreadProperties(ID);
			if (A && A->SetWaterUpdateValues.Num() < 100)
			{
				TPair<TrueSkyEnum, VariantPass> t;
				t.Key = p->TrueSkyEnum;
				t.Value.intVal = Value;
				//A->CriticalSection.Lock();
				A->SetWaterUpdateValues.Add(t);
				A->Updated = true;
				//A->CriticalSection.Unlock();
			}
		}
	}
}

void ATrueSkyWater::SetWaterVector(ETrueSkyWaterPropertiesVector Property, FVector Value)
{
	if (TrueSkyWaterPropertyVectorMaps.Num() != 0)
	{
		auto p = TrueSkyWaterPropertyVectorMaps[0].Find((int64)Property);
		if (p && p->TrueSkyEnum > 0)
		{
			WaterActorCrossThreadProperties* A = GetWaterActorCrossThreadProperties(ID);
			if (A && A->SetWaterUpdateVectors.Num() < 100)
			{
				TPair<TrueSkyEnum, vec3> t;
				t.Key = p->TrueSkyEnum;
				t.Value.x = Value.X;
				t.Value.y = Value.Y;
				t.Value.z = Value.Z;
				//A->CriticalSection.Lock();
				A->SetWaterUpdateVectors.Add(t);
				A->Updated = true;
				//A->CriticalSection.Unlock();
			}
		}
	}
}

void ATrueSkyWater::PostRegisterAllComponents()
{
	if(!initialised)
	{
		IDCount++;
		ID = IDCount;
		ActorCrossThreadProperties *S = GetActorCrossThreadProperties();
		WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
		if(A == NULL)
		{
			A = new WaterActorCrossThreadProperties;
			A->ID = ID;
			AddWaterActorCrossThreadProperties(A);
		}

		Render = true;
		initialised = true;
		Dimension = FVector(200.f, 200.f, 200.f) * GetActorScale();
		Location = GetActorLocation();
	}

	UWorld *world = GetWorld();
	// If any other Water Actor is already a boundless ocean, make this new one Inactive.
	if(boundlessOcean && world != nullptr && world->WorldType == EWorldType::Editor)
	{
		for(TActorIterator<ATrueSkyWater> Itr(world); Itr; ++Itr)
		{
			ATrueSkyWater* WaterActor = *Itr;
			if(WaterActor == this)
			{
				continue;
			}
			if(WaterActor->boundlessOcean)
			{
				boundlessOcean = false;
				break;
			}
		}
	}

	if(world->WorldType != EWorldType::Inactive)
	{
		TransferProperties();
		if(ITrueSkyPlugin::Get().GetWaterRenderingEnabled())
		{
			ApplyRenderingOptions();
		}
	}
}

void ATrueSkyWater::PostInitProperties()
{
	Super::PostInitProperties();

	if (RootComponent)
	{
		//RootComponent->SetWorldLocation(GetActorLocation());
		RootComponent->UpdateComponentToWorld();
	}

	ITrueSkyPlugin::EnsureLoaded();
	// TODO: don't create for reference object
	//if(!A->BoundedObjectCreated)
		//ITrueSkyPlugin::Get().CreateBoundedWaterObject(ID, (float*)&A->Dimension, (float*)&A->Location);
}

void ATrueSkyWater::PostLoad()
{
	Super::PostLoad();
	if (RootComponent)
	{
		//RootComponent->SetWorldLocation(GetActorLocation());
		RootComponent->UpdateComponentToWorld();
	}
}

void ATrueSkyWater::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (RootComponent)
	{
		//RootComponent->SetWorldLocation(GetActorLocation());
		RootComponent->UpdateComponentToWorld();
	}
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	if(A)
		if (!A->BoundedObjectCreated && A->Render)
		{
			ITrueSkyPlugin::Get().CreateBoundedWaterObject(ID, (float*)&A->Dimension, (float*)&A->Location);
		}
	TransferProperties();
	if (ITrueSkyPlugin::Get().GetWaterRenderingEnabled())
		ApplyRenderingOptions();
}

void ATrueSkyWater::Destroyed()
{
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	if (!A)
		return;
	if(A->BoundedObjectCreated)
		ITrueSkyPlugin::Get().RemoveBoundedWaterObject(ID);
	A->BoundlessOcean = false;
	A->Destroyed = true;
	AActor::Destroyed();
}

void ATrueSkyWater::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//ITrueSkyPlugin::Get().TriggerAction("Reset");
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	if (A)
	{
		A->Reset = true;
		A->Destroyed = true;
		A->BoundlessOcean = false;
		if (A->BoundedObjectCreated)
			ITrueSkyPlugin::Get().RemoveBoundedWaterObject(ID);
		RemoveWaterActorCrossThreadProperties(ID);
	}
}

void ATrueSkyWater::TransferProperties()
{
	if (!ITrueSkyPlugin::IsAvailable())
		return;
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	ActorCrossThreadProperties *S = GetActorCrossThreadProperties();
	if (RootComponent)
	{
		//RootComponent->SetWorldLocation(GetActorLocation());
		RootComponent->UpdateComponentToWorld();
	}
	if (A == NULL)
	{
		A = new WaterActorCrossThreadProperties;
		A->ID = ID;
		AddWaterActorCrossThreadProperties(A);
	}

	A->Render = Render;
	A->Destroyed = false;
	A->Reset = false;
	A->Updated = true;
	A->BoundlessOcean = boundlessOcean;
	A->EnableFoam = EnableFoam;
	A->BeaufortScale = BeaufortScale;
	A->WindDirection = WindDirection;
	A->WindDependency = WindDependency;
	A->Scattering = Scattering;
	A->Absorption = Absorption;
	Dimension = FVector(200.f, 200.f, 200.f) * GetActorScale() * S->MetresPerUnit;
	Dimension = FVector(Dimension.Y, Dimension.X, Dimension.Z);
	A->Dimension = Dimension; 
	//Location = (GetActorLocation() + S->WaterOffset + FVector(0.f, 0.f, A->Dimension.Z / (2.f * S->MetresPerUnit))) * S->MetresPerUnit;
	A->Location = GetActorLocation();
	Rotation = GetActorRotation().Yaw - S->Rotation;
	A->Rotation = Rotation;
	A->AdvancedSurfaceOptions = AdvancedSurfaceOptions;
	A->WindSpeed = WindSpeed;
	A->WaveAmplitude = WaveAmplitude;
	A->MaxWavelength = MaxWavelength;
	A->MinWavelength = MinWavelength;
	A->EnableWaveGrid = EnableWaveGrid;
	A->WaveGridDistance = WaveGridDistance;
	A->ProfileResolution = ProfileBufferResolution;
	A->FoamStrength = FoamStrength;

	if (!A->BoundedObjectCreated && A->Render && !A->BoundlessOcean)
	{
		ITrueSkyPlugin::Get().CreateBoundedWaterObject(ID, (float*)&A->Dimension, (float*)&A->Location);
	}
	if (A->BoundlessOcean && A->BoundedObjectCreated)
	{
		ITrueSkyPlugin::Get().RemoveBoundedWaterObject(ID);
	}
}

void ATrueSkyWater::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	if (!ITrueSkyPlugin::Get().GetWaterRenderingEnabled())
		return;

	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
	WaterActorCrossThreadProperties *W = GetWaterActorCrossThreadProperties(ID);
	if (ShoreDepthSceneCapture2D)
	{
		static bool cc = true;
		USceneCaptureComponent2D *sc = (USceneCaptureComponent2D*)ShoreDepthSceneCapture2D->GetCaptureComponent2D();
		if (sc->IsActive() && W && A)
		{
			sc->bUseCustomProjectionMatrix = cc;
			float ZHeight = ShoreDepthExtent;
			sc->OrthoWidth = ShoreDepthWidth;
			FMatrix proj;
			BuildProjectionMatrix(sc->OrthoWidth, ZHeight, proj);
			FMatrix ViewMat = sc->GetComponentToWorld().ToInverseMatrixWithScale();
			CaptureComponentToViewMatrix(ViewMat, A->MetresPerUnit, A->Transform.ToMatrixWithScale());
			sc->CustomProjectionMatrix = proj;
			AdaptOrthoProjectionMatrix(proj, A->MetresPerUnit);
			// Problem, UE doesn't fill in depth according to the projection matrix, it does it in cm! Or Unreal Units.
			W->ShoreDepthExtent = ShoreDepthExtent;
			W->ShoreDepthWidth = ShoreDepthWidth;
			W->ShoreDepthRT = GetTextureRHIRef(sc->TextureTarget);

			if (ShoreDepthSceneCapture2D->GetActorLocation() != W->ShoreDepthTextureLocation)
				W->Updated = true;

			W->ShoreDepthTextureLocation = ShoreDepthSceneCapture2D->GetActorLocation();
			W->EnableShoreEffects = EnableShoreEffects;
		}
	}
	else
	{
		A->EnableShoreEffects = false;
	}

	if (!W || W->Reset || (W->Render && !W->BoundedObjectCreated && !W->BoundlessOcean))
	{
		TransferProperties();
		if (A->RenderWater)
			ApplyRenderingOptions();
	}
}

#if WITH_EDITOR
void ATrueSkyWater::EditorApplyTranslation(const FVector & DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	Super::EditorApplyTranslation(DeltaTranslation, bAltDown, bShiftDown, bCtrlDown);
	WaterActorCrossThreadProperties *A = GetWaterActorCrossThreadProperties(ID);
	A->Location = GetActorLocation();
	A->Updated = true;

	TransferProperties();
	if(ITrueSkyPlugin::Get().GetWaterRenderingEnabled())
		ApplyRenderingOptions();
}

void ATrueSkyWater::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (RootComponent)
	{
		//RootComponent->SetWorldLocation(GetActorLocation());
		RootComponent->UpdateComponentToWorld();
	}
	if (boundlessOcean)
	{
		UWorld *world = GetWorld();
		if (!world || world->WorldType != EWorldType::Editor)
		{
			return;
		}
		for (TActorIterator<ATrueSkyWater> Itr(world); Itr; ++Itr)
		{
			ATrueSkyWater* WaterActor = *Itr;
			if (WaterActor == this)
				continue;
			WaterActor->boundlessOcean = false;
		}
		bColored = false;
		BrushColor.R = 0;
		BrushColor.G = 0;
		BrushColor.B = 0;
		BrushColor.A = 0;
	}
	else
	{
		bColored = true;
		BrushColor.R = 100;
		BrushColor.G = 100;
		BrushColor.B = 255;
		BrushColor.A = 255;
	}
	if (UseColourPresets)
	{
		switch (ColourPresets)
		{
		case EWaterColour::Clear:
			Scattering = FVector(0.028, 0.035, 0.039);
			Absorption = FVector(0.2916, 0.0474, 0.0092);
			break;
		case EWaterColour::Ocean:
			Scattering = FVector(0.17, 0.2, 0.234);
			Absorption = FVector(0.2916, 0.0474, 0.0092);
			break;
		case EWaterColour::Muddy:
			Scattering = FVector(1.0, 1.0, 1.0);
			Absorption = FVector(0.1616, 0.1274, 0.0792);
			break;
		default:
			break;
		}
	}
	TransferProperties();
	if (ITrueSkyPlugin::Get().GetWaterRenderingEnabled())
		ApplyRenderingOptions();
}

bool ATrueSkyWater::CanEditChange(const UProperty* InProperty) const
{
	if (InProperty)
	{
		if (!ITrueSkyPlugin::Get().GetWaterRenderingEnabled())
			return false;

		FString PropertyName = InProperty->GetName();
		if (UseColourPresets)
		{
			if (PropertyName == "Scattering"
				|| PropertyName == "Absorption")
				return false;
		}
		else
		{
			if (PropertyName == "ColourPresets")
				return false;
		}
		if (!AdvancedSurfaceOptions)
		{
			if (   PropertyName == "WindSpeed"
				|| PropertyName == "WaveAmplitude"
				|| PropertyName == "MaxWavelength"
				|| PropertyName == "MinWavelength"
				|| PropertyName == "FoamStrength")
				return false;
		}
		if (!boundlessOcean)
			if (   PropertyName == "EnableWaveGrid"
				|| PropertyName == "WaveGridDistance"
				|| PropertyName == "EnableFoam"
				|| PropertyName == "EnableShoreEffects"
				|| PropertyName == "FoamStrength"
				|| PropertyName == "ShoreDepthSceneCapture2D"
				|| PropertyName == "ShoreDepthExtent"
				|| PropertyName == "ShoreDepthWidth")
				return false;

		if (!EnableWaveGrid)
			if (   PropertyName == "WaveGridDistance")
				return false;

		if (!EnableFoam)
			if (   PropertyName == "FoamStrength")
				return false;

		if(!EnableShoreEffects)
			if (   PropertyName == "ShoreDepthSceneCapture2D"
				|| PropertyName == "ShoreDepthExtent"
				|| PropertyName == "ShoreDepthWidth")
				return false;
	}
	return true;
}
#endif

void ATrueSkyWater::PopulateWaterPropertyFloatMap(UEnum* EnumerationType, TrueSkyWaterPropertyFloatMap& EnumerationMap)
{
	FString prefix;
	// Enumerate through all of the values.
	for (int i = 0; i < EnumerationType->GetMaxEnumValue(); ++i)
	{
		FString keyName = EnumerationType->GetNameByIndex(i).ToString();

		TArray< FString > parsedKeyName;
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check(ParsedElements > 0);
		if (ParsedElements>2)
			prefix = parsedKeyName[ParsedElements - 2].ToLower() + ":";
		FString nameString = prefix + parsedKeyName[ParsedElements - 1];

		int64 enum_int64 = ITrueSkyPlugin::Get().GetEnum(nameString);
#ifdef UE_LOG_SIMUL_INTERNAL
		if (!enum_int64)
			UE_LOG_SIMUL_INTERNAL(TrueSky, Warning, TEXT("No enum for Float: %s"), *nameString);
#endif	
		EnumerationMap.Add(EnumerationType->GetValueByIndex(i), TrueSkyPropertyFloatData(FName(*nameString), 0.0f, 1.0f, enum_int64));
	}
}

void ATrueSkyWater::PopulateWaterPropertyIntMap(UEnum* EnumerationType, TrueSkyWaterPropertyIntMap& EnumerationMap)
{
	FString prefix;
	// Enumerate through all of the values.
	for (int i = 0; i < EnumerationType->GetMaxEnumValue(); ++i)
	{
		FString keyName = EnumerationType->GetNameByIndex(i).ToString();

		TArray< FString > parsedKeyName;
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check(ParsedElements > 0);
		if (ParsedElements>2)
			prefix = parsedKeyName[ParsedElements - 2].ToLower() + ":";
		FString nameString = prefix + parsedKeyName[ParsedElements - 1];
		// UL End

		int64 enum_int64 = ITrueSkyPlugin::Get().GetEnum(nameString);
#ifdef UE_LOG_SIMUL_INTERNAL
		if (!enum_int64)
			UE_LOG_SIMUL_INTERNAL(TrueSky, Warning, TEXT("No enum for Int: %s"), *nameString);
#endif
		EnumerationMap.Add(EnumerationType->GetValueByIndex(i), TrueSkyPropertyIntData(FName(*nameString), enum_int64));
	}
}

void ATrueSkyWater::PopulateWaterPropertyVectorMap(UEnum* EnumerationType, TrueSkyWaterPropertyVectorMap& EnumerationMap)
{
	FString prefix;
	// Enumerate through all of the values.
	for (int i = 0; i < EnumerationType->GetMaxEnumValue(); ++i)
	{
		FString keyName = EnumerationType->GetNameByIndex(i).ToString();

		TArray< FString > parsedKeyName;
		// UL Begin - @jeff.sult: fix crash in shipping build
		int32 ParsedElements = keyName.ParseIntoArray(parsedKeyName, TEXT("_"), false);
		check(ParsedElements > 0);
		if (ParsedElements>2)
			prefix = parsedKeyName[ParsedElements - 2].ToLower() + ":";
		FString nameString = prefix + parsedKeyName[ParsedElements - 1];
		// UL End

		int64 enum_int64 = ITrueSkyPlugin::Get().GetEnum(nameString);
#ifdef UE_LOG_SIMUL_INTERNAL
		if (!enum_int64)
			UE_LOG_SIMUL_INTERNAL(TrueSky, Warning, TEXT("No enum for Vector: %s"), *nameString);
#endif	
		if (enum_int64 != 0)
			EnumerationMap.Add(EnumerationType->GetValueByIndex(i), TrueSkyPropertyVectorData(FName(*nameString), enum_int64));
	}
}


void ATrueSkyWater::InitializeTrueSkyWaterPropertyMap()
{
	ActorCrossThreadProperties *A = GetActorCrossThreadProperties();
	A->CriticalSection.Lock();

	TrueSkyWaterPropertyFloatMaps.Empty();
	TrueSkyWaterPropertyIntMaps.Empty();
	TrueSkyWaterPropertyVectorMaps.Empty();

	TArray< FName > enumerationNames = {
		TEXT("ETrueSkyWaterPropertiesFloat"),
		TEXT("ETrueSkyWaterPropertiesInt"),
		TEXT("ETrueSkyWaterPropertiesVector")
	};

	TArray< int32* > enumerationIndices = {
		&TSWaterPropertiesFloatIdx,
		&TSWaterPropertiesIntIdx,
		&TSWaterPropertiesVectorIdx
	};
	// Populate the full enumeration property map.
	for (int i = 0; i < enumerationNames.Num(); ++i)
	{
		FName enumerator = enumerationNames[i];
		FString prefix = "";
		UEnum* pEnumeration = FindObject< UEnum >(ANY_PACKAGE, *enumerator.ToString(), true);
		if (!pEnumeration)
		{
			UE_LOG(TrueSky, Display, TEXT("Enum not found: %s"), *enumerator.ToString());
			continue;
		}

		if (enumerator.ToString().Contains(TEXT("Vector")))
		{
			// Vector-based map population.
			int32 idx = TrueSkyWaterPropertyVectorMaps.Add(TrueSkyWaterPropertyVectorMap());
			*enumerationIndices[i] = idx;

			PopulateWaterPropertyVectorMap(pEnumeration, TrueSkyWaterPropertyVectorMaps[idx]);
		}
		else if (enumerator.ToString().Contains(TEXT("Int")))
		{
			// Int-based map population.
			int32 idx = TrueSkyWaterPropertyIntMaps.Add(TrueSkyWaterPropertyIntMap());
			*enumerationIndices[i] = idx;

			PopulateWaterPropertyIntMap(pEnumeration, TrueSkyWaterPropertyIntMaps[idx]);
		}
		else
		{
			// Float-based map population.
			int32 idx = TrueSkyWaterPropertyFloatMaps.Add(TrueSkyWaterPropertyFloatMap());
			*enumerationIndices[i] = idx;

			PopulateWaterPropertyFloatMap(pEnumeration, TrueSkyWaterPropertyFloatMaps[idx]);
		}
	}
//		ApplyRenderingOptions();
	A->CriticalSection.Unlock();
}

