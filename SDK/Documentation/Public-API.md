# Installed public C++ API

All public declarations use namespace `Forsetti`. Include installed headers by their actual spellings, such as `#include <ForsettiCore/ForsettiProtocols.h>`. Link the imported product owning each interface. Required inline/template definitions in these headers are SDK-owned interface code and remain in the installation; approved scaffold headers are app-owned generated code.

## Core (`Forsetti::ForsettiCore`)

| Headers | Main declarations/role |
| --- | --- |
| `ForsettiProtocols.h`, `ForsettiContext.h` | `IForsettiModule`, `IForsettiUIModule`, `IForsettiAppModule`, `IForsettiModuleContext`, `IEntitlementProvider`, `IOverlayRouter`, `IModuleCommunicationGuard`; lifecycle, scoped services, events/messages. |
| `SemVer.h`, `ForsettiVersion.h`, `ModuleModels.h`, `ModuleRequirements.h` | `SemVer`, `ForsettiVersion::current`, `ModuleDescriptor`, `ModuleManifest`, `ModuleRuntimeRequirements`, exact enums and JSON conversions. |
| `ModuleRegistry.h`, `StaticModuleRegistry.h`, `ModuleRegistration.h` | `ModuleRegistry::registerModule`, `ForsettiStaticModuleRegistry`, registration records/services, digest/store/clock interfaces. |
| `ForsettiRuntime.h`, `ModuleManager.h`, `ManifestLoader.h` | Runtime boot/shutdown and activation; manifest discovery; activation/restore errors and diagnostics. |
| `CompatibilityChecker.h`, `CapabilityPolicy.h`, `ModuleRequirementValidator.h`, `DefaultModuleRoles.h` | Compatibility reports, capability policy, provider requirements, and default-role catalogs/orchestration. |
| `ForsettiServices.h`, `ForsettiServiceContainer.h` | Service interfaces; `IServiceProvider`, `ServiceContainer`, capability-scoped providers and module-scoped storage/export wrappers. |
| `EntitlementProviders.h`, `ActivationStore.h` | Allow-all/static development entitlements and activation persistence contracts. |
| `ForsettiEventBus.h`, `ForsettiLogger.h` | Event bus/subscriptions, event models, logger interface and console logger. |
| `UIModels.h`, `UISurfaceManager.h` | Toolbar/view/overlay/theme contribution descriptors and framework-managed surface state. |

Implement the module interfaces in app-owned classes and receive `IForsettiModuleContext&` in `start`/`stop`. Register factories by the manifest's exact `entryPoint`. Keep `descriptor()` and `manifest()` aligned with deployed JSON. Activate through the runtime/host controller; do not call `start` directly. Resolve services from the module-scoped context.

Service registration is keyed by interface type. Register a concrete provider as the interface it implements, for example `services.registerService<IStorageService>(provider)`, so `resolve<IStorageService>()` uses the same key. `ServiceContainer`'s `std::any` values preserve the registered `std::shared_ptr<T>` type.

## Platform (`Forsetti::ForsettiPlatform`)

| Headers | Role |
| --- | --- |
| `PlatformServices.h`, `DefaultPlatformServices.h` | Windows networking, Registry storage, DPAPI storage, constrained local export, no-op telemetry, and `DefaultForsettiPlatformServices::registerAll`. |
| `EntitlementProviders.h` | Windows entitlement-provider implementations; select an appropriate app entitlement source. |
| `ModuleRegistrationPlatform.h` | Public registration persistence, digest, and clock adapters. |
| `WindowsViewFactoryRegistry.h` | `IWindowsViewFactory`, `IWindowsDispatcher`, `ForsettiViewFactoryRegistry`, declared view identity checks, and dispatcher-mediated native view creation. |

`IWindowsViewFactory::createView` returns `std::shared_ptr<void>`; native object ownership and presentation belong to the application. Register factories using the module's manifest-declared view IDs. The registry rejects undeclared, duplicate, and missing view factories. It does not implement a complete renderer.

## Host (`Forsetti::ForsettiHostTemplate`)

| Headers | Main declarations/role |
| --- | --- |
| `ForsettiHostBootstrap.h` | `ForsettiHostBootstrapConfiguration`, `IForsettiHostBootstrap`, `ForsettiHostBootstrap::makeController`; public composition of registry, services, state, policies, and runtime dependencies. |
| `ForsettiHostController.h`, `ForsettiHostState.h` | `IForsettiHostController`, boot/shutdown/activate/deactivate, snapshots, and restore-only/development/explicit-module launch strategies. |
| `ForsettiHostOverlayRouter.h` | Host-owned routing implementation through public overlay contracts. |
| `ForsettiSurfaceStateAdapter.h` | `IHostSurfaceStateSource`, `ForsettiSurfaceStateSnapshot`, and surface-state adaptation for native presentation. |

These final implementation classes are reusable sealed products. Extend the documented interfaces in app-owned code when necessary. Source implementation and tests can be read through [immutable references](Source-References.md); their presence in the repository does not make them copyable scaffold material.
