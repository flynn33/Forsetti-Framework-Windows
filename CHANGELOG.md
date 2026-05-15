# Changelog

All notable changes to Forsetti Framework - Windows are documented in this file.

The project uses Conventional Commit style for commit messages. Release entries should group user-visible changes by area and should call out runtime contract changes, validation changes, and follow-up owner decisions.

## [Unreleased] - 2026-05-15

### Documentation

- Expanded `README.md` into a comprehensive repository entry point with architecture, runtime flow, build/test, guardrail, manifest, and documentation sections.
- Updated `wiki.md` as the tracked index for the public GitHub Wiki page set.
- Updated contributor and governance documents so repository docs match the completed remediation state.
- Rebuilt the public GitHub Wiki with detailed pages and visual aids for architecture, lifecycle, capabilities, UI composition, platform services, validation, governance, and roadmap decisions.

### Validation And Guardrails

- Documented the completed Phase 00 through Phase 07 remediation sequence and final acceptance status.
- Recorded the local validation baseline: `cmake --preset debug`, `cmake --build --preset debug`, `ctest --preset debug --output-on-failure`, and `Scripts/verify-forsetti-guardrails.ps1`.
- Clarified that the repository currently exposes `debug` and `release` CMake presets.

### Runtime Contract Summary

- Documented manifest parity, factory identity validation, capability-scoped service access, source identity protection, UI contribution capability checks, and Windows platform adapter behavior.
- Documented remaining owner decision points: remote build/test parity restoration, vcpkg baseline pinning, planned host-template implementation, theme policy exposure, and module-originated framework event design.

## [v0.1.0] - 2026-03-09

### Other Changes

- Added initial repository automation and release metadata.
