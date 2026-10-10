# Forsetti Framework for Windows
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-ForsettiIdentifier([string]$Name) {
    $keywords = 'alignas alignof and and_eq asm atomic_cancel atomic_commit atomic_noexcept auto bitand bitor bool break case catch char char8_t char16_t char32_t class compl concept const consteval constexpr constinit const_cast continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected public reflexpr register reinterpret_cast requires return short signed sizeof static static_assert static_cast struct switch synchronized template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq'.Split(' ')
    if ($Name -cnotmatch '^[A-Za-z][A-Za-z0-9_]*$' -or $Name.Contains('__') -or $keywords -ccontains $Name -or
        $Name -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$' -or $Name -match '^ForsettiExample' -or
        $Name -match '^Template(App|UI|Service|SharedDatabase)Module$') {
        throw "Invalid C++/CMake identifier '$Name'. Use a nonreserved ASCII identifier beginning with a letter."
    }
}
function Assert-ForsettiModuleID([string]$ModuleID) {
    if ($ModuleID -cnotmatch '^[A-Za-z0-9]+(?:-[A-Za-z0-9]+)*(?:\.[A-Za-z0-9]+(?:-[A-Za-z0-9]+)*)+$' -or
        $ModuleID -match '^com\.forsetti\.(template|example)(\.|$)' -or $ModuleID -match '^forsetti\.internal\.') {
        throw "Invalid module ID '$ModuleID'. Use your own reverse-domain identity with nonempty alphanumeric/hyphen segments."
    }
}
function Get-ForsettiTemplateRoot {
    $root = Split-Path -Parent $PSScriptRoot
    # Extracted SDK: Tools is directly under the SDK root.
    $templateRoot = Join-Path $root 'templates'
    if (!(Test-Path -LiteralPath $templateRoot -PathType Container)) { throw 'The SDK templates directory is missing.' }
    return $templateRoot
}
function Get-ForsettiDependencyManifest([string]$Name) {
    return ([ordered]@{
        name = ($Name.ToLowerInvariant() -replace '_', '-')
        'version-string' = '0.1.0'
        'builtin-baseline' = 'cb2981c4e03d421fa03b9bb5044cd1986180e7e4'
        dependencies = @([ordered]@{ name = 'nlohmann-json'; 'version>=' = '3.12.0#2' })
    } | ConvertTo-Json -Depth 6) + "`n"
}
function Get-ForsettiModuleFiles([string]$Kind, [string]$Name, [string]$ModuleID) {
    $class = switch ($Kind) { app { 'TemplateAppModule' } ui { 'TemplateUIModule' } service { 'TemplateServiceModule' } 'shared-database' { 'TemplateSharedDatabaseModule' } }
    $source = Join-Path (Get-ForsettiTemplateRoot) "windows-$Kind-module"
    $result = [ordered]@{}
    foreach ($file in Get-ChildItem -LiteralPath $source -Recurse -File) {
        if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Template reparse point is forbidden: $($file.FullName)" }
        $relative = $file.FullName.Substring($source.Length + 1).Replace('\', '/').Replace($class, $Name)
        $text = [IO.File]::ReadAllText($file.FullName)
        $text = $text.Replace($class, $Name).Replace("com.forsetti.template.$Kind", $ModuleID)
        $text = $text.Replace("Template Service Module", $Name).Replace('Template App Module', $Name).Replace('Template UI Module', $Name).Replace('Template Shared Database Module', $Name)
        $text = $text.Replace('template-service-state', "$Name-state").Replace('storage.template-state', "storage.$Name-state").Replace('template-shared-database', "$Name-store").Replace('shared_database.template', "shared_database.$Name")
        $result[$relative] = $text
    }
    if ($Kind -ne 'shared-database') { $result['vcpkg.json'] = Get-ForsettiDependencyManifest $Name }
    return $result
}
function Assert-ForsettiNoDuplicate([string]$ProjectRoot, [string]$ModuleID, [string]$Name) {
    if (!$ProjectRoot) { return }
    if (!(Test-Path -LiteralPath $ProjectRoot -PathType Container)) { throw "Project root does not exist: $ProjectRoot" }
    foreach ($file in Get-ChildItem -LiteralPath $ProjectRoot -Recurse -File -Filter '*.json') {
        if ($file.FullName -match '[\\/](\.git|build|out|vcpkg_installed)[\\/]') { continue }
        try { $manifest = [IO.File]::ReadAllText($file.FullName) | ConvertFrom-Json } catch { continue }
        if ($manifest.PSObject.Properties['moduleID'] -and $manifest.PSObject.Properties['entryPoint']) {
            if ($manifest.moduleID -eq $ModuleID -or $manifest.entryPoint -eq $Name) {
                throw "Duplicate module ID or entry point in $($file.FullName): $ModuleID / $Name"
            }
        }
    }
}
function Write-ForsettiFiles([string]$Destination, [System.Collections.IDictionary]$Files) {
    $absolute = [IO.Path]::GetFullPath($Destination)
    $ancestor = $absolute
    while ($ancestor) {
        if (Test-Path -LiteralPath $ancestor) {
            $item = Get-Item -LiteralPath $ancestor
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Destination reparse points are forbidden: $ancestor" }
        }
        $parent = [IO.Path]::GetDirectoryName($ancestor)
        if ($parent -eq $ancestor) { break }
        $ancestor = $parent
    }
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($relative in $Files.Keys) {
        if (!$seen.Add([string]$relative) -or [IO.Path]::IsPathRooted($relative) -or $relative -match '(^|[\\/])\.\.([\\/]|$)') { throw "Unsafe or colliding scaffold path: $relative" }
        $path = Join-Path $absolute $relative
        if (Test-Path -LiteralPath $path) { throw "Refusing to overwrite existing path: $path" }
        $parent = [IO.Path]::GetDirectoryName($path)
        while ($parent -and $parent.Length -ge $absolute.Length) {
            if (Test-Path -LiteralPath $parent) {
                $item = Get-Item -LiteralPath $parent
                if (!$item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Unsafe scaffold parent: $parent" }
            }
            $parent = [IO.Path]::GetDirectoryName($parent)
        }
    }
    $created = [Collections.Generic.List[string]]::new()
    try {
        foreach ($relative in $Files.Keys) {
            $path = Join-Path $absolute $relative
            [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path))
            # CreateNew also refuses an overwrite if another process creates the file after preflight.
            $stream = [IO.File]::Open($path, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
            try {
                $created.Add($path)
                $bytes = [Text.UTF8Encoding]::new($false).GetBytes([string]$Files[$relative])
                $stream.Write($bytes, 0, $bytes.Length)
            } finally { $stream.Dispose() }
        }
    } catch {
        foreach ($path in $created) { [IO.File]::Delete($path) }
        throw
    }
    Write-Output "Generated $($Files.Count) app-owned files in $absolute"
}
