// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/ForsettiProtocols.h"
#include "ForsettiCore/ModuleModels.h"

namespace Forsetti {

class ModuleRegistry;

class ExampleServiceModule final : public IForsettiModule {
public:
    ModuleDescriptor descriptor() const override;
    ModuleManifest manifest() const override;

    void start(IForsettiModuleContext& context) override;
    void stop(IForsettiModuleContext& context) override;
};

void registerExampleServiceModule(ModuleRegistry& registry);

} // namespace Forsetti
