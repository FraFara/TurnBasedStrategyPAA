// Fill out your copyright notice in the Description page of Project Settings.

#include "AISelectionWidget.h"

void UAISelectionWidget::OnEasyAISelected()
{
    // Easy AI selected (NaiveAI)
    OnAIDifficultySelected.Broadcast(false);
}

void UAISelectionWidget::OnHardAISelected()
{
    // Hard AI selected (SmartAI)
    OnAIDifficultySelected.Broadcast(true);
}