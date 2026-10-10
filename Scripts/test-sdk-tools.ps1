# Forsetti Framework for Windows
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param([string]$PowerShellExe = 'pwsh', [switch]$KeepFixtures)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = Split-Path -Parent $PSScriptRoot
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('forsetti-sdk-tools-' + [guid]::NewGuid().ToString('N'))
$sdk = Join-Path $testRoot 'extracted SDK'
$consumer = Join-Path $testRoot 'consumer space 日本語'
$passed = 0
function Invoke-Tool([string]$Tool, [string[]]$Arguments, [bool]$ExpectedPass, [string]$RequiredOutput = '') {
    $output = & $PowerShellExe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $sdk "Tools/$Tool") @Arguments 2>&1 | Out-String
    $code = $LASTEXITCODE
    if (($code -eq 0) -ne $ExpectedPass -or ($RequiredOutput -and $output -notmatch $RequiredOutput)) {
        throw "$Tool expected success=$ExpectedPass, exit=$code. Required '$RequiredOutput'. $output"
    }
    $script:passed++
}
function Check([string]$Path, [bool]$ExpectedPass, [string]$Pattern = '', [string]$Shipping = '') {
    $arguments = @('-ProjectRoot', $Path, '-Json')
    if ($Shipping) { $arguments += '-ShippingRoot', $Shipping }
    Invoke-Tool 'Test-ForsettiIntegration.ps1' $arguments $ExpectedPass $Pattern
}
function Write-Utf8([string]$Path, [string]$Text) { [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false)) }
try {
    [void][IO.Directory]::CreateDirectory($sdk)
    Copy-Item -LiteralPath (Join-Path $repoRoot 'SDK/Tools') -Destination $sdk -Recurse
    Copy-Item -LiteralPath (Join-Path $repoRoot 'templates') -Destination $sdk -Recurse
    Push-Location ([IO.Path]::GetTempPath())
    try {
        Invoke-Tool 'New-ForsettiProject.ps1' @('-Destination',$consumer,'-Name','IndependentHost','-ModuleIDPrefix','com.test.independent') $true
        Check $consumer $true
        $originalMain = [IO.File]::ReadAllText((Join-Path $consumer 'src/Main.cpp'))
        if ($originalMain -match 'com\.forsetti\.template|TemplateHost|TemplateAppModule|TemplateUIModule|TemplateServiceModule') { throw 'Project identity replacement is incomplete.' }
        $coreOnly = Join-Path $testRoot 'core only'
        Invoke-Tool 'New-ForsettiModule.ps1' @('-Destination',$coreOnly,'-Kind','service','-Name','InventoryService','-ModuleID','com.test.inventory','-ProjectRoot',$consumer) $true
        Check $coreOnly $true
        $uppercase = Join-Path $testRoot 'uppercase module kind'
        Invoke-Tool 'New-ForsettiModule.ps1' @('-Destination',$uppercase,'-Kind','APP','-Name','UppercaseApp','-ModuleID','com.test.uppercase') $true
        Check $uppercase $true
        $database = Join-Path $testRoot 'database manifest'
        Invoke-Tool 'New-ForsettiModule.ps1' @('-Destination',$database,'-Kind','shared-database','-Name','SharedDatabase','-ModuleID','com.test.database') $true 'Manifest-only'
        Check $database $true
        foreach ($invalid in @('class','bad-name','_Reserved','CON','Name__Reserved')) {
            Invoke-Tool 'New-ForsettiModule.ps1' @('-Destination',(Join-Path $testRoot 'invalid'),'-Kind','service','-Name',$invalid,'-ModuleID','com.test.invalid') $false 'Invalid C\+\+/CMake'
        }
        foreach ($invalidID in @('com..bad','com.bad-','one','com.forsetti.template.bad')) {
            Invoke-Tool 'New-ForsettiModule.ps1' @('-Destination',(Join-Path $testRoot 'invalid'),'-Kind','service','-Name','ValidName','-ModuleID',$invalidID) $false
        }
        Invoke-Tool 'New-ForsettiModule.ps1' @('-Destination',(Join-Path $testRoot 'duplicate'),'-Kind','app','-Name','NewName','-ModuleID','com.test.independent.app','-ProjectRoot',$consumer) $false 'Duplicate'
        $before = (Get-FileHash -LiteralPath (Join-Path $consumer 'src/Main.cpp')).Hash
        Invoke-Tool 'New-ForsettiProject.ps1' @('-Destination',$consumer,'-Name','IndependentHost','-ModuleIDPrefix','com.test.independent') $false 'Duplicate|overwrite'
        if ((Get-FileHash -LiteralPath (Join-Path $consumer 'src/Main.cpp')).Hash -ne $before) { throw 'Overwrite refusal changed a preexisting file.' }
        $unrelated = Join-Path $testRoot 'existing directory'
        [void][IO.Directory]::CreateDirectory($unrelated)
        Write-Utf8 (Join-Path $unrelated 'personal.txt') 'preserve me'
        Invoke-Tool 'New-ForsettiModule.ps1' @('-Destination',$unrelated,'-Kind','ui','-Name','PersonalView','-ModuleID','com.test.personal') $true
        if ([IO.File]::ReadAllText((Join-Path $unrelated 'personal.txt')) -ne 'preserve me') { throw 'Unrelated file was changed.' }
        $renamed = Join-Path $consumer 'application_utilities.cpp'
        Write-Utf8 $renamed ([IO.File]::ReadAllText((Join-Path $repoRoot 'src/ForsettiCore/ForsettiRuntime.cpp')))
        Check $consumer $false 'Framework-owned implementation'
        [IO.File]::Delete($renamed)
        $rootCmakePath = Join-Path $consumer 'CMakeLists.txt'
        $rootCmake = [IO.File]::ReadAllText($rootCmakePath)
        $inventoryPath = Join-Path $consumer 'forsetti-project.json'
        $inventoryText = [IO.File]::ReadAllText($inventoryPath)
        Write-Utf8 (Join-Path $consumer 'src/Extra source 日本語.cpp') '#include "Extra header 日本語.h"'
        Write-Utf8 (Join-Path $consumer 'src/Extra header 日本語.h') '#pragma once'
        Write-Utf8 (Join-Path $consumer 'resources/Extra data 日本語.json') '{"appOwned":true}'
        Write-Utf8 $rootCmakePath ($rootCmake + [Environment]::NewLine + 'target_sources(IndependentHost PRIVATE "src/Extra source 日本語.cpp" "src/Extra header 日本語.h" "resources/Extra data 日本語.json")' + [Environment]::NewLine + 'set_source_files_properties("resources/Extra data 日本語.json" PROPERTIES HEADER_FILE_ONLY TRUE)' + [Environment]::NewLine)
        $inventory = $inventoryText | ConvertFrom-Json
        $inventory.sources += 'src/Extra source 日本語.cpp', 'src/Extra header 日本語.h'
        $inventory.resources += [pscustomobject]@{ source = 'resources/Extra data 日本語.json'; shipping = $null }
        Write-Utf8 $inventoryPath ($inventory | ConvertTo-Json -Depth 8)
        Check $consumer $true
        Write-Utf8 $inventoryPath $inventoryText
        Write-Utf8 $rootCmakePath $rootCmake
        [IO.File]::Delete((Join-Path $consumer 'src/Extra source 日本語.cpp'))
        [IO.File]::Delete((Join-Path $consumer 'src/Extra header 日本語.h'))
        [IO.File]::Delete((Join-Path $consumer 'resources/Extra data 日本語.json'))
        Write-Utf8 $rootCmakePath ($rootCmake.Replace('src/NativePresentation.cpp',''))
        Check $consumer $false 'CMake target membership'
        Write-Utf8 $rootCmakePath ($rootCmake + "`nadd_library(ForsettiCore STATIC app_owned.cpp)`n")
        Check $consumer $false 'Forbidden framework source'
        Write-Utf8 $rootCmakePath ($rootCmake + [Environment]::NewLine + 'set(CMAKE_PREFIX_PATH "C:/external SDK cache/lib/cmake")' + [Environment]::NewLine)
        Check $consumer $true
        Write-Utf8 $rootCmakePath ($rootCmake + [Environment]::NewLine + 'target_include_directories(IndependentHost PRIVATE "C:/external SDK cache/include/ForsettiCore")' + [Environment]::NewLine + 'target_link_libraries(IndependentHost PRIVATE "C:/external SDK cache/lib/Release/ForsettiCore.lib")' + [Environment]::NewLine)
        Check $consumer $true
        Write-Utf8 $rootCmakePath ($rootCmake + [Environment]::NewLine + 'target_include_directories(IndependentHost PRIVATE "vendored/include/ForsettiCore")' + [Environment]::NewLine)
        Check $consumer $false 'public headers transplanted'
        Write-Utf8 $rootCmakePath $rootCmake
        Write-Utf8 (Join-Path $consumer 'src/Main.cpp') ($originalMain.Replace('registerModule("IndependentHostAppModule"','registerModule("WrongEntry"'))
        Check $consumer $false 'Host registry factory is absent'
        Write-Utf8 (Join-Path $consumer 'src/Main.cpp') $originalMain
        $dependencyPath = Join-Path $consumer 'vcpkg.json'
        $dependency = [IO.File]::ReadAllText($dependencyPath)
        Write-Utf8 $dependencyPath ($dependency.Replace('nlohmann-json','other-json'))
        Check $consumer $false 'nlohmann-json'
        Write-Utf8 $dependencyPath $dependency
        $manifestPath = Join-Path $consumer 'Modules/IndependentHostAppModule/ForsettiManifests/IndependentHostAppModule.json'
        $manifest = [IO.File]::ReadAllText($manifestPath)
        Write-Utf8 $manifestPath ($manifest.Replace('"Windows"','"windows"'))
        Check $consumer $false 'platform'
        Write-Utf8 $manifestPath $manifest
        $shipping = Join-Path $testRoot 'shipping'
        [void][IO.Directory]::CreateDirectory((Join-Path $shipping 'ForsettiManifests'))
        foreach ($file in Get-ChildItem -LiteralPath (Join-Path $consumer 'Modules') -Recurse -File -Filter '*.json' | Where-Object { $_.Directory.Name -eq 'ForsettiManifests' }) {
            Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $shipping 'ForsettiManifests')
        }
        Check $consumer $true '' $shipping
        Write-Utf8 (Join-Path $shipping 'ForsettiManifests/IndependentHostAppModule.json') ($manifest.Replace('"patch": 0','"patch": 1'))
        Check $consumer $false 'differs from' $shipping
        Write-Utf8 (Join-Path $shipping 'ForsettiManifests/IndependentHostAppModule.json') $manifest
        Write-Utf8 (Join-Path $shipping 'runtime.cpp') '// copied shipping source'
        Check $consumer $false 'shipping payload' $shipping
        [IO.File]::Delete((Join-Path $shipping 'runtime.cpp'))
        & git -C $consumer init --quiet
        if ($LASTEXITCODE -ne 0) { throw 'Cannot initialize tracked fixture.' }
        [void][IO.Directory]::CreateDirectory((Join-Path $consumer 'build/renamed'))
        Write-Utf8 (Join-Path $consumer 'build/renamed/internal.cpp') ([IO.File]::ReadAllText((Join-Path $repoRoot 'src/ForsettiCore/ForsettiRuntime.cpp')))
        & git -C $consumer add -f build/renamed/internal.cpp
        if ($LASTEXITCODE -ne 0) { throw 'Cannot track renamed forbidden fixture.' }
        Check $consumer $false 'Framework-owned implementation'
        & git -C $consumer rm -f --quiet build/renamed/internal.cpp
        Check $consumer $true
    } finally { Pop-Location }
    Write-Output "PASS: $passed SDK tool fixture checks using $PowerShellExe."
    if ($KeepFixtures) { Write-Output "Fixtures retained: $testRoot" }
} finally {
    if (!$KeepFixtures -and (Test-Path -LiteralPath $testRoot)) {
        $resolved = [IO.Path]::GetFullPath($testRoot)
        $temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
        if (!$resolved.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notlike 'forsetti-sdk-tools-*') { throw 'Refusing unsafe test fixture cleanup.' }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
