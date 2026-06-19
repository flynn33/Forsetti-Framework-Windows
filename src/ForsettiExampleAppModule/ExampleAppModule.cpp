// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ExampleAppModule.h"

namespace Forsetti {

ModuleDescriptor ExampleAppModule::descriptor() const
{
    return ModuleDescriptor{
        .moduleID = "com.forsetti.module.example-app",
        .displayName = "Example App",
        .version = SemVer{0, 1, 0},
        .type = ModuleType::App
    };
}

ModuleManifest ExampleAppModule::manifest() const
{
    return ModuleManifest{
        .schemaVersion = "1.1",
        .moduleID = "com.forsetti.module.example-app",
        .displayName = "Example App",
        .moduleVersion = SemVer{0, 1, 0},
        .moduleType = ModuleType::App,
        .supportedPlatforms = {Platform::Windows},
        .minForsettiVersion = SemVer{0, 2, 0},
        .maxForsettiVersion = std::nullopt,
        .capabilitiesRequested = {
            Capability::ToolbarItems,
            Capability::ViewInjection,
            Capability::EventPublishing
        },
        .iapProductID = std::nullopt,
        .entryPoint = "ExampleAppModule",
        .manifestTemplateVersion = ManifestTemplateVersion::V1_1,
        .defaultModuleRole = DefaultModuleRole::UI,
        .runtimeRequirements = ModuleRuntimeRequirements{
            .io = {},
            .ui = ModuleUIRequirements{
                .controlSchemeID = "example-app.controls",
                .layoutID = "example-app.layout",
                .themeIDs = {},
                .viewIDs = {"ExampleAppShellView"},
                .slotIDs = {"appShell"},
                .toolbarItemIDs = {"example-app-toolbar"},
                .routeIDs = {},
                .pointerIDs = {}
            },
            .dataIsolation = ModuleDataIsolation{
                .mode = ModuleDataIsolationMode::PrivateToModule,
                .ownedStoreIDs = {},
                .requiredDefaultRoles = {}
            }
        }
    };
}

UIContributions ExampleAppModule::uiContributions() const
{
    return UIContributions{
        .themeMask = std::nullopt,
        .toolbarItems = {
            ToolbarItemDescriptor{
                .itemID = "example-app-toolbar",
                .title = "App Action",
                .systemImageName = "app.fill",
                .action = PublishEventAction{.eventType = "example.app.action"}
            }
        },
        .viewInjections = {
            ViewInjectionDescriptor{
                .injectionID = "example-app-shell",
                .slot = "appShell",
                .viewID = "ExampleAppShellView",
                .priority = 100
            }
        },
        .overlaySchema = std::nullopt
    };
}

void ExampleAppModule::start(IForsettiModuleContext& context)
{
    context.publishEvent("example.app.started");
}

void ExampleAppModule::stop(IForsettiModuleContext& context)
{
    context.publishEvent("example.app.stopped");
}

} // namespace Forsetti
