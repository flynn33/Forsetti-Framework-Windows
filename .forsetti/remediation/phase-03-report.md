# Phase Report

## Phase

```text
03 - Capability-Scoped Context and Spoofing Protection
```

## Accountable human owner

```text
Repository owner/reviewer
```

## Summary

Phase 03 introduces module-scoped context identity and capability-scoped service resolution for module runtime access.

Modules now receive a scoped `ForsettiContext` during start and stop. The scoped context carries immutable module identity and granted capabilities, injects the source module ID for module messages, strips caller-supplied source identity from payloads, and keeps framework events source-neutral. Service resolution for platform service interfaces is wrapped by `CapabilityScopedServiceProvider`, which denies access when the module lacks the matching capability and logs the denial with module ID and capability.

## Files changed

- `include/ForsettiCore/ForsettiContext.h` - changes module messaging to use scoped identity and exposes scoped context accessors.
- `src/ForsettiCore/ForsettiContext.cpp` - creates scoped module contexts, injects message source identity, strips source spoofing payload fields, and keeps framework events source-neutral.
- `include/ForsettiCore/ForsettiServiceContainer.h` - adds the capability-scoped service provider.
- `src/ForsettiCore/ForsettiServiceContainer.cpp` - maps service interfaces to capabilities and logs denied service resolution.
- `include/ForsettiCore/ModuleManager.h` - tracks module-scoped contexts for active modules.
- `src/ForsettiCore/ModuleManager.cpp` - passes scoped contexts into module start and stop lifecycle calls.
- `tests/ForsettiCoreTests/ModuleCommunicationTests.cpp` - adds spoofing and capability denial regression coverage.
- `.forsetti/remediation/phase-03-report.md` - records the Phase 03 report and gate decision.
- `.forsetti/remediation/phase-03-evidence.json` - records machine-readable Phase 03 evidence.

## Tests added or updated

- `SendModuleMessage_PublishesToEventBus`
- `SendModuleMessage_InjectsTargetModuleID`
- `SendModuleMessage_SetsSourceModuleID`
- `SendModuleMessage_InvalidIDs_Throws`
- `SendModuleMessage_UnscopedContext_Throws`
- `SendModuleMessage_CannotSpoofSourceInPayload`
- `ScopedServices_StorageDeniedWithoutCapability`
- `ScopedServices_StorageAllowedWithCapability`
- `ScopedServices_SecureStorageDeniedWithoutCapability`
- `ScopedContext_ExposesModuleIdentityAndCapabilities`
- `PublishFrameworkEvent_BypassesGuard`
- `ScopedModuleMessage_ReservedNamespaceStillThrows`

## Verification commands

| Command | Result | Notes |
|---|---|---|
| `gh pr view 21 --json state,statusCheckRollup` | pass | Confirmed Phase 02 PR was merged and its remote guard passed before starting Phase 03. |
| `git fetch origin --prune` | pass | Confirmed `origin/main` advanced and the merged Phase 02 branch was removed remotely. |
| `git switch main && git pull --ff-only origin main && git switch -c fix/capability-scoped-context` | pass | Created the Phase 03 branch from updated `main`. |
| Visual Studio 2022 bundled `cmake.exe --build --preset debug` | pass | Debug build completed successfully. |
| Visual Studio 2022 bundled `ctest.exe --preset debug --output-on-failure` | pass | 3 of 3 CTest suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1` | pass | Architecture checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-dependencies.ps1` | pass | Dependency checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-manifests.ps1` | pass | Existing repository manifests passed script validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1` | pass | Configure, build, tests, and architecture checks passed with Visual Studio-bundled CMake on `PATH` and `VCPKG_ROOT` set. |
| JSON parsing over tracked and unignored repository JSON files | pass | 16 JSON files parsed successfully after evidence was added. |
| Local prohibited-marker scan | pass | No prohibited attribution markers found in tracked or untracked working tree files. |
| `rg -n "sendModuleMessage\(|sourceModuleID|CapabilityScopedServiceProvider|Capability denied|scopedToModule|services\(\)" include src tests` | pass | Reviewed scoped identity, service gating, and expected source identity references. |
| `git diff --check` | pass | No whitespace errors reported. |
| `git diff --stat` | pass | Diff reviewed and scoped to Phase 03 context, service container, module manager, tests, and evidence files. |

## Commands that could not run and why

```text
None.
```

## Gate decision

```text
pass
```

`G03_CAPABILITY_ENFORCEMENT` passes for Phase 03 because module lifecycle now receives scoped context identity, module messages no longer accept caller-supplied source identity, framework events do not carry caller-supplied source identity, platform service resolution is capability-scoped, denials are logged with module ID and capability, regression tests are present, the debug build passed, CTest passed 3 of 3 suites, and repository guardrails passed.

## Risk and follow-up table

| Risk | Severity | Owner decision needed? |
|---|---|---|
| UI contribution capability validation remains scheduled for Phase 04, which explicitly owns toolbar, view, overlay, and UI activation semantics. | P1 | no |
| Capability-scoped service resolution currently gates known platform service interfaces and passes through unmapped service types. Future services should be added to the mapping when they become capability-governed runtime APIs. | P1 | no |
| Module-originated general event publishing beyond module-to-module messages remains an API design question; this phase keeps framework events source-neutral and module messages scoped. | P1 | no |
