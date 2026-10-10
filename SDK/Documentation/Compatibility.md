# Compatibility and ABI requirements

The installed SDK contains **x64 Windows static libraries in Debug and Release configurations**. No x86, ARM64, Linux, MinGW, Clang, DLL, NuGet, or universal C++ ABI support is declared by this payload. See `sdk-manifest.json` for the precise compiler version, MSVC toolset, Windows SDK version, settings, and environments actually built/tested.

| Setting | Debug | Release |
| --- | --- | --- |
| Installed library directory | `lib/Debug/` | `lib/Release/` |
| Runtime library | `/MDd` | `/MD` |
| MSVC iterator debugging | `_ITERATOR_DEBUG_LEVEL=2` | `_ITERATOR_DEBUG_LEVEL=0` |
| Language | C++20 | C++20 |
| Target machine | x64 | x64 |

Use the artifact's exact MSVC compiler/toolset. Keep RTTI, exception support, runtime library, iterator debugging, public JSON options, and other ABI-affecting settings consistent with its manifest. This C++ interface passes standard-library and JSON types across static-library boundaries; an arbitrary compiler or incompatible standard-library build is unsafe. The installed package and compatibility header diagnose known incompatible settings. Do not disable those checks to force a link.

For a multi-configuration generator, set `CMAKE_CONFIGURATION_TYPES=Debug;Release` at configuration time. For a single-configuration generator, select `CMAKE_BUILD_TYPE=Debug` or `Release`. RelWithDebInfo, MinSizeRel, blank/custom configurations, and mappings that silently substitute Release for Debug are unsupported. The SDK explicitly maps each supported imported configuration to its matching `.lib` files.

The static Forsetti libraries use the dynamic MSVC runtime. The final application must follow Microsoft's matching runtime and redistribution requirements. All modules in a linker invocation must use the same runtime-library compiler option. Debug runtime availability is a development prerequisite; a Debug build is not a production redistribution plan. See [Microsoft runtime-library documentation](https://learn.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=msvc-170).

`nlohmann_json::nlohmann_json` 3.12.0 is a required public package target. The inspected vcpkg baseline `cb2981c4e03d421fa03b9bb5044cd1986180e7e4` resolves the `nlohmann-json` port at `3.12.0#2`. Follow [Getting Started](Getting-Started.md) for a manifest-mode recipe. The SDK is not an offline dependency bundle. Do not bundle an entire vcpkg tree, modify dependency caches, or independently change JSON ABI settings.

Platform's Windows SDK link requirements include `advapi32`, `bcrypt`, `crypt32`, and `winhttp`; installed CMake metadata preserves these requirements for static consumers. HostTemplate depends transitively on Platform and Core; Core carries JSON usage requirements. Compiler warning-as-error settings used for Forsetti maintenance are not consumer policy.

The framework recognizes manifest schema `1.0` and `1.1`, with current scaffolds using schema/template `1.1` and minimum runtime `0.2.0`. The generated module version is app-owned and independent of the framework version. Compatibility is checked during activation, alongside entitlement, capability, registration, identity, required provider, and UI declaration checks. A compatible SDK link does not override a denied module activation.

The host scaffold uses native Win32 presentation via public view factory contracts. The surface models describe contributions; they do not provide a complete WinUI renderer. Shared-database is manifest-only. Declared integration scope, executed archive tests, and an application's own feature/UI validation are separate evidence; use the manifest's verification entries and test your deployed application.
