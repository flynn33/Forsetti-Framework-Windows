# Changelog

All notable changes to Forsetti Framework - Windows are documented in this file.

The project uses Conventional Commit style for commit messages. Release entries should group user-visible changes by area and should call out runtime contract changes, validation changes, and follow-up owner decisions.

## [Unreleased] - 2026-06-19

### Runtime Contract

- Added manifest schema 1.1 with runtime requirements, UI declarations, default roles, and 1.0 compatibility.
- Added `IForsettiModuleContext` as the module lifecycle surface.
- Added module registration records, canonical manifest hashing, and activation-time registration confirmation.
- Added runtime requirement validation, scoped storage wrappers, and default-role provider resolution.
- Added schema-declared UI contribution validation and preserved declared theme masks.

### Host And Examples

- Added the sealed `ForsettiHostTemplate` composition layer.
- Added a downstream `samples/ForsettiDemo` composition root.
- Split example service, UI, and app modules into isolated targets with owned manifests.
- Added Windows module and host starter templates.

### Validation And Guardrails

- Updated manifest validation for schema 1.1, recursive template/sample scanning, exact Windows platform casing, duplicate checks, and runtime requirement checks.
- Renamed repository surface checks to neutral terminology and removed stale historical completion claims.
- Native Debug/Release validation remains pending on a Windows toolchain.

## [v0.1.0] - 2026-03-09

### Other Changes

- Added initial repository automation and release metadata.
