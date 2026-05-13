# Phase Report

## Phase

```text
00 - Baseline, Toolchain, and Evidence
```

## Accountable human owner

```text
Repository owner/reviewer
```

## Summary

Phase 00 captured repository state, toolchain availability, CMake/vcpkg configuration, guardrail behavior, and build/test blockers before any source behavior changes. No production source files were modified.

The repository baseline is commit `644da6c97471cd2257d014ca62f4a4e1dd8b272a` on phase branch `audit/windows-baseline-evidence`. The checkout contains 111 tracked files, 58 C++ header/source/test files, 131 `TEST_METHOD` declarations, 19 workflow files, and 2 Forsetti manifest JSON files.

## Files changed

- `.forsetti/remediation/local-tool-inventory.md` - records repository inventory, toolchain availability, CMake/vcpkg state, and rerun instructions.
- `.forsetti/remediation/phase-00-report.md` - records the Phase 00 report and gate decision.
- `.forsetti/remediation/phase-00-evidence.json` - records machine-readable Phase 00 evidence.

## Tests added or updated

- None - Phase 00 is a baseline/evidence phase with no source behavior changes.

## Verification commands

| Command | Result | Notes |
|---|---|---|
| `git status --short --branch` | pass | Starting checkout was clean on `main`; phase branch `audit/windows-baseline-evidence` was created for evidence files. |
| `git rev-parse HEAD` | pass | Baseline commit is `644da6c97471cd2257d014ca62f4a4e1dd8b272a`. |
| `git ls-files` and inventory counting | pass | Counted 111 tracked files, 58 C++ header/source/test files, 131 `TEST_METHOD` declarations, 19 workflows, and 2 manifest files. |
| `Get-Command git, rg, python, py, pwsh, powershell, cmake, ctest, cl, msbuild, devenv, vstest.console, vcpkg` | pass | Local shell has Git, ripgrep, Python launcher, and PowerShell. CMake, CTest, MSVC, MSBuild, VSTest, and vcpkg are not on PATH. |
| `vswhere -all -products * -format json` | pass | Visual Studio Community 2026, Visual Studio Build Tools 2022, and Visual Studio Community 2022 are installed. |
| Visual Studio tool path probes | pass | Visual Studio 2022 includes CMake/CTest, MSBuild, VSTest, and MSVC compiler binaries by full path. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1` | fail | Script failed during configure because `cmake` is not on PATH in the current shell. |
| `cmake --preset windows-msvc-debug` | fail | `cmake` is not on PATH. The repo also does not define a `windows-msvc-debug` preset; actual configure presets are `debug` and `release`. |
| `cmake --build --preset windows-msvc-debug` | fail | `cmake` is not on PATH, and the preset name is not defined in this checkout. |
| `ctest --preset windows-msvc-debug --output-on-failure` | fail | `ctest` is not on PATH, and the preset name is not defined in this checkout. |
| `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --list-presets` | pass | Confirmed configure presets `debug` and `release`. |
| `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --preset debug` | fail | Configure failed because `VCPKG_ROOT` is unset and the preset resolved `CMAKE_TOOLCHAIN_FILE` to `/scripts/buildsystems/vcpkg.cmake`. |
| `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build --preset debug` | fail | Build failed because configure did not create `ALL_BUILD.vcxproj`. |
| `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe --preset debug --output-on-failure` | deferred | CTest reported no tests because configure/build did not complete. |
| `py -3 -m json.tool` over repository JSON files | pass | 9 JSON files parsed successfully. |
| `rg -n "iOS|macOS|sourceModuleID|WinHttpNetworkingService|DpapiSecureStorageService|RegistryStorageService|LocalFileExportService|HostTemplate|windows" .` | pass | Search completed and surfaced the expected baseline remediation markers for later phases. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1` | pass | Architecture include/layering checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-dependencies.ps1` | pass | Dependency audit passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/check-manifests.ps1` | pass | Manifest validation passed for 2 manifest files. |
| `git ls-files --others --exclude-standard` | pass | Only `.forsetti/remediation/local-tool-inventory.md`, `.forsetti/remediation/phase-00-report.md`, and `.forsetti/remediation/phase-00-evidence.json` are untracked. |
| `git diff --no-index --stat -- NUL <new evidence file>` | pass | New-file diff review is scoped to 112 inserted lines in local tool inventory, 82 inserted lines in the phase report, and 144 inserted lines in JSON evidence. |

## Commands that could not run and why

| Command | Reason | Rerun instruction |
|---|---|---|
| `pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1` | The current shell does not expose `cmake` on PATH. | Use a Visual Studio 2022 developer shell or add the Visual Studio 2022 CMake directory to PATH, then rerun from the repository root. |
| `cmake --preset debug` | With Visual Studio-bundled CMake, configure fails because `VCPKG_ROOT` is unset. | Set `VCPKG_ROOT` to a vcpkg checkout containing `scripts\buildsystems\vcpkg.cmake`, then rerun `cmake --preset debug`. |
| `cmake --build --preset debug` | Configure failed before generating build files. | Rerun after `cmake --preset debug` succeeds. |
| `ctest --preset debug --output-on-failure` | Configure/build did not complete, so no test binaries were available. | Rerun after configure and build succeed. |
| Package command preset `windows-msvc-debug` | The repository currently defines `debug` and `release`, not `windows-msvc-debug`. | Either use the repository's `debug` preset or add a matching preset in a later guardrail/documentation phase. |

## Gate decision

```text
pass
```

`G00_BASELINE_LOCK` passes for Phase 00 because the required report and evidence files exist, JSON evidence is parseable, repository/toolchain/build blockers are documented exactly, rerun instructions are explicit, and the diff review is scoped to baseline evidence files only.

## Remaining risks

| Risk | Severity | Owner decision needed? |
|---|---|---|
| Full CMake/MSVC build/test validation is not complete because `cmake`/`ctest` are not on PATH and `VCPKG_ROOT` is unset. | P0 | yes |
| The remediation package's preferred preset name `windows-msvc-debug` does not match this checkout's `debug` preset. | P1 | yes |
| `vcpkg.json` has no `builtin-baseline`, so dependency resolution is not locked by the manifest alone. | P1 | yes |
| Runtime/security findings from the audit remain unresolved by design; they belong to later phases. | P0 | no |
