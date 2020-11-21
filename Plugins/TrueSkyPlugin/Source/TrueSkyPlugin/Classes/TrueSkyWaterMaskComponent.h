// Copyright 2007-2019 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"

#include "TrueSkyWaterMaskComponent.generated.h"

UENUM()
enum class EMaskType : uint8
{
	Plane = 0x0,
	Cube = 0x1,
	Model = 0x2
};

UCLASS(Blueprintable, ClassGroup = Water, HideCategories = (Trigger, Activation, "Components|Activation", Physics, Light), meta = (BlueprintSpawnableComponent))
class TRUESKYPLUGIN_API UTrueSkyWaterMaskComponent : public UBoxComponent
{
	GENERATED_UCLASS_BODY()
	~UTrueSkyWaterMaskComponent();

	static TArray<UTrueSkyWaterMaskComponent*> TrueSkyWaterMaskComponents;

public:

	/** Whether the mask is to be rendered or not. */
	UPROPERTY(Config = Engine, EditAnywhere, BlueprintReadWrite, Category = Properties)
	bool Active;

	/** Set the mask to mask all water behind it completely */
	UPROPERTY(Config = Engine, EditAnywhere, BlueprintReadWrite, Category = Properties)
	bool TotalMask;

	/** Shape of the mask. */
	UPROPERTY(Config = Engine, EditAnywhere, BlueprintReadWrite, Category = Properties)
	EMaskType ObjectType;

	/** The mesh used by the mask. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties)
	UStaticMesh* CustomMesh;

	void BeginPlay() override;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	int ID;
	FVector location;
	bool maskObjectCreated;

private:
	void PostInitProperties() override;

	void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void UpdateValues(bool newModel);

	void PostLoad() override;

	void OnUpdateTransform(EUpdateTransformFlags UpdateTransform, ETeleportType Teleport) override;
#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;

	void PostDuplicate(bool bDuplicateForPIE) override;

	virtual bool CanEditChange(const UProperty* InProperty)const override;
#endif

	TArray<FVector> Vertices;
	TArray<uint32> Indices;
};