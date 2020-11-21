/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#pragma once

#include "CoreMinimal.h"
//#include "Engine.h"
#include "BoneContainer.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BoneContainer.h"

#include "UObject/NoExportTypes.h"

#include "Animation/AnimNodeBase.h"
#include "CommonAnimTypes.h"
#include "Curves/CurveFloat.h"

#include "DragonIK_Library.generated.h"






UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EIKTrace_Type_Plugin : uint8
{
	ENUM_LineTrace_Type 	UMETA(DisplayName = "Line Trace"),
	ENUM_SphereTrace_Type 	UMETA(DisplayName = "Sphere Trace"),
	ENUM_BoxTrace_Type 	UMETA(DisplayName = "Box Trace")


};



UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EInterpoLocation_Type_Plugin : uint8
{
	ENUM_DivisiveLoc_Interp 	UMETA(DisplayName = "Divisive Interpolation Method"),
	ENUM_LegacyLoc_Interp 	UMETA(DisplayName = "Normal (Location)Interpolation Method")
};



UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EInterpoRotation_Type_Plugin : uint8
{
	ENUM_DivisiveRot_Interp 	UMETA(DisplayName = "Divisive Interpolation Method"),
	ENUM_LegacyRot_Interp 	UMETA(DisplayName = "Normal (Rotation)Interpolation Method")
};





struct FDragonData_BoneStruct
{

	FBoneReference Start_Spine;
	FBoneReference Pelvis;

	FBoneReference Head_Bone;
	FBoneReference LookLimit_Bone;

	TArray<FBoneReference> FeetBones;
	TArray<FBoneReference> KneeBones;
	TArray<FBoneReference> ThighBones;

};



struct FDragonData_HitPairs
{
	FHitResult Parent_Spine_Hit;

	FHitResult Parent_Left_Hit;
	FHitResult Parent_Right_Hit;

	FHitResult Parent_Front_Hit;
	FHitResult Parent_Back_Hit;



	TArray<FHitResult> RV_SpinalFeet_Hits;

	TArray<FHitResult> RV_Feet_Hits;

	TArray<FHitResult> RV_FeetFront_Hits;

	TArray<FHitResult> RV_FeetBack_Hits;

	TArray<FHitResult> RV_FeetLeft_Hits;

	TArray<FHitResult> RV_FeetRight_Hits;




	TArray<FHitResult> RV_FeetBalls_Hits;

	TArray<FHitResult> RV_Knee_Hits;



};



struct FDragonData_SpineFeetPair_TRANSFORM_WSPACE
{

	FTransform Spine_Involved = FTransform::Identity;

	FVector last_spine_location = FVector(0,0,0);

	FVector last_target_location = FVector(0, 0, 0);


	float Z_Offset = 0;


	TArray<FTransform> Associated_Feet = TArray<FTransform>();


	TArray<FTransform> Associated_FeetBalls = TArray<FTransform>();


	TArray<FTransform> Associated_Knee = TArray<FTransform>();


};






struct FDragonData_SpineFeetPair_heights
{

	float Spine_Involved = 0;

	TArray<float> Associated_Feet = TArray<float>();

};





/** Transient structure for FABRIK node evaluation */
struct DragonChainLink
{
public:
	/** Position of bone in component space. */
	FVector Position;

	/** Distance to its parent link. */
	float Length;

	/** Bone Index in SkeletalMesh */
	FCompactPoseBoneIndex BoneIndex;

	/** Transform Index that this control will output */
	int32 TransformIndex;

	/** Child bones which are overlapping this bone.
	* They have a zero length distance, so they will inherit this bone's transformation. */
	TArray<int32> ChildZeroLengthTransformIndices;

	DragonChainLink()
		: Position(FVector::ZeroVector)
		, Length(0.f)
		, BoneIndex(INDEX_NONE)
		, TransformIndex(INDEX_NONE)
	{
	}

	DragonChainLink(const FVector& InPosition, const float& InLength, const FCompactPoseBoneIndex& InBoneIndex, const int32& InTransformIndex)
		: Position(InPosition)
		, Length(InLength)
		, BoneIndex(InBoneIndex)
		, TransformIndex(InTransformIndex)
	{
	}
};




struct DragonSpineOutput
{
public:
	/** Position of bone in component space. */
	TArray<DragonChainLink> chain_data;
	TArray<FCompactPoseBoneIndex> BoneIndices;
	TArray<FBoneTransform> temp_transforms;
	FTransform SpineFirstEffectorTransform;
	FTransform PelvicEffectorTransform;
	FVector RootDifference;
	bool is_moved;
	int32 NumChainLinks;
};




struct FDragonData_SpineFeetPair
{

	FBoneReference Spine_Involved;

	TArray<FBoneReference> Associated_Feet;

	TArray<FBoneReference> Associated_Knees;

	TArray<FBoneReference> Associated_Thighs;

	int32 Spine_Chain_Index = 0;

	FTransform inverted_spine_tf;

	TArray<float> Feet_Heights;

//	TArray<float> feet_yaw_offset;

	TArray<FRotator> feet_rotation_offset;

	TArray<float> feet_rotation_limit;



	TArray<FVector> knee_direction_offset;

	TArray<int> order_index;


	TArray<FVector> feet_trace_offset;



	int solver_count = 0;

	FVector total_spine_locdata;

	bool operator == (const FDragonData_SpineFeetPair &pair) const
	{

		if (Spine_Involved.BoneIndex == pair.Spine_Involved.BoneIndex && Associated_Feet.Num() == 0)
		{
			return true;
		}
		else
			return false;


	}

};




USTRUCT(BlueprintType)
struct FDragonData_ArmsData
{
	GENERATED_BODY()



		UPROPERTY(EditAnywhere, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FBoneReference Shoulder_Bone_Name;

	UPROPERTY(EditAnywhere, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FBoneReference Elbow_Bone_Name;



	UPROPERTY(EditAnywhere, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FBoneReference Hand_Bone_Name;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FVector Local_Direction_Axis = FVector(0, 1, 0);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		bool accurate_hand_rotation = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		float overridden_limb_clamp = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		bool relative_axis = true;




};


USTRUCT(BlueprintType)
struct FDragonData_FootData
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Feet_Bone_Name;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Knee_Bone_Name;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Thigh_Bone_Name;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FRotator Feet_Rotation_Offset;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FVector Knee_Direction_Offset = FVector(0,0,0);



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FVector Feet_Trace_Offset = FVector(0, 0, 0);


	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		float Feet_Rotation_Limit = 180;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (DisplayName = "Feet Height Offset"))
		float Feet_Heights;

};


USTRUCT(Blueprintable)
struct FDragonData_MultiInput
{
	GENERATED_USTRUCT_BODY()

public:

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Start_Spine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Pelvis;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		TArray<FDragonData_FootData> FeetBones;

};


///////////







USTRUCT()
struct FCCDIK_Modified_ChainLink
{
	GENERATED_BODY()
public:
	/** Position of bone in component space. */
	FVector Position;

	/** Distance to its parent link. */
	float Length;

	/** Bone Index in SkeletalMesh */
	int32 BoneIndex;

	/** Transform Index that this control will output */
	int32 TransformIndex;

	/** Default Direction to Parent */
	FVector DefaultDirToParent;

	/** Child bones which are overlapping this bone.
	* They have a zero length distance, so they will inherit this bone's transformation. */
	TArray<int32> ChildZeroLengthTransformIndices;

	FCCDIK_Modified_ChainLink()
		: Position(FVector::ZeroVector)
		, Length(0.f)
		, BoneIndex(INDEX_NONE)
		, TransformIndex(INDEX_NONE)
		, DefaultDirToParent(FVector(-1.f, 0.f, 0.f))
	{
	}

	FCCDIK_Modified_ChainLink(const FVector& InPosition, const float InLength, const FCompactPoseBoneIndex& InBoneIndex, const int32& InTransformIndex)
		: Position(InPosition)
		, Length(InLength)
		, BoneIndex(InBoneIndex.GetInt())
		, TransformIndex(InTransformIndex)
		, DefaultDirToParent(FVector(-1.f, 0.f, 0.f))
	{
	}

	FCCDIK_Modified_ChainLink(const FVector& InPosition, const float InLength, const FCompactPoseBoneIndex& InBoneIndex, const int32& InTransformIndex, const FVector& InDefaultDirToParent)
		: Position(InPosition)
		, Length(InLength)
		, BoneIndex(InBoneIndex.GetInt())
		, TransformIndex(InTransformIndex)
		, DefaultDirToParent(InDefaultDirToParent)
	{
	}

	FCCDIK_Modified_ChainLink(const FVector& InPosition, const float InLength, const int32 InBoneIndex, const int32 InTransformIndex)
		: Position(InPosition)
		, Length(InLength)
		, BoneIndex(InBoneIndex)
		, TransformIndex(InTransformIndex)
		, DefaultDirToParent(FVector(-1.f, 0.f, 0.f))
	{
	}
};






void Solve_Modified_TwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, const FVector& ThighEffector, FVector& OutJointPos, FVector& OutEndPos, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);
void Solve_Modified_TwoBoneIK_2(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, const FVector& ThighEffector, FVector& OutJointPos, FVector& OutEndPos, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);
void Solve_Modified_TwoBoneIK_3(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, const FVector& ThighEffector, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);
void Solve_Modified_TwoBoneIK_4(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, const FVector& ThighEffector, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);





void Solve_Modified_Direct_TwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);
void Solve_Modified_Direct_TwoBoneIK_2(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);
void Solve_Modified_Direct_TwoBoneIK_3(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);
void Solve_Modified_Direct_TwoBoneIK_4(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);




UCLASS(Blueprintable, BlueprintType)
class DRAGONIKPLUGIN_API UDragonIK_Library : public UObject
{
	GENERATED_BODY()



		static UDragonIK_Library* Constructor();


public :

	UFUNCTION(BlueprintPure, Category = "SolverFunctions")
		static FRotator CustomLookRotation(FVector lookAt, FVector upDirection);


	UFUNCTION(BlueprintPure, Category = "SolverFunctions")
		static FRotator LookAtVector_V2(FVector Source_Location, FVector lookAt, FVector upDirection);



	static FTransform QuatLookXatLocalDirection(const FTransform& LookAtFromTransform, const FVector& LookAtLocalDirection);

	UFUNCTION(BlueprintPure, Category = "SolverFunctions")
		static FTransform QuatLookXatLocation(const FTransform& LookAtFromTransform, const FVector& LookAtTarget);

	UFUNCTION(BlueprintPure, Category = "SolverFunctions")
	static FRotator LookAtRotation_V3(FVector source, FVector target,FVector upvector);



	static FRotator LookAt_Function(FTransform transform, FVector worldPosition, FVector up_direction, FVector rotationOffset);


	static bool Solve_Modified_CCDIK(TArray<FCCDIK_Modified_ChainLink>& InOutChain, const FVector& TargetPosition, float MaximumReach, float Precision, int32 MaxIteration);

	

	//FTransform GetTargetTransform_Modified(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, FBoneSocketTarget& InTarget, const FVector& InOffset);


	static void Evaluate_CCDIK_Modified(FComponentSpacePoseContext& Output, USkeletalMeshComponent *owning_skel, FBoneReference RootBone, FBoneReference TipBone, float Precision, int32 MaxIteration, bool bStartFromTail, bool bEnableRotationLimit, float Lookat_Radius,bool Lock_Forward_Axis,bool Lock_Side_Axis, FTransform EffectorTransform, FVector FabrikOffset, TArray<FBoneTransform>& OutBoneTransforms);

	static void Evaluate_ConsecutiveBoneRotations(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel,TArray<FDragonData_SpineFeetPair> spine_feet_pair, FRuntimeFloatCurve Look_Bending_Curve, FBoneReference RootBone_Input, FBoneReference TipBone_Input, float Lookat_Radius, FTransform EffectorTransform, FAxis LookAt_Axis, float Lookat_Clamp,float Downward_Dip_Multiplier, float Side_Multiplier,float Side_Down_Multiplier,bool alter_pelvis,FTransform PelvisLocTarget, FRuntimeFloatCurve Bending_Multiplier_Curve,float Up_Rot_Clamp,bool Use_Natural_Method, bool separate_head_clamp, float head_clamp_value, TArray<FBoneTransform>& OutBoneTransforms);

	bool Lock_Forward_Axis = true;




//	static void Solve_Modified_TwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, float UpperLimbLength, float LowerLimbLength, bool bAllowStretching, float StartStretchRatio, float MaxStretchScale);
	static void Evaluate_TwoBoneIK_Modified(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel, FBoneReference FeetBone, FBoneReference KneeBone, FBoneReference ThighBone, FTransform ThighTransform, FVector JointLocation, FVector Knee_Pole_Offset, TArray<FBoneTransform>& OutBoneTransforms);

	static void Evaluate_TwoBoneIK_Direct_Modified(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel, FBoneReference FeetBone, FBoneReference KneeBone, FBoneReference ThighBone, FTransform ThighTransform , FTransform Shoulder,FTransform Knee,FTransform Hand, FVector JointLocation, FVector Knee_Pole_Offset, FTransform transform_offset , FTransform Common_Spine_Modified_Transform, FRotator Limb_Rotation_Offset, FVector HandAxis, bool is_relative_axis,float Hand_Clamp_Value,bool accurate_hand_rotation, FTransform Extra_Hand_Offset, TArray<FBoneTransform>& OutBoneTransforms);

	static void Evaluate_TwoBoneIK_Spinal_Modified(FComponentSpacePoseContext& Output, USkeletalMeshComponent* owning_skel, FBoneReference FeetBone, FBoneReference KneeBone, FBoneReference ThighBone, FTransform ThighTransform, FTransform Shoulder_Trans, FTransform Knee_Trans, FTransform Hand_Trans, FVector JointLocation, FVector Knee_Pole_Offset, TArray<FBoneTransform>& OutBoneTransforms);

	static FTransform LookAt_Processor(FTransform ComponentBoneTransform, FVector HeadLocation, FVector Offset_Vector, FAxis LookAt_Axis, float Lookat_Clamp, bool Use_Natural_Method, float Up_Rotation_Clamp, float intensity);


	void OrthoNormalize(FVector& Normal, FVector& Tangent);

};





/**
 * 

UCLASS()
class DRAGONIKPLUGIN_API UDragonIK_Library : public UObject
{
	GENERATED_BODY()
	
	
	
	
};
*/