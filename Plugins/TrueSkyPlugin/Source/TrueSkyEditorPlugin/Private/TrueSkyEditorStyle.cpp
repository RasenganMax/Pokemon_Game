// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkyEditorStyle.h"

#include "Math/Vector2D.h"

#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

// Static class definition.
TOptional< FTrueSkyEditorStyle > FTrueSkyEditorStyle::Singleton;

//----------------------------------------------------------------------------------------------------
FTrueSkyEditorStyle::FTrueSkyEditorStyle( )
: FSlateStyleSet( "TrueSkyEditorStyle" )
{
	const FVector2D icon64 = FVector2D( 64.0f, 64.0f );
	const FVector2D icon16 = FVector2D( 16.0f, 16.0f );

	SetContentRoot( FPaths::EnginePluginsDir( )/TEXT( "TrueSkyPlugin/Resources" ) );

	Set( "ClassThumbnail.TrueSkySequenceActor", new FSlateImageBrush( RootToContentDir( TEXT( "Simul64.png" ) ), icon64 ) );
	Set( "ClassIcon.TrueSkySequenceActor", new FSlateImageBrush( RootToContentDir( TEXT( "Simul16.png" ) ), icon16 ) );

	Set( "ClassThumbnail.TrueSkySequenceAsset", new FSlateImageBrush( RootToContentDir( TEXT( "Simul64.png" ) ), icon64 ) );
	Set( "ClassIcon.TrueSkySequenceAsset", new FSlateImageBrush( RootToContentDir( TEXT( "Simul16.png" ) ), icon16 ) );

	Set( "ClassThumbnail.TrueSkyLight", new FSlateImageBrush( RootToContentDir( TEXT( "Simul64.png" ) ), icon64 ) );
	Set( "ClassIcon.TrueSkyLight", new FSlateImageBrush( RootToContentDir( TEXT( "Simul16.png" ) ), icon16 ) );

	Set( "ClassThumbnail.TrueSkyWater", new FSlateImageBrush( RootToContentDir( TEXT( "Simul64.png" ) ), icon64 ) );
	Set( "ClassIcon.TrueSkyWater", new FSlateImageBrush( RootToContentDir( TEXT( "Simul16.png" ) ), icon16 ) );
	
	
	FSlateStyleRegistry::RegisterSlateStyle( *this );
}

//----------------------------------------------------------------------------------------------------
FTrueSkyEditorStyle::~FTrueSkyEditorStyle( )
{
	FSlateStyleRegistry::UnRegisterSlateStyle( *this );
}