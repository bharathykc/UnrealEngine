// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SkillSystemDebugHUD.generated.h"

namespace EAlignHorizontal
{
	enum Type
	{
		Left,
		Center,
		Right,
	};
}

namespace EAlignVertical
{
	enum Type
	{
		Top,
		Center,
		Bottom,
	};
}

UCLASS()
class ASkillSystemDebugHUD : public AHUD
{
	GENERATED_UCLASS_BODY()

	/** main HUD update loop */
	virtual void DrawHUD() OVERRIDE;

	void DrawWithBackground(UFont* InFont, const FString& Text, const FColor& TextColor, EAlignHorizontal::Type HAlign, float& OffsetX, EAlignVertical::Type VAlign, float& OffsetY, float Alpha=1.f);

	void DrawDebugHUD(UCanvas* Canvas, APlayerController* PC);

private:

	void DrawDebugAttributeComponent(UAttributeComponent *Component);

};