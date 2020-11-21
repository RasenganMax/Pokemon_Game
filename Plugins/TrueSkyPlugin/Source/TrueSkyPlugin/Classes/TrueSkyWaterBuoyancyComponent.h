// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "PhysicsEngine/PhysicsThrusterComponent.h"
#include "Components/PrimitiveComponent.h"
#include "TrueSkyWaterProbeComponent.h"

#include "TrueSkyWaterBuoyancyComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = Water, HideCategories = (Trigger, Activation, "Components|Activation", Physics, Light), meta = (BlueprintSpawnableComponent))
class TRUESKYPLUGIN_API UTrueSkyWaterBuoyancyComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()
	~UTrueSkyWaterBuoyancyComponent();

	static TArray<UTrueSkyWaterProbeComponent*> TrueSkyWaterProbeComponents;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	void BeginPlay() override;

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	int tickCount;

	/**	Set false to disable buoyant forces (probes will still create waves)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsOptions)
	bool enablePhysics;

	/** Calculate depth of each vertex on the model. Internally stored*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsOptions)
	bool useModelData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsOptions)
	UStaticMesh* BuoyancyMesh;

	/** Model LOD to use for vertex updates. Set to -1 to use default LOD*/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsOptions, meta = (UIMin = "0", UIMax = "10"))
	//int buoyancyLOD;

	TArray<float> GetDepthResults();

private:
#if WITH_EDITOR
	virtual bool CanEditChange(const UProperty* InProperty)const override;
#endif
	UPROPERTY()
	int ID;

	TArray<FVector> Vertices;

	TArray<float> BuoyancyObjectResults;

	void PostInitProperties() override;

	bool BuoyancyObjectCreated;
};