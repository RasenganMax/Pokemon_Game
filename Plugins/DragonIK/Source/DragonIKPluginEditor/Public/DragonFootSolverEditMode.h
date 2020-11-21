// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnrealWidget.h"
#include "AnimGraphNode_DragonFeetSolver.h"

#include "DragonControlBaseEditMode.h"



struct DRAGONIKPLUGINEDITOR_API DragonFootSolverEditModes
{
	const static FEditorModeID DragonFootSolver;

};

class DRAGONIKPLUGINEDITOR_API FDragonFootSolverEditMode : public FDragonControlBaseEditMode
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

		float spine_index = 0;
		float foot_index = 0;

	private:
		struct FAnimNode_DragonFeetSolver* RuntimeNode;
		class UAnimGraphNode_DragonFeetSolver* GraphNode;
	};