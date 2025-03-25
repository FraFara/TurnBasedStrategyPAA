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

    // Convert numerical coordinates to letter+number format
    // X axis: A-Y (0-24 in grid)
    // Y axis: 1-25 (0-24 in grid)
    auto ConvertToGridFormat = [](const FVector2D& Position) -> FString
        {
            // Convert X to a letter (A-Y)
            TCHAR Letter = 'A' + static_cast<int32>(Position.X);
            // Convert Y to a number (1-25)
            int32 Number = static_cast<int32>(Position.Y) + 1;

            return FString::Printf(TEXT("%c%d"), Letter, Number);
        };

    // Format the move entry based on action type
    FString MoveEntry;
    if (ActionType == TEXT("Move"))
    {
        // Format: HP: S B4 -> D6
        MoveEntry = FString::Printf(TEXT("%s: %s %s -> %s"), *PlayerIdentifier, *UnitType.Left(1), //First letter of the unit
            *ConvertToGridFormat(FromPosition), *ConvertToGridFormat(ToPosition));
    }
    else if (ActionType == TEXT("Attack"))
    {
        // Format: HP: S G8 7
        MoveEntry = FString::Printf(TEXT("%s: %s %s %d"), *PlayerIdentifier, *UnitType.Left(1), // First letter of unit type
            *ConvertToGridFormat(ToPosition), Damage);
    }
    else if (ActionType == TEXT("Skip"))
    {
        // Format: HP: S Skip
        MoveEntry = FString::Printf(TEXT("%s: %s Skip"), *PlayerIdentifier, *UnitType.Left(1)); // First letter of unit type
    }
    else if (ActionType == TEXT("Place"))
    {
        // Format: HP: S Place at D6
        MoveEntry = FString::Printf(TEXT("%s: %s Place at %s"), *PlayerIdentifier, *UnitType.Left(1), // First letter of unit type
            *ConvertToGridFormat(ToPosition));
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
    ResetWinner(); // Reset the winner tracking
}

void UTBS_GameInstance::SetWinner(int32 WinnerIndex)
{
    CurrentWinner = WinnerIndex;

    // Update the turn message to include winner info
    FString WinnerMessage = GetWinnerMessage();

    // Only append winner message if not empty
    if (!WinnerMessage.IsEmpty())
    {
        TurnMessage = TurnMessage + TEXT(" | ") + WinnerMessage;
    }
}

int32 UTBS_GameInstance::GetWinner() const
{
    return CurrentWinner;
}

bool UTBS_GameInstance::HasWinner() const
{
    return CurrentWinner >= 0;
}

FString UTBS_GameInstance::GetWinnerMessage() const
{
    if (CurrentWinner < 0)
    {
        return FString(); // No winner yet
    }

    return (CurrentWinner == 0) ? TEXT("Human Player Won!") : TEXT("AI Won!");
}

void UTBS_GameInstance::ResetWinner()
{
    CurrentWinner = -1;
}