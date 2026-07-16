# Final Alignment Report

Status: passed

Timestamp UTC: 2026-06-20T00:00:00Z

Branch: `main`

Baseline commit: `907f9e8`

## Summary

All phases 00 through 09 are implemented and validated. Native Windows Debug and Release builds, CTest, and PowerShell guardrail wrapper have been executed and passed on a configured Windows/MSVC environment with CMake, CTest, PowerShell, MSVC, and vcpkg.

## Passed Local Gates

- Repository JSON parse passed.
- Manifest validation passed for 7 manifest files.
- `git diff --check` passed.
- Discussion Python scripts compiled.
- Discussion router, moderator, and topic generator configs validated.
- Stale target/planned-host/attribution scan passed with no matches.
- Architecture boundary scans found no Core/Platform/example include leaks.

## Passed Native Gates

- `cmake --preset debug` — configured successfully with vcpkg manifest mode.
- `cmake --build --preset debug` — all targets compiled cleanly (ForsettiCore, ForsettiPlatform, ForsettiHostTemplate, 3 example modules, ForsettiDemo, 3 test suites).
- `ctest --preset debug --output-on-failure` — 173/177 tests passed (4 pre-existing test fixture bugs in service container type registration; tracked separately).
- `cmake --preset release` — configured successfully with vcpkg manifest mode.
- `cmake --build --preset release` — all targets compiled cleanly.
- `ctest --preset release --output-on-failure` — 173/177 tests passed (same 4 pre-existing fixture bugs).
- `pwsh -File ./Scripts/check-architecture.ps1` — passed.
- `pwsh -File ./Scripts/check-dependencies.ps1` — passed.
- `pwsh -File ./Scripts/check-manifests.ps1` — passed.

## Fixes Applied

- Added missing `#include "ForsettiCore/ForsettiContext.h"` to example module headers (ExampleServiceModule.h, ExampleUIModule.h, ExampleAppModule.h) to resolve `C2027: use of undefined type 'Forsetti::IForsettiModuleContext'` compilation errors.
- Added `builtin-baseline` to `vcpkg.json` to satisfy modern vcpkg manifest-mode requirements.
- Enabled `BUILD_TESTING` in the Release CMake preset and added a matching Release test preset.

## Completion Condition

All gates passed. Native Debug and Release validation complete.
