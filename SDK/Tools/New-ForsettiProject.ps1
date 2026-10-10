# Forsetti Framework for Windows
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Destination,
    [Parameter(Mandatory)][string]$Name,
    [Parameter(Mandatory)][string]$ModuleIDPrefix
)
$ErrorActionPreference = 'Stop'
try {
    . (Join-Path $PSScriptRoot 'Forsetti-Scaffold.ps1')
    Assert-ForsettiIdentifier $Name
    Assert-ForsettiModuleID $ModuleIDPrefix
    $source = Join-Path (Get-ForsettiTemplateRoot) 'windows-host'
    $files = [ordered]@{}
    foreach ($file in Get-ChildItem -LiteralPath $source -Recurse -File) {
        if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Template reparse point is forbidden: $($file.FullName)" }
        $text = [IO.File]::ReadAllText($file.FullName).Replace('TemplateHost', $Name)
        foreach ($kind in @('app','ui','service')) {
            $class = switch ($kind) { app { 'TemplateAppModule' } ui { 'TemplateUIModule' } service { 'TemplateServiceModule' } }
            $suffix = switch ($kind) { app { 'AppModule' } ui { 'UIModule' } service { 'ServiceModule' } }
            $text = $text.Replace($class, "$Name$suffix").Replace("com.forsetti.template.$kind", "$ModuleIDPrefix.$kind")
        }
        $files[$file.FullName.Substring($source.Length + 1).Replace('\', '/')] = $text
    }
    $sources = @('src/Main.cpp','src/NativePresentation.cpp','src/NativePresentation.h','src/ActivationStore.cpp','src/ActivationStore.h')
    $resources = @([ordered]@{ source = 'resources/application.manifest'; shipping = $null })
    $manifests = @()
    $cmakeFiles = @('CMakeLists.txt')
    foreach ($kind in @('app','ui','service')) {
        $suffix = switch ($kind) { app { 'AppModule' } ui { 'UIModule' } service { 'ServiceModule' } }
        $class = "$Name$suffix"
        $id = "$ModuleIDPrefix.$kind"
        if (Test-Path -LiteralPath $Destination -PathType Container) { Assert-ForsettiNoDuplicate $Destination $id $class }
        $moduleFiles = Get-ForsettiModuleFiles $kind $class $id
        foreach ($relative in $moduleFiles.Keys) { $files["Modules/$class/$relative"] = $moduleFiles[$relative] }
        $sources += "Modules/$class/$class.cpp", "Modules/$class/$class.h"
        $cmakeFiles += "Modules/$class/CMakeLists.txt"
        $resources += [ordered]@{ source = "Modules/$class/ForsettiManifests/$class.json"; shipping = "ForsettiManifests/$class.json" }
        $manifests += [ordered]@{ path = "Modules/$class/ForsettiManifests/$class.json"; moduleID = $id; entryPoint = $class; type = $kind; implemented = $true }
    }
    $files['vcpkg.json'] = Get-ForsettiDependencyManifest $Name
    $files['.gitignore'] = "build/`nout/`nvcpkg_installed/`n"
    $files['forsetti-project.json'] = ([ordered]@{
        formatVersion = 1; kind = 'host'; target = $Name
        libraries = @('Forsetti::ForsettiHostTemplate','Forsetti::ForsettiCore')
        sources = $sources; cmakeFiles = $cmakeFiles; resources = $resources; manifests = $manifests
    } | ConvertTo-Json -Depth 8) + "`n"
    Write-ForsettiFiles $Destination $files
} catch { Write-Error $_ -ErrorAction Continue; exit 1 }
