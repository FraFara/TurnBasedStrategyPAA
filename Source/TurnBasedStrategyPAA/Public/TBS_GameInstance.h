// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Unit.h"
#include "TBS_GameInstance.generated.h"

/**
 * Game Instance class for the Turn-Based Strategy game
 */
UCLASS()
class TURNBASEDSTRATEGYPAA_API UTBS_GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:

    // Score values
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Statistics")
    int32 ScoreHumanPlayer = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Statistics")
    int32 ScoreAIPlayer = 0;

    // Round counter
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Statistics")
    int32 CurrentRound = 1;

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

    // Functions to manage round counter
    UFUNCTION(BlueprintCallable, Category = "Game Statistics")
    void IncrementRound();

    UFUNCTION(BlueprintCallable, Category = "Game Statistics")
    int32 GetCurrentRound() const;

    // Functions to manage turn messages
    UFUNCTION(BlueprintCallable, Category = "Game UI")
    FString GetTurnMessage() const;

    UFUNCTION(BlueprintCallable, Category = "Game UI")
    void SetStartingPlayerMessage(int32 PlayerIndex);

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

    // Track the winner of the current game
    void SetWinner(int32 WinnerIndex);
    int32 GetWinner() const;
    bool HasWinner() const;
    FString GetWinnerMessage() const;
    void ResetWinner();

    int32 CurrentWinner = -1; // -1 means no winner yet

};