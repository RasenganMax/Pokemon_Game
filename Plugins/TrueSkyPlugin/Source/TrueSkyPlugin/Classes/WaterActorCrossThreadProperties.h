#pragma once
#include "TrueSkyPluginPrivatePCH.h"
#include "RHIResources.h"

struct WaterActorCrossThreadProperties
{
	WaterActorCrossThreadProperties()
		//UE4 Settings
		:Render(true)
		,Destroyed(false)
		,Reset(true)
		,Updated(true)
		,BoundedObjectCreated(false)
		,Dimension(200.0, 200.0, 200.0)
		,Rotation(0.0)

		//Ocean
		,BoundlessOcean(false)
		,BeaufortScale(3.0)

		//Wind
		,WindDirection(0.0)
		,WindDependency(0.95)

		//Lighting
		,Scattering(0.17, 0.2, 0.234)
		,Absorption(0.2916, 0.0474, 0.0092)

		//Res
		,ProfileResolution(2048)

		//Foam
		, EnableFoam(true)
		, FoamStrength(0.4)
		
		//Advanced
		,WindSpeed(5.0)
		,WaveAmplitude(1.0)
		,MaxWavelength(50.0)
		,MinWavelength(0.04)
		
		
		//Shore Effects
		,EnableShoreEffects(false)
		,ShoreDepthRT(nullptr)
		,ShoreDepthTextureLocation(0.0, 0.0, 0.0)
		,ShoreDepthWidth(3000)
		,ShoreDepthExtent(2500)

		,ID(-1)
	{
	}

		//UE4 Settings
		bool Render;
		bool Destroyed;
		bool Reset;
		bool Updated;
		bool BoundedObjectCreated;
		FVector Location;
		FVector Dimension;
		float Rotation;
		//Ocean
		bool BoundlessOcean;
		float BeaufortScale;

		//Wind
		float WindDirection;
		float WindDependency;

		//Lighting
		FVector Scattering;
		FVector Absorption;

		//Res
		int ProfileResolution;

		//Foam
		bool EnableFoam;
		float FoamStrength;

		//Wave Grid
		bool EnableWaveGrid;
		int WaveGridDistance;

		//Advanced
		bool AdvancedSurfaceOptions;
		float WindSpeed;
		float WaveAmplitude;
		float MaxWavelength;
		float MinWavelength;
	
		//Shore Effects
		bool EnableShoreEffects;
		FTextureRHIRef ShoreDepthRT;
		FVector ShoreDepthTextureLocation;
		float ShoreDepthWidth;
		float ShoreDepthExtent;
		

		TArray<TPair<TrueSkyEnum, simul::unreal::VariantPass> > SetWaterUpdateValues;
		TArray<TPair<TrueSkyEnum, simul::unreal::vec3> > SetWaterUpdateVectors;
		int ID;
};

extern WaterActorCrossThreadProperties* GetWaterActorCrossThreadProperties(int ID);
extern void AddWaterActorCrossThreadProperties(WaterActorCrossThreadProperties* newProperties);
extern void RemoveWaterActorCrossThreadProperties(int ID);
