// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "ACFGameplayAttributeGraphPin.h"



class FAscentAttributesEditor : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;

private:

	TSharedPtr<FACFGameplayAttributeGraphPanelPinFactory> ACFFactory;
};
