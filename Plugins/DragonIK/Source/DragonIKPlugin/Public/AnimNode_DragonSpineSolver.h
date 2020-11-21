/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#pragma once



#include "DragonIK_Library.h"
#include "CoreMinimal.h"

#include "DragonIKPlugin.h"

#include "Kismet/KismetMathLibrary.h"
#include "CollisionQueryParams.h"

#include "Animation/InputScaleBias.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_DragonSpineSolver.generated.h"
/**
 *
 */



class FPrimitiveDrawInterface;
class USkeletalMeshComponent;



UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class ERefPosePluginEnum : uint8
{
	VE_Animated 	UMETA(DisplayName = "Animated Pose"),
	VE_Rest 	UMETA(DisplayName = "Rest Pose")

};


//USTRUCT(BlueprintInternalUseOnly)
USTRUCT(BlueprintType)
struct DRAGONIKPLUGIN_API FAnimNode_DragonSpineSolver : public FAnimNode_Base
{
	//	GENERATED_USTRUCT_BODY()
	GENERATED_BODY()


		/*
* Type the input bones used by the solver - pelvis,spine-start and feets
* Pull this pin to create a "make struct" and easily type out the bones.
* Optionally you can type the bones directly inside the solver. Doing so is slightly more recommended because it activates fast path.
*/
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InputData, meta = (PinHiddenByDefault))
FDragonData_MultiInput dragon_input_data;

	FDragonData_BoneStruct dragon_bone_data;





public:




	/** Tolerance for final tip location delta from EffectorLocation*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float Precision = 0.1f;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float MaximumPitch = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float MinimumPitch = -30;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float MaximumRoll = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float MinimumRoll = -30;




	/*
	  * Maximum number of iterations allowed.
	  * Best left untouched. Higher values can be more costly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		int32 MaxIterations = 15;


	/*
	* Component Pose Input.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
		FComponentSpacePoseLink ComponentPose;

	/*
	* Alpha of the entire solver. 0 means no solving and 1 means maximum solving.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
		mutable float Alpha = 1;



	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
	mutable float Adaptive_Alpha = 1;




	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	//		mutable float Vertical_Solver_Upper_Scale = 1;


		/*
		 * The transistion speed between solve and unsolve state (eg:- when character jumps and falls back to ground).
		 * Lower values means slower but smoother transition.
		 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
		mutable float Shift_Speed = 1;



	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings)
	FInputScaleBias AlphaScaleBias;


	/*
	* Trace channel used by the solver traces. Recommended to create a new dedicated trace channel for the ik through project settings.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings)
		TEnumAsByte<ETraceTypeQuery> Trace_Channel = ETraceTypeQuery::TraceTypeQuery1;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings)
	    TEnumAsByte<ETraceTypeQuery> Anti_Trace_Channel = ETraceTypeQuery::TraceTypeQuery2;


	/*
	* Choose Trace type - Line,Sphere and Box.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Trace Type"))
		EIKTrace_Type_Plugin trace_type = EIKTrace_Type_Plugin::ENUM_LineTrace_Type;

	/*
	* If trace type is box or sphere, its radius is controlled using the Trace Radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Trace Radius", PinHiddenByDefault))
		float Trace_Radius = 20;


	/*
	* Max LOD that this node is allowed to run
	* For example if you have LODThreadhold to be 2, it will run until LOD 2 (based on 0 index)
	* when the component LOD becomes 3, it will stop update/evaluate
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "LOD Threshold", PinHiddenByDefault))
		int32 LODThreshold = -1;



	/*
	* If your character isn't dipping enough to touch the ground during slopes, slightly increase this value.
	* Only handles dipping based on the pairs of feet and the difference between their respective trace hit location.
	* Will not dip on flat surfaces.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Extra Dip Multiplier", PinHiddenByDefault))
		float dip_multiplier = 0;



	/*
	* Enable this to rotate the pelvis and chest is perfect perpendicular fashion.
	* Good for general purpose, but can cause extreme rotation and translations in wild,sharp and uneven surfaces.
	* Recommended to experiment between this enabled and disabled.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Rotate and move bone around terrain ? (Disabling gives more stable but slightly inaccurate solving)", PinHiddenByDefault))
		bool Rotate_Around_Translate = true;


	/*
	* Enable this to bypass interpolation and use instant solving data! */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Ignore Lerping", PinHiddenByDefault))
		bool Ignore_Lerping = false;


	int test_counter = 0;

	int trace_draw_counter = 0;

	virtual int32 GetLODThreshold() const override { return LODThreshold; }

	UPROPERTY(Transient)
		float ActualAlpha = 0;


	int Num_Valid_Spines = 0;



	float component_scale = 1;


	/*
	* Line trace height below the spines/feets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Trace Downward Height", PinHiddenByDefault))
		float line_trace_downward_height = 350;


	/*
	* Line trace height above the spines/feets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Trace Upward Height", PinHiddenByDefault))
		float line_trace_upper_height = 350;




	/*
* Use the anti-channel in the solving logic. Use meshes with the anti-channel set to "block" to repel the traces from touching ceilings and closed spaces.
* Also useful when under stairs or narrow multi-storied buildings. Cover the ceilings and under stairs with anti-channel blocked meshes.
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Use Anti-Channel Functionality", PinHiddenByDefault))
		bool Use_Anti_Channel = false;



	/*
	* Extra dip when pelvis is on an upward slope*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Extra dip When going up slopes", PinHiddenByDefault))
		float Slanted_Height_Up_Offset = 0;


	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Slope Detection Tolerance", PinHiddenByDefault))
	float Slope_Detection_Strength = 25;


	/*
	* Extra dip when pelvis is on a downward slope*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Extra dip When going down slopes", PinHiddenByDefault))
		float Slanted_Height_Down_Offset = 0;




	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Slanted height downwards offseting", PinHiddenByDefault))
	//		bool full_extend = true;



	/*
	* Enable this to prioritise chest over the pelvis.
	* The normal fabrik mode prioritises the pelvis transformation location when on extreme terrains.
	* The reverse fabrik mode ensures chest is given priority.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Reverse Fabrik", PinHiddenByDefault))
		bool reverse_fabrik = false;






	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Upward Push When Side Rotation", PinHiddenByDefault))
	float upward_push_side_rotation = 0;



	/*
	* Should solver calculation be based on the default reference pose or the current animated pose ?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Calculation Relative To Ref Pose", PinHiddenByDefault))
		bool Calculation_To_RefPose = false;


	/*
	* Extra dip when chest is on an upward slope*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Extra dip When going up slopes", PinHiddenByDefault))
		float Chest_Slanted_Height_Up_Offset = 0;


	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Slope Detection Tolerance", PinHiddenByDefault))
	//		float Chest_Slope_Detection_Strength = 25;

		/*
		* Extra dip when chest is on a downward slope*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Extra dip When going down slopes", PinHiddenByDefault))
		float Chest_Slanted_Height_Down_Offset = 0;



	/*
	* Optional additional Z offset for the chest*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Chest Z Offset", PinHiddenByDefault))
		float Chest_Base_Offset = 0;


	/*
	* Optional additional Z offset for the pelvis*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Pelvis Z Offset", PinHiddenByDefault))
		float Pelvis_Base_Offset = 0;









	/*
	* This is the spacing width of the 4 trace lines arranged in a cross pattern, around both pelvis and chest bones
	*  Ensure the spacing radius is tweaked to the spacing of the feet
	*  This is used for important subtle calculations, such as detecting slopes/flat surfaces.
	*  Used for rotation calculation if "Alternate cross based rotation" is enabled.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Alternate Cross Radius", PinHiddenByDefault))
		float virtual_leg_width = 25;



	/*
	* Maximum height the pelvis can dip below the base pose. Lower values means pelvis will dip less.
	* For biped (2 feet) characters, this is only used.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Maximum Dip Height - Pelvis", PinHiddenByDefault))
		float Maximum_Dip_Height = 100;


	/*
	* Maximum height the chest can dip below the base pose. Lower value means chest will dip less.
	* Not applicable for biped (2 feet) characters.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Maximum Dip Height - Chest", PinHiddenByDefault))
		float Maximum_Dip_Height_Chest = 100;



	/*
	* Differentiate between flat planar terrains and rotate terrains
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Differentiate between hard flat and rotated slopes", PinHiddenByDefault))
		bool Pelvis_Flat_Slope_Detection = false;


	/*
* Differentiate between flat planar terrains and rotate terrains
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Differentiate between hard flat and rotated slopes", PinHiddenByDefault))
		bool Chest_Flat_Slope_Detection = false;


	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Slope Detection Strength - Pelvis", PinHiddenByDefault))
	float Pelvis_Slope_Detection_Strength = 50;


	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Slope Detection Strength - Chest", PinHiddenByDefault))
	float Chest_Slope_Detection_Strength = 50;




	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Extra Forward Trace Offset", PinHiddenByDefault))
	float extra_forward_trace_Offset = 0;




	/*
	* This is the rotation alpha value of all the bones between the pelvis and chest.
	* 1 value means full solved state, while 0 means 0 solved state.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Rotation alpha between end points", PinHiddenByDefault))
		float rotation_power_between = 1;



	/*
	* An experimental feature, where the solver automatically switch between fabrik and reverse fabrik mode depending on the slope.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Experimental, meta = (DisplayName = "Automatic Fabrik Selection (Development)", PinHiddenByDefault))
		bool Use_Automatic_Fabrik_Selection = false;



	bool initialize_anim_array = false;





	/*
	* Controls the location interpolation speed of the body.
	*   Higher values means faster interpolation.
	*   Slower values means slower but also smoother interpolation.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Body Location Lerp Speed", PinHiddenByDefault))
		float Location_Lerp_Speed = 15;





	/*
	* Controls the rotation interpolation speed of the body.
	*   Higher values means faster interpolation.
	*   Slower values means slower but also smoother interpolation.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Body Rotation Lerp Speed", PinHiddenByDefault))
		float Rotation_Lerp_Speed = 5;




	/*
	* The location alpha value for the chest.
	*   1 means full solving of the chest transformation.
	*   0 means no solving of the chest transformation.
	*   Chest rotation still works regardless.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Chest Influence Alpha", PinHiddenByDefault))
		float Chest_Influence_Alpha = 1;






	/*
	* Controls the intensity of the pelvis forward rotation with respect to the slopes.
	* 0 means no rotation applied.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Pelvis Forward Rotation Intensity", PinHiddenByDefault))
		float Pelvis_ForwardRotation_Intensity = 0.7f;


	/*
	* Controls the intensity of the pelvis sideward rotation with respect to the slopes.
	* 0 means no rotation applied.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Pelvis Sideward Rotation Intensity", PinHiddenByDefault))
		float Body_Rotation_Intensity = 0.25f;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Pelvis Post Rotation Offset", PinHiddenByDefault))
		float Pelvis_Rotation_Offset = 0;




	/*
	* Controls the intensity of the chest forward rotation with respect to the slopes.
	* 0 means no rotation applied.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Chest Forward Rotation Intensity", PinHiddenByDefault))
		float Chest_ForwardRotation_Intensity = 0.7f;


	/*
	 * Controls the intensity of the chest sideward rotation with respect to the slopes.
	 * 0 means no rotation applied.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Chest Sideward Rotation Intensity", PinHiddenByDefault))
		float Chest_SidewardRotation_Intensity = 0.25f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Chest Post Rotation Offset", PinHiddenByDefault))
		float Chest_Rotation_Offset = 0;



	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Should solving data persist on fail scenarios", PinHiddenByDefault))
	//		bool should_solving_persist = false;



	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Should move down ?", PinHiddenByDefault))
	//		bool should_move_down = true;


	bool atleast_one_hit = false;

	bool feet_is_empty = true;



	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	FTransform DebugEffectorTransform;




	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	//	FTransform DebugEffectorTransformTwo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpineAdvancedTweaks, meta = (DisplayName = "Is spine always fully extended ?", PinHiddenByDefault))
		bool Full_Extended_Spine = true;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpineAdvancedTweaks, meta = (DisplayName = "Fully extended ratio", PinHiddenByDefault))
		float max_extension_ratio = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpineAdvancedTweaks, meta = (DisplayName = "Non extended ratio", PinHiddenByDefault))
		float min_extension_ratio = 0.97f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpineAdvancedTweaks, meta = (DisplayName = "Extension Switch Speed", PinHiddenByDefault))
		float extension_switch_speed = 2.0f;



	float Max_Range_Limit_Lerp = 1.05f;

	/*
	* Toggle this parameter to turn on/off ik.
	* Example use case : Disable it when character is jumping or flying in the air.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinShownByDefault))
		bool Enable_Solver = true;




	/*
	* Enable this to use the 4 cross-based trace lines to determine the rotation of the chest instead of the default normal method
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Use Alternate Cross-Based Chest Rotation", PinHiddenByDefault))
		bool Use_Fake_Chest_Rotations = false;



	/*
	* Enable this to use the 4 cross-based trace lines to determine the rotation of the pelvis instead of the default normal method
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Use Alternate Cross-Based Pelvis Rotation", PinHiddenByDefault))
		bool Use_Fake_Pelvis_Rotations = false;


	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float True_Rotation_Alpha = 1;


	/*
	*This will force the ik to work at all times. Animals will not be able to jump if this is enabled.
	*Only recommended for testing and debugging purposes.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		bool Force_Activation = false;


	/*
	*Enable this to use proper accurate feet placement logic. But it might provide jumpy nature for certain quadrupeds like horses when they are moving on slopes.
	*Recommended use case is to disable this when a quadruped animal is moving, but enable it when it is idle.
	*Recommended to always turn it on for bipeds like humans.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		bool accurate_feet_placement = true;



	/*
	 *Enable this to use the 4 cross based traces to turn off the ik, if any of the trace hits are too far.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		bool use_crosshair_trace_also_for_fail_distance = true;


	/*
	*Enable this to only solve the pelvis bone, while the remaining bones remain unchanged.
	*Recommended to enable it for bipeds like humans and raptors.
	* Also recommended for single root spiders.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Is Biped Character ?", PinHiddenByDefault))
		bool Only_Root_Solve = false;




	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float Smooth_Factor = 10;


	bool every_foot_dont_have_child = false;


	/*
	*Additional offset parameters to the overall bones
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		FVector Overall_PostSolved_Offset = FVector(0, 0, 0);



	/*
	*The up direction vector of the character in component space.
	* 99% cases, this should not be altered.
	* Only needed to alter on characters that do not follow the standard unreal character orientations.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ComponentDirectionSettings, meta = (DisplayName = "Up Direction Vector", PinHiddenByDefault))
		FVector character_direction_vector_CS = FVector(0, 0, 1);



	/*
	*The forward direction vector of the character in component space.
	* 99% cases, this should not be altered.
	* Only needed to alter on characters that do not follow the standard unreal character orientations.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ComponentDirectionSettings, meta = (DisplayName = "Forward Direction Vector", PinHiddenByDefault))
		FVector Forward_Direction_Vector = FVector(0, 1, 0);


	/*
	*If character is rigged and animated in the opposite direction to the standard unreal forward/right directions, then enable this.
	* 99% cases, this should not be altered.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ComponentDirectionSettings, meta = (DisplayName = "Flip forward and right rotation", PinHiddenByDefault))
		bool flip_forward_and_right = false;




	FVector root_location_saved;

	/*
	*Whether to use the reference pose or animated pose for calculations.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Miscellaneous, meta = (DisplayName = "Solver Reference Pose"))
		ERefPosePluginEnum SolverReferencePose;

	/*
	*Automatically correct the spine and feet bone pairing with respect to the typed bone names in the input settings.
	*  If correct bone names are typed, it makes no difference in the end.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Miscellaneous, meta = (DisplayName = "Strict Spine-Feet pair finding", PinHiddenByDefault))
		bool Spine_Feet_Connect = false;


	bool Is_ChainOrder_Calculated = false;


	FBoneContainer* SavedBoneContainer;



	float Root_Roll_Value = 0;

	float Root_Pitch_Value = 0;

	float diff_heights[6];


	TArray<FDragonData_SpineFeetPair> spine_Feet_pair;
	TArray<FHitResult> spine_hit_between;
	TArray<FName> Total_spine_bones;
	TArray<FDragonData_HitPairs> spine_hit_pairs;
	TArray<FHitResult> spine_hit_edges;
	TArray<FVector> spine_vectors_between;
	TArray<FVector> Full_Spine_OriginalLocations;
	TArray<float> Full_Spine_Heights;
	TArray<float> Total_spine_heights;
	TArray<float> Total_spine_alphas;
	TArray<float> Total_spine_angles;
	TArray<FVector> Total_Terrain_Locations;

	TArray<FCompactPoseBoneIndex> Spine_Indices;



	TArray<FCompactPoseBoneIndex> Extra_Spine_Indices;

	TArray<FDragonData_SpineFeetPair_TRANSFORM_WSPACE> spine_Transform_pairs;
	TArray<FDragonData_SpineFeetPair_TRANSFORM_WSPACE> spine_AnimatedTransform_pairs;

	TArray<FVector> spine_between_transforms;


	TArray<FVector> spine_between_offseted_transforms;
	TArray<float> spine_between_heights;


	TArray<FVector> snake_spine_positions;
	TArray<FTransform> spine_ChangeTransform_pairs_Obsolete;
	TArray<FVector> spine_LocDifference;
	TArray<FRotator> spine_RotDifference;
	TArray<FBoneTransform> RestBoneTransforms;
	TArray<FBoneTransform> AnimatedBoneTransforms;
	TArray<FBoneTransform> Original_AnimatedBoneTransforms;
	TArray<FBoneTransform> FinalBoneTransforms;
	TArray<FBoneTransform> BoneTransforms;

	TArray<FBoneTransform> LegTransforms;

	TArray<FCompactPoseBoneIndex> combined_indices;



	//	TArray<FDragonData_SpineFeetPair_heights> Total_spine_heights;

	float midpoint_height = 0;



	float maximum_spine_length = 0;

	float angle_data = 0;



	FTransform ChestEffectorTransform = FTransform::Identity;

	FTransform RootEffectorTransform = FTransform::Identity;


	int zero_transform_set = 0;

	FTransform Last_RootEffectorTransform = FTransform::Identity;
	FTransform Last_ChestEffectorTransform = FTransform::Identity;





	float spine_median_result = 0;







	/**Spine interpolation speed if snake mode is enabled.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SnakeSettings, meta = (DisplayName = "Snake Joint Speed (If snake true)", PinHiddenByDefault))
		float Snake_Joint_Speed = 2;


	/*
	* Enable snake mode.
	* If character is either a snake or a worm, enable this.
	* Can also be turned on for biped characters sleeping or laying on the ground.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SnakeSettings, meta = (DisplayName = "Is it a snake ?", PinHiddenByDefault))
		bool is_snake = false;




	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Experimental, meta = (DisplayName = "Use FeetTips(if exist)", PinHiddenByDefault))
	bool Use_FeetTips = false;


	/*
	 * Maximum distance from feet to terrain to allow the solving to happen.
	 * Higher value makes the ik to solve even on terrains far below the character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Maximum Feet-Terrain Fail Distance", PinHiddenByDefault))
		float Maximum_Feet_Distance = 250;




	/*
	* The minimum distance between feet and terrain to allow solving to happen.
	* Increase this value if ik is turning off easilly on slopes.
	* Too high of a value can make the ik to "stick" to the ground more, which can be undesirable.
	* Recommended to tweak this value until it feels right
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Minimum Terrain-Capsule Activation Distance", PinHiddenByDefault))
		float Minimum_Feet_Distance = 45;



	/*
	* Enable this to render the traces in the animgraph viewports.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Display LineTracing", PinHiddenByDefault))
		bool DisplayLineTrace = true;




	bool is_single_spine = false;



	FVector SmoothApproach(FVector pastPosition, FVector pastTargetPosition, FVector targetPosition, float speed);


	void Dragon_ImpactRotation(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases, bool is_reverse);


	void Dragon_VectorCreation(bool isPelvis, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases);


	void Dragon_SingleImpactRotationv2(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases, bool is_reverse);



	FVector RotateAroundPoint(FVector input_point, FVector forward_vector, FVector origin_point, float angle);



	void Snake_ImpactRotation(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases);


	FName GetChildBone(FName BoneName, USkeletalMeshComponent* skel_mesh);


	TArray<FName> BoneArrayMachine(int32 index, FName starting, FName ending, bool is_foot = false);

	bool Check_Loop_Exist(FVector feet_trace_offset, float feet_height, FName start_bone, FName input_bone, TArray<FName>& total_spine_bones);

	TArray<FDragonData_SpineFeetPair> Swap_Spine_Pairs(TArray<FDragonData_SpineFeetPair>& test_list);


	FCollisionQueryParams getDefaultSpineColliParams(FName name, AActor *me, bool debug_mode);

	void line_trace_func(USkeletalMeshComponent& skelmesh, FVector startpoint, FVector endpoint, FHitResult RV_Ragdoll_Hit, FName bone_text, FName trace_tag, FHitResult& Output, FLinearColor debug_color, bool debug_mode = false);


	FComponentSpacePoseContext* saved_pose;

	USkeletalMeshComponent *owning_skel;

	USkeleton* skeleton_ref;


	int32 tot_len_of_bones;
	void GetResetedPoseInfo(FCSPose<FCompactPose>& MeshBases);
	void GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);


	void Process_Legs_Solving(FComponentSpacePoseContext& MeshBases, TArray<FBoneTransform>& InputBoneTransforms, TArray<FBoneTransform>& OutBoneTransforms);



	void Make_All_Bones(FCSPose<FCompactPose>& MeshBases);


	DragonSpineOutput DragonSpineProcessor(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);


	DragonSpineOutput DragonSpineProcessor_Direct(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);



	DragonSpineOutput DragonSpineProcessor_Snake(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);



	DragonSpineOutput Dragon_Transformation(DragonSpineOutput& input, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);

	FRotator BoneRelativeConversion(FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases);

	FRotator BoneRelativeConversion_LEGACY(FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases);


	FVector GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex);



	void FABRIK_BodySystem(FComponentSpacePoseContext& Output,FBoneReference TipBone, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);


	void OrthoNormalize(FVector& Normal, FVector& Tangent);

	FQuat LookRotation(FVector lookAt, FVector upDirection);

	bool solve_should_fail = false;

	TArray<FColor> TraceLinearColor = TArray<FColor>();

	TArray<FVector> TraceStartList = TArray<FVector>();
	TArray<FVector> TraceEndList = TArray<FVector>();


	virtual void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const;


	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)  override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) override;

	//	virtual void Evaluate(FPoseContext& Output);

	virtual void Evaluate_AnyThread(FPoseContext& Output);


protected:
	// Interface for derived skeletal controls to implement
	// use this function to update for skeletal control base
	virtual void UpdateInternal(const FAnimationUpdateContext& Context);
	// use this function to evaluate for skeletal control base
	virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context);
	// Evaluate the new component-space transforms for the affected bones.
	//	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);

	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);


	void LineTraceControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);


	// return true if it is valid to Evaluate
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones);
	// initialize any bone references you have
	virtual void InitializeBoneReferences(FBoneContainer& RequiredBones);


};

