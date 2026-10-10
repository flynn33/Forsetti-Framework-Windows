// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

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
