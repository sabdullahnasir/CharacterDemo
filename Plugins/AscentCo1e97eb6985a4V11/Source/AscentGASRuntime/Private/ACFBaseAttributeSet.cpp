// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACFBaseAttributeSet.h"
#include "ACFGASTypes.h"
#include "AbilitySystemComponent.h"

void UACFBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    ExecuteClamp(Attribute, NewValue);
    Super::PreAttributeChange(Attribute, NewValue);
}

void UACFBaseAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    ExecuteClamp(Attribute, NewValue);
    Super::PreAttributeBaseChange(Attribute, NewValue);
}

void UACFBaseAttributeSet::ExecuteClamp(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Clamps.Contains(Attribute)) {
        const UAbilitySystemComponent* abilityComp = GetOwningAbilitySystemComponent();
        const FAttributeClamps* clampData = Clamps.FindByKey(Attribute);
        if (abilityComp && clampData) {
            bool bFound;
            const float MaxValue = abilityComp->GetGameplayAttributeValue(clampData->MaxValueAttribute, bFound);
            const float currentValue = abilityComp->GetGameplayAttributeValue(clampData->AttributeToClamp, bFound);
            if (bFound) {
                if (clampData->bClampToZero) {
                    NewValue = FMath::Clamp(NewValue, 0.f, MaxValue);
                } else {
                    NewValue = FMath::Clamp(NewValue, -BIG_NUMBER, MaxValue);
                }
            }
        }
    }
}
