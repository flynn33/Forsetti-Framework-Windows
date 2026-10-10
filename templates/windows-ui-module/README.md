# Windows UI Module Template

Generate this app-owned starter with the installed SDK's `Tools/New-ForsettiModule.ps1 -Kind ui`. Its explicit CMake static-library target finds `ForsettiFramework` and links only `Forsetti::ForsettiCore`. The generated vcpkg manifest declares the SDK's JSON dependency recipe. Keep the SDK installation outside tracked application source and read its complete consumption policy.

`TemplateUIModule` implements `IForsettiUIModule`. Its constructor receives the same deployed `Forsetti::ModuleManifest` value captured by the owning host's registry factory. It validates the generated module ID, entry point, UI type, and one-view/one-slot declaration; `descriptor()` and `manifest()` use that value. Register the factory under the exact manifest entry point and explicitly add the module target and deployed JSON resource to the owning host.

The schema/template `1.1` manifest requests `view_injection`. The starter contributes one declared view in the `content` slot and writes lifecycle log entries through its module context. Native view creation/rendering belongs to the host; this module supplies no application window, toolbar, overlay, or theme renderer. UI and app modules share one active surface slot.

Activate through the runtime or host controller after registration, compatibility, entitlement, and requirement checks. Run the generated host's Debug/Release `--verify` checks and integration/resource verification after composition. Changing view/slot declarations also requires matching contribution and native-factory wiring. The module's initial `0.1.0` version is app-owned; its minimum framework version is `0.2.0`.
