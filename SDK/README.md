# Forsetti Windows SDK 0.2.0

**Do not copy or vendor the framework repository into an application.** Install this SDK outside the application's tracked source tree, consume its installed libraries, and generate app-owned scaffold files. Read the complete [CONSUMPTION_POLICY.md](CONSUMPTION_POLICY.md) before integration; its bytes and SHA-256 are part of the release manifest.

This x64 Windows SDK provides Debug and Release static `ForsettiCore`, `ForsettiPlatform`, and `ForsettiHostTemplate` libraries, their public C++20 headers, relocatable CMake metadata, canonical templates, consumer documentation, and PowerShell development tools. `ForsettiHostTemplate` is a sealed library product. Its name does not authorize copying its runtime implementation.

The installed entry point is:

```cmake
find_package(ForsettiFramework CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE
    Forsetti::ForsettiCore
    Forsetti::ForsettiPlatform
    Forsetti::ForsettiHostTemplate
)
```

Link Core alone for a module that only uses core contracts. Read [Getting Started](Documentation/Getting-Started.md) for prerequisites, exact generation commands, dependency pinning, build commands, and deployment. Read [Compatibility](Documentation/Compatibility.md) before selecting compiler settings: Debug uses `/MDd`, Release uses `/MD`, and the shipped manifest identifies the actual toolchain and executed tests. This is not a self-contained offline dependency bundle.

The tools under `Tools/` generate independent app-owned projects/modules and check integration boundaries. Templates, tools, headers, and static libraries are development material; ship the built app, its declared manifest resources, required runtime redistribution, and required notices. The shared-database starter is manifest-only; it supplies no database implementation. The host starter uses native Win32 presentation through public Windows view factories. It does not supply a full WinUI renderer.

| Document | Purpose |
| --- | --- |
| [AGENTS.md](AGENTS.md) | Instructions for agents integrating this installed SDK. |
| [Public API](Documentation/Public-API.md) | Public header groups and extension points. |
| [Module Authoring](Documentation/Module-Authoring.md) | Manifest, identity, services, registration, activation, and UI contracts. |
| [Source References](Documentation/Source-References.md) | Immutable release-source mappings and policy provenance. |
| [Release Notes](Release-Notes.md) | Delivery scope and verification evidence location. |
| [Third-Party Notices](THIRD-PARTY-NOTICES.md) | Declared JSON dependency and its license notices. |

Before using an archive, check its external `SHA256SUMS`; inspect `sdk-manifest.json` for source revision, candidate/release status, toolchain/configuration requirements, file inventory/hashes, and test evidence. No historical README test count substitutes for this archive's actual verification record. Forsetti is dual-licensed. See [LICENSE](LICENSE).
