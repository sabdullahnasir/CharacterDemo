// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFEffectsManagerComponent.h"
#include "ACMCollisionsFunctionLibrary.h"
#include "ACMEffectsDispatcherComponent.h"
#include "ACMTypes.h"
#include "Actors/ACFCharacter.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Components/ACFDamageHandlerComponent.h"
#include "Config/ACFEffectsConfigDataAsset.h"
#include "Engine/World.h"
#include "Game/ACFFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging.h"
#include <CollisionQueryParams.h>
#include <Components/AudioComponent.h>
#include <Components/SceneComponent.h>
#include <NiagaraComponent.h>
#include <Particles/ParticleSystemComponent.h>
#include <PhysicalMaterials/PhysicalMaterial.h>

// Sets default values for this component's properties
UACFEffectsManagerComponent::UACFEffectsManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    FootstepNoiseByLocomotionState.Add(ELocomotionState::EWalk, .2f);
    FootstepNoiseByLocomotionState.Add(ELocomotionState::EJog, .6f);
    FootstepNoiseByLocomotionState.Add(ELocomotionState::ESprint, 1.f);

    FootstepNoiseByLocomotionStateWhenCrouched.Add(ELocomotionState::EWalk, .1f);
    FootstepNoiseByLocomotionStateWhenCrouched.Add(ELocomotionState::EJog, .3f);
    FootstepNoiseByLocomotionStateWhenCrouched.Add(ELocomotionState::EJog, .5f);
}

// Called when the game starts
void UACFEffectsManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    CharacterOwner = Cast<ACharacter>(GetOwner());

    UACFDamageHandlerComponent* DamageHanlder = GetOwner()->FindComponentByClass<UACFDamageHandlerComponent>();

    if ( DamageHanlder && !DamageHanlder->OnDamageReceived.IsAlreadyBound(this, &UACFEffectsManagerComponent::HandleDamageReceived)) {
        DamageHanlder->OnDamageReceived.AddDynamic(this, &UACFEffectsManagerComponent::HandleDamageReceived);
    }
}

void UACFEffectsManagerComponent::TriggerFootstepFX(FName footBone /*= NAME_None*/)
{
    if (!CharacterEffectsConfig) {
        return;
    }

    FBaseFX baseFX;

    if (CharacterEffectsConfig->ShouldCheckForSurface()) {
        CharacterEffectsConfig->TryGetFootstepFXBySurfaceType(GetCurrentTerrain(), baseFX);
    } else {
        CharacterEffectsConfig->GetDefaultFootstepFX(baseFX);
    }

    FVector ownerLocation;

    if (footBone != NAME_None) {
        ownerLocation = CharacterOwner->GetMesh()->GetSocketLocation(footBone);
    } else {
        ownerLocation = CharacterOwner->GetActorLocation();
    }

    FImpactFX fxToPlay = FImpactFX(baseFX, ownerLocation);

    const float noise = GetNoiseToEmitForCurrentLocomotionState();

    if (UKismetSystemLibrary::IsServer(this) && noise != 0.f) {
        CharacterOwner->MakeNoise(noise, CharacterOwner, CharacterOwner->GetActorLocation());
    }

    //Not needed for footstep to spawn a cue as it's always handled only clientside
    UACMCollisionsFunctionLibrary::SpawnSoundAndParticleAtLocation(CharacterOwner, fxToPlay);
}

EPhysicalSurface UACFEffectsManagerComponent::GetCurrentTerrain()
{
    ensure(CharacterOwner);
    if (CharacterOwner) {
        FCollisionQueryParams outTraceParams = FCollisionQueryParams(FName(TEXT("Trace")), true, CharacterOwner);

        outTraceParams.bTraceComplex = true;
        outTraceParams.bReturnPhysicalMaterial = true;

        const FVector Start = CharacterOwner->GetActorLocation();
        const FVector End = Start + (FVector(0, 0, 1) * TraceLengthByActorLocation * -1);

        // Re-initialize hit info
        FHitResult outTerrain;

        const bool bHit = GetWorld()->LineTraceSingleByObjectType(outTerrain, Start, End, ECC_WorldStatic, outTraceParams);

        if (bHit) {
            const UPhysicalMaterial* PhysicsMtl = outTerrain.PhysMaterial.Get();
            const EPhysicalSurface surface = PhysicsMtl->SurfaceType;
            return surface;
        }
    }

    return EPhysicalSurface::SurfaceType_Max;
}

void UACFEffectsManagerComponent::HandleDamageReceived(const FACFDamageEvent& damageEvent)
{
    if (GetOwner()->HasAuthority()) {
        PlayHitReactionEffect(damageEvent);
    }
    OnDamageImpactReceived(damageEvent);
}

void UACFEffectsManagerComponent::OnDamageImpactReceived_Implementation(const FACFDamageEvent& damageEvent)
{
}



void UACFEffectsManagerComponent::PlayEffectAttached_Implementation(const FActionEffect& attachedFX)
{
    ClientsPlayEffectAttached(attachedFX);
}

void UACFEffectsManagerComponent::StopEffectAttached_Implementation(const FActionEffect& attachedFX)
{
    ClientsStopEffectAttached(attachedFX);
}

void UACFEffectsManagerComponent::ClientsPlayEffectAttached_Implementation(const FActionEffect& attachedFX)
{
    UACMEffectsDispatcherComponent* effDisp = UACMCollisionsFunctionLibrary::GetEffectDispatcher(this);
    if (effDisp) {
        FAttachedComponents attComp = UACMCollisionsFunctionLibrary::SpawnSoundAndParticleAttached(attachedFX, CharacterOwner);
        ActiveFX.Add(attachedFX.GetGuid(), attComp);
    }
}

void UACFEffectsManagerComponent::ClientsStopEffectAttached_Implementation(const FActionEffect& attachedFX)
{
    if (ActiveFX.Contains(attachedFX.GetGuid())) {
        FAttachedComponents attComp = ActiveFX.FindChecked(attachedFX.GetGuid());
        if (attComp.AudioComp) {
            
            attComp.AudioComp->DestroyComponent();
        }
        if (attComp.CascadeComp) {
            attComp.CascadeComp->DestroyComponent();
        }

        if (attComp.NiagaraComp) {
            attComp.NiagaraComp->DestroyComponent();
        }
        ActiveFX.Remove(attachedFX.GetGuid());
    }
}

void UACFEffectsManagerComponent::PlayHitReactionEffect(const FACFDamageEvent& damageEvent)
{
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(ACFLog, Warning, TEXT("Trying to play a replicated GameplayCue from a client!"));
        return;
    }
    UAbilitySystemComponent* ASC = GetOwner()->FindComponentByClass<UAbilitySystemComponent>();
    if (!ASC) {
        UE_LOG(ACFLog, Error, TEXT("Missing Ability Comp in Instigator!! - UACFEffectsManagerComponent "));
        return;
    }
    FACFGameplayEffectContext* context = new FACFGameplayEffectContext();
    context->AddInstigator(GetOwner(), GetOwner());
    context->DamageTypeClass = damageEvent.DamageClass;
    context->AddHitResult(damageEvent.hitResult);
    context->ActionEffectTag = damageEvent.HitResponseAction;
    FGameplayEffectContextHandle ContextHandle = FGameplayEffectContextHandle(context);

    FGameplayCueParameters Params;
    Params.EffectContext = ContextHandle;

    ASC->ExecuteGameplayCue(DefaultHitCue, Params);
    /*
    FImpactFX outFX;
    if (TryGetDamageFX(damageEvent.HitResponseAction, damageEvent.DamageClass, outFX)) {
        UACMEffectsDispatcherComponent* effDisp = UACMCollisionsFunctionLibrary::GetEffectDispatcher(this);

        const FName boneName = damageEvent.hitResult.BoneName != NAME_None ? damageEvent.hitResult.BoneName : DefaultHitBoneName;
        if (effDisp) {
            FActionEffect effect(outFX, ESpawnFXLocation::ESpawnAttachedToSocketOrBone, boneName);
            effDisp->PlayActionEffectLocally(effect, CharacterOwner);
        }
    } else {
        UE_LOG(ACFLog, Error, TEXT("Missing Characters Effect Config! - UACFEffectsManagerComponent::TriggerFootstepFX "));
    }*/
}

bool UACFEffectsManagerComponent::TryGetDamageFX(const FGameplayTag& HitRection, const TSubclassOf<class UDamageType>& DamageType, FBaseFX& outFX)
{
    if (CharacterEffectsConfig && CharacterEffectsConfig->TryGetDamageEffectsByHitReactionAndDamageType(HitRection, DamageType, outFX)) {
        return true;
    }
    return false;
}

float UACFEffectsManagerComponent::GetNoiseToEmitForCurrentLocomotionState() const
{
    const UACFCharacterMovementComponent* locComp = CharacterOwner->FindComponentByClass<UACFCharacterMovementComponent>();
    if (locComp) {
        const ELocomotionState currentLocState = locComp->GetCurrentLocomotionState();
        return GetNoiseToEmitByLocomotionState(currentLocState);
    }
    UE_LOG(ACFLog, Warning, TEXT("Missing Locomotion Component! - UACFEffectsManagerComponent::GetNoiseToEmitForCurrentLocomotionState "));
    return 0.f;
}

float UACFEffectsManagerComponent::GetNoiseToEmitByLocomotionState(ELocomotionState locState) const
{
    float outNoise = 0.f;
    if (CharacterOwner->bIsCrouched && FootstepNoiseByLocomotionStateWhenCrouched.Contains(locState)) {
        outNoise = *FootstepNoiseByLocomotionStateWhenCrouched.Find(locState);
    } else if (FootstepNoiseByLocomotionState.Contains(locState)) {
        outNoise = *FootstepNoiseByLocomotionState.Find(locState);
    }
    return outNoise;
}
