# Phase Report

## Phase

```text
02 - Runtime Activation and Factory Identity
```

## Accountable human owner

```text
Repository owner/reviewer
```

## Summary

Phase 02 hardens runtime activation so manifest type and factory output must agree before a module can enter active runtime state.

The runtime now validates factory-created module descriptor ID, type, version, and bundled manifest against the activation manifest. UI and app manifests no longer fall back to service activation when the returned module does not satisfy the required runtime contract. Activation mutates persisted active state only after the module has passed validation and started successfully.

Persisted restore now returns and stores structured diagnostics, logs restore failures with module ID and error text, restores the selected UI module last when multiple UI IDs are present, and persists reconciled restore state once after restore processing.

## Files changed

- `include/ForsettiCore/ModuleManager.h` - adds activation mismatch errors, restore diagnostic types, and restore result accessors.
- `src/ForsettiCore/ModuleManager.cpp` - validates factory output, removes UI/app service fallback, defers activation state mutation until success, and records restore diagnostics.
- `src/ForsettiCore/ForsettiRuntime.cpp` - keeps boot flow while explicitly ignoring the restore result.
- `tests/ForsettiCoreTests/RuntimeLifecycleTests.cpp` - adds regression coverage for type mismatch, identity mismatch, version mismatch, manifest mismatch, failed starts, and restore behavior.
- `tests/ForsettiCoreTests/TestHelpers.h` - adds test-store save counting and a module double that fails during start.
- `.forsetti/remediation/phase-02-report.md` - records the Phase 02 report and gate decision.
- `.forsetti/remediation/phase-02-evidence.json` - records machine-readable Phase 02 evidence.

## Tests added or updated

- `Runtime_UIManifestReturningServiceOnlyFailsActivationWithoutPersistingState`
- `Runtime_AppManifestWithoutAppUIContractFailsActivationWithoutPersistingState`
- `Runtime_FactoryWrongModuleIDFailsActivation`
- `Runtime_FactoryWrongTypeFailsActivation`
- `Runtime_FactoryWrongVersionFailsActivation`
- `Runtime_FactoryMismatchedManifestFailsActivation`
- `Runtime_ServiceStartFailureDoesNotPersistActiveState`
- `Runtime_RestoreFailureProducesDiagnosticsAndPersistsReconciledStateOnce`
- `Runtime_RestoreHonorsSelectedUIModule`

## Verification commands

| Command | Result | Notes |
|---|---|---|
| `git fetch origin --prune` | pass | Confirmed Phase 01 PR was merged before starting Phase 02. |
| `git switch main && git pull --ff-only origin main && git switch -c fix/runtime-activation-factory-identity` | pass | Created the Phase 02 branch from updated `main`. |
| Visual Studio 2022 bundled `cmake.exe --build --preset debug` | pass | Debug build completed successfully. |
| Visual Studio 2022 bundled `ctest.exe --preset debug --output-on-failure` | pass | 3 of 3 CTest suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1` | pass | Architecture checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-dependencies.ps1` | pass | Dependency checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-manifests.ps1` | pass | Existing repository manifests passed script validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1` | pass | Configure, build, tests, and architecture checks passed with Visual Studio-bundled CMake on `PATH` and `VCPKG_ROOT` set. |
| JSON parsing over tracked and unignored repository JSON files | pass | 15 JSON files parsed successfully after evidence was added. |
| Local prohibited-marker scan | pass | No prohibited attribution markers found in tracked or untracked working tree files. |
| `rg -n "Fallback: treat as service|fallback service activation|Silently skip failures during restoration" ...` | pass | No removed fallback or silent-restore marker remains in the Phase 02 runtime files. |
| `git diff --check` | pass | No whitespace errors reported. |
| `git diff --stat` | pass | Diff reviewed and scoped to Phase 02 runtime, tests, and evidence files. |

## Commands that could not run and why

```text
None.
```

## Gate decision

```text
pass
```

`G02_RUNTIME_IDENTITY` passes for Phase 02 because activation now rejects mismatched factory output, UI/app modules cannot fall back to service activation, failed activation does not mark modules active, restore diagnostics are recorded, selected UI restore intent is honored, regression tests are present, the debug build passed, CTest passed 3 of 3 suites, and repository guardrails passed.

## Risk and follow-up table

| Risk | Severity | Owner decision needed? |
|---|---|---|
| App modules are rejected unless the factory object satisfies both the app marker and UI runtime contract. Broader app/UI semantics remain scheduled for Phase 04. | P1 | no |
| Restore now persists reconciled state once after processing persisted activations; this intentionally drops modules that failed restore from persisted active state. | P1 | no |
| Capability-scoped service/context enforcement remains unresolved by design and belongs to Phase 03. | P0 | no |
