// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/ForsettiContext.h"
#include "ForsettiCore/ForsettiProtocols.h"
#include "ForsettiCore/ModuleModels.h"
#include "ForsettiCore/UIModels.h"

namespace Forsetti {

class ModuleRegistry;

class ExampleUIModule final : public IForsettiUIModule {
public:
    ModuleDescriptor descriptor() const override;
    ModuleManifest manifest() const override;
    UIContributions uiContributions() const override;

    void start(IForsettiModuleContext& context) override;
    void stop(IForsettiModuleContext& context) override;
};

void registerExampleUIModule(ModuleRegistry& registry);

} // namespace Forsetti
