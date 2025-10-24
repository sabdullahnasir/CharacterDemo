// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFCurrencyComponent.h"
#include "ACFItemSystemFunctionLibrary.h"
#include "ARSFunctionLibrary.h"
#include "ARSStatisticsComponent.h"
#include "Net/UnrealNetwork.h"
#include <GameFramework/Pawn.h>
#include <Kismet/KismetSystemLibrary.h>

// Sets default values for this component's properties
UACFCurrencyComponent::UACFCurrencyComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    CurrencyAmount = 0.f;
    bCurrencyDropped = false;
}

void UACFCurrencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UACFCurrencyComponent, CurrencyAmount);
}

void UACFCurrencyComponent::RemoveCurrency_Implementation(float amount)
{
    CurrencyAmount -= amount;
    CurrencyAmount = FMath::Clamp(CurrencyAmount, 0.f, BIG_NUMBER);
    DispatchCurrencyChanged(-amount);
}

void UACFCurrencyComponent::SetCurrency_Implementation(float amount)
{
    const float delta = CurrencyAmount - amount;
    CurrencyAmount = amount;
    DispatchCurrencyChanged(delta);
}

void UACFCurrencyComponent::GatherCurrency_Implementation(float amount, UACFCurrencyComponent* sourceComp)
{
    if (sourceComp && sourceComp->GetCurrentCurrencyAmount() >= amount) {
        sourceComp->RemoveCurrency(amount);
        AddCurrency(amount);
    }
}

void UACFCurrencyComponent::AddCurrency_Implementation(float amount)
{
    CurrencyAmount += amount;
    DispatchCurrencyChanged(amount);
}

// Called when the game starts
void UACFCurrencyComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bDropCurrencyOnOwnerDeath && UKismetSystemLibrary::IsServer(this)) {
        UARSStatisticsComponent* statComp = GetOwner()->FindComponentByClass<UARSStatisticsComponent>();

        
        if (statComp && !statComp->OnHealthReachesZero.IsAlreadyBound(this, &UACFCurrencyComponent::HandleStatReachedZero)) {
            statComp->OnHealthReachesZero.AddDynamic(this, &UACFCurrencyComponent::HandleStatReachedZero);
        }
    }
}

void UACFCurrencyComponent::HandleCurrencyChanged()
{
}

void UACFCurrencyComponent::HandleStatReachedZero()
{
    if (GetOwner()->HasAuthority() ) {
        const float randomVariation = FMath::FRandRange(-CurrencyDropVariation, CurrencyDropVariation);
        const float finalDrop = CurrencyAmount + randomVariation;
        const APawn* CharacterOwner = Cast<APawn>(GetOwner());
        if (finalDrop > 1.f && CharacterOwner && !bCurrencyDropped) {
            const FVector startLoc = CharacterOwner->GetNavAgentLocation();
            UACFItemSystemFunctionLibrary::SpawnCurrencyItemNearLocation(this, finalDrop, startLoc, 100.f);
            RemoveCurrency(finalDrop);
            bCurrencyDropped = true;
        }
    }
}

void UACFCurrencyComponent::OnRep_Currency()
{
    OnCurrencyChanged.Broadcast(CurrencyAmount, 0);
    HandleCurrencyChanged();
}

void UACFCurrencyComponent::DispatchCurrencyChanged(float amount)
{
    OnCurrencyChanged.Broadcast(CurrencyAmount, amount);
    HandleCurrencyChanged();
}
