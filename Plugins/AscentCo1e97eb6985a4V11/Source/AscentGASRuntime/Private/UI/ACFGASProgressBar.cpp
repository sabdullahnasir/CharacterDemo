// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "UI/ACFGASProgressBar.h"
#include "AbilitySystemComponent.h"

void UACFGASProgressBar::NativeConstruct()
{
    Super::NativeConstruct();
    if (ACFProgressBar) {
        ACFProgressBar->SetFillColorAndOpacity(FillColor);
    }
}

void UACFGASProgressBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    //Can be optimized?
    UpdateBar();
    if (bSmoothInterpolation) {
        const float currentpercent = ACFProgressBar->GetPercent();
        if (!FMath::IsNearlyEqual(currentpercent, targetPercentage)) {
            const float newPercent = FMath::FInterpTo(currentpercent, targetPercentage, InDeltaTime, InterpolationSpeed);
            ACFProgressBar->SetPercent(newPercent);
        }
    }

    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UACFGASProgressBar::InitBar(UAbilitySystemComponent* inAbilityComp)
{
    if (!inAbilityComp) {
        return;
    }

    abilityComp = inAbilityComp;
    // Delegate per CurrentHealth
    abilityComp->GetGameplayAttributeValueChangeDelegate(CurrentAttribute).AddUObject(this, &UACFGASProgressBar::OnValueChanged);
    abilityComp->GetGameplayAttributeValueChangeDelegate(MaxAttribute).AddUObject(this, &UACFGASProgressBar::OnValueChanged);

    UpdateBar();
}

UACFGASProgressBar::UACFGASProgressBar(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UACFGASProgressBar::OnValueChanged(const FOnAttributeChangeData& Data)
{
    UpdateBar();
}

void UACFGASProgressBar::UpdateBar()
{
    if (!abilityComp) {
        return;
    }

    bool bValid;
    const float currValue = abilityComp->GetGameplayAttributeValue(CurrentAttribute, bValid);

    if (!bValid) {
        UE_LOG(LogTemp, Warning, TEXT("Missing CurrentValueAttribute - UACFGASProgressBar::UpdateBar"));
        return;
    }
    const float maxValue = abilityComp->GetGameplayAttributeValue(MaxAttribute, bValid);
    if (!bValid || maxValue == 0.f) {
        UE_LOG(LogTemp, Warning, TEXT("Missing MaxValueAttribute - UACFGASProgressBar::UpdateBar"));
        return;
    }
    const float percentage = currValue / maxValue;

    if (bSmoothInterpolation) {
        targetPercentage = percentage;
    } else if (ACFProgressBar) {
        ACFProgressBar->SetPercent(percentage);
    }
}
