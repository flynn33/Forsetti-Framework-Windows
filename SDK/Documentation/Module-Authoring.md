# Authoring app-owned modules

Use the approved [scaffolds and installed products](../CONSUMPTION_POLICY.md). A service module implements `IForsettiModule`; a UI module implements `IForsettiUIModule`; an app module implements `IForsettiAppModule`. App and UI modules share one active UI slot. Service modules can remain active alongside that slot.

## Identities, manifests, and factories

Choose an app-owned reverse-domain module ID and a valid C++ class/target identifier. The generator validates these inputs and refuses unwanted overwrites. Each module owns its `.h`, `.cpp`, factory registration, explicit CMake target, and JSON manifest. Keep the module ID, type, version, display name, entry point, and runtime requirements in `descriptor()`/`manifest()` consistent with the deployed JSON.

```json
{
  "schemaVersion": "1.1",
  "manifestTemplateVersion": "1.1",
  "moduleID": "com.example.myapplication.reports",
  "displayName": "Reports",
  "moduleVersion": { "major": 0, "minor": 1, "patch": 0, "prerelease": null },
  "moduleType": "service",
  "supportedPlatforms": ["Windows"],
  "minForsettiVersion": { "major": 0, "minor": 2, "patch": 0, "prerelease": null },
  "maxForsettiVersion": null,
  "capabilitiesRequested": ["storage"],
  "iapProductID": null,
  "entryPoint": "ReportsModule",
  "defaultModuleRole": null,
  "runtimeRequirements": {
    "io": [{ "requirementID": "reports.state", "kind": "storage", "access": "read_write", "required": true, "description": "Private report state." }],
    "ui": null,
    "dataIsolation": { "mode": "private_to_module", "ownedStoreIDs": ["reports-state"], "requiredDefaultRoles": [] }
  }
}
```

Versions are objects, not strings. Platform is exactly `Windows`; module types are `service`, `ui`, and `app`. Current schema/template version is `1.1`; the runtime also loads `1.0` with default requirements. Module version `0.1.0` is an example app-owned version, not a different framework SDK version.

Register a factory under the exact `entryPoint`, using the module's public constructor and the same manifest value. Add source and manifest membership explicitly in CMake. The generated project supplies a factory pattern; adapt its selected module's declaration rather than inventing a new runtime target. Duplicate factory identities are errors.

## Activation and provider requirements

Boot discovers the application's deployed manifest directory and confirms registration records. Activation validates confirmed registration, runtime/platform/schema compatibility, entitlements, capabilities, required providers/default roles, and the resolved module's descriptor/manifest before startup. UI contributions receive additional declaration and capability checks. Failure is part of the contract: report runtime/host diagnostics and keep denied modules inactive.

In `start(IForsettiModuleContext& context)`, resolve services through `context.services()`. Register providers in the owning host under their interface types. A required I/O provider must exist and the module must request its matching capability; an unavailable optional provider produces a requirement warning. Do not call native networking/storage as a shortcut around scoped services. Private storage keys and exported filenames are scoped by module wrappers.

Exact capability names are `networking`, `storage`, `secure_storage`, `file_export`, `crypto_utilities`, `telemetry`, `routing_overlay`, `toolbar_items`, `view_injection`, `ui_theme_mask`, `event_publishing`, `shared_database`, `authentication`, `diagnostics`, `api`, and `security`. An app-owned capability policy decides which requested features are allowed. Theme masking also requires manifest-declared, policy-approved theme IDs; host chrome remains framework-owned.

Runtime I/O requirements specify ID, kind, access, required status, and optional description. Default roles use `ui`, `shared_database`, `authentication`, `diagnostics`, `api`, or `security`; declaring a role is distinct from supplying a service implementation. Keep `requiredDefaultRoles` and provider registrations consistent. The shared-database scaffold is manifest-only: implement `ISharedDatabaseService` through public contracts and validate provider behavior before claiming a database feature.

## Messaging and UI

Use `context.publishEvent`, `sendMessage`, `subscribeToMessages`, and `subscribeToFrameworkEvents`. The scoped context assigns the module's source identity and validates communication. Empty identities, self-messaging, and `forsetti.internal.*` events are rejected by the communication guard. Event publishing requires the matching capability; avoid direct module references or a private bypass bus.

UI/app `uiContributions()` declares toolbar items, view injections, overlays, and optional theme mask. Declare corresponding identifiers under `runtimeRequirements.ui`, request matching capabilities, and register native factories with the same manifest view IDs. Instantiate/present active views only after runtime activation succeeds. Replacing the UI/app module preserves the single slot and updates the framework-managed surface.

The host starter's Win32 view factory wiring is app-owned presentation code using `IWindowsViewFactory` and the surface state adapter. Customize the application view implementation through these public contracts. The runtime's contribution model and an observed native window are separate concerns; validate the actual native controls when reporting UI results.

## Deployment and consumer verification

Deploy only explicitly declared module manifests to `ForsettiManifests` beside the executable (or a deliberately configured deployment resource location). The host computes the location from its deployment, not the current directory. Run Debug/Release success and denial checks, inspect actual deployed resources, and run the integration verifier described in [Getting Started](Getting-Started.md). Preserve required notices and remove sample identities from generated application ownership.
