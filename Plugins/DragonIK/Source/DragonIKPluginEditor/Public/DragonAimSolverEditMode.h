// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnrealWidget.h"
#include "AnimGraphNode_DragonAimSolver.h"

#include "DragonControlBaseEditMode.h"



struct DRAGONIKPLUGINEDITOR_API DragonAimSolverEditModes
{
	const static FEditorModeID DragonAimSolver;

};

class DRAGONIKPLUGINEDITOR_API FDragonAimSolverEditMode : public FDragonControlBaseEditMode
{
	public:
		/** IAnimNodeEditMode interface */
		virtual void EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode) override;
		virtual void ExitMode() override;
		virtual FVector GetWidgetLocation() const override;
		virtual FWidget::EWidgetMode GetWidgetMode() const override;
		virtual FName GetSelectedBone() const override;
		virtual void DoTranslation(FVector& InTranslation) override;
		virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
		virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;

		FVector Target_Transform_Value;
		
		float spine_index = 0;
		float foot_index = 0;


	private:
		struct FAnimNode_DragonAimSolver* RuntimeNode;
		class UAnimGraphNode_DragonAimSolver* GraphNode;
	};