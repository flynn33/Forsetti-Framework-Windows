// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "ForsettiCore/ForsettiProtocols.h"

class TemplateServiceModule final : public Forsetti::IForsettiModule {
public:
    explicit TemplateServiceModule(Forsetti::ModuleManifest manifest);
    Forsetti::ModuleDescriptor descriptor() const override;
    Forsetti::ModuleManifest manifest() const override;
    void start(Forsetti::IForsettiModuleContext& context) override;
    void stop(Forsetti::IForsettiModuleContext& context) override;
private:
    Forsetti::ModuleManifest manifest_;
};
