// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "TemplateAppModule.h"
#include "ForsettiCore/ForsettiContext.h"
#include <stdexcept>
#include <utility>

TemplateAppModule::TemplateAppModule(Forsetti::ModuleManifest manifest)
    : manifest_(std::move(manifest))
{
    if (manifest_.moduleID != "com.forsetti.template.app" ||
        manifest_.entryPoint != "TemplateAppModule" ||
        manifest_.moduleType != Forsetti::ModuleType::App ||
        !manifest_.runtimeRequirements.ui ||
        manifest_.runtimeRequirements.ui->viewIDs.size() != 1 ||
        manifest_.runtimeRequirements.ui->slotIDs.size() != 1) {
        throw std::invalid_argument("Manifest identity or declaration does not match its factory.");
    }
}
Forsetti::ModuleDescriptor TemplateAppModule::descriptor() const
{
    return {manifest_.moduleID, manifest_.displayName, manifest_.moduleVersion, manifest_.moduleType};
}
Forsetti::ModuleManifest TemplateAppModule::manifest() const { return manifest_; }
Forsetti::UIContributions TemplateAppModule::uiContributions() const
{
    Forsetti::UIContributions contributions;
    contributions.viewInjections.push_back({"TemplateAppModule.content",
        manifest_.runtimeRequirements.ui->slotIDs.front(), manifest_.runtimeRequirements.ui->viewIDs.front(), 0});
    return contributions;
}
void TemplateAppModule::start(Forsetti::IForsettiModuleContext& context)
{
    context.publishEvent("com.forsetti.template.app.started");
}
void TemplateAppModule::stop(Forsetti::IForsettiModuleContext& context)
{
    context.publishEvent("com.forsetti.template.app.stopped");
}
