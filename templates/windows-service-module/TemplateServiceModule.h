// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/ForsettiProtocols.h"
#include "ForsettiCore/ModuleModels.h"

#include <string>

class TemplateServiceModule final : public Forsetti::IForsettiModule {
public:
    explicit TemplateServiceModule(std::string configurationName);

    Forsetti::ModuleDescriptor descriptor() const override;
    Forsetti::ModuleManifest manifest() const override;
    void start(Forsetti::IForsettiModuleContext& context) override;
    void stop(Forsetti::IForsettiModuleContext& context) override;

private:
    std::string configurationName_;
};
