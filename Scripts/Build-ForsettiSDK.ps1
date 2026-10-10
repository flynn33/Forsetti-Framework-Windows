#requires -Version 7.2
# Copyright (c) 2026 James Daley. SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param(
    [string]$OutputRoot,
    [string]$VisualStudioInstance,
    [switch]$AllowDirty
)
. (Join-Path $PSScriptRoot 'SDK/Common.ps1')

try {
    $repoRoot = Split-Path -Parent $PSScriptRoot
    $revision = (& git -C $repoRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Cannot determine source revision.' }
    $status = @(& git -C $repoRoot status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0) { throw 'Cannot determine source status.' }
    $dirty = $status.Count -gt 0
    if ($dirty -and -not $AllowDirty) { throw 'Commit reviewed changes before building a release; -AllowDirty creates an unverified candidate only.' }
    if (-not $OutputRoot) { $OutputRoot = Join-Path $repoRoot 'out/sdk' }
    $outputFull = [IO.Path]::GetFullPath($OutputRoot)
    Assert-SdkOrdinaryPath $outputFull | Out-Null
    # Source snapshot/output must not create new tracked or recursively copied files.
    $relativeOutput = Get-SdkRelativePath $repoRoot $outputFull
    & git -C $repoRoot check-ignore --quiet -- "$relativeOutput/ignore-probe"
    if ($LASTEXITCODE -ne 0) { throw 'SDK build output must be inside an ignored repository directory (normally out/sdk).' }
    if (-not $env:VCPKG_ROOT -or -not (Test-Path -LiteralPath (Join-Path $env:VCPKG_ROOT 'scripts/buildsystems/vcpkg.cmake'))) { throw 'Set VCPKG_ROOT to a working vcpkg installation containing the pinned baseline history.' }
    if (-not $VisualStudioInstance) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        $instances = @((& $vswhere -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json))
        if ($instances.Count -eq 0) { throw 'Visual Studio 2022 with native C++ tools is required.' }
        $VisualStudioInstance = $instances[0].installationPath
    }
    $run = Join-Path $outputFull ('run-' + [guid]::NewGuid().ToString('N'))
    $source = Join-Path $run 'source'
    $build = Join-Path $run 'build'
    $install = Join-Path $run 'install'
    [IO.Directory]::CreateDirectory($source) | Out-Null
    $paths = if ($dirty) { @(& git -C $repoRoot -c core.quotepath=false ls-files --cached --others --exclude-standard) }
             else { @(& git -C $repoRoot -c core.quotepath=false ls-tree -r --name-only $revision) }
    if ($LASTEXITCODE -ne 0) { throw 'Cannot enumerate source files.' }
    $uniquePaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($path in $paths) {
        if (-not (Test-Path -LiteralPath (Join-Path $repoRoot $path))) { continue }
        if (-not $uniquePaths.Add($path)) { throw "Case-colliding source path: $path" }
        if ($dirty) { Copy-SdkFile $repoRoot $path $source $path }
    }
    if (-not $dirty) {
        $tree = @(& git -C $repoRoot ls-tree -r $revision)
        if (@($tree | Where-Object { $_ -match '^(120000|160000) ' }).Count -gt 0) { throw 'Source snapshots cannot contain symlinks or submodule entries.' }
        $snapshotArchive = Join-Path $run 'source-snapshot.zip'
        Invoke-SdkCommand 'git' @('-c', 'core.autocrlf=false', '-C', $repoRoot, 'archive', '--format=zip', "--output=$snapshotArchive", $revision) (Join-Path $run 'source-snapshot.log') | Out-Null
        [IO.Compression.ZipFile]::ExtractToDirectory($snapshotArchive, $source)
        # The policy is a byte-exact Git contract, independent of archive filters.
        Export-SdkGitFile $repoRoot $revision 'CONSUMPTION_POLICY.md' (Join-Path $source 'CONSUMPTION_POLICY.md')
        Get-SdkTreeFiles $source | Out-Null
    }
    $logs = Join-Path $run 'logs'
    $configure = @('-S', $source, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64',
        "-DCMAKE_GENERATOR_INSTANCE=$VisualStudioInstance", "-DCMAKE_TOOLCHAIN_FILE=$($env:VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake",
        '-DVCPKG_TARGET_TRIPLET=x64-windows', '-DFORSETTI_INSTALL_SDK=ON', '-DFORSETTI_BUILD_HOST_TEMPLATE=ON',
        '-DFORSETTI_BUILD_EXAMPLES=OFF', '-DFORSETTI_BUILD_SAMPLES=OFF', '-DBUILD_TESTING=OFF', '-DCMAKE_CONFIGURATION_TYPES=Debug;Release')
    Invoke-SdkCommand 'cmake' $configure (Join-Path $logs 'configure.log') | Out-Null
    foreach ($configuration in @('Debug', 'Release')) {
        Write-Host "Building and installing $configuration SDK products..."
        Invoke-SdkCommand 'cmake' @('--build', $build, '--config', $configuration, '--parallel', '8') (Join-Path $logs "build-$configuration.log") | Out-Null
        Invoke-SdkCommand 'cmake' @('--install', $build, '--config', $configuration, '--component', 'ForsettiSdk', '--prefix', $install) (Join-Path $logs "install-$configuration.log") | Out-Null
    }
    $finalStatus = @(& git -C $repoRoot status --porcelain --untracked-files=all)
    $finalRevision = (& git -C $repoRoot rev-parse HEAD).Trim()
    if ($finalRevision -ne $revision) { throw 'Source revision changed during SDK build. Retry from a stable commit.' }
    if (($status -join "`n") -ne ($finalStatus -join "`n")) { throw 'Source status changed during SDK build. Retry from a stable snapshot.' }
    $packageArguments = @{ SourceRoot = $source; InstallRoot = $install; BuildRoot = $build; SourceRevision = $revision; OutputDirectory = (Join-Path $run 'package') }
    if ($dirty) { $packageArguments.DirtyCandidate = $true }
    $package = & (Join-Path $PSScriptRoot 'Package-ForsettiSDK.ps1') @packageArguments | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0) { throw 'SDK assembly failed.' }
    $record = [ordered]@{ sourceRevision = $revision; dirtyCandidate = $dirty; sourceRoot = $source; buildRoot = $build; installRoot = $install; visualStudioInstance = $VisualStudioInstance; archive = $package.archive; checksum = $package.checksum; payload = $package.payload; logs = $logs }
    Write-SdkText (Join-Path $run 'build-record.json') (($record | ConvertTo-Json -Depth 8) + "`n")
    Write-SdkText (Join-Path $outputFull 'latest-build.json') (($record | ConvertTo-Json -Depth 8) + "`n")
    $record | ConvertTo-Json -Depth 8
} catch {
    Write-Error $_
    exit 1
}
