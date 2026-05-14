# Phase Report

## Phase

```text
04 - UI Contribution Model and Activation Semantics
```

## Accountable human owner

```text
Flynn33
```

## Summary

Phase 04 makes UI/app activation semantics explicit and enforces UI contribution capabilities at activation time.

Service modules may run concurrently. UI and app modules share one active surface slot: activating a UI/app module replaces the previous UI/app module, removes its surface contributions, stops it, and persists only the selected UI/app module. UI/app activation now validates declared capabilities before module start or surface mutation. Toolbar items require `toolbar_items`, view injections require `view_injection`, overlay schemas and toolbar overlay actions require `routing_overlay`, and toolbar event actions require `event_publishing`. Theme masks remain reserved for framework-owned presentation policy and continue to be stripped from module contributions before surface publication.

## Files changed

- `include/ForsettiCore/ModuleManager.h` - adds a capability-denied manager error and declares UI contribution validation.
- `src/ForsettiCore/ModuleManager.cpp` - validates UI contribution capabilities, applies single-active UI/app replacement semantics, and removes replaced UI/app modules from active runtime state.
- `tests/ForsettiCoreTests/RuntimeLifecycleTests.cpp` - adds regression coverage for UI capability denials and active UI replacement behavior.
- `README.md` - documents single-active UI/app semantics and UI contribution capability requirements.
- `forsetti-instructions.json` - aligns repository instructions with the selected UI/app activation model and contribution capabilities.
- `.forsetti/remediation/phase-04-report.md` - records the Phase 04 report and gate decision.
- `.forsetti/remediation/phase-04-evidence.json` - records machine-readable Phase 04 evidence.

## Tests added or updated

- `Runtime_UISurfaceCallbackFailurePreservesPreviousUIModule` - updated fixture capabilities for toolbar contribution validation.
- `Runtime_UIStartFailureRestoresPreviousSurfaceAndStopsIncoming` - updated fixture capabilities for toolbar contribution validation.
- `Runtime_UIToolbarContributionRequiresToolbarCapability` - verifies toolbar items are denied without `toolbar_items`.
- `Runtime_UIViewContributionRequiresViewCapability` - verifies view injections are denied without `view_injection`.
- `Runtime_UIOverlayContributionRequiresRoutingCapability` - verifies overlay schemas are denied without `routing_overlay`.
- `Runtime_UIPublishingToolbarActionRequiresEventCapability` - verifies toolbar event actions are denied without `event_publishing`.
- `Runtime_UISwitchReplacesActiveModuleAndSurface` - verifies UI/app replacement removes previous active state and surface contributions.

## Verification commands

| Command | Result | Notes |
|---|---|---|
| `gh pr view 22 --json state,statusCheckRollup` | pass | Confirmed Phase 03 PR was merged and its remote marker guard passed before starting Phase 04. |
| `git fetch origin --prune; git switch main; git pull --ff-only origin main; git switch -c fix/ui-activation-contribution-semantics` | pass | Created the Phase 04 branch from updated `main`. |
| Visual Studio 2022 bundled `cmake.exe --build --preset debug` | pass | Debug build completed successfully. |
| Visual Studio 2022 bundled `ctest.exe --preset debug --output-on-failure` | pass | 3 of 3 CTest suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1` | pass | Architecture checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-dependencies.ps1` | pass | Dependency checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-manifests.ps1` | pass | Existing repository manifests passed script validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1` | pass | Configure, build, tests, and architecture checks passed with Visual Studio-bundled CMake on `PATH` and `VCPKG_ROOT` set. |
| JSON parsing over tracked and unignored repository JSON files | pass | Repository JSON files parsed successfully after evidence was added. |
| Local marker scan | pass | No prohibited markers found in tracked repository files. |
| `rg -n "CapabilityDenied|validateUIContributions|Runtime_UI.*Capability|Runtime_UISwitchReplacesActiveModuleAndSurface|single active|uiContributionCapabilities" include src tests README.md forsetti-instructions.json` | pass | Reviewed expected Phase 04 runtime, tests, and documentation references. |
| `git diff --check` | pass | No whitespace errors reported. |
| `git diff --stat` | pass | Diff reviewed and scoped to Phase 04 runtime, tests, docs, and evidence files. |

## Commands that could not run and why

```text
None.
```

## Gate decision

```text
pass
```

`G04_UI_SEMANTICS` passes for Phase 04 because UI/app activation semantics are explicit, contribution capabilities are validated before activation, surface contributions are replaced when the active UI/app module changes, regression tests are present, the debug build passed, CTest passed 3 of 3 suites, repository guardrails passed, and the diff is scoped to the phase.

## Risk and follow-up table

| Risk | Severity | Owner decision needed? |
|---|---|---|
| Theme masks remain reserved and stripped even when a module declares `ui_theme_mask`; a future owner decision is needed before exposing module-owned theme policy. | P2 | yes |
| UI field-shape validation beyond capability checks, such as duplicate toolbar IDs or unknown host slots, remains a future UI contract hardening task. | P2 | no |
| Real Windows platform adapter behavior remains scheduled for Phase 05. | P1 | no |
