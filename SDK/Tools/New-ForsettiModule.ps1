# Forsetti Framework for Windows
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Destination,
    [Parameter(Mandatory)][ValidateSet('app','ui','service','shared-database')][string]$Kind,
    [Parameter(Mandatory)][string]$Name,
    [Parameter(Mandatory)][string]$ModuleID,
    [string]$ProjectRoot
)
$ErrorActionPreference = 'Stop'
try {
    $Kind = $Kind.ToLowerInvariant()
    . (Join-Path $PSScriptRoot 'Forsetti-Scaffold.ps1')
    Assert-ForsettiIdentifier $Name
    Assert-ForsettiModuleID $ModuleID
    Assert-ForsettiNoDuplicate $ProjectRoot $ModuleID $Name
    $files = Get-ForsettiModuleFiles $Kind $Name $ModuleID
    $implemented = $Kind -ne 'shared-database'
    $files['forsetti-project.json'] = ([ordered]@{
        formatVersion = 1; kind = $Kind; target = $Name
        libraries = @(if ($implemented) { 'Forsetti::ForsettiCore' })
        sources = @(if ($implemented) { "$Name.cpp"; "$Name.h" })
        cmakeFiles = @(if ($implemented) { 'CMakeLists.txt' })
        resources = @([ordered]@{ source = "ForsettiManifests/$Name.json"; shipping = "ForsettiManifests/$Name.json" })
        manifests = @([ordered]@{ path = "ForsettiManifests/$Name.json"; moduleID = $ModuleID; entryPoint = $Name; type = $(if ($Kind -eq 'shared-database') { 'service' } else { $Kind }); implemented = $implemented })
    } | ConvertTo-Json -Depth 8) + "`n"
    Write-ForsettiFiles $Destination $files
    if (!$implemented) { Write-Output 'Manifest-only starter: provide and verify an app-owned database service implementation before activation.' }
} catch { Write-Error $_ -ErrorAction Continue; exit 1 }
