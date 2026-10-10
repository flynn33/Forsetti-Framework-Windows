# Copyright (c) 2026 James Daley. SPDX-License-Identifier: Apache-2.0
# Maintainer-only helpers. These files are never included in the SDK.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-SdkOrdinaryPath {
    param([string]$Path)
    $full = [IO.Path]::GetFullPath($Path)
    $current = $full
    while ($current) {
        if (Test-Path -LiteralPath $current) {
            $item = Get-Item -LiteralPath $current -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse-point path is forbidden: $current" }
        }
        $parent = [IO.Path]::GetDirectoryName($current.TrimEnd('\', '/'))
        if (-not $parent -or $parent -eq $current) { break }
        $current = $parent
    }
    return $full
}

function Write-SdkText {
    param([string]$Path, [string]$Text)
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path)) | Out-Null
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}

function Assert-SdkRelativePath {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path) -or $Path.Contains('\') -or
        $Path.Contains(':') -or $Path.StartsWith('/') -or
        @($Path.Split('/') | Where-Object { $_ -in @('', '.', '..') }).Count -gt 0) {
        throw "Unsafe payload path: $Path"
    }
}

function Get-SdkTreeFiles {
    param([string]$Root)
    Assert-SdkOrdinaryPath $Root | Out-Null
    $rootItem = Get-Item -LiteralPath $Root -Force
    if (-not $rootItem.PSIsContainer -or ($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Expected an ordinary directory: $Root"
    }
    $pending = [Collections.Generic.Stack[string]]::new()
    $pending.Push($rootItem.FullName)
    while ($pending.Count -gt 0) {
        foreach ($item in Get-ChildItem -LiteralPath $pending.Pop() -Force) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Symlinks and junctions are not permitted: $($item.FullName)"
            }
            if ($item.PSIsContainer) { $pending.Push($item.FullName) }
            else { $item }
        }
    }
}

function Get-SdkRelativePath {
    param([string]$Root, [string]$Path)
    $prefix = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $fullPath = [IO.Path]::GetFullPath($Path)
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside its allowed root: $fullPath"
    }
    return $fullPath.Substring($prefix.Length).Replace('\', '/')
}

function Get-SdkSha256 {
    param([string]$Path)
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Copy-SdkFile {
    param([string]$SourceRoot, [string]$SourcePath, [string]$DestinationRoot, [string]$DestinationPath)
    Assert-SdkRelativePath $SourcePath
    Assert-SdkRelativePath $DestinationPath
    Assert-SdkOrdinaryPath $SourceRoot | Out-Null
    Assert-SdkOrdinaryPath $DestinationRoot | Out-Null
    $source = Join-Path $SourceRoot $SourcePath
    $item = Get-Item -LiteralPath $source -Force
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Not an ordinary file: $source"
    }
    # Check every ancestor as well as the file, before following it.
    $parent = $item.Directory
    $rootFull = [IO.Path]::GetFullPath($SourceRoot).TrimEnd('\', '/')
    while ($null -ne $parent) {
        if ($parent.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse-point ancestor: $parent" }
        if ($parent.FullName.TrimEnd('\', '/') -eq $rootFull) { break }
        $parent = $parent.Parent
    }
    Get-SdkRelativePath $SourceRoot $source | Out-Null
    $destination = Join-Path $DestinationRoot $DestinationPath
    Assert-SdkOrdinaryPath $destination | Out-Null
    if (Test-Path -LiteralPath $destination) { throw "Duplicate destination: $DestinationPath" }
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
    [IO.File]::Copy($source, $destination)
}

function Export-SdkGitFile {
    param([string]$RepoRoot, [string]$Revision, [string]$Path, [string]$Destination)
    Assert-SdkRelativePath $Path
    $info = [Diagnostics.ProcessStartInfo]::new('git')
    $info.WorkingDirectory = $RepoRoot
    $info.ArgumentList.Add('show')
    $info.ArgumentList.Add("${Revision}:$Path")
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.UseShellExecute = $false
    $process = [Diagnostics.Process]::Start($info)
    $errors = $process.StandardError.ReadToEndAsync()
    $stream = [IO.File]::Create($Destination)
    try { $process.StandardOutput.BaseStream.CopyTo($stream) } finally { $stream.Dispose() }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "Cannot read committed $Path : $($errors.GetAwaiter().GetResult())" }
    $process.Dispose()
}

function Invoke-SdkCommand {
    param([string]$Command, [string[]]$Arguments, [string]$LogPath, [switch]$ExpectFailure)
    $result = & $Command @Arguments 2>&1
    $code = $LASTEXITCODE
    $text = $result | Out-String
    if ($LogPath) { Write-SdkText $LogPath $text }
    if ($ExpectFailure) {
        if ($code -eq 0) { throw "Negative check unexpectedly succeeded: $Command $($Arguments -join ' ')" }
    } elseif ($code -ne 0) {
        throw "Command failed ($code): $Command $($Arguments -join ' ')`n$text"
    }
    return [pscustomobject]@{ command = $Command; arguments = $Arguments; exitCode = $code; log = $LogPath }
}

function Test-SdkPayload {
    param([string]$Root)
    $manifest = Get-Content -LiteralPath (Join-Path $Root 'sdk-manifest.json') -Raw | ConvertFrom-Json
    if ($manifest.schemaVersion -ne '1.0' -or $manifest.architecture -ne 'x64') { throw 'Unsupported SDK manifest.' }
    $expected = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $manifest.files) {
        Assert-SdkRelativePath $entry.path
        if ($entry.path -eq 'sdk-manifest.json' -or $expected.ContainsKey($entry.path)) { throw "Invalid/duplicate manifest entry: $($entry.path)" }
        $expected.Add($entry.path, $entry)
    }
    $actual = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($file in Get-SdkTreeFiles $Root) {
        $path = Get-SdkRelativePath $Root $file.FullName
        if (-not $actual.Add($path)) { throw "Case-colliding entry: $path" }
        if ($path -eq 'sdk-manifest.json') { continue }
        if (-not $expected.ContainsKey($path)) { throw "Unexpected SDK file: $path" }
        $entry = $expected[$path]
        if ($path -cne $entry.path) { throw "Case does not match manifest: $path" }
        if ($file.Length -ne $entry.bytes -or (Get-SdkSha256 $file.FullName) -ne $entry.sha256) { throw "SDK file tampered: $path" }
    }
    foreach ($path in $expected.Keys) { if (-not $actual.Contains($path)) { throw "Missing SDK file: $path" } }
    if ((Get-SdkPayloadDigest $manifest.files) -ne $manifest.payloadSha256) { throw 'Payload inventory digest mismatch.' }
    if ($manifest.consumptionPolicy.path -cne 'CONSUMPTION_POLICY.md' -or -not $manifest.consumptionPolicy.complete -or
        -not $expected.ContainsKey('CONSUMPTION_POLICY.md') -or
        $expected['CONSUMPTION_POLICY.md'].sha256 -ne $manifest.consumptionPolicy.sha256 -or
        (Get-SdkSha256 (Join-Path $Root 'CONSUMPTION_POLICY.md')) -ne $manifest.consumptionPolicy.sha256) { throw 'Complete canonical consumption policy mismatch.' }
    $libraries = @('ForsettiCore', 'ForsettiPlatform', 'ForsettiHostTemplate')
    foreach ($configuration in @('Debug', 'Release')) {
        foreach ($library in $libraries) {
            if (-not $expected.ContainsKey("lib/$configuration/$library.lib")) { throw "Missing $configuration product: $library" }
        }
    }
    foreach ($required in @('README.md', 'AGENTS.md', 'CONSUMPTION_POLICY.md', 'LICENSE', 'THIRD-PARTY-NOTICES.md', 'Release-Notes.md',
        'Documentation/Getting-Started.md', 'Documentation/Public-API.md', 'Documentation/Module-Authoring.md',
        'Documentation/Compatibility.md', 'Documentation/Source-References.md', 'Tools/New-ForsettiProject.ps1',
        'Tools/New-ForsettiModule.ps1', 'Tools/Test-ForsettiIntegration.ps1')) {
        if (-not $expected.ContainsKey($required)) { throw "Required SDK file missing: $required" }
    }
    return $manifest
}

function Get-SdkPayloadDigest {
    param([object[]]$Files)
    $paths = [string[]]@($Files.path)
    [Array]::Sort($paths, [StringComparer]::Ordinal)
    $entries = @{}
    foreach ($file in $Files) { $entries[$file.path] = $file }
    $text = [Text.StringBuilder]::new()
    foreach ($path in $paths) {
        $file = $entries[$path]
        $text.Append($path).Append([char]0).Append([string]$file.bytes).Append([char]0).Append($file.sha256).Append("`n") | Out-Null
    }
    $bytes = [Text.Encoding]::UTF8.GetBytes($text.ToString())
    return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
}

function Expand-SdkArchive {
    param([string]$Archive, [string]$Destination)
    Add-Type -AssemblyName System.IO.Compression
    $zip = [IO.Compression.ZipFile]::OpenRead($Archive)
    try {
        $names = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach ($entry in $zip.Entries) {
            $name = $entry.FullName.TrimEnd('/')
            Assert-SdkRelativePath $name
            if (-not $names.Add($name)) { throw "Case-colliding ZIP entry: $name" }
            $unixType = ($entry.ExternalAttributes -shr 16) -band 0xF000
            if ($unixType -eq 0xA000 -or ($entry.ExternalAttributes -band [int][IO.FileAttributes]::ReparsePoint)) { throw "Link entry in ZIP: $name" }
        }
    } finally { $zip.Dispose() }
    if (Test-Path -LiteralPath $Destination) { throw "Extraction destination already exists: $Destination" }
    Assert-SdkOrdinaryPath $Destination | Out-Null
    [IO.Compression.ZipFile]::ExtractToDirectory($Archive, $Destination)
    $roots = @(Get-ChildItem -LiteralPath $Destination -Force)
    if ($roots.Count -ne 1 -or -not $roots[0].PSIsContainer) { throw 'SDK archive must contain one top-level directory.' }
    Test-SdkPayload $roots[0].FullName | Out-Null
    return $roots[0].FullName
}

function New-SdkArchive {
    param([string]$Root, [string]$Archive)
    if (Test-Path -LiteralPath $Archive) { throw "Archive already exists: $Archive" }
    Add-Type -AssemblyName System.IO.Compression
    $stream = [IO.File]::Create($Archive)
    $zip = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create)
    try {
        $folderName = [IO.Path]::GetFileName($Root)
        $paths = [string[]]@(Get-SdkTreeFiles $Root | ForEach-Object { Get-SdkRelativePath $Root $_.FullName })
        [Array]::Sort($paths, [StringComparer]::Ordinal)
        foreach ($path in $paths) {
            $entry = $zip.CreateEntry("$folderName/$path", [IO.Compression.CompressionLevel]::Optimal)
            $entry.LastWriteTime = [DateTimeOffset]::new(2000, 1, 1, 0, 0, 0, [TimeSpan]::Zero)
            $inputStream = [IO.File]::OpenRead((Join-Path $Root $path))
            $outputStream = $entry.Open()
            try { $inputStream.CopyTo($outputStream) } finally { $inputStream.Dispose(); $outputStream.Dispose() }
        }
    } finally { $zip.Dispose(); $stream.Dispose() }
}
