// Copyright Epic Games, Inc. All Rights Reserved.

#include "DragonFootSolverEditMode.h"
#include "AnimGraphNode_DragonFeetSolver.h"
#include "IPersonaPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"

#include "SceneManagement.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"



const FEditorModeID DragonFootSolverEditModes::DragonFootSolver("AnimGraph.DragonControl.DragonFootSolver");


void FDragonFootSolverEditMode::EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode)
{
	RuntimeNode = static_cast<FAnimNode_DragonFeetSolver*>(InRuntimeNode);
	GraphNode = CastChecked<UAnimGraphNode_DragonFeetSolver>(InEditorNode);

	FDragonControlBaseEditMode::EnterMode(InEditorNode, InRuntimeNode);
}

void FDragonFootSolverEditMode::ExitMode()
{
	RuntimeNode = nullptr;
	GraphNode = nullptr;

	FDragonControlBaseEditMode::ExitMode();
}

FVector FDragonFootSolverEditMode::GetWidgetLocation() const
{
	USkeletalMeshComponent* SkelComp = GetAnimPreviewScene().GetPreviewMeshComponent();

	if (RuntimeNode->knee_Animated_transform_array.Num() > spine_index)
	{
		if (RuntimeNode->dragon_input_data.FeetBones.Num() > spine_index)
		{
			return (RuntimeNode->knee_Animated_transform_array[spine_index].GetLocation() + RuntimeNode->dragon_input_data.FeetBones[spine_index].Knee_Direction_Offset);
		}
	}

	return FVector(0,0,0);
}


FWidget::EWidgetMode FDragonFootSolverEditMode::GetWidgetMode() const
{
	return FWidget::WM_Translate;
}







struct HDragonFootSolverHandleHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()

	int32 spine_index;

	int32 foot_index;

	HDragonFootSolverHandleHitProxy(int32 spine_index_in, int32 foot_index_in): HHitProxy(HPP_World), spine_index(spine_index_in), foot_index(foot_index_in)
	{
	}

	// HHitProxy interface
	virtual EMouseCursor::Type GetMouseCursor() override { return EMouseCursor::CardinalCross; }
	// End of HHitProxy interface
};

IMPLEMENT_HIT_PROXY(HDragonFootSolverHandleHitProxy, HHitProxy)






void FDragonFootSolverEditMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	UDebugSkelMeshComponent* SkelComp = GetAnimPreviewScene().GetPreviewMeshComponent();


	for (int32 i = 0; i < RuntimeNode->dragon_input_data.FeetBones.Num(); i++)
	{			

			UMaterialInstanceDynamic* Knee_Mat = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
			Knee_Mat->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Cyan));

			const FMaterialRenderProxy* SphereMaterialProxy = Knee_Mat->GetRenderProxy();


			PDI->SetHitProxy(new HDragonFootSolverHandleHitProxy(i, 0));

			FTransform StartTransform = FTransform::Identity;
			if (RuntimeNode->knee_Animated_transform_array.Num() > i)
			{
				StartTransform = RuntimeNode->knee_Animated_transform_array[i];
				StartTransform.AddToTranslation(RuntimeNode->dragon_input_data.FeetBones[i].Knee_Direction_Offset);

				const float Scale = View->WorldToScreen(StartTransform.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);
				DrawSphere(PDI, StartTransform.GetLocation(), FRotator::ZeroRotator, FVector(8.0f) * Scale, 64, 64, SphereMaterialProxy, SDPG_Foreground);
				DrawDashedLine(PDI, RuntimeNode->knee_Animated_transform_array[i].GetLocation(), StartTransform.GetLocation(), FLinearColor::Black, 2, 5);
			}		
	}

	RuntimeNode->ConditionalDebugDraw(PDI, SkelComp);
	PDI->SetHitProxy(nullptr);
}




bool FDragonFootSolverEditMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	bool bResult = FDragonControlBaseEditMode::HandleClick(InViewportClient, HitProxy, Click);

	if (HitProxy != nullptr && HitProxy->IsA(HDragonFootSolverHandleHitProxy::StaticGetType()))
	{
		HDragonFootSolverHandleHitProxy* HandleHitProxy = static_cast<HDragonFootSolverHandleHitProxy*>(HitProxy);
		spine_index = HandleHitProxy->spine_index;
		foot_index = HandleHitProxy->foot_index;

		bResult = true;
	}

	return bResult;
}



FName FDragonFootSolverEditMode::GetSelectedBone() const
{

	if ((GraphNode->ik_node.dragon_input_data.FeetBones.Num() > spine_index))
		return GraphNode->ik_node.dragon_input_data.FeetBones[spine_index].Feet_Bone_Name;
	else
		return "None";

	//return GraphNode->;
}

// @todo: will support this since now we have LookAtLocation
void FDragonFootSolverEditMode::DoTranslation(FVector& InTranslation)
{

	if (RuntimeNode->feet_knee_offset_array.Num() > spine_index && (RuntimeNode->dragon_input_data.FeetBones.Num() > spine_index) && (GraphNode->ik_node.dragon_input_data.FeetBones.Num() > spine_index) )
	{
		RuntimeNode->dragon_input_data.FeetBones[spine_index].Knee_Direction_Offset += InTranslation;
		GraphNode->ik_node.dragon_input_data.FeetBones[spine_index].Knee_Direction_Offset += InTranslation;

	}

}