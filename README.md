# Forsetti Framework — Windows

A modular runtime framework for Windows 11, built with C++20 and WinUI 3.

## Overview

Forsetti is a proprietary modular runtime framework that provides:

- **Module System** — Manifest-driven module discovery, compatibility checking, and lifecycle management
- **Event Bus** — Pub/sub communication between modules and the framework
- **Service Container** — Type-erased dependency injection
- **UI Surface Management** — Toolbar items, view injections, overlay routing
- **Entitlement Gating** — IAP-based module unlocking with runtime reconciliation

## Architecture

```
ForsettiCore          (Pure C++20, no platform deps)
ForsettiPlatform      (Windows SDK service implementations) -> Core
ForsettiModulesExample (Example modules)                    -> Core
ForsettiHostTemplate  (WinUI 3 host application template)   -> Core + Platform
```

## Building

### Prerequisites

- Visual Studio 2022 with C++ Desktop Development workload
- CMake 3.28+
- vcpkg

### Build

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

## License

Proprietary. Copyright (c) 2026 James Daley. All Rights Reserved.
