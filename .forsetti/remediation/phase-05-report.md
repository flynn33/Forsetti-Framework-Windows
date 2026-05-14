# Phase Report

## Phase

```text
05 - Windows Platform Adapters and Security
```

## Accountable human owner

```text
Flynn33
```

## Summary

Phase 05 replaces misleading platform stubs with native Windows-backed implementations and adds regression coverage for adapter security behavior.

`WinHttpNetworkingService` now performs HTTP and HTTPS GET requests through WinHTTP and rejects unsupported schemes and unsafe header values. `RegistryStorageService` persists string values under HKCU with validated value names. `DpapiSecureStorageService` protects values with Windows DPAPI and stores protected blobs under HKCU. `LocalFileExportService` writes files only inside its configured export directory, sanitizes filenames, handles reserved device names, and rejects path escape attempts by construction.

## Files changed

- `include/ForsettiPlatform/PlatformServices.h` - removes stub comments and adds configurable registry/export roots for platform services.
- `src/ForsettiPlatform/PlatformServices.cpp` - implements WinHTTP networking, registry-backed storage, DPAPI-backed secure storage, and constrained local file export.
- `src/ForsettiPlatform/CMakeLists.txt` - links required Windows SDK libraries for WinHTTP, DPAPI, and registry APIs.
- `tests/ForsettiPlatformTests/PlatformServicesTests.cpp` - adds security and persistence regression tests for platform adapters.
- `.forsetti/remediation/phase-05-report.md` - records the Phase 05 report and gate decision.
- `.forsetti/remediation/phase-05-evidence.json` - records machine-readable Phase 05 evidence.

## Tests added or updated

- `RegistryStorageServiceTests::PersistsAcrossInstances` - verifies registry-backed persistence.
- `RegistryStorageServiceTests::RejectsUnsafeKeys` - rejects empty and path-like registry value names.
- `DpapiSecureStorageServiceTests::PersistsAcrossInstances` - verifies protected values can be read by a new service instance.
- `DpapiSecureStorageServiceTests::RejectsUnsafeKeys` - rejects unsafe secure-storage keys.
- `WinHttpNetworkingServiceTests::Data_RejectsUnsupportedScheme` - rejects non-HTTP URL schemes.
- `WinHttpNetworkingServiceTests::Data_RejectsHeaderLineBreaks` - rejects header values that could inject extra headers.
- `LocalFileExportServiceTests::ExportData_WritesFileInsideConfiguredDirectory` - verifies local export writes inside the configured root.
- `LocalFileExportServiceTests::ExportData_SanitizesTraversalFilename` - verifies path traversal input cannot escape the export root.
- `LocalFileExportServiceTests::SanitizesReservedDeviceFilename` - verifies reserved Windows device names are made safe.
- `LocalFileExportServiceTests::SanitizesEmptyFilename` - verifies empty/path-only names receive a safe fallback.

## Verification commands

| Command | Result | Notes |
|---|---|---|
| `gh pr view 23 --json state,statusCheckRollup` | pass | Confirmed Phase 04 PR was merged and its remote marker guard passed before starting Phase 05. |
| `git fetch origin --prune; git switch main; git pull --ff-only origin main; git switch -c fix/windows-platform-adapters-security` | pass | Created the Phase 05 branch from updated `main`. |
| Visual Studio 2022 bundled `cmake.exe --build --preset debug` | pass | Debug build completed successfully after native SDK libraries were linked. |
| Visual Studio 2022 bundled `ctest.exe --preset debug --output-on-failure` | pass | 3 of 3 CTest suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1` | pass | Architecture checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-dependencies.ps1` | pass | Dependency checks passed with native Windows SDK libraries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-manifests.ps1` | pass | Existing repository manifests passed script validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1` | pass | Configure, build, tests, and architecture checks passed with Visual Studio-bundled CMake on `PATH` and `VCPKG_ROOT` set. |
| `rg -n "Stub\|Phase 2a\|returns false\|empty byte vector\|in-memory\|no-op" include/ForsettiPlatform src/ForsettiPlatform tests/ForsettiPlatformTests` | pass | No misleading platform-stub markers remain in Phase 05 platform files. |
| JSON parsing over tracked and unignored repository JSON files | pass | Repository JSON files parsed successfully after evidence was added. |
| Local marker scan | pass | No prohibited markers found in tracked and unignored repository files. |
| `git diff --check` | pass | No whitespace errors reported. |
| `git diff --stat` | pass | Diff reviewed and scoped to Phase 05 platform adapters, tests, SDK linking, and evidence files. |

## Commands that could not run and why

```text
None.
```

## Gate decision

```text
pass
```

`G05_PLATFORM_SECURITY` passes for Phase 05 because the platform adapters are no longer misleading memory-only or no-op stubs, storage keys and export filenames are constrained, file exports cannot escape the configured root, DPAPI secure storage is native-backed, regression tests are present, the debug build passed, CTest passed 3 of 3 suites, and repository guardrails passed.

## Risk and follow-up table

| Risk | Severity | Owner decision needed? |
|---|---|---|
| Unit tests avoid external network calls for determinism, so live WinHTTP success-path coverage should be handled by an integration test with a controlled local HTTP server in a future test phase. | P2 | no |
| Registry-backed test roots are unique per run; future cleanup tooling may remove old test roots under the repository test registry path. | P3 | no |
| Broader CI and documentation guardrail alignment remains scheduled for Phase 06. | P1 | no |
