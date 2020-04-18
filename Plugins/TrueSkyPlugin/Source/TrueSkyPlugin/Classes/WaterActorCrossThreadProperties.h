#pragma once
#include "TrueSkyPluginPrivatePCH.h"
#include "RHIResources.h"

struct WaterActorCrossThreadProperties
{
	WaterActorCrossThreadProperties()
		:Render(true)
		,Destroyed(false)
		,Reset(true)
		,Updated(true)
		,BoundedObjectCreated(false)
		,BoundlessOcean(false)
		,BeaufortScale(3.0)
		,WindDirection(0.0)
		,WindDependency(0.95)
		,Scattering(0.17, 0.2, 0.234)
		,Absorption(0.2916, 0.0474, 0.0092)
		,ProfileResolution(2048)
		,WindSpeed(5.0)
		,WaveAmplitude(1.0)
		,MaxWavelength(50.0)
		,MinWavelength(0.04)
		,Dimension(200.0, 200.0, 200.0)
		,Rotation(0.0)
		,EnableFoam(true)
		,FoamStrength(0.4)
		,EnableShoreEffects(false)
		,ShoreDepthRT(nullptr)
		,ShoreDepthTextureLocation(0.0, 0.0, 0.0)
		,ShoreDepthWidth(3000)
		,ShoreDepthExtent(2500)
		,ID(-1)
	{
	}
		bool Render;
		bool Destroyed;
		bool Reset;
		bool Updated;
		bool BoundedObjectCreated;
		bool BoundlessOcean;

		float BeaufortScale;
		float WindDirection;
		float WindDependency;
		FVector Scattering;
		FVector Absorption;
		FVector Location;
		FVector Dimension;
		float Rotation;

		bool EnableFoam;
		float FoamStrength;

		bool EnableWaveGrid;
		int WaveGridDistance;

		bool AdvancedSurfaceOptions;
		float WindSpeed;
		float WaveAmplitude;
		float MaxWavelength;
		float MinWavelength;
		int ProfileResolution;

		bool EnableShoreEffects;
		FTextureRHIRef ShoreDepthRT;
		FVector ShoreDepthTextureLocation;
		float ShoreDepthExtent;
		float ShoreDepthWidth;

		TArray<TPair<TrueSkyEnum, simul::unreal::VariantPass> > SetWaterUpdateValues;
		TArray<TPair<TrueSkyEnum, simul::unreal::vec3> > SetWaterUpdateVectors;
		int ID;
};

extern WaterActorCrossThreadProperties* GetWaterActorCrossThreadProperties(int ID);
extern void AddWaterActorCrossThreadProperties(WaterActorCrossThreadProperties* newProperties);
extern void RemoveWaterActorCrossThreadProperties(int ID);
