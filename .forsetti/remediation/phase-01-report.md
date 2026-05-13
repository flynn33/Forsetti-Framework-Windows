# Phase Report

## Phase

```text
01 - Object Model and Manifest Parity
```

## Accountable human owner

```text
Repository owner/reviewer
```

## Summary

Phase 01 hardens manifest and object-model parsing so Windows manifests must identify Windows explicitly and must satisfy stricter structural validation before entering the runtime.

The platform parser no longer maps `iOS` or `macOS` to `Windows`. Manifest loading now rejects malformed manifest-like JSON, blank or unsafe module IDs, blank or unsafe entry points, empty platform lists, negative version components, unsupported schema versions, unsafe optional product IDs, and a maximum framework version lower than the minimum framework version.

## Files changed

- `src/ForsettiCore/ModuleModels.cpp` - removes non-Windows platform aliases from `platformFromString`.
- `src/ForsettiCore/ManifestLoader.cpp` - adds strict manifest shape and semantic validation before accepting parsed manifests.
- `include/ForsettiCore/ManifestLoader.h` - updates loader exception documentation.
- `tests/ForsettiCoreTests/ManifestLoaderTests.cpp` - adds regression coverage for strict platform parsing and manifest validation failures.
- `.forsetti/remediation/phase-01-report.md` - records the Phase 01 report and gate decision.
- `.forsetti/remediation/phase-01-evidence.json` - records machine-readable Phase 01 evidence.

## Tests added or updated

- `PlatformFromString_RejectsApplePlatforms` - confirms `iOS` and `macOS` are not accepted as Windows.
- `LoadManifests_RejectsIOSPlatform` - confirms an iOS-only manifest fails loading.
- `LoadManifests_RejectsMacOSPlatform` - confirms a macOS-only manifest fails loading.
- `LoadManifests_RejectsMixedNonWindowsPlatform` - confirms mixed Windows/non-Windows manifests fail when they include an unsupported platform string.
- `LoadManifests_RejectsMaxVersionLowerThanMinVersion` - confirms invalid framework version ranges fail loading.
- `LoadManifests_RejectsMissingRequiredManifestField` - confirms manifest-like JSON missing a required runtime field fails loading.
- `LoadManifests_RejectsNegativeVersionComponent` - confirms negative semantic version components fail loading.
- `LoadManifests_RejectsBlankModuleID` - confirms blank module IDs fail loading.
- `LoadManifests_RejectsUnsafeModuleID` - confirms path-like module IDs fail loading.
- `LoadManifests_RejectsBlankEntryPoint` - confirms blank entry points fail loading.
- `LoadManifests_RejectsUnsafeEntryPoint` - confirms path-like entry points fail loading.

## Verification commands

| Command | Result | Notes |
|---|---|---|
| `git fetch origin --prune` | pass | Confirmed Phase 00B PR was merged before starting Phase 01. |
| `git switch main && git pull --ff-only origin main && git switch -c fix/object-model-manifest-parity` | pass | Created the Phase 01 branch from updated `main`. |
| JSON parsing over tracked and unignored repository JSON files | pass | 14 JSON files parsed successfully. |
| Local prohibited-marker scan | pass | No prohibited attribution markers found in tracked or untracked working tree files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1` | pass | Architecture checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-dependencies.ps1` | pass | Dependency checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-manifests.ps1` | pass | Existing repository manifests passed script validation. |
| `rg -n "iOS|macOS|Map to Windows|Unknown Platform string|platformFromString" src include tests` | pass | Confirmed non-Windows platform strings remain only in negative tests and no alias mapping remains. |
| `git diff --check` | pass | No whitespace errors reported. |
| Visual Studio 2022 bundled `cmake.exe --preset debug` | pass | Configure passed after setting `VCPKG_ROOT` to the local vcpkg checkout. |
| Visual Studio 2022 bundled `cmake.exe --build --preset debug` | pass | Debug build completed successfully. |
| Visual Studio 2022 bundled `ctest.exe --preset debug --output-on-failure` | pass | 3 of 3 CTest suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1` | pass | Passed after adding Visual Studio-bundled CMake to `PATH` and setting `VCPKG_ROOT`. |
| `git diff --stat` | pass | Diff reviewed and scoped to Phase 01 code, tests, and evidence files. |

## Commands that could not run and why

```text
None.
```

## Gate decision

```text
pass
```

`G01_MANIFEST_PARITY` passes for Phase 01 because the required behavior changes and regression tests are present, JSON validation passed, repository guardrail scripts passed, the debug build passed, CTest passed 3 of 3 suites, and the diff is scoped to Phase 01.

## Remaining risks

| Risk | Severity | Owner decision needed? |
|---|---|---|
| Entry-point and module-ID validation now rejects path-like and blank values; consumers with previously lax manifests will need to correct those manifests. | P1 | no |
| Runtime activation/factory identity findings remain unresolved by design; they belong to Phase 02. | P0 | no |
