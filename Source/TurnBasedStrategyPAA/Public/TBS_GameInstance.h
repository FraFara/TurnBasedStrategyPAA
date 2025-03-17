// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TBS_GameInstance.generated.h"

/**
 * Game Instance class for the Turn-Based Strategy game
 */
UCLASS()
class TURNBASEDSTRATEGYPAA_API UTBS_GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    //// Initial game configuration -> not sure i need this
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Configuration")
    //int32 DefaultGridSize = 25;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Configuration")
    //int32 UnitsPerPlayer = 2;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Configuration")
    //float ObstaclePercentage = 10.0f;

    // Score values
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Statistics")
    int32 ScoreHumanPlayer = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Statistics")
    int32 ScoreAIPlayer = 0;

    // Turn message
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game UI")
    FString TurnMessage = TEXT("Current Player");

    // Game statistics
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Statistics")
    int32 TotalGamesPlayed = 0;

    // Move history
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game History")
    TArray<FString> MoveHistory;

    // Functions to manage scores
    UFUNCTION(BlueprintCallable, Category = "Game Statistics")
    void IncrementScoreHumanPlayer();

    UFUNCTION(BlueprintCallable, Category = "Game Statistics")
    void IncrementScoreAiPlayer();

    UFUNCTION(BlueprintCallable, Category = "Game Statistics")
    int32 GetScoreHumanPlayer() const;

    UFUNCTION(BlueprintCallable, Category = "Game Statistics")
    int32 GetScoreAiPlayer() const;

    // Functions to manage turn messages
    UFUNCTION(BlueprintCallable, Category = "Game UI")
    FString GetTurnMessage() const;

    UFUNCTION(BlueprintCallable, Category = "Game UI")
    void SetTurnMessage(const FString& Message);

    // Functions to manage move history
    UFUNCTION(BlueprintCallable, Category = "Game History")
    void AddMoveToHistory(int32 PlayerIndex, const FString& UnitType, const FString& ActionType, const FVector2D& FromPosition, const FVector2D& ToPosition, int32 Damage);

    UFUNCTION(BlueprintCallable, Category = "Game History")
    void ClearMoveHistory();

    UFUNCTION(BlueprintCallable, Category = "Game History")
    FString GetFormattedMoveHistory() const;

    // Functions to update game statistics
    UFUNCTION(BlueprintCallable, Category = "Game Statistics")
    void RecordGameResult(bool bPlayerWon);

    UFUNCTION(BlueprintCallable, Category = "Game Statistics")
    void ResetGameStatistics();
};