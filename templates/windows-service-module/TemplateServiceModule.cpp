// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "TemplateServiceModule.h"
#include "ForsettiCore/ForsettiContext.h"
#include "ForsettiCore/ForsettiServices.h"
#include <stdexcept>
#include <utility>

TemplateServiceModule::TemplateServiceModule(Forsetti::ModuleManifest manifest)
    : manifest_(std::move(manifest))
{
    if (manifest_.moduleID != "com.forsetti.template.service" ||
        manifest_.entryPoint != "TemplateServiceModule" ||
        manifest_.moduleType != Forsetti::ModuleType::Service) {
        throw std::invalid_argument("Manifest identity or declaration does not match its factory.");
    }
}
Forsetti::ModuleDescriptor TemplateServiceModule::descriptor() const
{
    return {manifest_.moduleID, manifest_.displayName, manifest_.moduleVersion, manifest_.moduleType};
}
Forsetti::ModuleManifest TemplateServiceModule::manifest() const { return manifest_; }
void TemplateServiceModule::start(Forsetti::IForsettiModuleContext& context)
{
    auto storage = context.services()->resolve<Forsetti::IStorageService>();
    if (!storage) {
        throw std::runtime_error("Required capability-scoped storage service is unavailable.");
    }
    storage->set("started", "true");
}
void TemplateServiceModule::stop(Forsetti::IForsettiModuleContext& context)
{
    if (auto storage = context.services()->resolve<Forsetti::IStorageService>()) {
        storage->remove("started");
    }
}
