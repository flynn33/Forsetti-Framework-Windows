# Getting started with the installed SDK

Read the complete [consumption policy](../CONSUMPTION_POLICY.md). Extract `Forsetti-Windows-SDK-0.2.0-x64.zip` to an installation outside application source, for example `D:\SDKs\Forsetti 0.2.0`. Verify the external ZIP checksum first, then review the installed `sdk-manifest.json` for source revision, file hashes, tested environments, and ABI settings.

## Prerequisites and dependency recipe

Use Windows, x64 MSVC with the compiler/toolset recorded in the manifest, CMake 3.28 or newer, the Windows SDK, PowerShell 7, and Git when reviewing tracked files. See [Compatibility](Compatibility.md) for configuration/runtime requirements. Visual Studio 2022 with the Desktop development with C++ workload provides the MSVC and Windows SDK tools used by the maintainer presets; installed-package compatibility is narrower than a generic Visual Studio version claim.

Forsetti publicly depends on `nlohmann_json::nlohmann_json` 3.12.0. The SDK does not include a vcpkg installation or the JSON headers. Use an external vcpkg installation and an app-owned dependency manifest. The generators write a manifest equivalent to:

```json
{
  "name": "my-application",
  "version-string": "0.1.0",
  "builtin-baseline": "cb2981c4e03d421fa03b9bb5044cd1986180e7e4",
  "dependencies": [
    { "name": "nlohmann-json", "version>=": "3.12.0#2" }
  ]
}
```

The immutable baseline's port is `3.12.0#2` ([port definition](https://github.com/microsoft/vcpkg/blob/cb2981c4e03d421fa03b9bb5044cd1986180e7e4/ports/nlohmann-json/vcpkg.json)). The installed package requires version 3.12.0 exactly. Keep the baseline and resolved dependency consistent; do not change JSON compile-time options independently of the binary's recorded settings. The dependency is MIT licensed with embedded component notices; see [Third-Party Notices](../THIRD-PARTY-NOTICES.md).

## Generate an independent application

Run tools by their installed absolute paths. They resolve templates relative to their own SDK location, accept quoted paths with spaces/non-ASCII characters, refuse overwrites, and perform no global installation:

```powershell
$sdk = 'D:\SDKs\Forsetti 0.2.0'
$app = 'D:\Applications\My Application'
& "$sdk\Tools\New-ForsettiProject.ps1" `
    -Destination $app -Name MyApplication -ModuleIDPrefix com.example.myapplication
```

The project contains app-owned host source under `src/`, app/UI/service modules under `Modules/`, independent CMake and vcpkg manifests, and `forsetti-project.json` with explicit scaffold inventory. No framework implementation directory is generated. Review these files before modifying features. The generated native host owns its presentation and activation-state persistence; the library owns activation validation and surface composition.

To generate another app-owned module:

```powershell
& "$sdk\Tools\New-ForsettiModule.ps1" `
    -Destination "$app\Modules\ReportsModule" -Kind service `
    -Name ReportsModule -ModuleID com.example.myapplication.reports -ProjectRoot $app
```

Kinds are `app`, `ui`, `service`, and `shared-database`. Shared-database produces a manifest starter only. `-ProjectRoot` enables project identity checks; inspect and explicitly add the new module target/source/resource and registry factory to the owning application's build/bootstrap. Update the owning root `forsetti-project.json` entries for `sources`, `cmakeFiles`, `resources`, and `manifests` to match those additions. The module's own generated inventory does not update its parent inventory. A generated directory alone does not activate a module.

## Build the application using the package

Use the vcpkg toolchain to resolve JSON. The example assumes an existing external vcpkg installation is set in `VCPKG_ROOT`; the SDK tools do not install it:

```powershell
cmake -S $app -B "$app\build" -G 'Visual Studio 17 2022' -A x64 `
    "-DCMAKE_CONFIGURATION_TYPES=Debug;Release" `
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows "-DCMAKE_PREFIX_PATH=$sdk" `
    '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL'
cmake --build "$app\build" --config Debug
cmake --build "$app\build" --config Release
```

The quoted runtime expression is literal CMake syntax. Select the actual compiler version/toolset recorded in the artifact; package diagnostics reject incompatible configurations. The normal consumer target links the imported products, never relative paths into a maintainer worktree. CMake chooses `lib/Debug` for Debug and `lib/Release` for Release.

For a core-only target:

```cmake
find_package(ForsettiFramework CONFIG REQUIRED)
add_library(ReportsModule STATIC ReportsModule.cpp ReportsModule.h)
target_link_libraries(ReportsModule PRIVATE Forsetti::ForsettiCore)
```

## Run, verify, and deploy

The generated build copies the explicitly declared app-owned manifests to `ForsettiManifests` beside the executable. The host locates that deployment folder from the executable location. Running from a different working directory must not select a different manifest set.

The starter boots with restore-only behavior; use its explicit app/UI/service activation controls for development. `--verify` checks runtime activation, denied entitlement, UI/app slot replacement, and native view creation. It uses `FileActivationStore` in a fresh temporary path with spaces and Japanese characters to check the desired service/UI selection on disk, preservation across shutdown, a fresh store's disk roundtrip, and restore-only reboot with native UI presentation. The check shuts down again and limits cleanup to its owned state filenames and empty temporary directory. Consult this archive's recorded results before claiming it passed. Review production entitlement/capability policy and persistent state ownership for the application's needs.

```powershell
& "$app\build\Debug\MyApplication.exe" --verify
& "$app\build\Release\MyApplication.exe" --verify
& "$sdk\Tools\Test-ForsettiIntegration.ps1" -ProjectRoot $app
& "$sdk\Tools\Test-ForsettiIntegration.ps1" `
    -ProjectRoot $app -ShippingRoot "$app\build\Release" -Json
```

Inspect the generated target's executable output location if customizing CMake. Return codes are part of verification; do not suppress failures. The integration verifier checks tracked source/content, dependency references, explicit source/resource membership, identities, and supplied shipping resources within its documented limits. Review unknown or generated content manually. It cannot prove arbitrary renamed, transformed, or omitted code was never copied.

Ship the app-owned executable and its explicit manifest resources, the application's required Microsoft runtime redistribution, and applicable licenses/notices. The development SDK, generators, templates, static libraries, and public-header installation remain outside shipping resources. Static linking does not eliminate required third-party notices.
