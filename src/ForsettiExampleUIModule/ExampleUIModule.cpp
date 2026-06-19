// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ExampleUIModule.h"

namespace Forsetti {

ModuleDescriptor ExampleUIModule::descriptor() const
{
    return ModuleDescriptor{
        .moduleID = "com.forsetti.module.example-ui",
        .displayName = "Example UI",
        .version = SemVer{0, 1, 0},
        .type = ModuleType::UI
    };
}

ModuleManifest ExampleUIModule::manifest() const
{
    return ModuleManifest{
        .schemaVersion = "1.1",
        .moduleID = "com.forsetti.module.example-ui",
        .displayName = "Example UI",
        .moduleVersion = SemVer{0, 1, 0},
        .moduleType = ModuleType::UI,
        .supportedPlatforms = {Platform::Windows},
        .minForsettiVersion = SemVer{0, 2, 0},
        .maxForsettiVersion = std::nullopt,
        .capabilitiesRequested = {
            Capability::RoutingOverlay,
            Capability::ToolbarItems,
            Capability::ViewInjection,
            Capability::UIThemeMask,
            Capability::EventPublishing
        },
        .iapProductID = "com.forsetti.iap.example-ui",
        .entryPoint = "ExampleUIModule",
        .manifestTemplateVersion = ManifestTemplateVersion::V1_1,
        .defaultModuleRole = DefaultModuleRole::UI,
        .runtimeRequirements = ModuleRuntimeRequirements{
            .io = {},
            .ui = ModuleUIRequirements{
                .controlSchemeID = "example-ui.controls",
                .layoutID = "example-ui.layout",
                .themeIDs = {"example-ui.default-theme"},
                .viewIDs = {
                    "ExampleHomeBannerView",
                    "ExampleDashboardView",
                    "ExampleOverlayView"
                },
                .slotIDs = {
                    "homeBanner",
                    "dashboardPrimary",
                    "overlayMain"
                },
                .toolbarItemIDs = {"example-toolbar-1"},
                .routeIDs = {"example-route-1"},
                .pointerIDs = {"example-pointer-1"}
            },
            .dataIsolation = ModuleDataIsolation{
                .mode = ModuleDataIsolationMode::PrivateToModule,
                .ownedStoreIDs = {},
                .requiredDefaultRoles = {}
            }
        }
    };
}

UIContributions ExampleUIModule::uiContributions() const
{
    return UIContributions{
        .themeMask = std::nullopt,
        .toolbarItems = {
            ToolbarItemDescriptor{
                .itemID = "example-toolbar-1",
                .title = "Example Action",
                .systemImageName = "star.fill",
                .action = NavigateAction{.destinationID = "home"}
            }
        },
        .viewInjections = {
            ViewInjectionDescriptor{
                .injectionID = "example-home-banner",
                .slot = "homeBanner",
                .viewID = "ExampleHomeBannerView",
                .priority = 100
            },
            ViewInjectionDescriptor{
                .injectionID = "example-dashboard",
                .slot = "dashboardPrimary",
                .viewID = "ExampleDashboardView",
                .priority = 50
            },
            ViewInjectionDescriptor{
                .injectionID = "example-overlay",
                .slot = "overlayMain",
                .viewID = "ExampleOverlayView",
                .priority = 0
            }
        },
        .overlaySchema = OverlaySchema{
            .navigationPointers = {
                NavigationPointer{
                    .pointerID = "example-pointer-1",
                    .label = "Example Pointer",
                    .baseDestinationID = "home"
                }
            },
            .overlayRoutes = {
                OverlayRoute{
                    .routeID = "example-route-1",
                    .label = "Example Route",
                    .presentation = OverlayPresentation::Sheet,
                    .destination = ModuleOverlayDestination{
                        "com.forsetti.module.example-ui",
                        "ExampleOverlayView"
                    }
                }
            }
        }
    };
}

void ExampleUIModule::start(IForsettiModuleContext& context)
{
    context.publishEvent("example.ui.started");
}

void ExampleUIModule::stop(IForsettiModuleContext& context)
{
    context.publishEvent("example.ui.stopped");
}

} // namespace Forsetti
