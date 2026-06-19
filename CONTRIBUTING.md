# Contributing to Forsetti Framework - Windows

Thank you for contributing to the Windows implementation of Forsetti. This repository is the Windows C++20 framework baseline with Windows SDK service adapters, a sealed host composition layer, isolated example modules, and downstream samples, so changes should preserve its modular-runtime contracts, public headers, and validation surfaces.

## Before You Open A Pull Request

1. Build the framework from the documented presets.
2. Run the test suite.
3. Keep architecture and manifest guardrails passing.
4. Update docs when behavior, setup, validation, governance rules, or owner-facing status changes.
5. Keep changes scoped to the layer and contract you are modifying.

## Local Verification

Use the repo-standard commands:

```powershell
.\Scripts\verify-forsetti-guardrails.ps1
```

The wrapper configures and builds the debug preset, runs CTest, checks architecture and dependency boundaries, validates manifests, runs pull request compatibility checks, and exercises script regression tests.

For environments where CMake is not on `PATH`, use the Visual Studio bundled CMake and set `VCPKG_ROOT` before running the wrapper.

## Contribution Expectations

- Keep `ForsettiCore` free of platform dependencies.
- Treat `ForsettiPlatform` as the Windows service implementation layer above Core.
- Keep examples and host surfaces dependent on Core, and on Platform only where intended.
- Preserve manifest-driven module discovery, compatibility checks, entitlement handling, and UI-surface rules.
- Preserve capability-scoped service access. New framework services that require a capability should be added to the capability mapping and tested.
- Preserve source identity protection. Module-scoped contexts assign source module IDs; callers do not provide their own source identity.
- Treat `ui_theme_mask` as limited to declared, policy-approved UI theme IDs while keeping framework host chrome framework-owned.
- Follow the coding and dependency constraints in `implementation-policy.json` and `framework-policy.json`.

## Documentation Expectations

- Update `README.md` for user-facing setup or build changes.
- Update `CHANGELOG.md` for notable runtime, guardrail, governance, or documentation changes.
- Update `wiki.md` and the public GitHub Wiki for conceptual, operational, or architectural explanations.
- Update governance docs when workflows or automated enforcement change.
- Keep diagrams and examples aligned with repository source, not future intent, unless the section is clearly marked as planned.

## Discussions

GitHub Discussions are automated with repo-grounded technical, support, and framework/governance responders. Those responders answer only from repository-tracked sources, so documentation updates directly improve discussion coverage.
