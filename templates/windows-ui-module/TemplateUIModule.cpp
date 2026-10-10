// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "TemplateUIModule.h"
#include "ForsettiCore/ForsettiContext.h"
#include <stdexcept>
#include <utility>

TemplateUIModule::TemplateUIModule(Forsetti::ModuleManifest manifest)
    : manifest_(std::move(manifest))
{
    if (manifest_.moduleID != "com.forsetti.template.ui" ||
        manifest_.entryPoint != "TemplateUIModule" ||
        manifest_.moduleType != Forsetti::ModuleType::UI ||
        !manifest_.runtimeRequirements.ui ||
        manifest_.runtimeRequirements.ui->viewIDs.size() != 1 ||
        manifest_.runtimeRequirements.ui->slotIDs.size() != 1) {
        throw std::invalid_argument("Manifest identity or declaration does not match its factory.");
    }
}
Forsetti::ModuleDescriptor TemplateUIModule::descriptor() const
{
    return {manifest_.moduleID, manifest_.displayName, manifest_.moduleVersion, manifest_.moduleType};
}
Forsetti::ModuleManifest TemplateUIModule::manifest() const { return manifest_; }
Forsetti::UIContributions TemplateUIModule::uiContributions() const
{
    Forsetti::UIContributions contributions;
    contributions.viewInjections.push_back({"TemplateUIModule.content",
        manifest_.runtimeRequirements.ui->slotIDs.front(), manifest_.runtimeRequirements.ui->viewIDs.front(), 0});
    return contributions;
}
void TemplateUIModule::start(Forsetti::IForsettiModuleContext& context)
{
    context.logger()->log(Forsetti::LogLevel::Info, "UI module started", context.moduleID());
}
void TemplateUIModule::stop(Forsetti::IForsettiModuleContext& context)
{
    context.logger()->log(Forsetti::LogLevel::Info, "UI module stopped", context.moduleID());
}
