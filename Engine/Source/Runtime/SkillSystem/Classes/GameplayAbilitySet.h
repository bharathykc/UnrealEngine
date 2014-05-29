// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilitySet.generated.h"

USTRUCT()
struct FGameplayAbilityBindInfoCommandIDPair
{
	GENERATED_USTRUCT_BODY()

	// We want this to be a drop down of valid lists but needs to be defined per game
	UPROPERTY(EditAnywhere, Category = BindInfo)
	FString		CommandString;

	UPROPERTY(EditAnywhere, Category = BindInfo)
	int32		InputID;
};

USTRUCT()
struct FGameplayAbilityBindInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=BindInfo)
	TArray<FGameplayAbilityBindInfoCommandIDPair>	CommandBindings;

	UPROPERTY(EditAnywhere, Category=BindInfo)
	TSubclassOf<UGameplayAbility>	GameplayAbilityClass;

	UPROPERTY(EditAnywhere, Category = BindInfo)
	class UGameplayAbility * GameplayAbilityInstance;
};

/**
* UGameplayAbilitySet
*	This contains a list of abilities along with key bindings. This will be very game specific, so it is expected for games to override this class.
*	Functions like InitializeAbilities and BindInputComponentToAbilities are setup to act on the attribute component (rather than attribute component processing the ability set).
*	Ideally this avoids games having to implement their own UAttributeComponent and UGameplayAbilitySet, and they can just implement thei own UGameplayAbilitySet.
*/
UCLASS(Blueprintable)
class SKILLSYSTEM_API UGameplayAbilitySet : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category=AbilitySet)
	TArray<FGameplayAbilityBindInfo>	Abilities;

	/** Give all of the abilities in this set to given AttributeComponent */
	virtual void InitializeAbilities(UAttributeComponent *AttributeComponent) const;

	/** Binds the ability set directly to this inputcomponent and attributecomponent. This is a convenience method, not all games will want to bind input directly to abilities. */
	virtual void BindInputComponentToAbilities(UInputComponent *InputComponent, UAttributeComponent *AttributeComponent) const;
};