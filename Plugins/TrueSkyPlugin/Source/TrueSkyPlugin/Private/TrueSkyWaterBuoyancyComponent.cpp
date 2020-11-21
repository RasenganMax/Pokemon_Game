#include "TrueSkyWaterBuoyancyComponent.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/MeshDescription/Public/MeshDescription.h"
#include "Runtime/MeshDescription/Public/MeshAttributes.h"
#include "Runtime/MeshDescription/Public/MeshAttributeArray.h"
#include "Engine/StaticMesh.h"
#include <algorithm>
#include <atomic>

#include "PhysicsEngine/PhysicsThrusterComponent.h"
#include "ITrueSkyPlugin.h"
#include "TrueSkyWaterProbeComponent.h"

#define LOCTEXT_NAMESPACE "TrueSkyWaterBuoyancyComponent"

static std::atomic<int> BuoyancyObjectIDCount(0);

UTrueSkyWaterBuoyancyComponent::UTrueSkyWaterBuoyancyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	enablePhysics(true),
	useModelData(false)
	//buoyancyLOD(0)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	tickCount = 0;
}

UTrueSkyWaterBuoyancyComponent::~UTrueSkyWaterBuoyancyComponent()
{
	
}

void UTrueSkyWaterBuoyancyComponent::PostInitProperties()
{
	Super::PostInitProperties();

	BuoyancyObjectIDCount++;
	ID = BuoyancyObjectIDCount;
}

void UTrueSkyWaterBuoyancyComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	tickCount++;

	if (!useModelData)
	{
		TArray<USceneComponent*> waterProbes;

		GetChildrenComponents(false, waterProbes);

		if (waterProbes.Num() != 0)
		{
			UPrimitiveComponent* BasePrimComp = Cast<UPrimitiveComponent>(GetAttachParent());
			if (BasePrimComp)
			{
				int i;

				float AdjustedDepth = 0.f;
				float sampledDepth = 0.f;
				FVector directionalForce = FVector(0.f, 0.f, 0.f);
				int activeProbeCount = 0;
				FBoxSphereBounds bounds = BasePrimComp->Bounds;
				UTrueSkyWaterProbeComponent* waterProbe;
				for (i = 0; i < waterProbes.Num(); i++)
				{
					waterProbe = Cast<UTrueSkyWaterProbeComponent>(waterProbes[i]);
					if (waterProbe != nullptr)
					{
						waterProbe->UpdateValues();
						if (waterProbe->active && waterProbe->ProbeType == EProbeType::Physics)
						{
							activeProbeCount++;
							sampledDepth = waterProbe->GetDepth() * 100.f;
							directionalForce = waterProbe->GetDirection() * 100.f;

							AdjustedDepth = sampledDepth;

							float BuoyancyForce;
							float volume;
							float R = waterProbe->GetScaledSphereRadius();
							AdjustedDepth += R;

							if (AdjustedDepth > 0.f)
							{
								if (AdjustedDepth > 2.f * R)
								{
									volume = (4.f / 3.f) * PI * R * R * R;
									sampledDepth = R;
									//AdjustedDepth = 2.0f * R;
								}
								else
								{
									if (sampledDepth > 0.f)
										AdjustedDepth = R - sampledDepth;

									float h = AdjustedDepth;// waterProbe->GetDepth();

									volume = ((PI * h * h) / 3.f) * ((3.f * R) - h);

									//float d = R - h;
									//volume = PI * h * ((R*R) - (d*d) - (h * d) - ((h * h) / 3));

									if (sampledDepth > 0.f)
										volume = ((4.f / 3.f) * PI * R * R * R) - volume;// bounds.GetSphere().GetVolume() - volume;
								}
								BuoyancyForce = volume * 0.001f * 9.81f;

								if (BuoyancyForce < 0.f)
								{
									return;
								}

								FVector direction = BasePrimComp->GetComponentVelocity();
								float mass = (BasePrimComp->CalculateMass() * (volume / ((4.f / 3.f) * PI * R * R * R)));
								float oldVelocity = waterProbe->velocity.Size() * 0.01;
								float newVelocity = direction.Size() * 0.01;
								waterProbe->dEnergy = std::min(((mass * newVelocity * newVelocity * 0.5) - (mass * oldVelocity * oldVelocity * 0.5)), 0.0);
								waterProbe->velocity = direction;

								if (enablePhysics)
								{
								static float c = 0.3f;
								float dragArea = 0.f;

								direction.Normalize();

								//direction = FVector(0.0, 1.0, 0.1);

								if (AdjustedDepth > 2.f * R)
								{
									dragArea = (PI * R * R);
								}
								else
								{

									FQuat BetweenQuat = FQuat::FindBetweenVectors(direction, FVector(0.f, 0.f, 1.f * (direction.Z / direction.Z)));

									float angle;
									FVector temp = FVector(0.f, 0.f, 0.f);
									BetweenQuat.ToAxisAndAngle(temp, angle);
									angle = (PI / 2) - angle;
									float h = R - (abs(sampledDepth));

									if (h < 0.f || h > R)
										dragArea = 0.f;
									else
										dragArea = ((R * R) * cosh((R - h) / R)) - ((R - h) * sqrt((2.f * R * h) - (h * h)));

									if (sampledDepth > 0.f)
										dragArea = (PI * R * R) - dragArea;
								}

								//dragArea = (PI * R * R);

								directionalForce.Z = 0.f;

								float DragForce = 0.5f * c * dragArea * pow(waterProbe->velocity.Size(), 2.0f) * 0.0001f;
								float WaterMovementForce = 0.5f * c * dragArea * pow(directionalForce.Size(), 2.0f) * 0.0001f;

								FVector WorldBuoyancyForce = FVector(0.f, 0.f, BuoyancyForce);
								FVector WorldDragForce = DragForce * -direction;
								FVector WorldWaterMovementForce = WaterMovementForce * (directionalForce / directionalForce.Size());
								WorldWaterMovementForce = FVector(WorldWaterMovementForce.X, WorldWaterMovementForce.Y, 0.0);
								FVector ForceLocation = waterProbe->GetComponentLocation() - FVector(0.f, 0.f, (sampledDepth - R) / 2.f);

								if (BuoyancyForce > 0.f)
									BasePrimComp->AddForceAtLocation(WorldBuoyancyForce, ForceLocation, NAME_None);
								if (WorldDragForce.Size() > 0.f)
									BasePrimComp->AddForceAtLocation(WorldDragForce, ForceLocation, NAME_None);
								if (WorldWaterMovementForce.Size() > 0.f)
									BasePrimComp->AddForceAtLocation(WorldWaterMovementForce, ForceLocation, NAME_None);
							}
						}
					}
				}
			}
		}
	} 
	} 
	else
	{
		if (BuoyancyObjectCreated)
		{
			ITrueSkyPlugin::Get().UpdateWaterBuoyancyObjectValues(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale());

			BuoyancyObjectResults = ITrueSkyPlugin::Get().GetWaterBuoyancyObjectResults(ID, Vertices.Num());
		}
		else
		{
			UStaticMesh* Mesh;
			if (BuoyancyMesh == NULL)
				Mesh = Cast<UStaticMeshComponent>(GetAttachParent())->GetStaticMesh();
			else
				Mesh = BuoyancyMesh;

			Vertices.Empty();

			const FStaticMeshLODResources& LOD = Mesh->RenderData->LODResources[0];
			const FPositionVertexBuffer& PosVertexBuffer = LOD.VertexBuffers.PositionVertexBuffer;

			const int32 NumVerts = LOD.GetNumVertices();
			for (int32 VertIndex = 0; VertIndex < NumVerts; VertIndex++)
			{
				const FVector va = PosVertexBuffer.VertexPosition(VertIndex);
				Vertices.Add(va);
			}

			BuoyancyObjectCreated = ITrueSkyPlugin::Get().AddWaterBuoyancyObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale(), Vertices);
		}
	}
}

void UTrueSkyWaterBuoyancyComponent::BeginPlay()
{
	if (useModelData)
	{
		UStaticMesh* Mesh;
		if (BuoyancyMesh == NULL)
		{
			if(Cast<UStaticMeshComponent>(GetAttachParent()))
				Mesh = Cast<UStaticMeshComponent>(GetAttachParent())->GetStaticMesh();
			else
			{
				useModelData = false; //need a warning to say could not find static mesh
				Super::BeginPlay();
				return;
			}
		}
		else
			Mesh = BuoyancyMesh;

		Vertices.Empty();

		const FStaticMeshLODResources& LOD = Mesh->RenderData->LODResources[0];
		const FPositionVertexBuffer& PosVertexBuffer = LOD.VertexBuffers.PositionVertexBuffer;

		const int32 NumVerts = LOD.GetNumVertices();
		for (int32 VertIndex = 0; VertIndex < NumVerts; VertIndex++)
		{
			const FVector va = PosVertexBuffer.VertexPosition(VertIndex);
			Vertices.Add(va);
		}

		BuoyancyObjectCreated = ITrueSkyPlugin::Get().AddWaterBuoyancyObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale(), Vertices);

	}
	Super::BeginPlay();
}

void UTrueSkyWaterBuoyancyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BuoyancyObjectCreated && useModelData)
	{
		ITrueSkyPlugin::Get().RemoveWaterBuoyancyObject(ID);
	}
}

TArray<float> UTrueSkyWaterBuoyancyComponent::GetDepthResults()
{
	return BuoyancyObjectResults;
}

#if WITH_EDITOR
bool UTrueSkyWaterBuoyancyComponent::CanEditChange(const UProperty* InProperty) const
{
	if (InProperty)
	{
		FString PropertyName = InProperty->GetName();
		//if (!useModelData && PropertyName == GET_MEMBER_NAME_STRING_CHECKED(UTrueSkyWaterBuoyancyComponent, buoyancyLOD))
		//	return false;
	}

	return(Super::CanEditChange(InProperty));
}
#endif

#undef LOCTEXT_NAMESPACE