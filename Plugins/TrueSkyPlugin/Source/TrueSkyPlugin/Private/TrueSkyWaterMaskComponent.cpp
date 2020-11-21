
#include "TrueSkyWaterMaskComponent.h"

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
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/MeshDescription/Public/MeshDescription.h"
#include "Runtime/MeshDescription/Public/MeshAttributes.h"
#include "Runtime/MeshDescription/Public/MeshAttributeArray.h"
#include "Engine/StaticMesh.h"
#include "UObject/UObjectIterator.h"
#include <algorithm>
#include <atomic>
#include <string>

#define LOCTEXT_NAMESPACE "TrueSkyWaterMaskComponent"

static std::atomic<int> MaskObjectIDCount(0);

UTrueSkyWaterMaskComponent::UTrueSkyWaterMaskComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	Active(true),
	TotalMask(false),
	ObjectType(EMaskType::Cube),
	ID(++MaskObjectIDCount)
{
	PrimaryComponentTick.bCanEverTick = true;
}

UTrueSkyWaterMaskComponent::~UTrueSkyWaterMaskComponent()
{
	ITrueSkyPlugin::Get().RemoveWaterMaskObject(ID);
}

void UTrueSkyWaterMaskComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	ITrueSkyPlugin::Get().RemoveWaterMaskObject(ID);
	maskObjectCreated = false;
}

void UTrueSkyWaterMaskComponent::PostInitProperties()
{
	Super::PostInitProperties();
	
	switch (ObjectType)
	{
		case EMaskType::Model:
		{
			if (CustomMesh == NULL)
			{
				UE_LOG(TrueSky, Warning, TEXT("Attempting to use a water mask with a custom mask model without setting a model"));
				return;
			}

			UStaticMesh* Mesh = CustomMesh;

			Vertices.Empty();
			Indices.Empty();

			const FStaticMeshLODResources& LOD = Mesh->RenderData->LODResources[0];
			const FPositionVertexBuffer& PosVertexBuffer = LOD.VertexBuffers.PositionVertexBuffer;
			const FRawStaticIndexBuffer& IndexBuffer = LOD.IndexBuffer;

			const int32 NumVerts = LOD.GetNumVertices();
			const int32 NumIndicies = IndexBuffer.GetNumIndices();

			for (int32 VertIndex = 0; VertIndex < NumVerts; VertIndex++)
			{
				const FVector va = PosVertexBuffer.VertexPosition(VertIndex);
				Vertices.Add(va);
			}

			for (int32 Index = 0; Index < NumIndicies; Index++)
			{
				const uint32 index = IndexBuffer.GetIndex(Index);
				Indices.Add(index);
			}

			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale(), Vertices, Indices, custom, Active, TotalMask);
			break;
		}
		case EMaskType::Cube:
		{
			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale() * 2.f * BoxExtent, Vertices, Indices, cube, Active, TotalMask);
			break;
		}
		case EMaskType::Plane:
		{
			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation() + FVector(0.0,0.0, GetComponentScale().Z * BoxExtent.Z), FQuat(GetComponentQuat()), GetComponentScale() * 2.f * BoxExtent, Vertices, Indices, plane, Active, TotalMask);
			break;
		}
		default:
			break;
	}
}

void UTrueSkyWaterMaskComponent::PostLoad()
{
	//if(!maskObjectCreated)
	/*
	switch (ObjectType)
	{
	case EMaskType::Model:
	{
		if (CustomMesh == nullptr)
		{
			UE_LOG(TrueSky, Warning, TEXT("Attempting to use a water mask with a custom mask model without setting a model"));
			return;
		}

		UStaticMesh* Mesh = CustomMesh;

		Vertices.Empty();
		Indices.Empty();

		const FStaticMeshLODResources& LOD = Mesh->RenderData->LODResources[0];
		const FPositionVertexBuffer& PosVertexBuffer = LOD.VertexBuffers.PositionVertexBuffer;
		const FRawStaticIndexBuffer& IndexBuffer = LOD.IndexBuffer;

		const int32 NumVerts = LOD.GetNumVertices();
		const int32 NumIndicies = IndexBuffer.GetNumIndices();

		for (int32 VertIndex = 0; VertIndex < NumVerts; VertIndex++)
		{
			const FVector va = PosVertexBuffer.VertexPosition(VertIndex);
			Vertices.Add(va);
		}

		for (int32 Index = 0; Index < NumIndicies; Index++)
		{
			const uint32 index = IndexBuffer.GetIndex(Index);
			Indices.Add(index);
		}

		maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale(), Vertices, Indices, custom, Active, TotalMask);
		break;
	}
	case EMaskType::Cube:
	{
		maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale()* 2.f * BoxExtent, Vertices, Indices, cube, Active, TotalMask);
		break;
	}
	case EMaskType::Plane:
	{
		maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation() + FVector(0.0, 0.0, GetComponentScale().Z * BoxExtent.Z), FQuat(GetComponentQuat()), GetComponentScale()* 2.f * BoxExtent, Vertices, Indices, plane, Active, TotalMask);
		break;
	}
	default:
		break;
	}*/
	Super::PostLoad();
}

void UTrueSkyWaterMaskComponent::BeginPlay()
{
	Super::BeginPlay();
	if(!maskObjectCreated)
	switch (ObjectType)
	{
		case EMaskType::Model:
		{
			/*
			if (CustomMesh == nullptr)
			{
				UE_LOG(TrueSky, Warning, TEXT("Attempting to use a water mask with a custom mask model without setting a model"));
				return;
			}

			UStaticMesh* Mesh = CustomMesh;

			Vertices.Empty();
			Indices.Empty();

			const FStaticMeshLODResources& LOD = Mesh->RenderData->LODResources[0];
			const FPositionVertexBuffer& PosVertexBuffer = LOD.VertexBuffers.PositionVertexBuffer;
			const FRawStaticIndexBuffer& IndexBuffer = LOD.IndexBuffer;

			const int32 NumVerts = LOD.GetNumVertices();
			const int32 NumIndicies = IndexBuffer.GetNumIndices();

			if (NumVerts > 0)
				for (int32 VertIndex = 0; VertIndex < NumVerts; VertIndex++)
				{
					const FVector va = PosVertexBuffer.VertexPosition(VertIndex);
					Vertices.Add(va);
				}

			if(NumIndicies > 0)
				for (int32 Index = 0; Index < NumIndicies; Index++)
				{
					const uint32 index = IndexBuffer.GetIndex(Index);
					Indices.Add(index);
				}

			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale(), Vertices, Indices, custom, Active, TotalMask);
			*/break;
		}
		case EMaskType::Cube:
		{
			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale()* 2.f * BoxExtent, Vertices, Indices, cube, Active, TotalMask);
			break;
		}
		case EMaskType::Plane:
		{
			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation() + FVector(0.0, 0.0, GetComponentScale().Z * BoxExtent.Z), FQuat(GetComponentQuat()), GetComponentScale()* 2.f * BoxExtent, Vertices, Indices, plane, Active, TotalMask);
			break;
		}
		default:
			break;
	}

}

void UTrueSkyWaterMaskComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ITrueSkyPlugin::Get().RemoveWaterMaskObject(ID);
	maskObjectCreated = false;
}

void UTrueSkyWaterMaskComponent::UpdateValues(bool newModel)
{
	if (!maskObjectCreated || newModel)
	{
		switch (ObjectType)
		{
		case EMaskType::Model:
		{
			if (CustomMesh == nullptr)
			{
				UE_LOG(TrueSky, Warning, TEXT("Attempting to use a water mask with a custom mask model without setting a model"));
				return;
			}

			UStaticMesh* Mesh = CustomMesh;

			Vertices.Empty();
			Indices.Empty();

			FStaticMeshLODResources& LOD = Mesh->RenderData->LODResources[0];
			FPositionVertexBuffer& PosVertexBuffer = LOD.VertexBuffers.PositionVertexBuffer;
			FRawStaticIndexBuffer& IndexBuffer = LOD.IndexBuffer;

			int32 NumVerts = LOD.GetNumVertices();
			int32 NumIndicies = IndexBuffer.GetNumIndices();

			for (int32 VertIndex = 0; VertIndex < NumVerts; VertIndex++)
			{
				FVector va = PosVertexBuffer.VertexPosition(VertIndex);
				Vertices.Add(va);
			}

			for (int32 Index = 0; Index < NumIndicies; Index++)
			{
				uint32 index = IndexBuffer.GetIndex(Index);
				Indices.Add(index);
			}

			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale(), Vertices, Indices, custom, Active, TotalMask);
			break;
		}
		case EMaskType::Cube:
		{
			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale()* 2.f * BoxExtent, Vertices, Indices, cube, Active, TotalMask);
			break;
		}
		case EMaskType::Plane:
		{
			maskObjectCreated = ITrueSkyPlugin::Get().AddWaterMaskObject(ID, GetComponentLocation() + FVector(0.0, 0.0, GetComponentScale().Z * BoxExtent.Z), FQuat(GetComponentQuat()), GetComponentScale()* 2.f * BoxExtent, Vertices, Indices, plane, Active, TotalMask);
			break;
		}
		default:
			break;
		}
	}
	else
	{
		switch (ObjectType)
		{
			case EMaskType::Model:
			{
				if (CustomMesh != nullptr)
					ITrueSkyPlugin::Get().UpdateWaterMaskObjectValues(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale(), custom, Active, TotalMask);
				break;
			}
			case EMaskType::Cube:
			{
				ITrueSkyPlugin::Get().UpdateWaterMaskObjectValues(ID, GetComponentLocation(), FQuat(GetComponentQuat()), GetComponentScale()* 2.f * BoxExtent, cube, Active, TotalMask);
				break;
			}
			case EMaskType::Plane:
			{
				ITrueSkyPlugin::Get().UpdateWaterMaskObjectValues(ID, GetComponentLocation() + FVector(0.0, 0.0, GetComponentScale().Z * BoxExtent.Z), FQuat(GetComponentQuat()), GetComponentScale()* 2.f * BoxExtent, plane, Active, TotalMask);
				break;
			}
			default:
				break;
		}
	}
}

void UTrueSkyWaterMaskComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateValues(false);
}

void UTrueSkyWaterMaskComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransform, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransform, Teleport);

	UpdateValues(false);
}

#if WITH_EDITOR
void UTrueSkyWaterMaskComponent::PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateValues(PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UTrueSkyWaterMaskComponent, CustomMesh));
}

void UTrueSkyWaterMaskComponent::PostDuplicate(bool bDuplicateForPIE)
{
	ITrueSkyPlugin::Get().RemoveWaterMaskObject(ID);
	maskObjectCreated = false;
}

bool UTrueSkyWaterMaskComponent::CanEditChange(const UProperty* InProperty) const
{
	if (InProperty)
	{
		FString PropertyName = InProperty->GetName();

		if (!Active && (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(UTrueSkyWaterMaskComponent, CustomMesh) ||
						PropertyName == GET_MEMBER_NAME_STRING_CHECKED(UTrueSkyWaterMaskComponent, TotalMask) ||
						PropertyName == GET_MEMBER_NAME_STRING_CHECKED(UTrueSkyWaterMaskComponent, ObjectType)))
			return false;

		if ((ObjectType != EMaskType::Model) && PropertyName == GET_MEMBER_NAME_STRING_CHECKED(UTrueSkyWaterMaskComponent, CustomMesh))
			return false;
	}

	return(Super::CanEditChange(InProperty));
}
#endif

#undef LOCTEXT_NAMESPACE