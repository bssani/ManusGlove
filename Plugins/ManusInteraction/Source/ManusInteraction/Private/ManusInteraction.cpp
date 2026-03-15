// Copyright 2024-2025 ManusInteraction Plugin

#include "ManusInteraction.h"

DEFINE_LOG_CATEGORY(LogManusInteraction);

#define LOCTEXT_NAMESPACE "FManusInteractionModule"

void FManusInteractionModule::StartupModule()
{
	UE_LOG(LogManusInteraction, Log, TEXT("ManusInteraction module started."));
}

void FManusInteractionModule::ShutdownModule()
{
	UE_LOG(LogManusInteraction, Log, TEXT("ManusInteraction module shut down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FManusInteractionModule, ManusInteraction)
