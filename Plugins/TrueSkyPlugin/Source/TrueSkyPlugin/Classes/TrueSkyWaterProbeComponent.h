// Copyright 2007-2019 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "TrueSkyWaterProbeComponent.generated.h"

UENUM()
enum class EProbeType : uint8
{
	Physics	= 0x0,
	Source	= 0x1,
	DepthTest  = 0x2
};


UCLASS(Blueprintable, ClassGroup = Water, HideCategories = (Trigger, Activation, "Components|Activation", Physics, Light), meta = (BlueprintSpawnableComponent))
class TRUESKYPLUGIN_API UTrueSkyWaterProbeComponent : public USphereComponent
{
	GENERATED_UCLASS_BODY()
	~UTrueSkyWaterProbeComponent();

	static TArray<UTrueSkyWaterProbeComponent*> TrueSkyWaterProbeComponents;

public:

	/** Functionality of the probe. */
	UPROPERTY(Config = Engine, EditAnywhere, BlueprintReadWrite, Category = Properties)
	EProbeType ProbeType;

	/** Intensity of the source */
	UPROPERTY(Config = Engine, EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float Intensity;

	UPROPERTY(Config = Engine, EditAnywhere, BlueprintReadWrite, Category = Properties)
	bool GenerateWaves;

	UFUNCTION(BlueprintCallable, Category = "TrueSky|Water")
	float GetDepth();

	int ID;
	FVector location;
	FVector velocity;
	FVector direction;
	float depth;
	float dEnergy;
	bool active;

	UArrowComponent* arrowComponent;

	void UpdateValues();

	FVector GetDirection();



	void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual bool CanEditChange(const UProperty* InProperty)const override;
#endif

	void PostLoad() override;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);

private:
	void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void PostInitProperties() override;

	bool waterProbeCreated;

};