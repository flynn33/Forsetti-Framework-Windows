# Claude Code Instructions — Forsetti Framework Windows

## What This Project Is

This is a C++20 port of the **Forsetti Framework**, a proprietary modular runtime framework owned by **James Daley**. The original framework is written in Swift targeting iOS/macOS. This port targets **Windows 11** with **WinUI 3** for the UI layer.

The goal is a **1:1 architectural equivalent** — same module system, same manifest contracts, same event bus, same layered dependency rules, same enforcement guardrails — expressed in idiomatic modern C++.

---

## Owner Rules — READ THESE FIRST

1. **Do NOT push anything to GitHub** without explicit permission from the owner.
2. **Do NOT add yourself as a contributor** to the project.
3. **This is a standalone repository.** All work happens on `main`. This is NOT a branch of the Swift Forsetti repo — it is a separate C++ port.
4. **Proprietary license.** Copyright James Daley 2026, All Rights Reserved.
5. Read `agentic-coding-policy.json` and `forsetti-instructions.json` in the project root — they contain the binding rules (R001–R009) and architecture spec.

---

## What's Been Completed

### Phase 0 — Project Skeleton ✅
- `CMakeLists.txt` (root + all subdirectories)
- `CMakePresets.json` (debug/release presets)
- `vcpkg.json` (nlohmann-json dependency)
- `.gitignore`, `README.md`, `LICENSE.md`
- `forsetti-instructions.json`, `agentic-coding-policy.json`
- `.github/workflows/guardrails.yml`
- `Scripts/verify-forsetti-guardrails.ps1`, `Scripts/check-architecture.ps1`

### Phase 1 — ForsettiCore (All files written) ✅
All 18 headers in `include/ForsettiCore/` and 17 source files in `src/ForsettiCore/`:

| Component | Header | Source | Status |
|-----------|--------|--------|--------|
| SemVer | `SemVer.h` | `SemVer.cpp` | ✅ Written |
| ForsettiVersion | `ForsettiVersion.h` | (header-only) | ✅ Written |
| ModuleModels | `ModuleModels.h` | `ModuleModels.cpp` | ✅ Written |
| UIModels | `UIModels.h` | `UIModels.cpp` | ✅ Written |
| ForsettiProtocols | `ForsettiProtocols.h` | (header-only) | ✅ Written |
| ForsettiLogger | `ForsettiLogger.h` | `ForsettiLogger.cpp` | ✅ Written |
| ForsettiEventBus | `ForsettiEventBus.h` | `ForsettiEventBus.cpp` | ✅ Written |
| ForsettiServices | `ForsettiServices.h` | (header-only) | ✅ Written |
| ForsettiServiceContainer | `ForsettiServiceContainer.h` | `ForsettiServiceContainer.cpp` | ✅ Written |
| ActivationStore | `ActivationStore.h` | `ActivationStore.cpp` | ✅ Written |
| CapabilityPolicy | `CapabilityPolicy.h` | `CapabilityPolicy.cpp` | ✅ Written |
| EntitlementProviders | `EntitlementProviders.h` | `EntitlementProviders.cpp` | ✅ Written |
| ManifestLoader | `ManifestLoader.h` | `ManifestLoader.cpp` | ✅ Written |
| CompatibilityChecker | `CompatibilityChecker.h` | `CompatibilityChecker.cpp` | ✅ Written |
| ModuleRegistry | `ModuleRegistry.h` | `ModuleRegistry.cpp` | ✅ Written |
| StaticModuleRegistry | `StaticModuleRegistry.h` | `StaticModuleRegistry.cpp` | ✅ Written |
| ForsettiContext | `ForsettiContext.h` | `ForsettiContext.cpp` | ✅ Written |
| UISurfaceManager | `UISurfaceManager.h` | `UISurfaceManager.cpp` | ✅ Written |
| ModuleManager | `ModuleManager.h` | `ModuleManager.cpp` | ✅ Written |
| ForsettiRuntime | `ForsettiRuntime.h` | `ForsettiRuntime.cpp` | ✅ Written |

### Phase 2 — ForsettiPlatform (Stub implementations) ✅
- `PlatformServices.h/.cpp` — stub implementations (in-memory maps instead of real Windows APIs)
- `EntitlementProviders.h/.cpp` — re-exports ForsettiCore providers
- `DefaultPlatformServices.h/.cpp` — registers all services into container

### Phase 3 — ForsettiModulesExample ✅
- `ExampleModules.h/.cpp` — ExampleServiceModule + ExampleUIModule
- `ExampleModuleRegistry.cpp` — registers both modules
- `Resources/ForsettiManifests/ExampleServiceModule.json`
- `Resources/ForsettiManifests/ExampleUIModule.json`

### Phase 5 — Tests (Partial) ✅
- `TestHelpers.h` — mock/stub test doubles (InMemoryActivationStore, MockEntitlementProvider, RecordingLogger, StubForsettiModule, StubForsettiUIModule)
- `SemVerTests.cpp` — 18 test methods using Microsoft CppUnitTest
- `tests/CMakeLists.txt` — configured for CppUnitTest DLL targets

---

## What Still Needs To Be Done

### IMMEDIATE: Build Verification 🔴
The code was written on macOS and has **never been compiled**. The first priority is:

1. Ensure vcpkg is installed and `VCPKG_ROOT` environment variable is set
2. Run `cmake --preset debug` to configure
3. Run `cmake --build --preset debug` to build
4. **Fix any compilation errors** — there will likely be issues since this was written without a compiler

### Remaining Test Files 🟡
Only `SemVerTests.cpp` exists. The following test files still need to be written (all using Microsoft CppUnitTest format):

| Test File | What It Tests |
|-----------|--------------|
| `tests/ForsettiCoreTests/ManifestLoaderTests.cpp` | Load valid manifests, detect duplicates, reject invalid JSON, validate required fields |
| `tests/ForsettiCoreTests/RuntimeLifecycleTests.cpp` | boot → activate → deactivate → shutdown lifecycle |
| `tests/ForsettiCoreTests/BootstrapTests.cpp` | Full bootstrap wiring, default providers |
| `tests/ForsettiCoreTests/ModuleCommunicationTests.cpp` | Event bus pub/sub, module messages, communication guard validation |
| `tests/ForsettiCoreTests/EntitlementProviderTests.cpp` | AllowAll, Static providers, change notifications, reconciliation |
| `tests/ForsettiCoreTests/CompatibilityCheckerTests.cpp` | Schema version, platform, version range, capability policy, reserved UIThemeMask |
| `tests/ForsettiCoreTests/UISurfaceManagerTests.cpp` | Merge toolbar items, group injections by slot, overlay schema composition |
| `tests/ForsettiCoreTests/ModuleLoggingTests.cpp` | Logger routing, level filtering, module context |
| `tests/ForsettiPlatformTests/PlatformServicesTests.cpp` | Platform service stub round-trip tests |
| `tests/ForsettiArchitectureTests/ArchitectureEnforcementTests.cpp` | #include scanning + dependency graph validation |

Each test file should follow this pattern:
```cpp
#include "CppUnitTest.h"
// ... includes ...

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Forsetti;

TEST_CLASS(ClassName)
{
public:
    TEST_METHOD(MethodName)
    {
        // Arrange, Act, Assert
        Assert::AreEqual(expected, actual);
        Assert::IsTrue(condition);
        Assert::IsFalse(condition);
        Assert::ExpectException<ExceptionType>([]() { /* code */ });
    }
};
```

Add each new test file to `tests/CMakeLists.txt` in the `ForsettiCoreTests` target.

### Platform Service Real Implementations 🟡
The platform services in `src/ForsettiPlatform/PlatformServices.cpp` are currently **stubs** using in-memory maps. Once the build is working, replace with real Windows implementations:

- `RegistryStorageService` → Windows Registry (`HKCU\Software\Forsetti`) via `RegSetValueExW`/`RegQueryValueExW`
- `DpapiSecureStorageService` → DPAPI (`CryptProtectData`/`CryptUnprotectData`)
- `WinHttpNetworkingService` → WinHTTP (`winhttp.h`)
- `LocalFileExportService` → `std::filesystem` + `SHGetKnownFolderPath`

Also add `JsonFileActivationStore` — persists ActivationState to `%LOCALAPPDATA%\Forsetti\activation_state.json`.

When implementing real Windows services, uncomment the Windows SDK link libraries in `src/ForsettiPlatform/CMakeLists.txt`:
```cmake
target_link_libraries(ForsettiPlatform PRIVATE winhttp crypt32 shell32)
```

### Phase 4 — ForsettiHostTemplate (WinUI 3) 🔴 Not Started
This is the UI layer. Files to create in `include/ForsettiHostTemplate/` and `src/ForsettiHostTemplate/`:

| File | Purpose |
|------|---------|
| `ForsettiHostController.h/.cpp` | Host lifecycle controller — manages module lists, boot/shutdown, toolbar dispatch |
| `ForsettiHostBootstrap.h/.cpp` | `makeController()` static factory |
| `OverlayRouter.h/.cpp` | Resolves navigation pointers and overlay routes |
| `HostCatalogs.h/.cpp` | BaseDestinationCatalog (home/settings/modules), SlotCatalog (homeBanner/dashboardPrimary/overlayMain/moduleWorkspace) |
| `HostModuleItem.h/.cpp` | ForsettiHostModuleItem wrapping manifest + availability |
| `ViewInjectionRegistry.h/.cpp` | viewID → WinUI 3 UIElement factory map |

The HostTemplate requires WinUI 3 (C++/WinRT) which must be set up in CMake. Uncomment the `add_subdirectory(src/ForsettiHostTemplate)` line in the root `CMakeLists.txt` when ready.

---

## Build Instructions

### Prerequisites
- **Visual Studio 2022** with "Desktop development with C++" workload
- **CMake 3.28+**
- **vcpkg** installed with `VCPKG_ROOT` environment variable set

### Build Steps
```powershell
# 1. Configure (vcpkg will auto-install nlohmann-json)
cmake --preset debug

# 2. Build
cmake --build --preset debug

# 3. Run tests
ctest --preset debug --output-on-failure

# 4. Run architecture checks
.\Scripts\check-architecture.ps1
```

### Full Guardrails Check
```powershell
.\Scripts\verify-forsetti-guardrails.ps1
```

---

## Architecture Rules (R006 — One-Way Dependencies)

```
ForsettiCore          → depends on NOTHING (pure C++20 + nlohmann/json)
ForsettiPlatform      → depends on ForsettiCore only
ForsettiModulesExample → depends on ForsettiCore only
ForsettiHostTemplate  → depends on ForsettiCore + ForsettiPlatform
```

**Forbidden includes:**
- ForsettiCore must NOT include ForsettiPlatform, ForsettiHostTemplate, or ForsettiModulesExample
- ForsettiPlatform must NOT include ForsettiHostTemplate or ForsettiModulesExample
- ForsettiModulesExample must NOT include ForsettiPlatform or ForsettiHostTemplate

**All concrete classes must be `final`** unless extension is intentional and documented (R005).

---

## JSON Key Convention

All JSON serialization uses **camelCase** keys to match the Swift manifest format:
- `schemaVersion`, `moduleID`, `displayName`, `moduleVersion`, `moduleType`
- `supportedPlatforms`, `minForsettiVersion`, `maxForsettiVersion`
- `capabilitiesRequested`, `iapProductID`, `entryPoint`
- `enabledServiceModuleIDs`, `enabledUIModuleIDs`, `selectedUIModuleID`

Capability strings use **snake_case**: `routing_overlay`, `toolbar_items`, `view_injection`, `ui_theme_mask`, `event_publishing`, `secure_storage`, `file_export`

---

## Verification Checklist

When the build compiles successfully, verify these behaviors:

- [ ] `cmake --preset debug && cmake --build --preset debug` — zero warnings (`/W4 /WX`)
- [ ] `ctest --preset debug` — all tests pass
- [ ] `.\Scripts\check-architecture.ps1` — zero violations
- [ ] ManifestLoader parses both example JSON manifests from `Resources/ForsettiManifests/`
- [ ] JSON round-trip: parse manifest → serialize → parse again → values match
- [ ] Runtime lifecycle: boot → discover → activate service → activate UI → publish event → deactivate → shutdown
- [ ] Entitlement reconciliation: mock entitlement revocation triggers module deactivation
- [ ] UISurfaceManager: toolbar items concatenate, view injections group by slot sorted by priority
- [ ] Communication guard: empty IDs rejected, self-messages rejected, "forsetti.internal." namespace blocked

---

## Git Workflow

1. This is a **standalone repository** (separate from the Swift Forsetti repo).
2. Initialize git if needed (`git init`) and commit to `main`.
3. **Do NOT push to GitHub** without explicit owner permission.
4. **Do NOT add yourself as a contributor.**

---

## Key Reference

The full implementation plan is in the file that was used during development. The binding rules are in:
- `agentic-coding-policy.json` — Rules R001–R009
- `forsetti-instructions.json` — Architecture spec, module model, core API

The Swift originals that this port is based on live in a separate repository on the owner's macOS machine. If you need to reference the Swift source for any implementation detail, ask the owner.
