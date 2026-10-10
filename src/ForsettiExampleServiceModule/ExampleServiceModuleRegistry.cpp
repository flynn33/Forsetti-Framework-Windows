// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "ExampleServiceModule.h"
#include "ForsettiCore/ModuleRegistry.h"

namespace Forsetti {

void registerExampleServiceModule(ModuleRegistry& registry)
{
    if (!registry.hasEntryPoint("ExampleServiceModule")) {
        registry.registerModule("ExampleServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            return std::make_unique<ExampleServiceModule>();
        });
    }
}

} // namespace Forsetti
