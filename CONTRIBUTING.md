# Contributing to Forsetti Framework - Windows

Thank you for contributing to the Windows implementation of Forsetti. This repository is the Windows C++20 and WinUI 3 framework baseline, so changes should preserve its modular-runtime contracts and validation surfaces.

## Before You Open A Pull Request

1. Build the framework from the documented presets.
2. Run the test suite.
3. Keep architecture and manifest guardrails passing.
4. Update docs when behavior, setup, or governance rules change.

## Local Verification

Use the repo-standard commands:

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
.\Scripts\check-architecture.ps1
.\Scripts\check-dependencies.ps1
.\Scripts\check-manifests.ps1
```

## Contribution Expectations

- Keep `ForsettiCore` free of platform dependencies.
- Treat `ForsettiPlatform` as the Windows service implementation layer above Core.
- Keep examples and host surfaces dependent on Core, and on Platform only where intended.
- Preserve manifest-driven module discovery, compatibility checks, entitlement handling, and UI-surface rules.
- Follow the coding and dependency constraints in `agentic-coding-policy.json` and `forsetti-instructions.json`.

## Documentation Expectations

- Update `README.md` for user-facing setup or build changes.
- Update `wiki.md` for conceptual, operational, or architectural explanations.
- Update governance docs when workflows or automated enforcement change.

## Discussions

GitHub Discussions are automated with repo-grounded technical, support, and framework/governance agents. Those agents answer only from repository-tracked sources, so documentation updates directly improve discussion coverage.
