# Forsetti Framework - Windows Wiki

## Getting Started

Forsetti Framework - Windows is the Windows 11 implementation of the framework's modular runtime model. The fastest onboarding path is to install the required Visual Studio workload, configure `vcpkg`, and build through the checked-in CMake presets.

## Core Runtime

`ForsettiCore` is the framework's pure C++20 authority layer. It owns module models, runtime lifecycle orchestration, manifest loading, compatibility checking, event routing, service-container behavior, and UI surface models without taking platform dependencies.

## Platform Layer

`ForsettiPlatform` provides Windows-specific service implementations. It sits above Core and should remain a realization layer for Windows SDK and WinUI 3 behavior rather than a place where core runtime truth is redefined.

## Example Modules

`ForsettiModulesExample` demonstrates how modules plug into the runtime, register themselves, and interact with the framework contracts. It is a reference surface for examples and validation, not the source of core framework rules.

## Validation And Guardrails

The repo ships guardrail scripts under `Scripts/` for architecture flow, dependency boundaries, manifest validation, and pull request compatibility. These scripts are part of the framework governance model and should stay aligned with any structural change.

## Framework Governance

This repository carries both implementation code and governance surfaces. `agentic-coding-policy.json`, `forsetti-instructions.json`, workflow guardrails, and the moderation/automation docs together define how contributors and automation should preserve the framework's modular-runtime baseline.
