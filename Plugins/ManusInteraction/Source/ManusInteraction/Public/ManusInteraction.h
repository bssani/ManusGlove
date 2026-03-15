// Copyright 2024-2025 ManusInteraction Plugin

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogManusInteraction, Log, All);

class FManusInteractionModule : public IModuleInterface
{
public:
	static inline FManusInteractionModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FManusInteractionModule>("ManusInteraction");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ManusInteraction");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
