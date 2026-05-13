# Phase 00 Local Tool Inventory

## Repository

| Field | Value |
|---|---|
| Repository path | Local checkout root |
| Origin | `https://github.com/flynn33/Forsetti-Framework-Windows` |
| Baseline commit | `644da6c97471cd2257d014ca62f4a4e1dd8b272a` |
| Baseline commit date | `2026-03-15T10:02:58-05:00` |
| Starting branch | `main` |
| Phase branch | `audit/windows-baseline-evidence` |

## Repository Inventory

| Item | Value |
|---|---:|
| Tracked files | 111 |
| C++ header/source/test files under `include/`, `src/`, and `tests/` | 58 |
| `TEST_METHOD` declarations | 131 |
| GitHub workflow files | 19 |
| Forsetti manifest JSON files | 2 |

Main CMake targets observed from repository configuration:

- `ForsettiCore`
- `ForsettiPlatform`
- `ForsettiModulesExample`

`ForsettiHostTemplate` is referenced in comments/docs/policy files but is not an active CMake subdirectory.

## CMake Presets

`CMakePresets.json` has version `6`.

| Preset type | Names |
|---|---|
| Configure | `debug`, `release` |
| Build | `debug`, `release` |
| Test | `debug` |

The remediation package verification commands reference `windows-msvc-debug`, but this checkout currently defines `debug` and `release` only.

## vcpkg

| Item | Value |
|---|---|
| `vcpkg.json` package | `forsetti-framework-windows` |
| Version string | `0.1.0` |
| Dependencies | `nlohmann-json` |
| `builtin-baseline` | Not present |
| `VCPKG_ROOT` | Not set |
| `vcpkg.exe` on PATH | Not found |
| `vcpkg.exe` in checked local search paths | Not found under the checked local drive and user-profile search roots used during Phase 00 |

The repo preset expands `CMAKE_TOOLCHAIN_FILE` to `/scripts/buildsystems/vcpkg.cmake` when `VCPKG_ROOT` is empty, which blocks CMake configure.

## Local Tool Availability

| Tool | PATH availability | Observed version or path | Notes |
|---|---|---|---|
| `git` | Available | `git version 2.53.0.windows.1`; `C:\Program Files\Git\cmd\git.exe` | Used for repository state and diff evidence. |
| `rg` | Available | `ripgrep 15.1.0`; local PATH entry ending in `rg.exe` | Used for inventory and audit-marker search. |
| `pwsh` | Available | `PowerShell 7.5.5`; `C:\Program Files\PowerShell\7\pwsh.exe` | Used for guardrail scripts. |
| `powershell` | Available | `C:\WINDOWS\System32\WindowsPowerShell\v1.0\powershell.exe` | Available fallback shell. |
| `py` | Available | `Python 3.9.13`; `C:\WINDOWS\py.exe` | Used for JSON parsing validation. |
| `python` | Available | `C:\Users\james\AppData\Local\Microsoft\WindowsApps\python.exe` | Present, but `py -3` was used for validation. |
| `cmake` | Not on PATH | Visual Studio 2022 bundled CMake: `3.31.6-msvc6` | Full path used for deeper configure probe. |
| `ctest` | Not on PATH | Visual Studio 2022 bundled CTest: `3.31.6-msvc6` | Full path used for deeper test probe. |
| `cl` | Not on PATH | Visual Studio 2022 MSVC `19.44.35223` at `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe` | Available by full path; developer environment is not active in the current shell. |
| `MSBuild.exe` | Not on PATH | Visual Studio 2022 MSBuild `17.14.40.60911` at `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe` | Available by full path. |
| `vstest.console.exe` | Not on PATH | Visual Studio Test Platform `17.14.0` at `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\Extensions\TestPlatform\vstest.console.exe` | Available by full path. |
| `vcpkg` | Not available | Not found | Blocks configured CMake preset. |

## Visual Studio Installations

`vswhere` reported these installed instances:

- Visual Studio Community 2026 at `C:\Program Files\Microsoft Visual Studio\18\Community`
- Visual Studio Build Tools 2022 at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`
- Visual Studio Community 2022 at `C:\Program Files\Microsoft Visual Studio\2022\Community`

The repository CMake preset generator is `Visual Studio 17 2022`, so the Visual Studio 2022 tools are the relevant baseline target.

## Baseline Verification Summary

| Check | Result |
|---|---|
| Git status before evidence files | Clean on phase branch |
| Full guardrail script | Failed at CMake lookup because `cmake` is not on PATH |
| Visual Studio-bundled CMake configure with repo `debug` preset | Failed because `VCPKG_ROOT` is unset and `/scripts/buildsystems/vcpkg.cmake` does not exist |
| Visual Studio-bundled build with repo `debug` preset | Failed because configure did not produce `ALL_BUILD.vcxproj` |
| Visual Studio-bundled CTest with repo `debug` preset | Returned no tests because configure/build did not complete |
| JSON validation | Passed for 9 JSON files |
| `Scripts/check-architecture.ps1` | Passed |
| `Scripts/check-dependencies.ps1` | Passed |
| `Scripts/check-manifests.ps1` | Passed for 2 manifest files |

## Rerun Instructions

To rerun the full Windows/MSVC baseline validation, use a shell where Visual Studio 2022 CMake/MSBuild/MSVC tools are on PATH and set `VCPKG_ROOT` to a vcpkg checkout containing `scripts\buildsystems\vcpkg.cmake`.

Preferred repository commands after that environment is available:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File Scripts/verify-forsetti-guardrails.ps1
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

If the remediation package command names are required verbatim, add or map a `windows-msvc-debug` preset first; this checkout currently defines `debug`.
