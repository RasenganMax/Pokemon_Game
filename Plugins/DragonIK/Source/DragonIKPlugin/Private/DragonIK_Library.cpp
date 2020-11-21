/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#include "DragonIK_Library.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimationRuntime.h"

#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"



UDragonIK_Library * UDragonIK_Library::Constructor()
{
	return nullptr;
}



FRotator UDragonIK_Library::LookAt_Function(FTransform transform, FVector worldPosition, FVector up_direction, FVector rotationOffset)
{
	FVector diff = (worldPosition - transform.GetLocation()).GetSafeNormal();

	FRotator rotation = CustomLookRotation(diff, up_direction);

	//FRotator result = FRotator(rotation.Quaternion() * FQuat::MakeFromEuler(rotationOffset));

	return rotation;

}






void UDragonIK_Library::Evaluate_TwoBoneIK_Spinal_Modified(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel, FBoneReference FeetBone, FBoneReference KneeBone, FBoneReference ThighBone, FTransform ThighTransform, FTransform Shoulder_Trans, FTransform Knee_Trans, FTransform Hand_Trans, FVector JointLocation, FVector Knee_Pole_Offset, TArray<FBoneTransform>& OutBoneTransforms)
{



	//check(OutBoneTransforms.Num() == 0);

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidLimb = false;

	FCompactPoseBoneIndex IKBoneCompactPoseIndex = FeetBone.GetCompactPoseIndex(BoneContainer);

	FCompactPoseBoneIndex CachedLowerLimbIndex = KneeBone.GetCompactPoseIndex(BoneContainer);

	FCompactPoseBoneIndex CachedUpperLimbIndex = ThighBone.GetCompactPoseIndex(BoneContainer);



	// Get Local Space transforms for our bones. We do this first in case they already are local.
	// As right after we get them in component space. (And that does the auto conversion).
	// We might save one transform by doing local first...
	const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
	const FTransform LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedLowerLimbIndex);
	const FTransform UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedUpperLimbIndex);

	// Now get those in component space...
//	FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex) * transform_offset;
//	FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex) * transform_offset;
//	FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex) * transform_offset;


	FTransform LowerLimbCSTransform = Knee_Trans;
	FTransform UpperLimbCSTransform = Shoulder_Trans;
	FTransform EndBoneCSTransform = Hand_Trans;






	const FTransform EndBoneCSTransform_Const = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);


	FTransform EndBoneCSTransform_Always = EndBoneCSTransform_Const;



	EndBoneCSTransform.SetRotation(EndBoneCSTransform_Const.GetRotation());


	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();

	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform EffectorTransform = EndBoneCSTransform;





	//FAnimationRuntime::ConvertBoneSpaceTransformToCS(InComponentTransform, MeshBases, OutTransform, InTarget.GetCompactPoseBoneIndex(), EBoneControlSpace::BCS_WorldSpace);


	// Get joint target (used for defining plane that joint should be in).
	FTransform JointTargetTransform = LowerLimbCSTransform;



	//FVector	JointTargetPos = JointLocation;


	FVector cs_forward = (((UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) - LowerLimbCSTransform.GetLocation()).GetSafeNormal();

	//BendingDirectionTransform.SetLocation(BendingDirectionTransform.GetLocation() + cs_forward *-1000);

	JointTargetTransform.SetLocation(JointTargetTransform.GetLocation() + (cs_forward * -1000));
	JointTargetTransform.AddToTranslation(Knee_Pole_Offset);

	FVector	JointTargetPos = JointTargetTransform.GetTranslation();


	// IK solver
	UpperLimbCSTransform.SetLocation(RootPos);
	//	UpperLimbCSTransform.SetLocation(ThighTransform.GetLocation());

	LowerLimbCSTransform.SetLocation(InitialJointPos);
	EndBoneCSTransform.SetLocation(InitialEndPos);


	// This is our reach goal.
	//FVector DesiredPos = UpperLimbCSTransform.GetTranslation();
	//	FVector DesiredPos = owning_skel->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());
	FVector DesiredPos = ThighTransform.GetLocation();

	//	FVector Far_Target = (DesiredPos - Common_Spine_Modified_Transform.GetLocation()).GetSafeNormal()*100000;


	FVector DesiredThighPos = owning_skel->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());



	Solve_Modified_Direct_TwoBoneIK_4(UpperLimbCSTransform, LowerLimbCSTransform, EndBoneCSTransform, JointTargetPos, DesiredThighPos, false, 1, 1);



	// Update transform for upper bone.
	{
		// Order important. First bone is upper limb.
		OutBoneTransforms.Add(FBoneTransform(CachedUpperLimbIndex, UpperLimbCSTransform));
	}

	// Update transform for lower bone.
	{
		// Order important. Second bone is lower limb.
			OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, LowerLimbCSTransform));
	}

	// Update transform for end bone.
	{

		//if (bMaintainEffectorRelRot)
		{
			EndBoneCSTransform = EndBoneLocalTransform * LowerLimbCSTransform;
		}

		
		OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));

	}

	// Make sure we have correct number of bones
	//check(OutBoneTransforms.Num() == 3);



}




void UDragonIK_Library::Evaluate_TwoBoneIK_Direct_Modified(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel, FBoneReference FeetBone, FBoneReference KneeBone, FBoneReference ThighBone, FTransform ThighTransform, FTransform Shoulder_Trans, FTransform Knee_Trans, FTransform Hand_Trans, FVector JointLocation, FVector Knee_Pole_Offset,FTransform transform_offset,FTransform Common_Spine_Modified_Transform, FRotator Limb_Rotation_Offset, FVector HandAxis,bool is_relative_axis, float Hand_Clamp_Value,bool accurate_hand_rotation, FTransform Extra_Hand_Offset, TArray<FBoneTransform>& OutBoneTransforms)
{



	//check(OutBoneTransforms.Num() == 0);

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidLimb = false;

	FCompactPoseBoneIndex IKBoneCompactPoseIndex = FeetBone.GetCompactPoseIndex(BoneContainer);

	FCompactPoseBoneIndex CachedLowerLimbIndex = KneeBone.GetCompactPoseIndex(BoneContainer);

	FCompactPoseBoneIndex CachedUpperLimbIndex = ThighBone.GetCompactPoseIndex(BoneContainer);



	// Get Local Space transforms for our bones. We do this first in case they already are local.
	// As right after we get them in component space. (And that does the auto conversion).
	// We might save one transform by doing local first...
	const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
	const FTransform LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedLowerLimbIndex);
	const FTransform UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedUpperLimbIndex);

	// Now get those in component space...
//	FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex) * transform_offset;
//	FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex) * transform_offset;
//	FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex) * transform_offset;


	FTransform LowerLimbCSTransform = Knee_Trans;
	FTransform UpperLimbCSTransform = Shoulder_Trans;
	FTransform EndBoneCSTransform = Hand_Trans;






	const FTransform EndBoneCSTransform_Const = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);


	FTransform EndBoneCSTransform_Always = EndBoneCSTransform_Const;



	EndBoneCSTransform.SetRotation(EndBoneCSTransform_Const.GetRotation());


	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();

	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform EffectorTransform = EndBoneCSTransform;


	


	//FAnimationRuntime::ConvertBoneSpaceTransformToCS(InComponentTransform, MeshBases, OutTransform, InTarget.GetCompactPoseBoneIndex(), EBoneControlSpace::BCS_WorldSpace);


	// Get joint target (used for defining plane that joint should be in).
	FTransform JointTargetTransform = LowerLimbCSTransform;



	//FVector	JointTargetPos = JointLocation;


	FVector cs_forward = (((UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) - LowerLimbCSTransform.GetLocation()).GetSafeNormal();

	//BendingDirectionTransform.SetLocation(BendingDirectionTransform.GetLocation() + cs_forward *-1000);

	JointTargetTransform.SetLocation(JointTargetTransform.GetLocation() + (cs_forward * -1000));
	JointTargetTransform.AddToTranslation(Knee_Pole_Offset);

	FVector	JointTargetPos = JointTargetTransform.GetTranslation();


	// IK solver
	UpperLimbCSTransform.SetLocation(RootPos);
	//	UpperLimbCSTransform.SetLocation(ThighTransform.GetLocation());

	LowerLimbCSTransform.SetLocation(InitialJointPos);
	EndBoneCSTransform.SetLocation(InitialEndPos);


	// This is our reach goal.
	//FVector DesiredPos = UpperLimbCSTransform.GetTranslation();
	//	FVector DesiredPos = owning_skel->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());
	FVector DesiredPos = ThighTransform.GetLocation();

//	FVector Far_Target = (DesiredPos - Common_Spine_Modified_Transform.GetLocation()).GetSafeNormal()*100000;

	FVector Far_Target = Common_Spine_Modified_Transform.GetLocation();


	FVector DesiredThighPos = owning_skel->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());



	Solve_Modified_Direct_TwoBoneIK_4(UpperLimbCSTransform, LowerLimbCSTransform, EndBoneCSTransform, JointTargetPos, DesiredPos, false, 1, 1);


	bool rotate_everything = true;



	// Update transform for upper bone.
	{
		// Order important. First bone is upper limb.
		
		OutBoneTransforms.Add(FBoneTransform(CachedUpperLimbIndex, UpperLimbCSTransform));
	}

	// Update transform for lower bone.
	{
		// Order important. Second bone is lower limb.
		if (rotate_everything)
		OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, LowerLimbCSTransform));
	}

	// Update transform for end bone.
	{

		//if (bMaintainEffectorRelRot)

		if(Extra_Hand_Offset.Equals(FTransform::Identity))
		{
			EndBoneCSTransform = EndBoneLocalTransform * LowerLimbCSTransform;
		}
		else
		EndBoneCSTransform.SetRotation(Extra_Hand_Offset.GetRotation() * EndBoneCSTransform.GetRotation());


		//EndBoneCSTransform.SetRotation();

		if (rotate_everything)
		{


			if (!accurate_hand_rotation)
			{
				OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));
			}
			else
			{
				FAxis LookAt_Axis;
				LookAt_Axis.bInLocalSpace = !is_relative_axis;
				LookAt_Axis.Axis = HandAxis;

				EndBoneCSTransform_Always.SetLocation(EndBoneCSTransform.GetLocation());

				FTransform EndBoneCSTransform_Output = LookAt_Processor(EndBoneCSTransform_Always, EndBoneCSTransform_Always.GetLocation(), Far_Target, LookAt_Axis, Hand_Clamp_Value,true, 1,1);

				//	EndBoneCSTransform_Output.SetRotation(EndBoneCSTransform_Output.GetRotation() * Limb_Rotation_Offset.Quaternion());

					// Order important. Third bone is End Bone.
				OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform_Output));
			}

		}

	}

	// Make sure we have correct number of bones
	//check(OutBoneTransforms.Num() == 3);



}




void Solve_Modified_Direct_TwoBoneIK_4(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale)
{
	float LowerLimbLength = (InOutEndTransform.GetLocation() - InOutJointTransform.GetLocation()).Size();
	float UpperLimbLength = (InOutJointTransform.GetLocation() - InOutRootTransform.GetLocation()).Size();
	Solve_Modified_Direct_TwoBoneIK_3(InOutRootTransform, InOutJointTransform, InOutEndTransform, JointTarget, Effector, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);
}

void Solve_Modified_Direct_TwoBoneIK_3(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale)
{
	FVector OutJointPos, OutEndPos;

	FVector RootPos = InOutRootTransform.GetLocation();
	FVector JointPos = InOutJointTransform.GetLocation();
	FVector EndPos = InOutEndTransform.GetLocation();

	// IK solver
	Solve_Modified_Direct_TwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, OutJointPos, OutEndPos, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);

	// Update transform for upper bone.
	{
		// Get difference in direction for old and new joint orientations
		FVector const OldDir = (JointPos - RootPos).GetSafeNormal();
		FVector const NewDir = (OutJointPos - RootPos).GetSafeNormal();
		// Find Delta Rotation take takes us from Old to New dir
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation
		InOutRootTransform.SetRotation(DeltaRotation * InOutRootTransform.GetRotation());
		// And put joint where it should be.
		InOutRootTransform.SetTranslation(RootPos);

	}

	// update transform for middle bone
	{
		// Get difference in direction for old and new joint orientations
		FVector const OldDir = (EndPos - JointPos).GetSafeNormal();
		FVector const NewDir = (OutEndPos - OutJointPos).GetSafeNormal();

		// Find Delta Rotation take takes us from Old to New dir
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation
		InOutJointTransform.SetRotation(DeltaRotation * InOutJointTransform.GetRotation());
		// And put joint where it should be.
		InOutJointTransform.SetTranslation(OutJointPos);

	}

	// Update transform for end bone.
	// currently not doing anything to rotation
	// keeping input rotation
	// Set correct location for end bone.
	InOutEndTransform.SetTranslation(OutEndPos);
}

void Solve_Modified_Direct_TwoBoneIK_2(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale)
{
	float LowerLimbLength = (EndPos - JointPos).Size();
	float UpperLimbLength = (JointPos - RootPos).Size();

	Solve_Modified_Direct_TwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, OutJointPos, OutEndPos, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);
}

void Solve_Modified_Direct_TwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale)
{
	// This is our reach goal.
	FVector DesiredPos = Effector;
	FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();

	// Find lengths of upper and lower limb in the ref skeleton.
	// Use actual sizes instead of ref skeleton, so we take into account translation and scaling from other bone controllers.
	float MaxLimbLength = LowerLimbLength + UpperLimbLength;

	// Check to handle case where DesiredPos is the same as RootPos.
	FVector	DesiredDir;
	if (DesiredLength < (float)KINDA_SMALL_NUMBER)
	{
		DesiredLength = (float)KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1, 0, 0);
	}
	else
	{
		DesiredDir = DesiredDelta.GetSafeNormal();
	}

	// Get joint target (used for defining plane that joint should be in).
	FVector JointTargetDelta = JointTarget - RootPos;
	const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

	// Same check as above, to cover case when JointTarget position is the same as RootPos.
	FVector JointPlaneNormal, JointBendDir;
	if (JointTargetLengthSqr < FMath::Square((float)KINDA_SMALL_NUMBER))
	{
		JointBendDir = FVector(0, 1, 0);
		JointPlaneNormal = FVector(0, 0, 1);
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;

		// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
		// we have to just pick 2 random vector perp to DesiredDir and each other.
		if (JointPlaneNormal.SizeSquared() < FMath::Square((float)KINDA_SMALL_NUMBER))
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();

			// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
			// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
			JointBendDir.Normalize();
		}
	}

	//UE_LOG(LogAnimationCore, Log, TEXT("UpperLimb : %0.2f, LowerLimb : %0.2f, MaxLimb : %0.2f"), UpperLimbLength, LowerLimbLength, MaxLimbLength);

	if (bAllowStretching)
	{
		const float ScaleRange = MaxStretchScale - StartStretchRatio;
		if (ScaleRange > KINDA_SMALL_NUMBER&& MaxLimbLength > KINDA_SMALL_NUMBER)
		{
			const float ReachRatio = DesiredLength / MaxLimbLength;
			const float ScalingFactor = (MaxStretchScale - 1.f) * FMath::Clamp<float>((ReachRatio - StartStretchRatio) / ScaleRange, 0.f, 1.f);
			if (ScalingFactor > KINDA_SMALL_NUMBER)
			{
				LowerLimbLength *= (1.f + ScalingFactor);
				UpperLimbLength *= (1.f + ScalingFactor);
				MaxLimbLength *= (1.f + ScalingFactor);
			}
		}
	}

	OutEndPos = DesiredPos;
	OutJointPos = JointPos;

	// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
	if (DesiredLength >= MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
		OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
	}
	else
	{
		// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
		// using the sin rule:
		const float TwoAB = 2.f * UpperLimbLength * DesiredLength;

		const float CosAngle = (TwoAB != 0.f) ? ((UpperLimbLength * UpperLimbLength) + (DesiredLength * DesiredLength) - (LowerLimbLength * LowerLimbLength)) / TwoAB : 0.f;

		// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
		const bool bReverseUpperBone = (CosAngle < 0.f);

		// Angle between upper limb and DesiredDir
		// ACos clamps internally so we dont need to worry about out-of-range values here.
		const float Angle = FMath::Acos(CosAngle);

		// Now we calculate the distance of the joint from the root -> effector line.
		// This forms a right-angle triangle, with the upper limb as the hypotenuse.
		const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);

		// And the final side of that triangle - distance along DesiredDir of perpendicular.
		// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
		const float ProjJointDistSqr = (UpperLimbLength * UpperLimbLength) - (JointLineDist * JointLineDist);
		// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
		// so now I branch it. 						
		float ProjJointDist = (ProjJointDistSqr > 0.f) ? FMath::Sqrt(ProjJointDistSqr) : 0.f;
		if (bReverseUpperBone)
		{
			ProjJointDist *= -1.f;
		}

		// So now we can work out where to put the joint!
		OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
	}
}







void UDragonIK_Library::Evaluate_TwoBoneIK_Modified(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel, FBoneReference FeetBone, FBoneReference KneeBone, FBoneReference ThighBone, FTransform ThighTransform,FVector JointLocation,FVector Knee_Pole_Offset, TArray<FBoneTransform>& OutBoneTransforms)
{



	//check(OutBoneTransforms.Num() == 0);

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidLimb = false;

	FCompactPoseBoneIndex IKBoneCompactPoseIndex = FeetBone.GetCompactPoseIndex(BoneContainer);

	FCompactPoseBoneIndex CachedLowerLimbIndex = KneeBone.GetCompactPoseIndex(BoneContainer);

	FCompactPoseBoneIndex CachedUpperLimbIndex = ThighBone.GetCompactPoseIndex(BoneContainer);



	// Get Local Space transforms for our bones. We do this first in case they already are local.
	// As right after we get them in component space. (And that does the auto conversion).
	// We might save one transform by doing local first...
	const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
	const FTransform LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedLowerLimbIndex);
	const FTransform UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedUpperLimbIndex);

	// Now get those in component space...
	FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex);
	FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex);

//	FTransform UpperLimbCSTransform = ThighTransform;	

	FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();

	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform EffectorTransform = EndBoneCSTransform;

	//FAnimationRuntime::ConvertBoneSpaceTransformToCS(InComponentTransform, MeshBases, OutTransform, InTarget.GetCompactPoseBoneIndex(), EBoneControlSpace::BCS_WorldSpace);


	// Get joint target (used for defining plane that joint should be in).
	FTransform JointTargetTransform = LowerLimbCSTransform;

	

	//FVector	JointTargetPos = JointLocation;

/*
	FVector cs_forward = (((UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) - LowerLimbCSTransform.GetLocation()).GetSafeNormal();
	JointTargetTransform.SetLocation(JointTargetTransform.GetLocation() + (cs_forward * -1000) );
	JointTargetTransform.AddToTranslation(Knee_Pole_Offset);
*/

	FVector cs_forward = (((UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) - (LowerLimbCSTransform.GetLocation() + Knee_Pole_Offset)).GetSafeNormal();

	JointTargetTransform.SetLocation(JointTargetTransform.GetLocation() + cs_forward * -1000);


	FVector	JointTargetPos = JointTargetTransform.GetTranslation();
	

	// IK solver
	UpperLimbCSTransform.SetLocation(RootPos);
//	UpperLimbCSTransform.SetLocation(ThighTransform.GetLocation());

	LowerLimbCSTransform.SetLocation(InitialJointPos);
	EndBoneCSTransform.SetLocation(InitialEndPos);


	// This is our reach goal.
	//FVector DesiredPos = UpperLimbCSTransform.GetTranslation();
	//	FVector DesiredPos = owning_skel->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());
	FVector DesiredPos = ThighTransform.GetLocation();

	FVector DesiredThighPos = owning_skel->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());



	Solve_Modified_TwoBoneIK_4(EndBoneCSTransform, LowerLimbCSTransform,  UpperLimbCSTransform, JointTargetPos, DesiredPos, DesiredThighPos,false, 1, 1);


	// Update transform for upper bone.
	{
		// Order important. First bone is upper limb.
		OutBoneTransforms.Add(FBoneTransform(CachedUpperLimbIndex, UpperLimbCSTransform));
	}

	// Update transform for lower bone.
	{
		// Order important. Second bone is lower limb.
		OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, LowerLimbCSTransform));
	}

	// Update transform for end bone.
	{
		
		//if (bMaintainEffectorRelRot)
		{
	//		EndBoneCSTransform = EndBoneLocalTransform * LowerLimbCSTransform;
		}
		// Order important. Third bone is End Bone.
		OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));
	}

	// Make sure we have correct number of bones
	//check(OutBoneTransforms.Num() == 3);



}





void Solve_Modified_TwoBoneIK_4(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector,const FVector& ThighEffector, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale)
{
	float LowerLimbLength = (InOutEndTransform.GetLocation() - InOutJointTransform.GetLocation()).Size();
	float UpperLimbLength = (InOutJointTransform.GetLocation() - InOutRootTransform.GetLocation()).Size();
	Solve_Modified_TwoBoneIK_3(InOutRootTransform, InOutJointTransform, InOutEndTransform, JointTarget, Effector, ThighEffector, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);
}

void Solve_Modified_TwoBoneIK_3(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, const FVector& ThighEffector, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale)
{
	FVector OutJointPos, OutEndPos;

	FVector RootPos = InOutRootTransform.GetLocation();
	FVector JointPos = InOutJointTransform.GetLocation();
	FVector EndPos = InOutEndTransform.GetLocation();


	FTransform const Const_RootPos = InOutRootTransform;



	// IK solver
	Solve_Modified_TwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, ThighEffector, OutJointPos, OutEndPos, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);

	//Solve_Modified_TwoBoneIK(EndPos, JointPos,  RootPos, JointTarget, Effector, OutJointPos, OutEndPos, LowerLimbLength , UpperLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);


	// Update transform for upper bone.
	{
		// Get difference in direction for old and new joint orientations
		FVector const OldDir = (JointPos - RootPos).GetSafeNormal();
		FVector const NewDir = (OutJointPos - RootPos).GetSafeNormal();
		// Find Delta Rotation take takes us from Old to New dir
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation
	//	InOutRootTransform.SetRotation(DeltaRotation * InOutRootTransform.GetRotation());
		// And put joint where it should be.
		InOutRootTransform.SetTranslation(RootPos);

	//	InOutRootTransform.SetTranslation(Const_RootPos.GetLocation());
	//	InOutRootTransform.SetRotation(Const_RootPos.GetRotation());


	}

	// update transform for middle bone
	{
		// Get difference in direction for old and new joint orientations
	//	FVector const OldDir = (EndPos - JointPos).GetSafeNormal();
	//	FVector const NewDir = (OutEndPos - OutJointPos).GetSafeNormal();


		FVector const OldDir = (RootPos - JointPos).GetSafeNormal();
		FVector const NewDir = (RootPos - OutJointPos).GetSafeNormal();

		// Find Delta Rotation take takes us from Old to New dir
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation
		InOutJointTransform.SetRotation(DeltaRotation * InOutJointTransform.GetRotation());
		// And put joint where it should be.
		InOutJointTransform.SetTranslation(OutJointPos);

	}

	// Update transform for end bone.
	// currently not doing anything to rotation
	// keeping input rotation
	// Set correct location for end bone.

	{

		FVector const OldDir = (EndPos - JointPos).GetSafeNormal();
		FVector const NewDir = (OutEndPos - OutJointPos).GetSafeNormal();

		// Find Delta Rotation take takes us from Old to New dir
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation
		InOutEndTransform.SetRotation(DeltaRotation * InOutEndTransform.GetRotation());

		InOutEndTransform.SetTranslation(OutEndPos);
	}

}

void Solve_Modified_TwoBoneIK_2(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector,const FVector& ThighEffector, FVector& OutJointPos, FVector& OutEndPos, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale)
{
	float LowerLimbLength = (EndPos - JointPos).Size();
	float UpperLimbLength = (JointPos - RootPos).Size();

	Solve_Modified_TwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, ThighEffector, OutJointPos, OutEndPos, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);
}

void Solve_Modified_TwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, const FVector& ThighEffector, FVector& OutJointPos, FVector& OutEndPos, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale)
{
	// This is our reach goal.
	FVector DesiredPos = Effector;
	FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();

	// Find lengths of upper and lower limb in the ref skeleton.
	// Use actual sizes instead of ref skeleton, so we take into account translation and scaling from other bone controllers.
	float MaxLimbLength = LowerLimbLength + UpperLimbLength;

	// Check to handle case where DesiredPos is the same as RootPos.
	FVector	DesiredDir;
	if (DesiredLength < (float)KINDA_SMALL_NUMBER)
	{
		DesiredLength = (float)KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1, 0, 0);
	}
	else
	{
		DesiredDir = DesiredDelta.GetSafeNormal();
	}

	// Get joint target (used for defining plane that joint should be in).
	FVector JointTargetDelta = JointTarget - RootPos;
	const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

	// Same check as above, to cover case when JointTarget position is the same as RootPos.
	FVector JointPlaneNormal, JointBendDir;
	if (JointTargetLengthSqr < FMath::Square((float)KINDA_SMALL_NUMBER))
	{
		JointBendDir = FVector(0, 1, 0);
		JointPlaneNormal = FVector(0, 0, 1);
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;

		// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
		// we have to just pick 2 random vector perp to DesiredDir and each other.
		if (JointPlaneNormal.SizeSquared() < FMath::Square((float)KINDA_SMALL_NUMBER))
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();

			// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
			// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
			JointBendDir.Normalize();
		}
	}

	//UE_LOG(LogAnimationCore, Log, TEXT("UpperLimb : %0.2f, LowerLimb : %0.2f, MaxLimb : %0.2f"), UpperLimbLength, LowerLimbLength, MaxLimbLength);

	if (bAllowStretching)
	{
		const float ScaleRange = MaxStretchScale - StartStretchRatio;
		if (ScaleRange > KINDA_SMALL_NUMBER&& MaxLimbLength > KINDA_SMALL_NUMBER)
		{
			const float ReachRatio = DesiredLength / MaxLimbLength;
			const float ScalingFactor = (MaxStretchScale - 1.f) * FMath::Clamp<float>((ReachRatio - StartStretchRatio) / ScaleRange, 0.f, 1.f);
			if (ScalingFactor > KINDA_SMALL_NUMBER)
			{
				LowerLimbLength *= (1.f + ScalingFactor);
				UpperLimbLength *= (1.f + ScalingFactor);
				MaxLimbLength *= (1.f + ScalingFactor);
			}
		}
	}

	OutEndPos = DesiredPos;
	OutJointPos = JointPos;

	// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
	if (DesiredLength >= MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
		OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
	}
	else
	{
		// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
		// using the sin rule:
		const float TwoAB = 2.f * UpperLimbLength * DesiredLength;

		const float CosAngle = (TwoAB != 0.f) ? ((UpperLimbLength * UpperLimbLength) + (DesiredLength * DesiredLength) - (LowerLimbLength * LowerLimbLength)) / TwoAB : 0.f;

		// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
		const bool bReverseUpperBone = (CosAngle < 0.f);

		// Angle between upper limb and DesiredDir
		// ACos clamps internally so we dont need to worry about out-of-range values here.
		const float Angle = FMath::Acos(CosAngle);

		// Now we calculate the distance of the joint from the root -> effector line.
		// This forms a right-angle triangle, with the upper limb as the hypotenuse.
		const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);

		// And the final side of that triangle - distance along DesiredDir of perpendicular.
		// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
		const float ProjJointDistSqr = (UpperLimbLength * UpperLimbLength) - (JointLineDist * JointLineDist);
		// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
		// so now I branch it. 						
		float ProjJointDist = (ProjJointDistSqr > 0.f) ? FMath::Sqrt(ProjJointDistSqr) : 0.f;
		if (bReverseUpperBone)
		{
			ProjJointDist *= -1.f;
		}

		// So now we can work out where to put the joint!
		OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
	}
}







bool UDragonIK_Library::Solve_Modified_CCDIK(TArray<FCCDIK_Modified_ChainLink>& InOutChain, const FVector& TargetPosition, float MaximumReach, float Precision, int32 MaxIteration)
{
	

	bool bBoneLocationUpdated = false;
	float const RootToTargetDistSq = FVector::DistSquared(InOutChain[0].Position, TargetPosition);
	int32 const NumChainLinks = InOutChain.Num();

	// FABRIK algorithm - bone translation calculation
	// If the effector is further away than the distance from root to tip, simply move all bones in a line from root to effector location
	if (RootToTargetDistSq > FMath::Square(MaximumReach))
	{
		for (int32 LinkIndex = 1; LinkIndex < NumChainLinks; LinkIndex++)
		{
			FCCDIK_Modified_ChainLink const& ParentLink = InOutChain[LinkIndex - 1];
			FCCDIK_Modified_ChainLink& CurrentLink = InOutChain[LinkIndex];
			CurrentLink.Position = ParentLink.Position + (TargetPosition - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length;
		}
		bBoneLocationUpdated = true;
	}
	else // Effector is within reach, calculate bone translations to position tip at effector location
	{
		int32 const TipBoneLinkIndex = NumChainLinks - 1;

		// Check distance between tip location and effector location
		float Slop = FVector::Dist(InOutChain[TipBoneLinkIndex].Position, TargetPosition);
		if (Slop > Precision)
		{
			// Set tip bone at end effector location.
			InOutChain[TipBoneLinkIndex].Position = TargetPosition;

			int32 IterationCount = 0;
			while ((Slop > Precision) && (IterationCount++ < MaxIteration))
			{
				// "Forward Reaching" stage - adjust bones from end effector.
				for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
				{
					FCCDIK_Modified_ChainLink& CurrentLink = InOutChain[LinkIndex];
					FCCDIK_Modified_ChainLink const& ChildLink = InOutChain[LinkIndex + 1];


					CurrentLink.Position = ChildLink.Position + ( (CurrentLink.Position - TargetPosition) ).GetUnsafeNormal() * ChildLink.Length;

				//	CurrentLink.Position = ChildLink.Position + ( (CurrentLink.Position - ChildLink.Position) ).GetUnsafeNormal() * ChildLink.Length;
				
				}

				// "Backward Reaching" stage - adjust bones from root.
				for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
				{
					FCCDIK_Modified_ChainLink const& ParentLink = InOutChain[LinkIndex - 1];
					FCCDIK_Modified_ChainLink& CurrentLink = InOutChain[LinkIndex];

				//	CurrentLink.Position = ParentLink.Position + ( (CurrentLink.Position - ParentLink.Position) ).GetUnsafeNormal() * CurrentLink.Length;
				
					CurrentLink.Position = ParentLink.Position + ((CurrentLink.Position - ParentLink.Position)).GetUnsafeNormal() * CurrentLink.Length;

				}

				// Re-check distance between tip location and effector location
				// Since we're keeping tip on top of effector location, check with its parent bone.
				Slop = FMath::Abs(InOutChain[TipBoneLinkIndex].Length - FVector::Dist(InOutChain[TipBoneLinkIndex - 1].Position, TargetPosition));
			}

			// Place tip bone based on how close we got to target.
			{
				FCCDIK_Modified_ChainLink const& ParentLink = InOutChain[TipBoneLinkIndex - 1];
				FCCDIK_Modified_ChainLink& CurrentLink = InOutChain[TipBoneLinkIndex];

				CurrentLink.Position = ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length;
			}

			bBoneLocationUpdated = true;
		}
	}

	return bBoneLocationUpdated;


}





/*
FTransform UDragonIK_Library::GetTargetTransform_Modified(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, FBoneSocketTarget& InTarget, const FVector& InOffset)
{
	FTransform OutTransform;
	

	{
		// parent bone space still goes through this way
		// if your target is socket, it will try find parents of joint that socket belongs to
		OutTransform.SetLocation(InOffset);
		FAnimationRuntime::ConvertBoneSpaceTransformToCS(InComponentTransform, MeshBases, OutTransform, InTarget.GetCompactPoseBoneIndex(), EBoneControlSpace::BCS_WorldSpace);
	}

	return OutTransform;
}*/


void UDragonIK_Library::Evaluate_ConsecutiveBoneRotations(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel, TArray<FDragonData_SpineFeetPair> spine_feet_pair, FRuntimeFloatCurve Look_Bending_Curve, FBoneReference RootBone_Input, FBoneReference TipBone_Input, float Lookat_Radius, FTransform EffectorTransform, FAxis LookAt_Axis, float Lookat_Clamp, float Downward_Dip_Multiplier, float Side_Multiplier, float Side_Down_Multiplier, bool alter_pelvis, FTransform PelvisLocTarget, FRuntimeFloatCurve Bending_Multiplier_Curve,float Up_Rot_Clamp,bool use_natural_rotation,  bool separate_head_clamp,float head_clamp_value  , TArray<FBoneTransform>& OutBoneTransforms)
{



	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();


	FBoneReference RootBone = FBoneReference(RootBone_Input);
	RootBone.Initialize(BoneContainer);


	FBoneReference TipBone = FBoneReference(TipBone_Input);
	TipBone.Initialize(BoneContainer);



	// Update EffectorLocation if it is based off a bone position
	//FTransform CSEffectorTransform = GetTargetTransform_Modified(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, EffectorTarget, EffectorLocation);

	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, CSEffectorTransform, RootBone.GetCompactPoseIndex(BoneContainer), EBoneControlSpace::BCS_WorldSpace);



	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();



	// Gather all bone indices between root and tip.
	TArray<FCompactPoseBoneIndex> BoneIndices;

	{
		const FCompactPoseBoneIndex RootIndex = RootBone.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex BoneIndex = TipBone.GetCompactPoseIndex(BoneContainer);
		do
		{
			BoneIndices.Insert(BoneIndex, 0);
			BoneIndex = Output.Pose.GetPose().GetParentBoneIndex(BoneIndex);
		} while (BoneIndex != RootIndex);
		BoneIndices.Insert(BoneIndex, 0);
	}

	// Maximum length of skeleton segment at full extension
	float MaximumReach = 0;

	// Gather transforms
	int32 const NumTransforms = BoneIndices.Num();
	OutBoneTransforms.AddUninitialized(NumTransforms);

	// Gather chain links. These are non zero length bones.
	TArray<FCCDIK_Modified_ChainLink> Chain;
	Chain.Reserve(NumTransforms);

	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = Output.Pose.GetComponentSpaceTransform(RootBoneIndex);

		OutBoneTransforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);
		Chain.Add(FCCDIK_Modified_ChainLink(BoneCSTransform.GetLocation(), 0.f, RootBoneIndex, 0));
	}

	// Go through remaining transforms
	for (int32 TransformIndex = 1; TransformIndex < NumTransforms; TransformIndex++)
	{
		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];

		const FTransform& BoneCSTransform = Output.Pose.GetComponentSpaceTransform(BoneIndex);
		FVector const BoneCSPosition = BoneCSTransform.GetLocation();

		OutBoneTransforms[TransformIndex] = FBoneTransform(BoneIndex, BoneCSTransform);

		// Calculate the combined length of this segment of skeleton
		float const BoneLength = FVector::Dist(BoneCSPosition, OutBoneTransforms[TransformIndex - 1].Transform.GetLocation());

		if (!FMath::IsNearlyZero(BoneLength))
		{
			Chain.Add(FCCDIK_Modified_ChainLink(BoneCSPosition, BoneLength, BoneIndex, TransformIndex));
			MaximumReach += BoneLength;
		}
		else
		{
			// Mark this transform as a zero length child of the last link.
			// It will inherit position and delta rotation from parent link.
			FCCDIK_Modified_ChainLink& ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}

	int32 const NumChainLinks = Chain.Num();



//CLAMP BACKWARDS	CSEffectorLocation.Y = FMath::Clamp<float>( CSEffectorLocation.Y, Chain[Chain.Num() - 1].Position.Y , Chain[Chain.Num() - 1].Position.Y + 1000000);



	//TArray<FRichCurveKey> SteerKeys = Look_Bending_Curve.GetRichCurve()->GetCopyOfKeys();
	
	//SteeringCurve.GetRichCurve()->UpdateOrAddKey(SteerKeys[KeyIdx].Time, NewValue);
	
	
	FTransform Head_Location = Output.Pose.GetComponentSpaceTransform(BoneIndices[BoneIndices.Num()-1]);


	FTransform Pelvis_Location = Output.Pose.GetComponentSpaceTransform(BoneIndices[0]);



	float Up_Length = 0;

	Up_Length = -(CSEffectorLocation.Z - Chain[NumChainLinks - 1].Position.Z);

	Up_Length = FMath::Clamp<float>(Up_Length, -(FMath::Abs(Chain[NumChainLinks - 1].Position.Z*0.5f)), 0);



//	FVector Diff_Pelvis = Pelvis_Location.InverseTransformPosition(PelvisLocTarget.GetLocation());

//	FVector Diff_Pelvis = (Pelvis_Location.Inverse() * PelvisLocTarget).GetLocation();

	FVector Diff_Pelvis = (Pelvis_Location.GetLocation() - PelvisLocTarget.GetLocation());

	FTransform Pelvis_Ref_FullTransform;


	for (int32 LinkIndex = 0; LinkIndex < NumChainLinks; LinkIndex++)
	{

		FCCDIK_Modified_ChainLink const& ChainLink = Chain[LinkIndex];

		float NewValue = Look_Bending_Curve.GetRichCurve()->Eval( (float)LinkIndex/ (float)NumChainLinks);

		float Multiplier_Value =  Bending_Multiplier_Curve.GetRichCurve()->Eval((float)LinkIndex / (float)NumChainLinks);


		FVector Temp_Target_Loc = CSEffectorLocation;


		if (LinkIndex == 0)
		{
			Pelvis_Ref_FullTransform = LookAt_Processor(OutBoneTransforms[0].Transform, Head_Location.GetLocation(), Temp_Target_Loc, LookAt_Axis, 80, use_natural_rotation, 1,1);
		}


		


		const FRotator Original_Rot = OutBoneTransforms[ChainLink.TransformIndex].Transform.Rotator();

		float Lookat_Modified_Clamp = Lookat_Clamp * NewValue;

		float Up_Look_Ratio = Up_Rot_Clamp;

		if (separate_head_clamp)
		{
			if (LinkIndex == NumChainLinks - 1)
			{
				Lookat_Modified_Clamp = head_clamp_value;

				Up_Look_Ratio = 1;
			}

		}

		OutBoneTransforms[ChainLink.TransformIndex].Transform = LookAt_Processor(OutBoneTransforms[ChainLink.TransformIndex].Transform, Head_Location.GetLocation(), Temp_Target_Loc, LookAt_Axis, Lookat_Modified_Clamp, use_natural_rotation, Up_Look_Ratio, Multiplier_Value);


		if (LinkIndex == 0)
		{
			/*
			FRotator Inverted_Pelvis_Rot = FRotator(OutBoneTransforms[0].Transform.GetRotation()* Pelvis_Location.GetRotation().Inverse());

			float Side_Angle = Inverted_Pelvis_Rot.Yaw;
			float Vertical_Angle = Inverted_Pelvis_Rot.Roll;

			GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " TEST "+ Inverted_Pelvis_Rot.ToString());
			*/
		}


		if (alter_pelvis)
		{
			if (LinkIndex == 0)
			{

				//	OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(-Diff_Pelvis);

				OutBoneTransforms[ChainLink.TransformIndex].Transform.SetLocation(PelvisLocTarget.GetLocation());

				OutBoneTransforms[ChainLink.TransformIndex].Transform.SetRotation(Original_Rot.Quaternion() *  PelvisLocTarget.GetRotation());


				Temp_Target_Loc += Diff_Pelvis;
			}
		}




		/*
		//	if (LinkIndex == 0)
		{


			FRotator Inverted_Pelvis_Rot = FRotator(Pelvis_Ref_FullTransform.GetRotation() * Pelvis_Location.GetRotation().Inverse());

			float Side_Angle = -Inverted_Pelvis_Rot.Yaw;
			float Vertical_Angle = FMath::Abs(Inverted_Pelvis_Rot.Roll);


			float Side_Down_Val = FMath::Abs(Side_Angle);

			//GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " TEST " + Inverted_Pelvis_Rot.ToString());

			FVector Right_Dir = FVector::CrossProduct(LookAt_Axis.Axis, FVector::UpVector);


			//	OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1) * Up_Length * -Downward_Dip_Multiplier * Vertical_Angle);

			OutBoneTransforms[0].Transform.AddToTranslation(FVector(0, 0, 1) * Downward_Dip_Multiplier * Vertical_Angle);

			OutBoneTransforms[0].Transform.AddToTranslation(Right_Dir * Side_Multiplier * Side_Angle);

			OutBoneTransforms[0].Transform.AddToTranslation(FVector(0, 0, 1) * Side_Down_Multiplier * Side_Down_Val);

			Aim_Pelvis_Diff = OutBoneTransforms[0].Transform.GetLocation() - Pelvis_Ref_FullTransform.GetLocation();

		}
		*/



		/*
		if (alter_pelvis)
		{
			if (LinkIndex == 0)
				OutBoneTransforms[ChainLink.TransformIndex].Transform.SetLocation(PelvisLocTarget.GetLocation() );
		}
		*/


		
	

		FTransform Old_Parent = FTransform();
		FTransform Current_Parent = FTransform();

		if (ChainLink.TransformIndex > 0)
		{
			//Get new parent rotation
			Current_Parent = OutBoneTransforms[ChainLink.TransformIndex - 1].Transform;

			//Get old parent rotation
			const FCompactPoseBoneIndex& Parent_BoneIndex = BoneIndices[ChainLink.TransformIndex - 1];
			Old_Parent = Output.Pose.GetComponentSpaceTransform(Parent_BoneIndex);
		}

		//Get Delta result
		FTransform Parent_Difference = Old_Parent.Inverse() * Current_Parent;

		//compose current child transform with delta parent rotation
	//	if (LinkIndex < NumChainLinks)
		OutBoneTransforms[ChainLink.TransformIndex].Transform.SetLocation( (OutBoneTransforms[ChainLink.TransformIndex].Transform * Parent_Difference).GetLocation() );

	//	OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(Aim_Pelvis_Diff);


		
	}


	FRotator Inverted_Pelvis_Rot = FRotator(Pelvis_Ref_FullTransform.GetRotation() * Pelvis_Location.GetRotation().Inverse());

	float Side_Angle = -Inverted_Pelvis_Rot.Yaw;
	float Vertical_Angle = FMath::Abs(Inverted_Pelvis_Rot.Roll);


	float Side_Down_Val = FMath::Abs(Side_Angle);

	//GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " TEST " + Inverted_Pelvis_Rot.ToString());

	FVector Right_Dir = FVector::CrossProduct(LookAt_Axis.Axis, FVector::UpVector);

	for (int32 LinkIndex = 0; LinkIndex < NumChainLinks; LinkIndex++)
	{

		FCCDIK_Modified_ChainLink const& ChainLink = Chain[LinkIndex];


		OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1) * Downward_Dip_Multiplier * Vertical_Angle);
		OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(Right_Dir * Side_Multiplier * Side_Angle);
		OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1) * Side_Down_Multiplier * Side_Down_Val);

	}

}


FTransform UDragonIK_Library::LookAt_Processor(FTransform ComponentBoneTransform, FVector HeadLocation, FVector Offset_Vector, FAxis LookAt_Axis, float Lookat_Clamp,bool Use_Natural_Method = true, float Up_Rotation_Clamp=1, float intensity=1)
{

	FVector TargetLocationInComponentSpace = Offset_Vector;

	// lookat vector
	FVector LookAtVector = LookAt_Axis.GetTransformedAxis(ComponentBoneTransform).GetSafeNormal();

	//COMPARISON MODE

	FVector target_dir = (TargetLocationInComponentSpace - HeadLocation).GetSafeNormal();

	FVector Horizontal_Target_Dir = target_dir;
//	Horizontal_Target_Dir.Y = 0;
	FVector Opposite_Target_Dir = (HeadLocation - TargetLocationInComponentSpace).GetSafeNormal();
	Opposite_Target_Dir.Y = 0;

	float AimClampInRadians = FMath::DegreesToRadians(FMath::Min(Lookat_Clamp, 180.f));
	float DiffAngle = FMath::Acos(FVector::DotProduct(LookAtVector, Opposite_Target_Dir));

	FVector Selected_Vector = Horizontal_Target_Dir;
	
	if (DiffAngle > AimClampInRadians)
	{
		check(DiffAngle > 0.f);
		FVector DeltaTarget = Opposite_Target_Dir - LookAtVector;
		DeltaTarget *= (AimClampInRadians / DiffAngle);
		Opposite_Target_Dir = LookAtVector + DeltaTarget;
		
		Opposite_Target_Dir.Normalize();
	}


	float AimClampInRadians_Forward = FMath::DegreesToRadians(FMath::Min(Lookat_Clamp, 180.f));
	float DiffAngle_Forward = FMath::Acos(FVector::DotProduct(LookAtVector, Horizontal_Target_Dir));


	if (DiffAngle_Forward > AimClampInRadians_Forward)
	{
		check(DiffAngle_Forward > 0.f);
		FVector DeltaTarget_Forward = Horizontal_Target_Dir - LookAtVector;
		DeltaTarget_Forward *= (AimClampInRadians_Forward / DiffAngle_Forward);
		Horizontal_Target_Dir = LookAtVector + DeltaTarget_Forward;
		Horizontal_Target_Dir.Normalize();
	}

//	if(DiffAngle_Forward > AimClampInRadians_Forward)
	Selected_Vector = Horizontal_Target_Dir;


	Selected_Vector.Z = Selected_Vector.Z * Up_Rotation_Clamp;


//	else
//		Selected_Vector = Horizontal_Target_Dir;


//	GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, " DiffAngle[0] : " +FString::SanitizeFloat(DiffAngle)+" AimClampInRadians : "+ FString::SanitizeFloat(AimClampInRadians));


	//GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, " DiffAngle[0] : " +FString::SanitizeFloat(DiffAngle)+" AimClampInRadians : "+ FString::SanitizeFloat(AimClampInRadians));


	FQuat Normalized_Delta = FQuat::FindBetweenNormals(LookAtVector, Selected_Vector);


	if (!Use_Natural_Method)
	{
		/**/
		//FRotator Rot_Ref_01 = UDragonIK_Library::CustomLookRotation(Selected_Vector, FVector(0, 0, -1));
		//FRotator Rot_Ref_02 = UDragonIK_Library::CustomLookRotation(LookAtVector, FVector(0, 0, -1));
		
	
		
		FRotator Rot_Ref_01 = UKismetMathLibrary::FindLookAtRotation(FVector(0,0,0),Selected_Vector*100);
		FRotator Rot_Ref_02 = UKismetMathLibrary::FindLookAtRotation(FVector(0,0,0),LookAtVector*100);
		Normalized_Delta = Rot_Ref_01.Quaternion()*Rot_Ref_02.Quaternion().Inverse();
		

		/*
		Normalized_Delta = UKismetMathLibrary::NormalizedDeltaRotator(target_dir.ToOrientationRotator(), LookAtVector.ToOrientationRotator()).Quaternion();

		FRotator temp_ND = FRotator(Normalized_Delta);

		if(LookAtVector==FVector(0.0f,1.0f,0.0f))
		Normalized_Delta = FRotator(-temp_ND.Roll, temp_ND.Yaw, - temp_ND.Pitch).Quaternion();
		else
			if (LookAtVector == FVector(0.0f, -1.0f, 0.0f))
				Normalized_Delta = FRotator(temp_ND.Roll, temp_ND.Yaw, temp_ND.Pitch).Quaternion();
			else
				if (LookAtVector == FVector(-1.0f, 0.0f, 0.0f))
					Normalized_Delta = FRotator(-temp_ND.Pitch, temp_ND.Yaw, -temp_ND.Roll).Quaternion();
				else
					if (LookAtVector == FVector(1.0f, 0.0f, 0.0f))
						Normalized_Delta = FRotator(temp_ND.Pitch, temp_ND.Yaw, temp_ND.Roll).Quaternion();
					else
						Normalized_Delta = FRotator(-temp_ND.Roll, temp_ND.Yaw, -temp_ND.Pitch).Quaternion();
						*/
	}

//	GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, " Normalized_Delta : " + FRotator(Normalized_Delta).ToString());

	FQuat Normalized_Delta_Ref = FQuat::FindBetweenNormals(LookAtVector, LookAtVector);


	FVector Rot_Euler = Normalized_Delta.Euler();

//	Rot_Euler.X = Rot_Euler.X * Up_Rotation_Clamp;

	
//	if(Up_Rotation_Clamp ==1.0f)
	Rot_Euler.X = FMath::ClampAngle(Rot_Euler.X, -Lookat_Clamp, Lookat_Clamp);

	Rot_Euler.Y = FMath::ClampAngle(Rot_Euler.Y, -Lookat_Clamp, Lookat_Clamp);
	Rot_Euler.Z = FMath::ClampAngle(Rot_Euler.Z, -Lookat_Clamp, Lookat_Clamp);

//	Rot_Euler.Y = 0;
//	Rot_Euler.Z = 0;


	Normalized_Delta = FQuat::MakeFromEuler(Rot_Euler);
	

	Normalized_Delta = FQuat::Slerp(Normalized_Delta_Ref, Normalized_Delta, intensity);


	FTransform World_Rotatation_Transform = ComponentBoneTransform;

	World_Rotatation_Transform.SetRotation(Normalized_Delta);


	ComponentBoneTransform.SetRotation(World_Rotatation_Transform.GetRotation() * ComponentBoneTransform.GetRotation());


	return ComponentBoneTransform;


}

void UDragonIK_Library::Evaluate_CCDIK_Modified(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel, FBoneReference RootBone_Input, FBoneReference TipBone_Input, float Precision, int32 MaxIteration, bool bStartFromTail, bool bEnableRotationLimit, float Lookat_Radius,bool Lock_Forward_Axis,bool Lock_Side_Axis, FTransform EffectorTransform, FVector FabrikOffset, TArray<FBoneTransform>& OutBoneTransforms)
{
//	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
	
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();


	FBoneReference RootBone = FBoneReference(RootBone_Input);
	RootBone.Initialize(BoneContainer);


	FBoneReference TipBone = FBoneReference(TipBone_Input);
	TipBone.Initialize(BoneContainer);



	// Update EffectorLocation if it is based off a bone position
	//FTransform CSEffectorTransform = GetTargetTransform_Modified(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, EffectorTarget, EffectorLocation);

	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, CSEffectorTransform, RootBone.GetCompactPoseIndex(BoneContainer), EBoneControlSpace::BCS_WorldSpace);

	

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();



	// Gather all bone indices between root and tip.
	TArray<FCompactPoseBoneIndex> BoneIndices;

	{
		const FCompactPoseBoneIndex RootIndex = RootBone.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex BoneIndex = TipBone.GetCompactPoseIndex(BoneContainer);
		do
		{
			BoneIndices.Insert(BoneIndex, 0);
			BoneIndex = Output.Pose.GetPose().GetParentBoneIndex(BoneIndex);
		} while (BoneIndex != RootIndex);
		BoneIndices.Insert(BoneIndex, 0);
	}

	// Maximum length of skeleton segment at full extension
	float MaximumReach = 0;

	// Gather transforms
	int32 const NumTransforms = BoneIndices.Num();
	OutBoneTransforms.AddUninitialized(NumTransforms);

	// Gather chain links. These are non zero length bones.
	TArray<FCCDIK_Modified_ChainLink> Chain;
	Chain.Reserve(NumTransforms);

	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = Output.Pose.GetComponentSpaceTransform(RootBoneIndex);

		OutBoneTransforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);
		Chain.Add(FCCDIK_Modified_ChainLink(BoneCSTransform.GetLocation(), 0.f, RootBoneIndex, 0));
	}

	// Go through remaining transforms
	for (int32 TransformIndex = 1; TransformIndex < NumTransforms; TransformIndex++)
	{
		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];

		const FTransform& BoneCSTransform = Output.Pose.GetComponentSpaceTransform(BoneIndex);
		FVector const BoneCSPosition = BoneCSTransform.GetLocation();

		OutBoneTransforms[TransformIndex] = FBoneTransform(BoneIndex, BoneCSTransform);

		// Calculate the combined length of this segment of skeleton
		float const BoneLength = FVector::Dist(BoneCSPosition, OutBoneTransforms[TransformIndex - 1].Transform.GetLocation());

		if (!FMath::IsNearlyZero(BoneLength))
		{
			Chain.Add(FCCDIK_Modified_ChainLink(BoneCSPosition, BoneLength, BoneIndex, TransformIndex));
			MaximumReach += BoneLength;
		}
		else
		{
			// Mark this transform as a zero length child of the last link.
			// It will inherit position and delta rotation from parent link.
			FCCDIK_Modified_ChainLink& ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}

	int32 const NumChainLinks = Chain.Num();

	float Height_To_Head = FMath::Abs(Chain[Chain.Num() - 1].Position.Z);


	float Physical_Reach = (Chain[Chain.Num()-1].Position - Chain[0].Position).Size();
	CSEffectorLocation = CSEffectorLocation + FVector(0,0,Height_To_Head);


	CSEffectorLocation = CSEffectorLocation + FabrikOffset;

	//CSEffectorLocation.Z = FMath::Clamp<float>(CSEffectorLocation.Z, Chain[Chain.Num() - 1].Position.Z - Lookat_Radius, Chain[Chain.Num() - 1].Position.Z + Lookat_Radius);

	//CSEffectorLocation.Y = FMath::Clamp<float>(CSEffectorLocation.Y, Chain[Chain.Num() - 1].Position.Y - Lookat_Radius, Chain[Chain.Num() - 1].Position.Y + Lookat_Radius);


	FVector direction_bw_head_target = (CSEffectorLocation - Chain[Chain.Num() - 1].Position);

	CSEffectorLocation = Chain[Chain.Num() - 1].Position + direction_bw_head_target.GetSafeNormal() * (FMath::Clamp<float>(direction_bw_head_target.Size(), 0, Lookat_Radius));

	if(Lock_Forward_Axis)
	CSEffectorLocation.Y = Chain[Chain.Num() - 1].Position.Y;

	if(Lock_Side_Axis)
	CSEffectorLocation.X = Chain[Chain.Num() - 1].Position.X;

	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal() * Physical_Reach*1.0f;





	bool bBoneLocationUpdated = Solve_Modified_CCDIK(Chain, CSEffectorLocation, MaximumReach, Precision, MaxIteration);
	// If we moved some bones, update bone transforms.
	if (bBoneLocationUpdated)
	{
		// First step: update bone transform positions from chain links.
		for (int32 LinkIndex = 0; LinkIndex < NumChainLinks; LinkIndex++)
		{
			FCCDIK_Modified_ChainLink const& ChainLink = Chain[LinkIndex];
			OutBoneTransforms[ChainLink.TransformIndex].Transform.SetTranslation(ChainLink.Position);

			// If there are any zero length children, update position of those
			int32 const NumChildren = ChainLink.ChildZeroLengthTransformIndices.Num();
			for (int32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
			{
				OutBoneTransforms[ChainLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform.SetTranslation(ChainLink.Position);
			}
		}

		// FABRIK algorithm - re-orientation of bone local axes after translation calculation
		for (int32 LinkIndex = 0; LinkIndex < NumChainLinks - 1; LinkIndex++)
		{
			FCCDIK_Modified_ChainLink const& CurrentLink = Chain[LinkIndex];
			FCCDIK_Modified_ChainLink const& ChildLink = Chain[LinkIndex + 1];

			// Calculate pre-translation vector between this bone and child
			FVector const OldDir = (Output.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(ChildLink.BoneIndex)).GetLocation() - Output.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(CurrentLink.BoneIndex)).GetLocation()).GetUnsafeNormal();

			// Get vector from the post-translation bone to it's child
			FVector const NewDir = (ChildLink.Position - CurrentLink.Position).GetUnsafeNormal();

			// Calculate axis of rotation from pre-translation vector to post-translation vector
			FVector const RotationAxis = FVector::CrossProduct(OldDir, NewDir).GetSafeNormal();
			float const RotationAngle = FMath::Acos(FVector::DotProduct(OldDir, NewDir));
			FQuat const DeltaRotation = FQuat(RotationAxis, RotationAngle);
			// We're going to multiply it, in order to not have to re-normalize the final quaternion, it has to be a unit quaternion.
			checkSlow(DeltaRotation.IsNormalized());

			// Calculate absolute rotation and set it
			FTransform& CurrentBoneTransform = OutBoneTransforms[CurrentLink.TransformIndex].Transform;
			CurrentBoneTransform.SetRotation(DeltaRotation * CurrentBoneTransform.GetRotation());
			CurrentBoneTransform.NormalizeRotation();

			// Update zero length children if any
			int32 const NumChildren = CurrentLink.ChildZeroLengthTransformIndices.Num();
			for (int32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
			{
				FTransform& ChildBoneTransform = OutBoneTransforms[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform;
				ChildBoneTransform.SetRotation(DeltaRotation * ChildBoneTransform.GetRotation());
				ChildBoneTransform.NormalizeRotation();
			}
		}
	}

	//int32 const TipBoneTransformIndex = OutBoneTransforms.Num() - 1;


	//	OutBoneTransforms[TipBoneTransformIndex].Transform = Output.Pose.GetLocalSpaceTransform(BoneIndices[TipBoneTransformIndex]) * OutBoneTransforms[TipBoneTransformIndex - 1].Transform;

	//	OutBoneTransforms[TipBoneTransformIndex].Transform.SetRotation(CSEffectorTransform.GetRotation());

	
}




void UDragonIK_Library::OrthoNormalize(FVector& Normal, FVector& Tangent)
{
	Normal = Normal.GetSafeNormal();
	Tangent = Tangent - (Normal * FVector::DotProduct(Tangent, Normal));
	Tangent = Tangent.GetSafeNormal();
}





FTransform UDragonIK_Library::QuatLookXatLocalDirection(const FTransform& LookAtFromTransform, const FVector& LookAtLocalDirection)
{
	const FQuat Q = FQuat::FindBetweenNormals(FVector(1, 0, 0), LookAtLocalDirection);
	return FTransform((LookAtFromTransform.GetRotation() * Q).GetNormalized(), LookAtFromTransform.GetLocation(), LookAtFromTransform.GetScale3D());
}


FTransform UDragonIK_Library::QuatLookXatLocation(const FTransform& LookAtFromTransform, const FVector& LookAtTarget)
{
	if (!LookAtFromTransform.GetLocation().Equals(LookAtTarget, 0.01f))
	{
		const FVector LookAtLocalDirection = (LookAtFromTransform.Inverse().TransformPositionNoScale(LookAtTarget)).GetSafeNormal();
		return QuatLookXatLocalDirection(LookAtFromTransform, LookAtLocalDirection);
	}
	else
	{
		return LookAtFromTransform;
	}
}

FRotator UDragonIK_Library::LookAtRotation_V3(FVector source, FVector target, FVector upvector)
{
	FVector newForward = target - source;
	newForward.Normalize();

	const FVector WorldUp(0.0f, 0.0f, 1.0f);

	FVector newRight = FVector::CrossProduct(newForward, upvector);
	FVector newUp = FVector::CrossProduct(newRight, newForward);

	return FTransform(newForward, newRight, newUp, source).Rotator().GetNormalized();
}



FRotator UDragonIK_Library::LookAtVector_V2(FVector Source_Location,FVector lookAt, FVector upDirection)
{
	FVector Forward = (lookAt - Source_Location);
	FRotator Rot = UKismetMathLibrary::MakeRotFromXZ(Forward, upDirection);
	return Rot;

}


FRotator UDragonIK_Library::CustomLookRotation(FVector lookAt, FVector upDirection)
{


	FVector forward = lookAt;
	FVector up = upDirection;

	//UDragonIK_Object::OrthoNormalize(forward, up);

	//UDragonIK_Object::OrthoNormalize(forward, up);

	forward = forward.GetSafeNormal();
	up = up - (forward * FVector::DotProduct(up, forward));
	up = up.GetSafeNormal();

	///////////////////////





	FVector vector = forward.GetSafeNormal();
	FVector vector2 = FVector::CrossProduct(up, vector);
	FVector vector3 = FVector::CrossProduct(vector, vector2);
	float m00 = vector2.X;
	float m01 = vector2.Y;
	float m02 = vector2.Z;
	float m10 = vector3.X;
	float m11 = vector3.Y;
	float m12 = vector3.Z;
	float m20 = vector.X;
	float m21 = vector.Y;
	float m22 = vector.Z;


	float num8 = (m00 + m11) + m22;
	FQuat quaternion = FQuat();
	if (num8 > 0.0f)
	{
		float num = (float)FMath::Sqrt(num8 + 1.0f);
		quaternion.W = num * 0.5f;
		num = 0.5f / num;
		quaternion.X = (m12 - m21) * num;
		quaternion.Y = (m20 - m02) * num;
		quaternion.Z = (m01 - m10) * num;
		return FRotator(quaternion);
	}
	if ((m00 >= m11) && (m00 >= m22))
	{
		float num7 = (float)FMath::Sqrt(((1.0f + m00) - m11) - m22);
		float num4 = 0.5f / num7;
		quaternion.X = 0.5f * num7;
		quaternion.Y = (m01 + m10) * num4;
		quaternion.Z = (m02 + m20) * num4;
		quaternion.W = (m12 - m21) * num4;
		return FRotator(quaternion);
	}
	if (m11 > m22)
	{
		float num6 = (float)FMath::Sqrt(((1.0f + m11) - m00) - m22);
		float num3 = 0.5f / num6;
		quaternion.X = (m10 + m01) * num3;
		quaternion.Y = 0.5f * num6;
		quaternion.Z = (m21 + m12) * num3;
		quaternion.W = (m20 - m02) * num3;
		return FRotator(quaternion);
	}
	float num5 = (float)FMath::Sqrt(((1.0f + m22) - m00) - m11);
	float num2 = 0.5f / num5;
	quaternion.X = (m20 + m02) * num2;
	quaternion.Y = (m21 + m12) * num2;
	quaternion.Z = 0.5f * num5;
	quaternion.W = (m01 - m10) * num2;


	return FRotator(quaternion);




	//////////////////////



	//	return ret;

}