# Phase 00 Baseline Report

Repository: `flynn33/Forsetti-Framework-Windows`

Branch: `alignment/windows-runtime-boundaries`

Baseline commit: `42738a3772999092b6ef49e49a8a506f853928a6`

Current commit: `42738a3772999092b6ef49e49a8a506f853928a6`

## Inventory

- Tracked files: 117
- Tracked text lines: 15400
- Inventory: `.forsetti/alignment/baseline-file-inventory.json`

## Baseline Results

- `git diff --check`: passed with no output.
- Tracked JSON parse: passed for 20 files.
- Diff from package baseline commit: none.
- Native build/test baseline: blocked by host toolchain availability.

## External Toolchain Blockers

- `cmake --version`: `zsh:1: command not found: cmake`
- `cl`: `zsh:1: command not found: cl`
- `pwsh --version`: `zsh:1: command not found: pwsh`
- `ctest --preset debug --output-on-failure`: `zsh:1: command not found: ctest`
- `VCPKG_ROOT`: unset

## Existing Protections To Preserve

- Factory-created module identity, type, version, and bundled manifest validation.
- UI/app interface rejection before activation.
- Capability-scoped module service resolution.
- Module-bound message source identity.
- UI contribution capability validation.
- Single-active UI/app switching with rollback protections.
- Structured restore diagnostics and logging.
- Strict Windows platform parsing.
- WinHTTP, Registry, DPAPI, and constrained local export platform services.
- Duplicate-safe module registry.

## Phase 00 Status

Phase 00 evidence is recorded. Native Debug/Release acceptance remains blocked on this host until the Windows/MSVC, CMake, PowerShell, and vcpkg toolchain is available.
