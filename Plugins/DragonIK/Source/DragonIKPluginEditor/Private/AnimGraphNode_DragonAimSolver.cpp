/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#include "AnimGraphNode_DragonAimSolver.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "AnimNodeEditModes.h"

#include "Animation/AnimInstance.h"
#include "UObject/AnimPhysObjectVersion.h"

#include "DragonAimSolverEditMode.h"


#include "AnimationGraphSchema.h"



void UAnimGraphNode_DragonAimSolver::CreateOutputPins()
{
	CreatePin(EGPD_Output, UAnimationGraphSchema::PC_Struct, FComponentSpacePoseLink::StaticStruct(), TEXT("Pose"));
}


UAnimGraphNode_DragonAimSolver::UAnimGraphNode_DragonAimSolver(const FObjectInitializer & ObjectInitializer)
{
}

FEditorModeID UAnimGraphNode_DragonAimSolver::GetEditorMode() const
{
	return DragonAimSolverEditModes::DragonAimSolver;
}

//FEditorModeID UAnimGraphNode_DragonAimSolver::GetEditorMode() const
//{
//	return AnimNodeEditModes::ModifyBone;
//}



FText UAnimGraphNode_DragonAimSolver::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(FString("Dragon Aim Look Solver"));
}

FText UAnimGraphNode_DragonAimSolver::GetTooltipText() const
{
	return FText::FromString(FString("Responsible for handling aiming towards a specific target."));
}

FString UAnimGraphNode_DragonAimSolver::GetNodeCategory() const
{
	return FString("Dragon.IK Plugin");
}

FLinearColor UAnimGraphNode_DragonAimSolver::GetNodeTitleColor() const
{
	return FLinearColor(10.0f / 255.0f, 127.0f / 255.0f, 248.0f / 255.0f);

}


