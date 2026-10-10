# Windows Service Module Template

Generate this app-owned starter with the installed SDK's `Tools/New-ForsettiModule.ps1 -Kind service`. Its explicit CMake static-library target finds `ForsettiFramework` and links only `Forsetti::ForsettiCore`. The generated vcpkg manifest declares the SDK's JSON dependency recipe. Keep the SDK installation outside tracked application source and read its complete consumption policy.

`TemplateServiceModule` implements `IForsettiModule`. Its constructor receives the same deployed `Forsetti::ModuleManifest` value captured by the owning host's registry factory. It validates the generated module ID, entry point, and service type; `descriptor()` and `manifest()` use that value. Register the factory under the exact manifest entry point and explicitly add the module target and deployed JSON resource to the owning host.

The schema/template `1.1` manifest declares required read/write storage and the matching `storage` capability, with private-to-module data isolation. Startup resolves `IStorageService` through `IForsettiModuleContext::services()`, reports an unavailable required provider, and writes its `started` key; stop removes that key. Register storage in the owning host under its interface type. The framework supplies capability and module-key scoping; the module does not access native storage directly.

Activate through the runtime or host controller after registration, compatibility, entitlement, and requirement checks. Services can run alongside the shared UI/app slot. Build and execute the owning consumer in Debug and Release, exercise a denied activation, and verify source/resource and deployment membership. The module's initial `0.1.0` version is app-owned; its minimum framework version is `0.2.0`.
