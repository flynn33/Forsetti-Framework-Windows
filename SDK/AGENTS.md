# Integrating the Forsetti Windows SDK

**Never copy, vendor, embed, or use the entire Forsetti repository as an application's starting tree, including an unchanged copy.** Read the complete [CONSUMPTION_POLICY.md](CONSUMPTION_POLICY.md) first. Keep this installed SDK outside tracked application source. The application owns generated scaffolds and feature code; Forsetti owns its libraries and installed public headers.

Before editing, state the selected `Forsetti::ForsettiCore`, `Forsetti::ForsettiPlatform`, and/or `Forsetti::ForsettiHostTemplate` targets. Name each exact template input and app-owned destination. Use `find_package(ForsettiFramework CONFIG REQUIRED)` with the SDK installation. Do not use a vendored checkout, source-tree include path, full-repository `FetchContent`, framework `add_subdirectory`, submodule/subtree, or recreated runtime target.

The three installed public include directories contain necessary SDK interfaces, including inline/template definitions. Include them from the installation and retain their ownership. Only designated headers inside approved scaffold templates become app-owned generated source. `ForsettiHostTemplate` is a compiled library, not a scaffold directory.

Use `Tools/New-ForsettiProject.ps1` and `Tools/New-ForsettiModule.ps1`, or adapt their exact designated inputs. Replace module IDs, display names, entry points, version objects, native view identifiers, factory registrations, and manifests consistently. Shared-database is a manifest starter only. Do not copy example modules or demo resources.

Read [Getting Started](Documentation/Getting-Started.md), [Module Authoring](Documentation/Module-Authoring.md), and [Compatibility](Documentation/Compatibility.md). Preserve runtime-mediated activation, capability-scoped services, entitlement/compatibility/registration checks, runtime requirements/default roles, framework-mediated messaging, and the single shared active UI/app slot. Resolve manifest resources from the deployment layout. Do not invoke module startup or render views as an activation shortcut.

After edits:

1. Review `git ls-files`, CMake source/link/resource membership, dependency manifests, and deployment resources.
2. Run `Tools/Test-ForsettiIntegration.ps1 -ProjectRoot <app>` and supply `-ShippingRoot <deployment>` when available. Inspect its results and documented detection limits; do not equate a scanner pass with proof that arbitrary copied code is absent.
3. Build and run the application in Debug and Release using the matching SDK configuration and declared prerequisites. Exercise a meaningful denied-activation path as well as normal activation.
4. Verify native presentation separately whenever claiming UI behavior. A headless activation result does not verify a window.
5. Inspect shipping contents: no SDK install, templates/tools, framework runtime source, demo resources, unresolved template placeholders, package caches, or maintenance infrastructure. Retain applicable licenses/notices and runtime redistribution material.
6. Report selected products, generated/adapted paths, exact commands/results, and any unrun checks. Read the final diff and preserve unrelated application files.

Missing public functionality belongs in an upstream framework change. Do not patch SDK or dependency internals for an app. No tool may search for a maintainer checkout or install globally as an integration shortcut.
