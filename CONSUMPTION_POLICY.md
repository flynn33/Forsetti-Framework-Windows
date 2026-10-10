# Forsetti Windows Consumption Policy

## Mandatory integration boundary

**Do not import, copy, vendor, embed, or use the entire Forsetti framework repository as an application's starting tree, including an unmodified copy.** Install the Windows SDK outside the application's tracked source tree. Link its compiled products through the installed CMake package and generate only the approved app-owned scaffold files described below.

These are engineering and supported-integration rules. They do not add license restrictions or change [LICENSE](LICENSE). Retain all applicable copyright, license, and third-party notices in permitted distribution material.

The application owns its features, module implementations, registration factories, native views, and deployment resources. Forsetti owns its runtime libraries and public SDK interface. Reading runtime source to understand a contract does not transfer ownership of that source to the application.

## Runtime product allowlist

| Installed target | Installed public interface | Permitted role |
| --- | --- | --- |
| `Forsetti::ForsettiCore` | `include/ForsettiCore/` | Module contracts, models/manifests, runtime lifecycle, registration, compatibility, capabilities, service interfaces, events/messages, and UI contribution contracts. |
| `Forsetti::ForsettiPlatform` | `include/ForsettiPlatform/` | Windows service adapters, registration persistence, entitlements, and native view factory contracts. Depends on Core. |
| `Forsetti::ForsettiHostTemplate` | `include/ForsettiHostTemplate/` | Sealed host bootstrap/controller, overlay routing, and surface-state composition. Depends on Core and Platform. |

`ForsettiFramework` is the CMake package name, not an additional library. `ForsettiHostTemplate` is a compiled product despite its name; its runtime implementation is not copyable scaffolding.

```cmake
find_package(ForsettiFramework CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE
    Forsetti::ForsettiCore
    Forsetti::ForsettiPlatform
    Forsetti::ForsettiHostTemplate
)
```

Select only the products each target uses. A module using core contracts links Core without a direct host dependency. Follow the SDK's configuration, architecture, compiler/runtime, dependency, and deployment requirements. Record the SDK version/revision and dependency baseline in the application's dependency workflow.

Public C++ headers, including required inline and template definitions, are essential installed SDK interfaces. Include them from the installation; do not transplant, rename, or patch them as app-owned code. Declared third-party public include dependencies are permitted with their licenses and notices. Dependency package caches outside tracked application source are permitted; do not edit them for application behavior or ship the caches.

## Approved generated scaffolds

The canonical scaffold inputs are the five `templates/` directories. The installed SDK retains these templates as development inputs. Their expressly designated `.cpp`, `.h`, `CMakeLists.txt`, and manifest `.json` payloads may be generated or adapted into app-owned files after replacing identities consistently.

| Canonical input | Permitted app-owned output |
| --- | --- |
| `templates/windows-host/` | Independent consumer CMake project, entry point/bootstrap, native presentation wiring, activation persistence, and explicit deployment/resource membership. |
| `templates/windows-app-module/` | App module class, CMake target, manifest resource, and registration/presentation guidance. The host starter supplies registry and native view factories. |
| `templates/windows-ui-module/` | UI module class, CMake target, manifest resource, and registration/presentation guidance. The host starter supplies registry and native view factories. |
| `templates/windows-service-module/` | Service module class, CMake target, manifest resource, and registration guidance. The host starter supplies the registry factory. |
| `templates/windows-shared-database-module/` | **Manifest starter only.** No supplied database engine or validated provider implementation. Implement a provider through the public service contracts before promising database functionality. |

The payload's `sdk-manifest.json` records the exact shipped scaffold inventory. Read the template README before adapting a file. Template README files and unresolved generator inputs are development guidance, not application resources. The SDK generators and verifier are development tools; installation of tools does not authorize shipping them with an application.

Generated classes and manifests must agree on module ID, entry point, descriptor type and version, schema/template version, and declared UI identifiers. Semantic versions are JSON objects. Use exact `Windows` platform and capability spellings. Preserve entitlement/compatibility/capability/registration checks, runtime requirements and default-role resolution, capability-scoped service access, framework-mediated events/messages, and the single shared active UI/app slot. Discover deployed manifests relative to the executable or another explicit application deployment location, never implicitly from the working directory. Do not bypass runtime activation by calling module startup or rendering unconditionally.

## Forbidden integration and reference-only material

Do not recreate Forsetti runtime targets or source directories inside an application. Do not point `add_subdirectory` at a vendored Forsetti checkout, use `FetchContent` to integrate the entire development repository, or use a framework submodule/subtree as the application foundation. Do not patch installed SDK or dependency implementations for app-specific behavior. Extend public interfaces in app-owned code; missing public functionality requires an upstream framework change.

| Repository material | Consumer boundary |
| --- | --- |
| `src/ForsettiCore/`, `src/ForsettiPlatform/`, `src/ForsettiHostTemplate/` | Runtime implementations; study only. Neither sources nor recreated build targets enter the application. |
| `src/ForsettiExample*Module/`, their nonpublic headers/manifests/libraries, `samples/ForsettiDemo/` | Evaluation material; excluded from SDK products and approved scaffolds. Do not ship sample identities or disguise copied demos as app-owned starters. |
| `tests/` | Framework-maintenance tests; author separate application tests. |
| `framework-policy.json`, `implementation-policy.json` | Framework architecture and implementation references; not runtime configuration or an instruction to reconstruct the runtime. |
| Root maintainer CMake files/presets, `.github/`, guardrail/release/discussion scripts | Framework build and maintenance infrastructure; do not transplant into an app. |
| Wholesale `docs/`, wiki archives, `.forsetti/alignment/`, Git metadata, build intermediates, package caches | Reference/maintenance material; excluded from consumer source and shipping resources. |

These exclusions apply to SDK assembly and consumer integration, not deletion from the framework repository. The allowlisted public headers, generated installed CMake metadata, designated scaffold payloads, and required licenses/notices are explicit exceptions to generic source/configuration exclusions.

A separate framework checkout may be used for framework maintenance, documentation, or evaluation. Keep it outside the application's tracked tree. An explicitly requested upstream experiment does not make a source checkout the application's supported dependency method.

## Implementation and verification sequence

1. Identify whether the work changes Forsetti itself or integrates an application.
2. Before app edits, name selected installed targets and each exact scaffold input/output path.
3. Keep the application an independent project. Resolve `ForsettiFramework` from its installed location and separately resolve declared prerequisites.
4. Own module identities, feature code, factories, native presentation, manifests, and deployment membership in the app. Preserve the checks described above.
5. Verify tracked files, CMake source/link/resource membership, resolved dependencies, runtime success and rejection paths, and shipping contents. Confirm no runtime source tree, demo resources, unresolved templates, maintenance infrastructure, or SDK installation was copied into the app.
6. Report generated files, selected products, actual commands/results, and unrun checks. Use the integration verifier within its documented coverage; it does not prove arbitrary copied code is absent.

The whole SDK archive is for installation outside application source. A shipping application contains its app-owned executable/resources and any required runtime redistribution and notice material. It does not contain the SDK development tools, templates, headers, static-library installation, or framework repository merely because they were present during development.

## Immutable provenance and reference mapping

The shared ownership principle was consulted in the Apple edition's [consumption policy at `baf20c37740911595be034231f60dc433c5445bf`](https://github.com/flynn33/Forsetti-Framework-Mac-iOS/blob/baf20c37740911595be034231f60dc433c5445bf/CONSUMPTION_POLICY.md). Apple products and tooling do not define this Windows allowlist.

The following links resolve repository-only references without copying their implementations into the SDK. They identify the inspected Windows baseline `63b9db87c575b2c72bfb6b3c988fcd7abd7fabe5`; the SDK manifest separately records the selected release revision and file hashes.

| Reference | Immutable Windows source mapping |
| --- | --- |
| Baseline product/build declarations | [Root CMake](https://github.com/flynn33/Forsetti-Framework-Windows/blob/63b9db87c575b2c72bfb6b3c988fcd7abd7fabe5/CMakeLists.txt), [Core](https://github.com/flynn33/Forsetti-Framework-Windows/blob/63b9db87c575b2c72bfb6b3c988fcd7abd7fabe5/src/ForsettiCore/CMakeLists.txt), [Platform](https://github.com/flynn33/Forsetti-Framework-Windows/blob/63b9db87c575b2c72bfb6b3c988fcd7abd7fabe5/src/ForsettiPlatform/CMakeLists.txt), [Host](https://github.com/flynn33/Forsetti-Framework-Windows/blob/63b9db87c575b2c72bfb6b3c988fcd7abd7fabe5/src/ForsettiHostTemplate/CMakeLists.txt) |
| Activation and ownership checks | [ModuleManager.cpp](https://github.com/flynn33/Forsetti-Framework-Windows/blob/63b9db87c575b2c72bfb6b3c988fcd7abd7fabe5/src/ForsettiCore/ModuleManager.cpp) |
| Framework architecture reference | [framework-policy.json](https://github.com/flynn33/Forsetti-Framework-Windows/blob/63b9db87c575b2c72bfb6b3c988fcd7abd7fabe5/framework-policy.json) |
| Framework implementation reference | [implementation-policy.json](https://github.com/flynn33/Forsetti-Framework-Windows/blob/63b9db87c575b2c72bfb6b3c988fcd7abd7fabe5/implementation-policy.json) |

The complete policy is shipped as `CONSUMPTION_POLICY.md` at the SDK root, byte-for-byte from its selected source revision. Its SHA-256 and source path are recorded in `sdk-manifest.json`. Do not replace it with an abbreviated rule or a remote link.
