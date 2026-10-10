#requires -Version 7.2
# Copyright (c) 2026 James Daley. SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Archive,
    [Parameter(Mandatory)][string]$BuildRecord,
    [string]$ChecksumFile,
    [string]$WorkRoot,
    [string]$VisualStudioInstance
)
. (Join-Path $PSScriptRoot 'SDK/Common.ps1')
$checks = [Collections.Generic.List[object]]::new()
$hiddenPaths = [Collections.Generic.List[object]]::new()
$passed = $false
$manifest = $null
if (-not $WorkRoot) { $WorkRoot = Join-Path ([IO.Path]::GetTempPath()) ('Forsetti SDK validation é ' + [guid]::NewGuid().ToString('N')) }
$workFull = [IO.Path]::GetFullPath($WorkRoot)
$repoRoot = Split-Path -Parent $PSScriptRoot
$reportPath = Join-Path $workFull 'Verification-Results.json'

function Add-SdkCheck([string]$Name, [object]$Evidence) {
    $checks.Add([ordered]@{ name = $Name; passed = $true; evidence = $Evidence })
    Write-Host "Passed: $Name"
}
function Assert-SdkRejected([string]$Name, [scriptblock]$Action, [string]$ExpectedPattern) {
    $rejected = $false
    try { & $Action | Out-Null } catch { $rejected = $true; $message = $_.Exception.Message }
    if (-not $rejected) { throw "Negative check unexpectedly accepted: $Name" }
    if ($ExpectedPattern -and $message -notmatch $ExpectedPattern) { throw "Negative check failed for an unexpected reason: $Name : $message" }
    Add-SdkCheck $Name $message
}
function Invoke-SdkConfigure([string]$Source, [string]$Build, [string[]]$Extra = @(), [switch]$ExpectFailure) {
    $architecture = 'x64'
    if ($Extra.Count -eq 2 -and $Extra[0] -eq '-A') { $architecture = $Extra[1]; $Extra = @() }
    $args = @('-S', $Source, '-B', $Build, '-G', 'Visual Studio 17 2022', '-A', $architecture,
        "-DCMAKE_GENERATOR_INSTANCE=$VisualStudioInstance", "-DCMAKE_PREFIX_PATH=$sdkRoot;$dependencyPrefix", '-DCMAKE_CONFIGURATION_TYPES=Debug;Release') + $Extra
    Invoke-SdkCommand 'cmake' $args (Join-Path $Build 'configure.log') -ExpectFailure:$ExpectFailure
}

try {
    Get-SdkRelativePath $repoRoot $workFull -ErrorAction Stop | Out-Null
    throw 'Verification must run outside the framework worktree.'
} catch {
    if ($_.Exception.Message -eq 'Verification must run outside the framework worktree.') { throw }
}
try {
    if (Test-Path -LiteralPath $workFull) { throw "Verification work directory already exists: $workFull" }
    Assert-SdkOrdinaryPath $workFull | Out-Null
    [IO.Directory]::CreateDirectory($workFull) | Out-Null
    if (-not $ChecksumFile) { $ChecksumFile = Join-Path (Split-Path -Parent ([IO.Path]::GetFullPath($Archive))) 'SHA256SUMS' }
    $checksumName = [IO.Path]::GetFileName($Archive)
    $checksumLines = @(Get-Content -LiteralPath $ChecksumFile | Where-Object { $_ -match "^[a-fA-F0-9]{64}\s+$([regex]::Escape($checksumName))$" })
    if ($checksumLines.Count -ne 1 -or $checksumLines[0].Substring(0, 64).ToLowerInvariant() -ne (Get-SdkSha256 $Archive)) { throw 'External archive checksum mismatch.' }
    Add-SdkCheck 'external SHA256SUMS archive checksum' (Get-SdkSha256 $Archive)
    $extracted = Expand-SdkArchive ([IO.Path]::GetFullPath($Archive)) (Join-Path $workFull 'initial extraction')
    $manifest = Test-SdkPayload $extracted
    Add-SdkCheck 'archive exact inventory and hashes' ([ordered]@{ archiveSha256 = Get-SdkSha256 $Archive; payloadFiles = @($manifest.files).Count + 1 })
    $sdkRoot = Join-Path $workFull 'relocated SDK é'
    Move-Item -LiteralPath $extracted -Destination $sdkRoot
    if (Test-Path -LiteralPath $extracted) { throw 'Original extraction location remains available.' }
    Test-SdkPayload $sdkRoot | Out-Null
    Add-SdkCheck 'archive relocation to spaces and non-ASCII path' $sdkRoot
    $record = $null
    if ($BuildRecord) {
        $record = Get-Content -LiteralPath $BuildRecord -Raw | ConvertFrom-Json
        if ($record.sourceRevision -ne $manifest.source.revision) { throw 'Build record revision mismatch.' }
        if (-not $VisualStudioInstance) { $VisualStudioInstance = $record.visualStudioInstance }
        if (-not $manifest.source.dirtyCandidate) {
            $committedPolicy = Join-Path $workFull 'committed-consumption-policy.md'
            Export-SdkGitFile $repoRoot $manifest.source.revision 'CONSUMPTION_POLICY.md' $committedPolicy
            if ((Get-SdkSha256 $committedPolicy) -ne $manifest.consumptionPolicy.sha256) { throw 'SDK policy differs from selected Git commit.' }
            Add-SdkCheck 'complete policy equals selected Git blob byte for byte' $manifest.consumptionPolicy.sha256
        } else {
            if ((Get-SdkSha256 (Join-Path $record.sourceRoot 'CONSUMPTION_POLICY.md')) -ne $manifest.consumptionPolicy.sha256) { throw 'Candidate policy differs from source snapshot.' }
            Add-SdkCheck 'candidate policy equals source snapshot byte for byte' $manifest.consumptionPolicy.sha256
        }
        $compilerConfig = @(Get-ChildItem -LiteralPath (Join-Path $record.buildRoot 'CMakeFiles') -Recurse -Filter CMakeCXXCompiler.cmake)[0]
        $compilerContent = Get-Content -LiteralPath $compilerConfig.FullName -Raw
        if ($compilerContent -notmatch 'set\(CMAKE_CXX_COMPILER "([^"]+)"\)') { throw 'Compiler path missing from CMake metadata.' }
        $dumpbin = Join-Path (Split-Path -Parent $Matches[1]) 'dumpbin.exe'
        foreach ($configuration in @('Debug', 'Release')) {
            foreach ($product in @('ForsettiCore', 'ForsettiPlatform', 'ForsettiHostTemplate')) {
                $library = Join-Path $sdkRoot "lib/$configuration/$product.lib"
                $headersLog = Join-Path $workFull "logs/$configuration-$product-headers.log"
                Invoke-SdkCommand $dumpbin @('/headers', $library) $headersLog | Out-Null
                $headers = Get-Content -LiteralPath $headersLog -Raw
                if ($headers -notmatch '8664 machine \(x64\)' -or $headers -match '14C machine|AA64 machine') { throw "Incorrect binary architecture: $library" }
                $directivesLog = Join-Path $workFull "logs/$configuration-$product-directives.log"
                Invoke-SdkCommand $dumpbin @('/directives', $library) $directivesLog | Out-Null
                $directives = Get-Content -LiteralPath $directivesLog -Raw
                $runtime = if ($configuration -eq 'Debug') { 'MDd_DynamicDebug' } else { 'MD_DynamicRelease' }
                $iterator = if ($configuration -eq 'Debug') { '2' } else { '0' }
                if ($directives -notmatch "RuntimeLibrary=$runtime" -or $directives -notmatch "_ITERATOR_DEBUG_LEVEL=$iterator") { throw "Binary CRT/iterator configuration mismatch: $library" }
            }
        }
        Add-SdkCheck 'six native x64 libraries CRT and iterator directives' @('Debug /MDd iterator=2', 'Release /MD iterator=0')
    }
    if (-not $VisualStudioInstance) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        $instances = @((& $vswhere -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json))
        if ($instances.Count -eq 0) { throw 'Visual Studio 2022 native C++ tools required.' }
        $VisualStudioInstance = $instances[0].installationPath
    }
    if (-not $env:VCPKG_ROOT) { throw 'Set VCPKG_ROOT to obtain the declared separate prerequisite.' }
    $prerequisites = Join-Path $workFull 'prerequisites'
    $dependency = $manifest.dependencies.'nlohmann-json'
    Write-SdkText (Join-Path $prerequisites 'vcpkg.json') (([ordered]@{
        name = 'forsetti-sdk-consumer-verification'; 'version-string' = '1.0.0'
        'builtin-baseline' = $dependency.vcpkgBuiltinBaseline; dependencies = @('nlohmann-json')
    } | ConvertTo-Json -Depth 6) + "`n")
    $dependencyInstall = Join-Path $workFull 'declared dependencies'
    Invoke-SdkCommand (Join-Path $env:VCPKG_ROOT 'vcpkg.exe') @('install', "--x-manifest-root=$prerequisites", "--x-install-root=$dependencyInstall", '--triplet=x64-windows') (Join-Path $workFull 'logs/dependency-install.log') | Out-Null
    $dependencyPrefix = Join-Path $dependencyInstall 'x64-windows'
    Add-SdkCheck 'separate pinned nlohmann prerequisite installation' $dependency

    if ($record) {
        # Hide precisely the SDK build's original source and build directories.
        # Both targets are validated within the ignored repository output before moving.
        foreach ($path in @($record.sourceRoot, $record.buildRoot)) {
            $full = [IO.Path]::GetFullPath($path)
            Assert-SdkOrdinaryPath $full | Out-Null
            $relative = Get-SdkRelativePath $repoRoot $full
            & git -C $repoRoot check-ignore --quiet -- "$relative/ignore-probe"
            if ($LASTEXITCODE -ne 0 -or $relative -notlike 'out/*') { throw "Refusing to hide a directory outside ignored out/: $full" }
            $destination = $full + '.unavailable-for-consumer-verification'
            if (Test-Path -LiteralPath $destination) { throw "Hide destination exists: $destination" }
            Move-Item -LiteralPath $full -Destination $destination
            $hiddenPaths.Add([pscustomobject]@{ original = $full; hidden = $destination })
        }
        Add-SdkCheck 'original SDK source and build paths made unavailable' @($hiddenPaths.original)
    }
    $consumer = Join-Path $workFull 'independent application é'
    Invoke-SdkCommand 'pwsh' @('-NoProfile', '-File', (Join-Path $sdkRoot 'Tools/New-ForsettiProject.ps1'), '-Destination', $consumer, '-Name', 'SdkProbe', '-ModuleIDPrefix', 'com.validation.forsetti') (Join-Path $workFull 'logs/generate-host.log') | Out-Null
    Invoke-SdkCommand 'git' @('-C', $consumer, 'init', '--quiet') (Join-Path $workFull 'logs/consumer-git-init.log') | Out-Null
    Invoke-SdkCommand 'git' @('-C', $consumer, 'add', '--all') (Join-Path $workFull 'logs/consumer-git-add.log') | Out-Null
    $consumerBuild = Join-Path $consumer 'build'
    Invoke-SdkConfigure $consumer $consumerBuild | Out-Null
    foreach ($configuration in @('Debug', 'Release')) {
        Invoke-SdkCommand 'cmake' @('--build', $consumerBuild, '--config', $configuration, '--parallel', '8') (Join-Path $workFull "logs/host-build-$configuration.log") | Out-Null
        Invoke-SdkCommand 'ctest' @('--test-dir', $consumerBuild, '-C', $configuration, '--verbose', '--output-on-failure', '--no-tests=error') (Join-Path $workFull "logs/host-test-$configuration.log") | Out-Null
        $executables = @(Get-ChildItem -LiteralPath $consumerBuild -Recurse -Filter SdkProbe.exe | Where-Object { $_.Directory.Name -eq $configuration })
        if ($executables.Count -ne 1) { throw "Expected one generated $configuration executable." }
        Invoke-SdkCommand 'pwsh' @('-NoProfile', '-File', (Join-Path $sdkRoot 'Tools/Test-ForsettiIntegration.ps1'), '-ProjectRoot', $consumer, '-ShippingRoot', $executables[0].Directory.FullName, '-Json') (Join-Path $workFull "logs/integration-$configuration.log") | Out-Null
        Add-SdkCheck "generated $configuration host runtime, denied activation and native presentation" (Join-Path $workFull "logs/host-test-$configuration.log")
        Add-SdkCheck "$configuration tracked source and deployment integration scan" (Join-Path $workFull "logs/integration-$configuration.log")
    }

    $core = Join-Path $workFull 'core only consumer é'
    Write-SdkText (Join-Path $core 'main.cpp') @'
#include <ForsettiCore/SemVer.h>
#include <ForsettiCore/ForsettiVersion.h>
#include <iostream>
int main() {
    auto parsed = Forsetti::SemVer::fromString("0.2.0");
    if (!parsed || *parsed != Forsetti::ForsettiVersion::current) return 1;
    std::cout << parsed->toString() << '\n';
    return 0;
}
'@
    Write-SdkText (Join-Path $core 'CMakeLists.txt') @'
cmake_minimum_required(VERSION 3.28)
project(CoreConsumer LANGUAGES CXX)
find_package(ForsettiFramework CONFIG REQUIRED)
add_executable(CoreProbe main.cpp)
target_link_libraries(CoreProbe PRIVATE Forsetti::ForsettiCore)
'@
    $coreBuild = Join-Path $core 'build'
    Invoke-SdkConfigure $core $coreBuild | Out-Null
    foreach ($configuration in @('Debug', 'Release')) {
        Invoke-SdkCommand 'cmake' @('--build', $coreBuild, '--config', $configuration, '--parallel', '8') (Join-Path $workFull "logs/core-build-$configuration.log") | Out-Null
        $log = Join-Path $workFull "logs/core-run-$configuration.log"
        Invoke-SdkCommand (Join-Path $coreBuild "$configuration/CoreProbe.exe") @() $log | Out-Null
        if ((Get-Content -LiteralPath $log -Raw).Trim() -ne $manifest.version) { throw 'Linked runtime version mismatch.' }
        Add-SdkCheck "core-only $configuration consumer links and runs" $manifest.version
    }
    $headerRoot = Join-Path $workFull 'public header isolation'
    $headerSources = [Collections.Generic.List[string]]::new()
    $index = 0
    foreach ($header in @($manifest.files | Where-Object { $_.role -eq 'sdk-owned-public-interface' })) {
        $name = "header-$index.cpp"
        Write-SdkText (Join-Path $headerRoot $name) ("#include <$($header.path.Substring(8))>`nint forsetti_header_$index() { return $index; }`n")
        $headerSources.Add($name)
        $index++
    }
    # No forced SDK compatibility include: it would mask missing header dependencies.
    $headerCmake = @'
cmake_minimum_required(VERSION 3.28)
project(HeaderIsolation LANGUAGES CXX)
find_package(nlohmann_json 3.12.0 EXACT CONFIG REQUIRED)
add_library(HeaderChecks OBJECT @HEADER_SOURCES@)
target_compile_features(HeaderChecks PRIVATE cxx_std_20)
target_include_directories(HeaderChecks PRIVATE "@SDK_ROOT@/include")
target_link_libraries(HeaderChecks PRIVATE nlohmann_json::nlohmann_json)
'@
    Write-SdkText (Join-Path $headerRoot 'CMakeLists.txt') $headerCmake.Replace('@HEADER_SOURCES@', ($headerSources -join ' ')).Replace('@SDK_ROOT@', $sdkRoot.Replace('\', '/'))
    $headerBuild = Join-Path $headerRoot 'build'
    Invoke-SdkConfigure $headerRoot $headerBuild | Out-Null
    foreach ($configuration in @('Debug', 'Release')) {
        Invoke-SdkCommand 'cmake' @('--build', $headerBuild, '--config', $configuration, '--parallel', '8') (Join-Path $workFull "logs/headers-$configuration.log") | Out-Null
        Add-SdkCheck "all $index public headers compile individually in $configuration" (Join-Path $workFull "logs/headers-$configuration.log")
    }
    foreach ($case in @(
        @{ name = 'missing nlohmann dependency rejected'; args = @('-DCMAKE_DISABLE_FIND_PACKAGE_nlohmann_json=TRUE'); pattern = 'nlohmann_json' },
        @{ name = 'unsupported SDK configuration rejected'; args = @('-DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo'); pattern = 'Unsupported Forsetti SDK configuration' },
        @{ name = 'incompatible CRT selection rejected'; args = @('-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'); pattern = 'requires /MDd Debug and /MD Release' },
        @{ name = 'unsupported x86 architecture rejected'; args = @('-A', 'Win32'); pattern = 'supports x64 only|version: [0-9.]+ \(64bit\)' }
    )) {
        $negativeBuild = Join-Path $workFull ('negative-' + [guid]::NewGuid().ToString('N'))
        Invoke-SdkConfigure $core $negativeBuild $case.args -ExpectFailure | Out-Null
        if ((Get-Content -LiteralPath (Join-Path $negativeBuild 'configure.log') -Raw) -notmatch $case.pattern) { throw "Negative configure failed for unexpected reason: $($case.name)" }
        Add-SdkCheck $case.name (Join-Path $negativeBuild 'configure.log')
    }
    $readmePath = Join-Path $sdkRoot 'README.md'
    $originalReadme = [IO.File]::ReadAllBytes($readmePath)
    try {
        [IO.File]::AppendAllText($readmePath, 'tampered')
        Assert-SdkRejected 'tampered payload rejected' { Test-SdkPayload $sdkRoot } 'SDK file tampered'
    } finally { [IO.File]::WriteAllBytes($readmePath, $originalReadme) }
    $unexpected = Join-Path $sdkRoot 'unexpected-file.txt'
    try { Write-SdkText $unexpected 'unexpected'; Assert-SdkRejected 'unexpected staged file rejected' { Test-SdkPayload $sdkRoot } 'Unexpected SDK file' }
    finally { Remove-Item -LiteralPath $unexpected }
    $libraryPath = Join-Path $sdkRoot 'lib/Debug/ForsettiCore.lib'
    $libraryBackup = Join-Path $workFull 'temporarily-removed-library.lib'
    try { Move-Item -LiteralPath $libraryPath -Destination $libraryBackup; Assert-SdkRejected 'missing library rejected' { Test-SdkPayload $sdkRoot } 'Missing SDK file' }
    finally { Move-Item -LiteralPath $libraryBackup -Destination $libraryPath }
    $manifestPath = Join-Path $sdkRoot 'sdk-manifest.json'
    $originalManifest = [IO.File]::ReadAllBytes($manifestPath)
    try {
        $invalidPolicyManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
        $invalidPolicyManifest.consumptionPolicy.path = '../outside-policy.md'
        Write-SdkText $manifestPath ($invalidPolicyManifest | ConvertTo-Json -Depth 20)
        Assert-SdkRejected 'escaping consumption policy path rejected' { Test-SdkPayload $sdkRoot } 'Complete canonical consumption policy mismatch'
    } finally { [IO.File]::WriteAllBytes($manifestPath, $originalManifest) }
    # Keep the junction in an isolated negative fixture. The real extracted SDK
    # stays ordinary, and no cleanup operation follows a link into its target.
    $junctionFixture = Join-Path $workFull 'junction payload fixture'
    [IO.Directory]::CreateDirectory($junctionFixture) | Out-Null
    [IO.File]::Copy($manifestPath, (Join-Path $junctionFixture 'sdk-manifest.json'))
    $junction = Join-Path $junctionFixture 'escaping junction'
    New-Item -ItemType Junction -Path $junction -Target $sdkRoot | Out-Null
    Assert-SdkRejected 'junction in payload rejected' { Test-SdkPayload $junctionFixture } 'Symlinks and junctions are not permitted'
    Assert-SdkRejected 'junction destination ancestor rejected' { Copy-SdkFile $sdkRoot 'README.md' $junction 'escaped-copy.md' } 'Reparse-point path is forbidden'
    foreach ($case in @(@{ name = 'escaping archive path rejected'; paths = @('../escape.txt'); pattern = 'Unsafe payload path' }, @{ name = 'case-colliding archive paths rejected'; paths = @('SDK/readme.md', 'SDK/README.md'); pattern = 'Case-colliding ZIP entry' })) {
        $fixture = Join-Path $workFull ([guid]::NewGuid().ToString('N') + '.zip')
        $stream = [IO.File]::Create($fixture)
        $zip = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create)
        try { foreach ($path in $case.paths) { $zip.CreateEntry($path) | Out-Null } } finally { $zip.Dispose(); $stream.Dispose() }
        Assert-SdkRejected $case.name { Expand-SdkArchive $fixture (Join-Path $workFull ([guid]::NewGuid().ToString('N'))) } $case.pattern
    }
    Test-SdkPayload $sdkRoot | Out-Null
    $passed = $true
} catch {
    $checks.Add([ordered]@{ name = 'verification failure'; passed = $false; evidence = $_.Exception.Message })
    Write-Error $_ -ErrorAction Continue
} finally {
    foreach ($entry in $hiddenPaths) {
        try { Move-Item -LiteralPath $entry.hidden -Destination $entry.original }
        catch { $passed = $false; $checks.Add([ordered]@{ name = 'source path restoration failure'; passed = $false; evidence = $_.Exception.Message }); Write-Error $_ -ErrorAction Continue }
    }
    $os = Get-CimInstance Win32_OperatingSystem
    $report = [ordered]@{
        sourceRevision = $(if ($manifest) { $manifest.source.revision } else { $null })
        dirtyCandidate = $(if ($manifest) { $manifest.source.dirtyCandidate } else { $true })
        archiveSha256 = $(if (Test-Path -LiteralPath $Archive) { Get-SdkSha256 $Archive } else { $null })
        payloadSha256 = $(if ($manifest) { $manifest.payloadSha256 } else { $null })
        passed = $passed
        environment = [ordered]@{ operatingSystem = $os.Caption; version = $os.Version; buildNumber = $os.BuildNumber; architecture = $os.OSArchitecture; visualStudioInstance = $VisualStudioInstance; toolchain = $(if ($manifest) { $manifest.toolchain } else { $null }) }
        checks = @($checks.ToArray())
        workRoot = $workFull
    }
    Write-SdkText $reportPath (($report | ConvertTo-Json -Depth 20) + "`n")
    Write-Host "Verification report: $reportPath"
}
if (-not $passed) { exit 1 }
