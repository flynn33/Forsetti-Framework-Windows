// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "TemplateServiceModule.h"
#include "ForsettiCore/ForsettiContext.h"
#include "ForsettiCore/ForsettiServices.h"

#include <utility>

TemplateServiceModule::TemplateServiceModule(std::string configurationName)
    : configurationName_(std::move(configurationName))
{
}

Forsetti::ModuleDescriptor TemplateServiceModule::descriptor() const
{
    return Forsetti::ModuleDescriptor{
        .moduleID = "com.forsetti.template.service",
        .displayName = "Template Service Module",
        .version = Forsetti::SemVer{0, 1, 0},
        .type = Forsetti::ModuleType::Service
    };
}

Forsetti::ModuleManifest TemplateServiceModule::manifest() const
{
    (void)configurationName_;
    return Forsetti::ModuleManifest{
        .schemaVersion = "1.1",
        .moduleID = "com.forsetti.template.service",
        .displayName = "Template Service Module",
        .moduleVersion = Forsetti::SemVer{0, 1, 0},
        .moduleType = Forsetti::ModuleType::Service,
        .supportedPlatforms = {Forsetti::Platform::Windows},
        .minForsettiVersion = Forsetti::SemVer{0, 2, 0},
        .maxForsettiVersion = std::nullopt,
        .capabilitiesRequested = {Forsetti::Capability::Storage},
        .iapProductID = std::nullopt,
        .entryPoint = "TemplateServiceModule",
        .manifestTemplateVersion = Forsetti::ManifestTemplateVersion::V1_1,
        .defaultModuleRole = std::nullopt,
        .runtimeRequirements = Forsetti::ModuleRuntimeRequirements{
            .io = {
                Forsetti::ModuleIORequirement{
                    .requirementID = "storage.template-state",
                    .kind = Forsetti::ModuleIOKind::Storage,
                    .access = Forsetti::ModuleIOAccess::ReadWrite,
                    .required = true
                }
            },
            .ui = std::nullopt,
            .dataIsolation = Forsetti::ModuleDataIsolation{
                .mode = Forsetti::ModuleDataIsolationMode::PrivateToModule,
                .ownedStoreIDs = {"template-service-state"},
                .requiredDefaultRoles = {}
            }
        }
    };
}

void TemplateServiceModule::start(Forsetti::IForsettiModuleContext& context)
{
    if (auto storage = context.services()->resolve<Forsetti::IStorageService>()) {
        storage->set("started", "true");
    }
}

void TemplateServiceModule::stop(Forsetti::IForsettiModuleContext& context)
{
    if (auto storage = context.services()->resolve<Forsetti::IStorageService>()) {
        storage->remove("started");
    }
}
