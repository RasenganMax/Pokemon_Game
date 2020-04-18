// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once
#include "TrueSkyPluginPrivatePCH.h"
#include "RHIResources.h"
#include "TrueSkyLightComponent.h"

namespace simul
{
	namespace unreal
	{
		struct ActorCrossThreadProperties
		{
			ActorCrossThreadProperties()
				:Destroyed(false)
				, Visible(false)
				, Reset(true)
				, Playing(false)
				, alteredPosition(true)
				, MetresPerUnit(0.01f)
				, Brightness(1.0f)
				, Gamma(1.0f)
				, CloudThresholdDistanceKm(0.1f)
				, CloudShadowTextureSize(512)
				, Amortization(1)
				, AtmosphericsAmortization(4)
				, activeSequence(nullptr)
				, MoonTexture(nullptr)
				, CosmicBackgroundTexture(nullptr)
				, InscatterRT(nullptr)
				, LossRT(nullptr)
				, CloudVisibilityRT(nullptr)
				, CloudShadowRT(nullptr)
				, AltitudeLightRT(nullptr)
				, OverlayRT(nullptr)
				, TranslucentRT(nullptr)
				, RainCubemap(nullptr)
				, RainDepthRT(nullptr)
				, CaptureCubemapRT(nullptr)
				, ShoreDepthRT(nullptr)
				, Time(0.0f)
				, SetTime(false)
				, GetTime(false)
				, RenderWater(true)
				, WaterFullResolution(true)
				, ReflectionFullResolution(true)
				, EnableReflection(true)
				,ReflectionSteps(100)
				,ReflectionPixelStep(2)
				,EnableBoundlessOcean(true)
				,EnableFoam(true)
				,Rotation(0.0)
				,WaterOffset(0.0,0.0,0.0)
				,AdvancedWaterOptions(false)
				,ShoreDepthTextureLocation(0.0,0.0,0.0)
				,ShoreDepthExtent(2500)
				,ShoreDepthWidth(3000)
				,EnableShoreEffects(false)
				,WaterUseGameTime(false)
				,RainDepthTextureScale(1.0f)
				,RainNearThreshold(30.0f)
				,RainProjMatrix(FMatrix::Identity)
				,DepthBlending(true)
				,MaxSunRadiance(150000.0f)
				,AdjustSunRadius(true)
				,InterpolationMode(0)
				,InstantUpdate(true)
				,InterpolationTimeSeconds(10.0f)
				,InterpolationTimeHours(5.0f)
				,InterpolationSubdivisions(8)
				,MinimumStarPixelSize(2.0f)
				,PrecipitationOptions(0)
				,ShareBuffersForVR(true)
				,MaximumResolution(0)
				,DepthSamplingPixelRange(1.5f)
				,DepthTemporalAlpha(0.1f)
				,GridRendering(false)
				,TrueSkyLightUnits(0)
				,SkyMultiplier(1.0f)
				,MoonsChecksum(0)
			{
				memset(lightningBolts,0,sizeof(simul::LightningBolt)*4);
				memset(GodraysGrid,0,sizeof(GodraysGrid));
			}
			void Shutdown()
			{
				MoonTexture.SafeRelease();
				CosmicBackgroundTexture.SafeRelease();
				InscatterRT.SafeRelease();
				LossRT.SafeRelease();
				CloudVisibilityRT.SafeRelease();
				CloudShadowRT.SafeRelease();
				AltitudeLightRT.SafeRelease();
				OverlayRT.SafeRelease();
				TranslucentRT.SafeRelease();
				RainCubemap.SafeRelease();
				RainDepthRT.SafeRelease();
				CaptureCubemapRT.SafeRelease();
			}
			bool Destroyed;
			bool Visible;
			bool Reset;
			bool Playing;
			bool alteredPosition;
			float MetresPerUnit;
			float Brightness;
			float Gamma;
			float CloudThresholdDistanceKm;
			int CloudShadowTextureSize;
			int Amortization;
			int AtmosphericsAmortization;
            TWeakObjectPtr<UTrueSkySequenceAsset> activeSequence;
			FTextureRHIRef MoonTexture;
			FTextureRHIRef CosmicBackgroundTexture;
			FTextureRHIRef InscatterRT;
			FTextureRHIRef LossRT;
			FTextureRHIRef CloudVisibilityRT;
			FTextureRHIRef CloudShadowRT;
			FTextureRHIRef AltitudeLightRT;
			FTextureRHIRef OverlayRT;
			FTextureRHIRef TranslucentRT;
			FTextureRHIRef RainCubemap;
			FTextureRHIRef RainDepthRT;
			FTextureRHIRef CaptureCubemapRT;
			FTextureRHIRef ShoreDepthRT;
			int32 CaptureCubemapResolution;
			float Time;
			bool SetTime;
			bool GetTime;
			FTransform Transform;
			bool RenderWater;
			bool WaterFullResolution;
			bool ReflectionFullResolution;
			bool EnableReflection;
			int ReflectionSteps;
			int ReflectionPixelStep;
			bool EnableBoundlessOcean;
			bool EnableFoam;
			float Rotation;
			FVector WaterOffset;
			bool AdvancedWaterOptions;
			FVector ShoreDepthTextureLocation;
			float ShoreDepthExtent;
			float ShoreDepthWidth;
			bool EnableShoreEffects;
			bool WaterUseGameTime;
			FMatrix RainDepthMatrix;
			float RainDepthTextureScale;
			float RainNearThreshold;
			FMatrix RainProjMatrix;
			simul::LightningBolt lightningBolts[4];
			bool DepthBlending;
			float MaxSunRadiance;
			bool AdjustSunRadius;
			int InterpolationMode;
			bool InstantUpdate;
			float InterpolationTimeSeconds;
			float InterpolationTimeHours;
			int32 InterpolationSubdivisions;
			float MinimumStarPixelSize;
			uint8_t PrecipitationOptions;
			bool ShareBuffersForVR;
			int MaximumResolution;
			float DepthSamplingPixelRange;
			float DepthTemporalAlpha;
			bool GridRendering;
			uint8_t TrueSkyLightUnits;
			float SkyMultiplier;
			int32 WorleyNoiseTextureSize;
			int32 DefaultCloudSteps;
			int32 GodraysGrid[3];
			TArray<TPair<TrueSkyEnum,vec3> > SetVectors;
			TArray<TPair<TrueSkyEnum,float> > SetFloats;
			TArray<TPair<TrueSkyEnum, VariantPass> > SetRenderingValues;
			TArray<TPair<TrueSkyEnum,int32> > SetInts;
			TMap<int32, ITrueSkyPlugin::LightingQueryResult> LightingQueries;
			TMap<uint32, ExternalMoonProxy> Moons;
			FCriticalSection CriticalSection;

			float			OriginLatitude;
			float			OriginLongitude;
			float			OriginHeading;
			unsigned MoonsChecksum;
		};
		extern TRUESKYPLUGIN_API ActorCrossThreadProperties *GetActorCrossThreadProperties();
	}
}
