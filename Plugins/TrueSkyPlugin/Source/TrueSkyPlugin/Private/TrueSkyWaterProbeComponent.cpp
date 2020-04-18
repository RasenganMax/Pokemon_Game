
#include "TrueSkyWaterProbeComponent.h"

#include "TrueSkyPluginPrivatePCH.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#endif
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Misc/ScopeLock.h"
#include "Misc/MapErrors.h"
#include "ComponentInstanceDataCache.h"
#include "ShaderCompiler.h"
#include "SceneManagement.h"
#include "Components/SphereComponent.h"
#include "UObject/UObjectIterator.h"
#include <algorithm>
#include <atomic>
#include <string>

#define LOCTEXT_NAMESPACE "TrueSkyWaterProbeComponent"

static std::atomic<int> ProbeIDCount(0);

UTrueSkyWaterProbeComponent::UTrueSkyWaterProbeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	ProbeType(EProbeType::Physics),
	Intensity(0.f),
	GenerateWaves(true),
	location(0.f, 0.f, 0.f),
	depth(0.f),
	waterProbeCreated(false)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

UTrueSkyWaterProbeComponent::~UTrueSkyWaterProbeComponent()
{
	ITrueSkyPlugin::Get().RemoveWaterProbe(ID);
	//arrowComponent->DestroyComponent();
}

void UTrueSkyWaterProbeComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	ITrueSkyPlugin::Get().RemoveWaterProbe(ID);
	waterProbeCreated = false;
	//arrowComponent->DestroyComponent();
}

void UTrueSkyWaterProbeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ITrueSkyPlugin::Get().RemoveWaterProbe(ID);
	waterProbeCreated = false;
}

void UTrueSkyWaterProbeComponent::PostInitProperties()
{
	Super::PostInitProperties();
	if (GetAttachmentRootActor() != nullptr)
		location = GetComponentLocation();
	else
		location = GetComponentLocation();
	ProbeIDCount++;
	ID = ProbeIDCount;
	/*
	std::string componentName = "Arrow Component " + std::to_string(ID);
	arrowComponent = NewObject<UArrowComponent>(this, UArrowComponent::StaticClass(), FName(componentName.c_str()));
	arrowComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	if (ProbeType == EProbeType::Source)
	{
		arrowComponent->SetVisibility(true);
	}
	else if (ProbeType == EProbeType::Physics)
	{
		arrowComponent->SetVisibility(false);
	}
	*/
	// Should use EnsureLoaded() here instead of Get(), else we fail a check on packaged console builds.
//	if(!waterProbeCreated)
	//	waterProbeCreated = ITrueSkyPlugin::EnsureLoaded().AddWaterProbe(ID, location, velocity, (int)GenerateWaves * GetScaledSphereRadius());
		//arrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow Component"));

}

void UTrueSkyWaterProbeComponent::PostLoad()
{
	Super::PostLoad();
	if (GetAttachmentRootActor() != nullptr)
		location = GetComponentLocation();
	else
		location = GetComponentLocation();


	//if (!waterProbeCreated)
	//	waterProbeCreated = ITrueSkyPlugin::EnsureLoaded().AddWaterProbe(ID, location, velocity, (int)GenerateWaves *GetScaledSphereRadius());
}

/*
#if WITH_EDITOR
void  UTrueSkyWaterProbeComponent::EditorApplyTranslation(const FVector & DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	Super::EditorApplyTranslation(DeltaTranslation, bAltDown, bShiftDown, bCtrlDown);
	ITrueSkyPlugin::Get().UpdateWaterProbePosition(ID, location);
}
#endif
*/
#if WITH_EDITOR
void UTrueSkyWaterProbeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	location = GetComponentLocation();
	/*
	if (ProbeType == EProbeType::Source)
	{
		arrowComponent->SetVisibility(true);
	}
	else if(ProbeType == EProbeType::Physics)
	{
		arrowComponent->SetVisibility(false);
	}
	*/
	if (waterProbeCreated)
		ITrueSkyPlugin::Get().UpdateWaterProbeValues(ID, location, ProbeType == EProbeType::Physics ? velocity : Intensity * GetComponentRotation().Vector(), (int)GenerateWaves * (ProbeType==EProbeType::Physics ? GetScaledSphereRadius() : -GetScaledSphereRadius()), dEnergy);
}

bool UTrueSkyWaterProbeComponent::CanEditChange(const UProperty* InProperty) const
{
	FString PropertyName = InProperty->GetName();

	if (ProbeType == EProbeType::Physics)
	{
		if (PropertyName == "Intensity")
			return false;
	}
	if (ProbeType == EProbeType::Source)
	{
		if (PropertyName == "GenerateWaves")
			return false;
	}
	if (ProbeType == EProbeType::DepthTest)
	{
		if (PropertyName == "GenerateWaves" ||
			PropertyName == "Intensity")
			return false;
	}

	return true;
}
#endif

void UTrueSkyWaterProbeComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
	if (GetAttachmentRootActor() != nullptr)
		location = GetComponentLocation();
	else
		location = GetComponentLocation();
//	ITrueSkyPlugin::Get().UpdateWaterProbeValues(ID, location, ProbeType == EProbeType::Physics ? velocity : Intensity * GetComponentRotation().Vector(), ProbeType == EProbeType::Physics ? GetScaledSphereRadius() : -GetScaledSphereRadius(), dEnergy);
}

void UTrueSkyWaterProbeComponent::UpdateValues()
{
	if (GetAttachmentRootActor() != nullptr)
		location = GetComponentLocation();
	else
		location = GetComponentLocation();
	if(!waterProbeCreated)
		waterProbeCreated = ITrueSkyPlugin::Get().AddWaterProbe(ID, location, ProbeType == EProbeType::Physics ? velocity : Intensity * GetComponentRotation().Vector(), (int)GenerateWaves * (ProbeType == EProbeType::Physics ? GetScaledSphereRadius() : -GetScaledSphereRadius()));

	if (!waterProbeCreated)
		return;

	FVector4 values = ITrueSkyPlugin::Get().GetWaterProbeValues(ID);
	if (values.X == -1.f && values.Y == -1.f && values.Z == -1.f && values.W == -1.f)
		active = false;
	else
		active = true;
	depth = values.X + values.W;
	direction = FVector(values.Z, values.Y, values.W);
}

void UTrueSkyWaterProbeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetAttachmentRootActor() != nullptr)
		location = GetComponentLocation();
	else
		location = GetComponentLocation();

	if (!waterProbeCreated)
		waterProbeCreated = ITrueSkyPlugin::EnsureLoaded().AddWaterProbe(ID, location, velocity, (int)GenerateWaves * GetScaledSphereRadius());

	if (waterProbeCreated)
		ITrueSkyPlugin::Get().UpdateWaterProbeValues(ID, location, ProbeType == EProbeType::Physics ? velocity : Intensity * GetComponentRotation().Vector(), (int)GenerateWaves * (ProbeType == EProbeType::Physics ? GetScaledSphereRadius() : -GetScaledSphereRadius()), dEnergy);

	if (ProbeType == EProbeType::DepthTest)
		UpdateValues();
}

FVector UTrueSkyWaterProbeComponent::GetDirection()
{
	return direction;
}

float UTrueSkyWaterProbeComponent::GetDepth()
{
	return depth;
}

#undef LOCTEXT_NAMESPACE