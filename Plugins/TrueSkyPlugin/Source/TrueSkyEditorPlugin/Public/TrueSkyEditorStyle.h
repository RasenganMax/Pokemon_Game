// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Styling/SlateStyle.h"

/*
 * trueSKY Editor Plugin Style Set.
*/
class FTrueSkyEditorStyle : public FSlateStyleSet
{
public:
	FTrueSkyEditorStyle();
	~FTrueSkyEditorStyle();

private:
	static TOptional< FTrueSkyEditorStyle > Singleton;

public:
	static FTrueSkyEditorStyle& Get( )
	{
		if( !Singleton.IsSet( ) )
		{
			Singleton.Emplace( );
		}

		return( Singleton.GetValue( ) );
	}

	static void Destroy( )
	{
		Singleton.Reset( );
	}
};