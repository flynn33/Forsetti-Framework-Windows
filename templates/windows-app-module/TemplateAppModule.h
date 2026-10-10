// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "ForsettiCore/ForsettiProtocols.h"

class TemplateAppModule final : public Forsetti::IForsettiAppModule {
public:
    explicit TemplateAppModule(Forsetti::ModuleManifest manifest);
    Forsetti::ModuleDescriptor descriptor() const override;
    Forsetti::ModuleManifest manifest() const override;
    Forsetti::UIContributions uiContributions() const override;
    void start(Forsetti::IForsettiModuleContext& context) override;
    void stop(Forsetti::IForsettiModuleContext& context) override;
private:
    Forsetti::ModuleManifest manifest_;
};
