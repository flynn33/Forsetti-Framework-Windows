# Forsetti Framework — Windows

A modular runtime framework for Windows 11, built with C++20 and Windows SDK service adapters. The WinUI 3 host template is planned and is not yet a repository target.

## Overview

Forsetti is a proprietary modular runtime framework that provides:

- **Module System** — Manifest-driven module discovery, compatibility checking, and lifecycle management
- **Event Bus** — Pub/sub communication between modules and the framework
- **Service Container** — Type-erased dependency injection
- **UI Surface Management** — Toolbar items, view injections, overlay routing
- **Entitlement Gating** — IAP-based module unlocking with runtime reconciliation

## Runtime Semantics

Service modules may run concurrently. UI and app modules share a single active surface slot: activating a UI/app module replaces the current active UI/app module, stops the previous one, removes its surface contributions, and persists only the selected UI/app module.

UI/app modules must declare the capabilities used by their UI contributions. Toolbar items require `toolbar_items`, view injections require `view_injection`, overlay schemas and toolbar overlay actions require `routing_overlay`, and toolbar event actions require `event_publishing`. Theme masks remain reserved for framework-owned presentation policy.

## Architecture

```
ForsettiCore          (Pure C++20, no platform deps)
ForsettiPlatform      (Windows SDK service implementations) -> Core
ForsettiModulesExample (Example modules)                    -> Core
ForsettiHostTemplate  (planned WinUI 3 host template)       -> Core + Platform
```

`ForsettiHostTemplate` is a planned layer. Current CMake targets build `ForsettiCore`, `ForsettiPlatform`, `ForsettiModulesExample`, and the native test suites.

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

### Guardrails

Run the local guardrail wrapper before opening a pull request:

```powershell
.\Scripts\verify-forsetti-guardrails.ps1
```

The wrapper configures, builds, runs CTest, checks architecture and dependency boundaries, validates manifests, runs pull request compatibility checks, and exercises script regression tests. The remote pull request workflow is intentionally limited to repository marker scanning while this remediation sequence is active; build and test evidence is recorded from local guardrail runs.

## Project Guides

- `wiki.md` - conceptual walkthroughs for the core runtime, platform layer, examples, and governance
- `CONTRIBUTING.md` - contribution and local verification expectations
- `docs/governance/github_automation_agents.md` - repo discussion automation behavior and boundaries

## Patent Notice

The architecture and design of Forsetti are the subject of a pending U.S. patent application:
**Compatibility-Governed, Entitlement-Aware Modular Runtime Framework for Native Application Modules** — U.S. Application No. 63/999,606, filed March 8, 2026. Patent Pending.

## License

Proprietary. Copyright (c) 2026 James Daley. All Rights Reserved.
