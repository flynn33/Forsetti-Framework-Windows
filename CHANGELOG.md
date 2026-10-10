# Changelog

All notable changes to Forsetti Framework - Windows are documented in this file.

The project uses Conventional Commit style for commit messages. Release entries should group user-visible changes by area and should call out runtime contract changes, validation changes, and follow-up owner decisions.

## [0.2.0] - 2026-10-10

### Windows SDK

- Added the installed `ForsettiFramework` CMake package for the three allowlisted Core, Platform, and HostTemplate products with separate x64 Debug/Release static libraries and public headers.
- Added the complete Windows consumption policy, immutable policy/source provenance, consumer documentation, and declared JSON dependency/ABI requirements.
- Added independent host/app/UI/service scaffolds and development-time generation/integration verification tools. The shared-database template remains manifest-only; host presentation uses app-owned native Win32 view factories.
- Added SDK assembly, explicit payload inventory/hashes, archive checksums, and extracted/relocated consumer verification. Exact executed results belong to the selected artifact's manifest and verification record.
- Aligned current policy and source headers with the authoritative Apache-2.0 license; retained framework version 0.2.0 and historical module/schema version distinctions.

### Build And Compilation

- Fixed missing `ForsettiContext.h` include in example module headers (ExampleServiceModule, ExampleUIModule, ExampleAppModule) that caused `C2027` compilation errors for `IForsettiModuleContext`.
- Added `builtin-baseline` to `vcpkg.json` to satisfy modern vcpkg manifest-mode requirements.
- Enabled `BUILD_TESTING` in the Release CMake preset and added a matching Release test preset.

### Runtime Contract

- Added manifest schema 1.1 with runtime requirements, UI declarations, default roles, and 1.0 compatibility.
- Added `IForsettiModuleContext` as the module lifecycle surface.
- Added module registration records, canonical manifest hashing, and activation-time registration confirmation.
- Added runtime requirement validation, scoped storage wrappers, and default-role provider resolution.
- Fixed scoped storage, secure-storage, and file-export service resolution by preserving their public interface pointer types before type erasure; added allowed/denied capability regressions.
- Fixed runtime test registration helpers to snapshot manifest entry points before moving the manifest, eliminating compiler-dependent argument evaluation failures.
- Fixed graceful shutdown to preserve the persisted desired service/UI selection for the next restore-only boot while stopping active modules. Explicit deactivation and entitlement revocation still persist their changes; shutdown continues cleanup of other modules when a stop throws. Added persistence, relaunch, and stop-failure regressions.
- Added schema-declared UI contribution validation and preserved declared theme masks.

### Host And Examples

- Added the sealed `ForsettiHostTemplate` composition layer.
- Added a downstream `samples/ForsettiDemo` composition root.
- Split example service, UI, and app modules into isolated targets with owned manifests.
- Added Windows module and host starter templates.

### Validation And Guardrails

- Updated manifest validation for schema 1.1, recursive template/sample scanning, exact Windows platform casing, duplicate checks, and runtime requirement checks.
- Renamed repository surface checks to neutral terminology and removed stale historical completion claims.
- Added native Debug/Release verification paths through CMake, CTest, and PowerShell guardrails. Current command results are recorded with the artifact's verification evidence rather than a historical README test count.

### Documentation

- Aligned tracked README and wiki.md documentation with the 0.2.0 SDK, runtime, HostTemplate, module registration, platform adapters, and artifact-specific validation evidence. The separately published GitHub Wiki is labeled historical; current repository and SDK documents are authoritative.

## [v0.1.0] - 2026-03-09

### Other Changes

- Added initial repository automation and release metadata.
