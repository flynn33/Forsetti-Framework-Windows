// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ExampleAppModule.h"
#include "ForsettiCore/ModuleRegistry.h"

namespace Forsetti {

void registerExampleAppModule(ModuleRegistry& registry)
{
    if (!registry.hasEntryPoint("ExampleAppModule")) {
        registry.registerModule("ExampleAppModule", []() -> std::unique_ptr<IForsettiModule> {
            return std::make_unique<ExampleAppModule>();
        });
    }
}

} // namespace Forsetti
