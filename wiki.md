# Forsetti Framework - Windows Wiki Index

This file is the repository-tracked companion to the public GitHub Wiki:

https://github.com/flynn33/Forsetti-Framework-Windows/wiki

The Wiki is the long-form documentation surface for architecture, runtime behavior, module authoring, validation, and governance. Repository documents should remain concise and point readers to the Wiki for deep explanations and diagrams.

## Wiki Page Set

| Page | Purpose |
|---|---|
| `Home` | Orientation, status, page map, and high-level diagrams |
| `Architecture` | Layering rules, component graph, dependency boundaries, and repository layout |
| `Runtime-Lifecycle` | Boot, discovery, activation, deactivation, restore, and entitlement reconciliation |
| `Module-System` | Module types, manifests, registry factories, compatibility, and examples |
| `Capabilities-and-Security` | Capability policy, scoped service access, messaging rules, and reserved namespaces |
| `UI-Surface-Model` | UI/app activation, toolbar items, view injections, overlays, and theme mask policy |
| `Platform-Services` | WinHTTP, Registry, DPAPI, local file export, telemetry, and service registration |
| `Build-and-Testing` | Prerequisites, CMake presets, CTest, guardrail scripts, and validation evidence |
| `API-Reference` | Public headers, interfaces, value types, enums, and platform services |
| `Coding-Policy` | Engineering rules, dependency invariants, style expectations, and verification policy |
| `Governance-and-Operations` | Repository automation, discussion agents, moderation, PR workflow, and evidence policy |
| `Roadmap-and-Risks` | Planned host template, remote workflow restoration, dependency pinning, and future hardening |

## Documentation Architecture

```mermaid
flowchart LR
    Repo["Repository docs\nREADME, CHANGELOG, CONTRIBUTING"]
    Wiki["GitHub Wiki\nlong-form guides"]
    Governance["Governance docs\npolicy and automation"]
    Evidence["Remediation evidence\n.forsetti/remediation"]
    Source["Source and tests\ninclude, src, tests, Scripts"]

    Source --> Repo
    Source --> Wiki
    Governance --> Wiki
    Evidence --> Wiki
    Repo --> Wiki
```

## Update Rules

- Update `README.md` when onboarding, build, validation, architecture, or release status changes.
- Update `CHANGELOG.md` for notable runtime, validation, governance, or documentation changes.
- Update Wiki pages when behavior needs explanation, diagrams, or examples beyond the README.
- Update `docs/governance` when repository automation, discussion routing, moderation, or owner-facing policy changes.
- Keep `forsetti-instructions.json` and `agentic-coding-policy.json` aligned with the source and public docs.

## Current Status

The remediation sequence is complete through final acceptance. The canonical validation path is:

```powershell
.\Scripts\verify-forsetti-guardrails.ps1
```

The wrapper configures, builds, runs CTest, checks architecture and dependency boundaries, validates manifests, runs pull request compatibility checks, and runs script regression tests.
