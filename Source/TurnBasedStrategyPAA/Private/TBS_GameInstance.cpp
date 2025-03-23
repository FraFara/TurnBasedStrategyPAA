// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_GameInstance.h"

// Score Functions
void UTBS_GameInstance::IncrementScoreHumanPlayer()
{
    ScoreHumanPlayer++;
    TotalGamesPlayed++;
}

void UTBS_GameInstance::IncrementScoreAiPlayer()
{
    ScoreAIPlayer++;
    TotalGamesPlayed++;
}

int32 UTBS_GameInstance::GetScoreHumanPlayer() const
{
    return ScoreHumanPlayer;
}

int32 UTBS_GameInstance::GetScoreAiPlayer() const
{
    return ScoreAIPlayer;
}

// Round Counter Functions
void UTBS_GameInstance::IncrementRound()
{
    CurrentRound++;
}

int32 UTBS_GameInstance::GetCurrentRound() const
{
    return CurrentRound;
}

// Turn Message Functions
FString UTBS_GameInstance::GetTurnMessage() const
{
    return TurnMessage;
}

void UTBS_GameInstance::SetStartingPlayerMessage(int32 PlayerIndex)
{
    TurnMessage = (PlayerIndex == 0) ?
        TEXT("Human Player starts the game") :
        TEXT("AI Player starts the game");
}

void UTBS_GameInstance::SetTurnMessage(const FString& Message)
{
    TurnMessage = Message;
}

// Move History Functions
void UTBS_GameInstance::AddMoveToHistory(int32 PlayerIndex, const FString& UnitType, const FString& ActionType,
    const FVector2D& FromPosition, const FVector2D& ToPosition, int32 Damage)
{
    // Format the player identifier
    FString PlayerIdentifier = (PlayerIndex == 0) ? TEXT("HP") : TEXT("AI");

    // Format the move entry based on action type
    FString MoveEntry;
    if (ActionType == TEXT("Move"))
    {
        // Format: HP: S B4 -> D6
        MoveEntry = FString::Printf(TEXT("%s: %s %d,%d -> %d,%d"), *PlayerIdentifier, *UnitType.Left(1), //First letter of the unit
            static_cast<int32>(FromPosition.X), static_cast<int32>(FromPosition.Y), static_cast<int32>(ToPosition.X), static_cast<int32>(ToPosition.Y));
    }
    else if (ActionType == TEXT("Attack"))
    {
        // Format: HP: S G8 7
        MoveEntry = FString::Printf(TEXT("%s: %s %d,%d %d"), *PlayerIdentifier, *UnitType.Left(1), // First letter of unit type
            static_cast<int32>(ToPosition.X), static_cast<int32>(ToPosition.Y), Damage);
    }
    else if (ActionType == TEXT("Skip"))
    {
        // Format: HP: S Skip
        MoveEntry = FString::Printf(TEXT("%s: %s Skip"), *PlayerIdentifier, *UnitType.Left(1)); // First letter of unit type
    }
    else if (ActionType == TEXT("Place"))
    {
        // Format: HP: S Place at D6
        MoveEntry = FString::Printf(TEXT("%s: %s Place at %d,%d"), *PlayerIdentifier, *UnitType.Left(1), // First letter of unit type
            static_cast<int32>(ToPosition.X), static_cast<int32>(ToPosition.Y));
    }

    // Add to history
    MoveHistory.Add(MoveEntry);
}

void UTBS_GameInstance::ClearMoveHistory()
{
    MoveHistory.Empty();
}

FString UTBS_GameInstance::GetFormattedMoveHistory() const
{
    FString History;

    for (int32 i = 0; i < MoveHistory.Num(); i++)
    {
        if (i > 0)
        {
            History.Append(TEXT("\n"));
        }
        History.Append(MoveHistory[i]);
    }

    return History;
}

// Game Statistics Functions
void UTBS_GameInstance::RecordGameResult(bool bPlayerWon)
{
    TotalGamesPlayed++;

    if (bPlayerWon)
    {
        ScoreHumanPlayer++;
    }
    else
    {
        ScoreAIPlayer++;
    }
}

void UTBS_GameInstance::ResetGameStatistics()
{
    TotalGamesPlayed = 0;
    ScoreHumanPlayer = 0;
    ScoreAIPlayer = 0;
    CurrentRound = 1;
    ClearMoveHistory();
}