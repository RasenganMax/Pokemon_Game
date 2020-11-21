// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "ActorPickerMode.h"

class AActor;

class SSimpleActorPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SSimpleActorPicker )
		: _InitialActor(NULL)
		, _AllowClear(true)
		, _ActorFilter()
	{}
		SLATE_ARGUMENT( AActor*, InitialActor )
		SLATE_ARGUMENT( bool, AllowClear )
		SLATE_ARGUMENT( FOnShouldFilterActor, ActorFilter )
		SLATE_EVENT( FOnActorSelected, OnSet )
		SLATE_EVENT( FSimpleDelegate, OnClose )
		SLATE_EVENT( FSimpleDelegate, OnUseSelected )
	SLATE_END_ARGS()

	/**
	 * Construct the widget.
	 * @param	InArgs				Arguments for widget construction
	 * @param	InPropertyHandle	The property handle that this widget will operate on.
	 */
	void Construct( const FArguments& InArgs );

private:
	/**
	 * Use the currently selected Actor
	 */
	void HandleUseSelected();

	/** 
	 * Clear the referenced object
	 */
	void OnClear();

	/** 
	 * Delegate for handling selection in the scene outliner.
	 * @param	InActor	The chosen actor
	 */
	void OnActorSelected( AActor* InActor );

	/** 
	 * Set the value of the asset referenced by this property editor.
	 * Will set the underlying property handle if there is one.
	 * @param	InObject	The object to set the reference to
	 */
	void SetValue( AActor* InActor );

private:
	AActor* CurrentActor;

	/** Whether the asset can be 'None' in this case */
	bool bAllowClear;

	/** Delegate used to test whether an actor should be displayed or not */
	FOnShouldFilterActor ActorFilter;

	/** Delegate to call when our object value should be set */
	FOnActorSelected OnSet;

	/** Delegate for closing the containing menu */
	FSimpleDelegate OnClose;

	/** Delegate for using the currently selected actor */
	FSimpleDelegate OnUseSelected;
};
