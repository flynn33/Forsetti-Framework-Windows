# Forsetti Windows SDK 0.2.0

The SDK version remains `0.2.0`, matching `CMakeLists.txt`, `vcpkg.json`, and `ForsettiVersion::current`. This delivery adds an installed distribution boundary; it does not change the runtime version or redesign the public C++ ABI.

The payload includes x64 Debug/Release static Core, Platform, and HostTemplate products; installed headers and CMake exports; the complete consumption policy; app/UI/service/host generators; a manifest-only shared-database starter; and consumer integration documentation/tools. Example and test targets, runtime implementation sources, maintainer policies/scripts, package caches, and repository metadata are excluded.

The host scaffold provides app-owned native Win32 presentation wiring through public view factories. It is not a complete WinUI renderer or a database implementation. Runtime checks and the shared UI/app slot remain framework-owned.

Use `sdk-manifest.json` for this artifact's exact clean source revision, candidate status, toolchain/ABI settings, configuration mapping, file hashes, dependency provenance, and executed verification environments. The external `SHA256SUMS` identifies the archive bytes. Documentation alone is not native build or execution evidence. Unsupported architectures, compilers, runtime settings, or UI frameworks are not implied by this release.
