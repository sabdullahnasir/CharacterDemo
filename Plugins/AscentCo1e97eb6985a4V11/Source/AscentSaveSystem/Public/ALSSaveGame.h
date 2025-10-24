// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ALSSaveTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

#include "ALSSaveGame.generated.h"

/**
 *
 */
UCLASS()
class ASCENTSAVESYSTEM_API UALSSaveGame : public USaveGame {
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    TMap<FString, FALSLevelData> Levels;

    UPROPERTY(SaveGame)
    TMap<FString, FALSPlayerData> Players;

    UPROPERTY(SaveGame)
    FALSPlayerData LocalPlayer;

    UPROPERTY(Savegame)
    int32 PlayTime;

public:
    UALSSaveGame()
    {
        PlayTime = 0; // Default-initialized (equivalent to a null value)
    }

    // Returns the saved playtime
    int32 GetPlayTime() const
    {
        return PlayTime;
    }

    // Sets the saved playtime
    void SetPlayTime(const int32& NewPlayTime)
    {
        PlayTime = NewPlayTime;
    }

    // Stores a waypoint actor data for a given level
    void StoreWPActors(const FString& levelName, const FALSActorData& actorData)
    {
        FALSLevelData* levelData = Levels.Find(levelName);
        if (levelData) {
            levelData->AddWPActorRecord(actorData);
        } else {
            FALSLevelData newLevelData;
            newLevelData.AddWPActorRecord(actorData);
            Levels.Add(levelName, newLevelData);
        }
    }

    // Retrieves stored waypoint actor data for a given level and actor
    bool TryGetStoredWPActor(const FString& levelName, AActor* actor, FALSActorData& outData)
    {
        FALSLevelData* levelData = Levels.Find(levelName);
        if (levelData && levelData->HasWPActor(actor)) {
            outData = *(levelData->GetWPActorData(actor));
            return true;
        }
        return false;
    }

    // Stores the local player's data
    void StoreLocalPlayer(const FALSPlayerData& actorData)
    {
        LocalPlayer = actorData;
    }

    // Retrieves the stored local player's data
    void GetLocalPlayer(FALSPlayerData& outData)
    {
        outData = LocalPlayer;
    }

    // Stores a player's data associated with a unique player ID
    void StorePlayer(const FString& playerID, const FALSPlayerData& actorData)
    {
        Players.Add(playerID, actorData);
    }

    // Retrieves a stored player's data using their unique player ID
    bool TryGetPlayer(const FString& playerID, FALSPlayerData& outData)
    {
        if (const FALSPlayerData* foundData = Players.Find(playerID)) {
            outData = *foundData;
            return true;
        }
        return false;
    }

    // Retrieves level data if it exists
    bool TryGetLevelData(const FString& levelName, FALSLevelData& outData)
    {
        if (const FALSLevelData* foundData = Levels.Find(levelName)) {
            outData = *foundData;
            return true;
        }
        return false;
    }

    // Adds a new level and stores its data
    void AddLevel(const FString& levelName, const FALSLevelData& levelData)
    {
        Levels.Add(levelName, levelData);
    }

    // Called before saving this slot
    UFUNCTION(BlueprintNativeEvent, Category = ALS)
    void OnSaved();

    // Called after loading this slot
    UFUNCTION(BlueprintNativeEvent, Category = ALS)
    void OnLoaded();
};
