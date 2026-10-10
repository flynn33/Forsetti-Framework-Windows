#requires -Version 7.2
# Copyright (c) 2026 James Daley. SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SourceRoot,
    [Parameter(Mandatory)][string]$InstallRoot,
    [Parameter(Mandatory)][string]$BuildRoot,
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{40}$')][string]$SourceRevision,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$DirtyCandidate,
    [string]$VerificationReport
)
. (Join-Path $PSScriptRoot 'SDK/Common.ps1')

try {
    $sourceFull = (Get-Item -LiteralPath $SourceRoot).FullName
    $installFull = (Get-Item -LiteralPath $InstallRoot).FullName
    $metadata = Get-Content -LiteralPath (Join-Path $BuildRoot 'ForsettiSdkBuildMetadata.json') -Raw | ConvertFrom-Json
    $version = $metadata.frameworkVersion
    if ($version -notmatch '^\d+\.\d+\.\d+$' -or $metadata.architecture -ne 'x64' -or $metadata.compilerId -ne 'MSVC') { throw 'Unsupported SDK build metadata.' }
    $sourceManifest = Get-Content -LiteralPath (Join-Path $sourceFull 'vcpkg.json') -Raw | ConvertFrom-Json
    $cmakeSource = Get-Content -LiteralPath (Join-Path $sourceFull 'CMakeLists.txt') -Raw
    $versionHeader = Get-Content -LiteralPath (Join-Path $sourceFull 'include/ForsettiCore/ForsettiVersion.h') -Raw
    $parts = $version.Split('.')
    if ($sourceManifest.'version-string' -ne $version -or $cmakeSource -notmatch "VERSION\s+$([regex]::Escape($version))\b" -or
        $versionHeader -notmatch "current\s*\{\s*$($parts[0])\s*,\s*$($parts[1])\s*,\s*$($parts[2])\s*\}") { throw 'Source versions do not match built SDK version.' }
    if (Test-Path -LiteralPath $OutputDirectory) { throw "Use a new output directory: $OutputDirectory" }
    Assert-SdkOrdinaryPath $OutputDirectory | Out-Null
    [IO.Directory]::CreateDirectory([IO.Path]::GetFullPath($OutputDirectory)) | Out-Null
    $outputFull = [IO.Path]::GetFullPath($OutputDirectory)
    $payloadName = "Forsetti-Windows-SDK-$version-x64"
    $payload = Join-Path $outputFull $payloadName
    [IO.Directory]::CreateDirectory($payload) | Out-Null
    $mapping = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
    function Add-PayloadFile([string]$Root, [string]$InputPath, [string]$OutputPath, [string]$Role) {
        if ($mapping.ContainsKey($OutputPath)) { throw "Case-colliding payload mapping: $OutputPath" }
        Copy-SdkFile $Root $InputPath $payload $OutputPath
        $mapping.Add($OutputPath, [ordered]@{ source = $InputPath; role = $Role })
    }

    # An explicit allowlist, assembled directly into an empty payload.
    foreach ($product in @('ForsettiCore', 'ForsettiPlatform', 'ForsettiHostTemplate')) {
        $headerRoot = Join-Path $sourceFull "include/$product"
        foreach ($header in Get-SdkTreeFiles $headerRoot) {
            if ($header.Extension -ne '.h') { throw "Unapproved public interface file: $($header.FullName)" }
            $relative = Get-SdkRelativePath $sourceFull $header.FullName
            Add-PayloadFile $installFull $relative $relative 'sdk-owned-public-interface'
            if ((Get-SdkSha256 $header.FullName) -ne (Get-SdkSha256 (Join-Path $payload $relative))) { throw "Installed header differs from selected source: $relative" }
        }
        foreach ($configuration in @('Debug', 'Release')) {
            $path = "lib/$configuration/$product.lib"
            Add-PayloadFile $installFull $path $path 'static-link-input'
        }
    }
    foreach ($name in @('ForsettiFrameworkConfig.cmake', 'ForsettiFrameworkConfigVersion.cmake', 'ForsettiFrameworkTargets.cmake',
        'ForsettiFrameworkTargets-debug.cmake', 'ForsettiFrameworkTargets-release.cmake', 'ForsettiSdkCompatibility.h')) {
        $path = "lib/cmake/ForsettiFramework/$name"
        Add-PayloadFile $installFull $path $path 'installed-package-metadata'
        $text = Get-Content -LiteralPath (Join-Path $payload $path) -Raw
        foreach ($forbiddenRoot in @($sourceFull, $installFull, [IO.Path]::GetFullPath($BuildRoot))) {
            if ($text.Contains($forbiddenRoot) -or $text.Contains($forbiddenRoot.Replace('\', '/'))) { throw "Build-machine path leaked into $path" }
        }
        if ($text -match 'ForsettiExample|ForsettiDemo|ForsettiCoreTests|ForsettiPlatformTests') { throw "Nonproduct export in $path" }
    }
    $installedExpected = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($path in $mapping.Keys) { $installedExpected.Add($path) | Out-Null }
    foreach ($installed in Get-SdkTreeFiles $installFull) {
        $relative = Get-SdkRelativePath $installFull $installed.FullName
        if (-not $installedExpected.Contains($relative)) { throw "Unexpected CMake-installed file: $relative" }
    }
    foreach ($templateName in @('windows-host', 'windows-app-module', 'windows-ui-module', 'windows-service-module', 'windows-shared-database-module')) {
        foreach ($file in Get-SdkTreeFiles (Join-Path $sourceFull "templates/$templateName")) {
            $relative = Get-SdkRelativePath $sourceFull $file.FullName
            if ($file.Extension -notin @('.md', '.h', '.cpp', '.txt', '.json', '.manifest', '.gitignore') -and $file.Name -ne '.gitignore') { throw "Unapproved scaffold file: $relative" }
            Add-PayloadFile $sourceFull $relative $relative 'app-owned-scaffold-input'
        }
    }
    foreach ($name in @('README.md', 'AGENTS.md', 'THIRD-PARTY-NOTICES.md', 'Release-Notes.md')) {
        Add-PayloadFile $sourceFull "SDK/$name" $name 'developer-documentation'
    }
    foreach ($name in @('Getting-Started.md', 'Public-API.md', 'Module-Authoring.md', 'Compatibility.md', 'Source-References.md')) {
        Add-PayloadFile $sourceFull "SDK/Documentation/$name" "Documentation/$name" 'developer-documentation'
    }
    foreach ($file in Get-SdkTreeFiles (Join-Path $sourceFull 'SDK/Tools')) {
        $relative = Get-SdkRelativePath $sourceFull $file.FullName
        if ($file.Extension -ne '.ps1') { throw "Unapproved developer tool: $relative" }
        Add-PayloadFile $sourceFull $relative $relative.Substring(4) 'development-tool'
    }
    foreach ($name in @('LICENSE', 'CONSUMPTION_POLICY.md')) { Add-PayloadFile $sourceFull $name $name 'policy-and-notices' }
    if (Test-Path -LiteralPath (Join-Path $sourceFull 'NOTICE')) { Add-PayloadFile $sourceFull 'NOTICE' 'NOTICE' 'policy-and-notices' }
    # Policy bytes are copied unchanged; only designated SDK documentation tokens are rendered.
    foreach ($path in @($mapping.Keys)) {
        if ($mapping[$path].role -ne 'developer-documentation') { continue }
        $filePath = Join-Path $payload $path
        $text = Get-Content -LiteralPath $filePath -Raw
        $text = $text.Replace('@FORSETTI_SOURCE_REVISION@', $SourceRevision).Replace('@FORSETTI_VERSION@', $version)
        Write-SdkText $filePath $text
    }
    $policyHash = Get-SdkSha256 (Join-Path $payload 'CONSUMPTION_POLICY.md')
    if ($policyHash -ne (Get-SdkSha256 (Join-Path $sourceFull 'CONSUMPTION_POLICY.md'))) { throw 'Complete consumption policy byte equality failed.' }
    $verification = [ordered]@{ status = 'unverified-candidate'; testedEnvironments = @(); results = @() }
    if ($VerificationReport) {
        $report = Get-Content -LiteralPath $VerificationReport -Raw | ConvertFrom-Json
        if ($DirtyCandidate -or $report.dirtyCandidate -or $report.sourceRevision -ne $SourceRevision -or -not $report.passed) { throw 'Verification evidence must pass and match the clean source revision.' }
        $verification.status = 'verified'
        $verification.testedEnvironments = @([ordered]@{
            operatingSystem = $report.environment.operatingSystem; version = $report.environment.version
            buildNumber = $report.environment.buildNumber; architecture = $report.environment.architecture
            toolchain = $report.environment.toolchain
        })
        $verification.results = @($report.checks | ForEach-Object { [ordered]@{ name = $_.name; passed = $_.passed } })
    }
    $files = @($mapping.Keys | Sort-Object -CaseSensitive | ForEach-Object {
        $path = $_
        $item = Get-Item -LiteralPath (Join-Path $payload $path)
        [ordered]@{ path = $path; bytes = $item.Length; sha256 = Get-SdkSha256 $item.FullName; source = $mapping[$path].source; role = $mapping[$path].role }
    })
    $payloadDigest = Get-SdkPayloadDigest $files
    if ($VerificationReport -and $report.payloadSha256 -ne $payloadDigest) { throw 'Verification evidence does not match the assembled payload hashes.' }
    $manifest = [ordered]@{
        schemaVersion = '1.0'; name = 'Forsetti Windows SDK'; version = $version; architecture = 'x64'
        source = [ordered]@{ repository = 'https://github.com/flynn33/Forsetti-Framework-Windows'; revision = $SourceRevision; dirtyCandidate = [bool]$DirtyCandidate }
        versionSources = @('CMakeLists.txt:project(VERSION)', 'vcpkg.json:version-string', 'include/ForsettiCore/ForsettiVersion.h:ForsettiVersion::current')
        products = @('Forsetti::ForsettiCore', 'Forsetti::ForsettiPlatform', 'Forsetti::ForsettiHostTemplate')
        payloadSha256 = $payloadDigest
        toolchain = $metadata
        dependencies = $metadata.dependencies
        consumptionPolicy = [ordered]@{ path = 'CONSUMPTION_POLICY.md'; sha256 = $policyHash; complete = $true }
        verification = $verification
        reproducibility = 'Payload and ZIP comparison is supported for repeated assembly from the same built libraries; independent MSVC rebuild identity is not claimed.'
        files = $files
    }
    Write-SdkText (Join-Path $payload 'sdk-manifest.json') (($manifest | ConvertTo-Json -Depth 20) + "`n")
    Test-SdkPayload $payload | Out-Null
    $archive = Join-Path $outputFull "$payloadName.zip"
    New-SdkArchive $payload $archive
    Write-SdkText (Join-Path $outputFull 'SHA256SUMS') ((Get-SdkSha256 $archive) + "  $payloadName.zip`n")
    [pscustomobject]@{ archive = $archive; checksum = (Join-Path $outputFull 'SHA256SUMS'); payload = $payload; sourceRevision = $SourceRevision; fileCount = $files.Count + 1 } | ConvertTo-Json
} catch {
    Write-Error $_
    exit 1
}
