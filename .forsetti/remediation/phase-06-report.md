# Phase Report

## Phase

```text
06 - Tests, Scripts, CI, and Documentation Guardrails
```

## Accountable human owner

```text
Flynn33
```

## Summary

Phase 06 aligns repository guardrails, script regression coverage, and governance documentation with the current remediation workflow.

Manifest validation now enforces exact `Windows` casing for `supportedPlatforms` and exact casing for requested capabilities. The local guardrail wrapper now runs configure, build, CTest, architecture checks, dependency checks, manifest validation, pull request compatibility checks, and script regression tests. Documentation and repository metadata now describe the current split: local guardrail output is the build and test source of truth during remediation, while the remote pull request workflow remains limited to repository marker scanning.

## Files changed

- `.github/discussion_agents.json` - replaces a stale workflow source path with the active workflows directory.
- `.github/pull_request_template.md` - removes the inaccurate version workflow wording.
- `CONTRIBUTING.md` - points contributors at the local guardrail wrapper and describes its coverage.
- `README.md` - marks the WinUI 3 host template as planned and documents the local guardrail wrapper.
- `Scripts/check-manifests.ps1` - adds repository-root injection for tests and exact casing validation for platforms and capabilities.
- `Scripts/test-guardrail-scripts.ps1` - adds script regression tests for manifest validation.
- `Scripts/verify-forsetti-guardrails.ps1` - expands the wrapper to run dependency, manifest, compatibility, and script regression checks.
- `agentic-coding-policy.json` - records the temporary remote/local enforcement split and planned host-template status.
- `forsetti-instructions.json` - marks `ForsettiHostTemplate` as planned and updates host contract wording.
- `wiki.md` - documents host-template status and guardrail responsibilities.
- `.forsetti/remediation/phase-06-report.md` - records this Phase 06 report.
- `.forsetti/remediation/phase-06-evidence.json` - records machine-readable Phase 06 evidence.

## Tests added or updated

- `Scripts/test-guardrail-scripts.ps1` verifies exact `Windows` platform casing passes.
- `Scripts/test-guardrail-scripts.ps1` verifies lowercase `windows` fails.
- `Scripts/test-guardrail-scripts.ps1` verifies unsupported mixed platforms fail.
- `Scripts/test-guardrail-scripts.ps1` verifies capability casing remains exact.

## Verification commands

| Command | Result | Notes |
|---|---|---|
| `gh pr view 24 --json number,url,state,statusCheckRollup,headRefName,baseRefName` | pass | Confirmed Phase 05 PR was merged and its remote marker guard passed before starting Phase 06. |
| `git fetch origin --prune; git switch main; git pull --ff-only origin main; git switch -c chore/tests-scripts-docs-guardrails` | pass | Created the Phase 06 branch from updated `main`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/test-guardrail-scripts.ps1` | pass | Script regression tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-manifests.ps1` | pass | Existing repository manifests passed exact casing validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-dependencies.ps1` | pass | Dependency checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1` | pass | Architecture checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-pr-compatibility.ps1` | pass | Compatibility checks passed with 173 `TEST_METHOD` declarations against the 131 baseline. |
| `py Scripts/github/discussion_agent.py --root . --validate-config` | pass | Discussion agent configuration is valid after the workflow source path update. |
| JSON parsing for edited JSON files | pass | `forsetti-instructions.json`, `agentic-coding-policy.json`, and `.github/discussion_agents.json` parsed successfully. |
| Local marker scan over tracked and unignored files | pass | No prohibited markers found. |
| `git diff --check` | pass | No whitespace errors reported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1` | pass | Passed with Visual Studio 2022 bundled CMake on `PATH` and `VCPKG_ROOT` set to the Visual Studio vcpkg path. |

## Commands that needed environment adjustment

```text
Initial PATH-only invocations of cmake and python were unavailable in this shell.
The wrapper was rerun successfully with Visual Studio 2022 bundled CMake on PATH and VCPKG_ROOT set.
The discussion agent config validation was rerun successfully with the py launcher.
```

## Gate decision

```text
pass
```

`G06_GUARDRAIL_CONSISTENCY` passes because manifest casing is enforced by script and regression tests, the local wrapper now covers the repository guardrails used for build/test evidence, stale workflow and versioning documentation has been corrected, the planned host-template status is consistent across instructions and docs, and all local verification commands passed after the required shell environment was exported.

## Risk and follow-up table

| Risk | Severity | Owner decision needed? |
|---|---|---|
| Full remote build/test parity remains intentionally paused during remediation and should be restored when the newly aligned workflow set is added. | P1 | no |
| Script regression coverage currently targets manifest validation only; future script behavior changes should add focused cases to the same harness. | P2 | no |
