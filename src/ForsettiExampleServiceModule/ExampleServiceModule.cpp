// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ExampleServiceModule.h"

namespace Forsetti {

ModuleDescriptor ExampleServiceModule::descriptor() const
{
    return ModuleDescriptor{
        .moduleID = "com.forsetti.module.example-service",
        .displayName = "Example Service",
        .version = SemVer{0, 1, 0},
        .type = ModuleType::Service
    };
}

ModuleManifest ExampleServiceModule::manifest() const
{
    return ModuleManifest{
        .schemaVersion = "1.1",
        .moduleID = "com.forsetti.module.example-service",
        .displayName = "Example Service",
        .moduleVersion = SemVer{0, 1, 0},
        .moduleType = ModuleType::Service,
        .supportedPlatforms = {Platform::Windows},
        .minForsettiVersion = SemVer{0, 2, 0},
        .maxForsettiVersion = std::nullopt,
        .capabilitiesRequested = {
            Capability::Storage,
            Capability::Telemetry,
            Capability::EventPublishing
        },
        .iapProductID = std::nullopt,
        .entryPoint = "ExampleServiceModule",
        .manifestTemplateVersion = ManifestTemplateVersion::V1_1,
        .defaultModuleRole = std::nullopt,
        .runtimeRequirements = ModuleRuntimeRequirements{
            .io = {
                ModuleIORequirement{
                    .requirementID = "storage.example-state",
                    .kind = ModuleIOKind::Storage,
                    .access = ModuleIOAccess::ReadWrite,
                    .required = true,
                    .description = "Private example service state."
                },
                ModuleIORequirement{
                    .requirementID = "telemetry.example-events",
                    .kind = ModuleIOKind::Telemetry,
                    .access = ModuleIOAccess::Emit,
                    .required = false,
                    .description = "Optional example service telemetry."
                }
            },
            .ui = std::nullopt,
            .dataIsolation = ModuleDataIsolation{
                .mode = ModuleDataIsolationMode::PrivateToModule,
                .ownedStoreIDs = {"example-service-state"},
                .requiredDefaultRoles = {}
            }
        }
    };
}

void ExampleServiceModule::start(IForsettiModuleContext& context)
{
    context.publishEvent("example.service.started");
}

void ExampleServiceModule::stop(IForsettiModuleContext& context)
{
    context.publishEvent("example.service.stopped");
}

} // namespace Forsetti
