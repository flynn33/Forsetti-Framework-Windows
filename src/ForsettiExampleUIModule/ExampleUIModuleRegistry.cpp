// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ExampleUIModule.h"
#include "ForsettiCore/ModuleRegistry.h"

namespace Forsetti {

void registerExampleUIModule(ModuleRegistry& registry)
{
    if (!registry.hasEntryPoint("ExampleUIModule")) {
        registry.registerModule("ExampleUIModule", []() -> std::unique_ptr<IForsettiModule> {
            return std::make_unique<ExampleUIModule>();
        });
    }
}

} // namespace Forsetti
