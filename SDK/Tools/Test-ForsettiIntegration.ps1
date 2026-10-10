# Forsetti Framework for Windows
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProjectRoot,
    [string]$ShippingRoot,
    [switch]$Json
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
try {
    $root = (Resolve-Path -LiteralPath $ProjectRoot).Path
    $issues = [Collections.Generic.List[string]]::new()
    $metadataPath = Join-Path $root 'forsetti-project.json'
    if (!(Test-Path -LiteralPath $metadataPath -PathType Leaf)) { throw 'Missing forsetti-project.json. Declare app-owned source, CMake, manifest and shipping resource membership before verification.' }
    $metadata = [IO.File]::ReadAllText($metadataPath) | ConvertFrom-Json
    if ($metadata.formatVersion -ne 1) { throw 'Unsupported integration inventory formatVersion.' }
    function Local-Path([string]$Relative) {
        if (!$Relative -or [IO.Path]::IsPathRooted($Relative) -or $Relative -match '(^|[\\/])\.\.([\\/]|$)') { throw "Inventory path must stay inside application: $Relative" }
        return Join-Path $root $Relative
    }
    $sourceEntries = @(Get-ChildItem -LiteralPath $root -Recurse | Where-Object {
        $_.FullName -notmatch '[\\/](\.git|build|out|vcpkg_installed)[\\/]'
    })
    foreach ($entry in $sourceEntries) {
        if (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { $issues.Add("Reparse point in application source: $($entry.FullName)") }
    }
    $allFiles = @($sourceEntries | Where-Object { $_ -is [IO.FileInfo] })
    # Git's tracked files are always included, even if they have a normally ignored directory name.
    if ((Get-Command git -ErrorAction SilentlyContinue) -and (Test-Path -LiteralPath (Join-Path $root '.git'))) {
        $tracked = @(& git -c core.quotepath=false -C $root ls-files)
        if ($LASTEXITCODE -ne 0) { throw 'git ls-files failed.' }
        foreach ($relative in $tracked) {
            $path = Local-Path $relative
            if (Test-Path -LiteralPath $path -PathType Leaf) { $allFiles += Get-Item -LiteralPath $path }
            else { $issues.Add("Tracked file is missing: $relative") }
        }
    }
    $allFiles = @($allFiles | Sort-Object FullName -Unique)
    foreach ($file in $allFiles) {
        $ancestor = $file
        while ($ancestor -and $ancestor.FullName.Length -ge $root.Length) {
            if (($ancestor.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { $issues.Add("Reparse point in application source: $($ancestor.FullName)"); break }
            $ancestor = if ($ancestor -is [IO.DirectoryInfo]) { $ancestor.Parent } else { $ancestor.Directory }
        }
    }
    $cmakeFiles = @($allFiles | Where-Object { $_.Name -eq 'CMakeLists.txt' -or $_.Extension -eq '.cmake' })
    $cmakeAll = ''
    $membership = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($file in $cmakeFiles) {
        $content = [IO.File]::ReadAllText($file.FullName) -replace '(?m)#.*$', ''
        $cmakeAll += "`n$content"
        if ($content -match '(?is)\b(FetchContent_Declare|ExternalProject_Add)\s*\([^)]*(Forsetti-Framework|github\.com/flynn33/Forsetti)' -or
            $content -match '(?is)\badd_(library|executable)\s*\(\s*(ForsettiCore|ForsettiPlatform|ForsettiHostTemplate|ForsettiExample\w*|ForsettiDemo)\b' -or
            $content -match '(?is)\badd_subdirectory\s*\([^)]*(Forsetti-Framework|src[/\\]Forsetti(Core|Platform|HostTemplate))' -or
            $content -match '(?is)\b(target_include_directories|include_directories)\s*\([^)]*src[/\\]Forsetti' -or
            $content -match '(?is)\btarget_link_libraries\s*\([^)]*(?:(?<!::)\b(ForsettiCore|ForsettiPlatform|ForsettiHostTemplate)\b(?!\.lib)|\b(ForsettiExample\w*|ForsettiDemo)\b)') {
            $issues.Add("Forbidden framework source target/include/dependency integration: $($file.FullName)")
        }
        foreach ($include in [regex]::Matches($content, '(?is)\b(target_include_directories|include_directories)\s*\((?<body>[^)]*)\)')) {
            $body = $include.Groups['body'].Value.Replace('${CMAKE_CURRENT_SOURCE_DIR}', $file.DirectoryName).Replace('${CMAKE_SOURCE_DIR}', $root).Replace('${PROJECT_SOURCE_DIR}', $root)
            # Quoted paths retain spaces. Unknown variables require native CMake review; documented detection limits apply.
            foreach ($tokenMatch in [regex]::Matches($body, '"(?<quoted>[^"]+)"|(?<bare>[^\s;"\r\n]+)')) {
                $token = if ($tokenMatch.Groups['quoted'].Success) { $tokenMatch.Groups['quoted'].Value } else { $tokenMatch.Groups['bare'].Value }
                if ($token -match 'include[/\\]Forsetti(Core|Platform|HostTemplate)' -and $token -notmatch '\$') {
                    $path = if ([IO.Path]::IsPathRooted($token)) { [IO.Path]::GetFullPath($token) } else { [IO.Path]::GetFullPath((Join-Path $file.DirectoryName $token)) }
                    if ($path.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { $issues.Add("SDK-owned public headers transplanted into application include path: $token") }
                }
            }
        }
        $variables = @{}
        foreach ($match in [regex]::Matches($content, '(?is)\bset\s*\(\s*(?<name>[A-Za-z_][A-Za-z0-9_]*)\s+(?<value>[^)]*)\)')) { $variables[$match.Groups['name'].Value] = $match.Groups['value'].Value }
        foreach ($match in [regex]::Matches($content, '(?is)\b(add_library|add_executable|target_sources)\s*\((?<body>[^)]*)\)')) {
            $body = $match.Groups['body'].Value
            for ($pass = 0; $pass -lt 5; ++$pass) {
                foreach ($variable in $variables.Keys) { $body = $body.Replace('${' + $variable + '}', $variables[$variable]) }
            }
            foreach ($tokenMatch in [regex]::Matches($body, '"(?<quoted>[^"]+)"|(?<bare>[^\s;"\r\n]+)')) {
                $token = if ($tokenMatch.Groups['quoted'].Success) { $tokenMatch.Groups['quoted'].Value } else { $tokenMatch.Groups['bare'].Value }
                if ($token -match '\.(cpp|cc|cxx|h|hpp|json|manifest|rc)$' -and $token -notmatch '\$') {
                    $path = [IO.Path]::GetFullPath((Join-Path $file.DirectoryName $token))
                    if ($path.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { [void]$membership.Add($path) }
                    else { $issues.Add("CMake source/resource escapes application source: $token") }
                }
            }
        }
    }
    $implemented = @($metadata.manifests | Where-Object { $_.implemented })
    if ($implemented.Count -gt 0) {
        if ($cmakeAll -notmatch '(?is)\bfind_package\s*\(\s*ForsettiFramework\s+CONFIG\s+REQUIRED\s*\)') { $issues.Add('Installed find_package(ForsettiFramework CONFIG REQUIRED) dependency is missing.') }
        foreach ($library in $metadata.libraries) {
            if ($library -cnotin @('Forsetti::ForsettiCore','Forsetti::ForsettiPlatform','Forsetti::ForsettiHostTemplate') -or
                $cmakeAll -notmatch ('(?is)\btarget_link_libraries\s*\([^)]*' + [regex]::Escape($library) + '\b')) {
                $issues.Add("Required installed target is not linked: $library")
            }
        }
        $dependency = Join-Path $root 'vcpkg.json'
        if (!(Test-Path -LiteralPath $dependency)) { $issues.Add('Missing app-owned vcpkg.json dependency recipe for nlohmann-json.') }
        else {
            $recipe = [IO.File]::ReadAllText($dependency) | ConvertFrom-Json
            if (!$recipe.PSObject.Properties['builtin-baseline'] -or !$recipe.'builtin-baseline' -or
                !(@($recipe.dependencies | ForEach-Object { if ($_ -is [string]) { $_ } else { $_.name } }) -ccontains 'nlohmann-json')) {
                $issues.Add('Dependency recipe must declare a baseline and nlohmann-json.')
            }
        }
    }
    foreach ($relative in @($metadata.sources) + @($metadata.cmakeFiles)) {
        if (!(Test-Path -LiteralPath (Local-Path $relative) -PathType Leaf)) { $issues.Add("Declared app-owned file is missing: $relative") }
    }
    foreach ($relative in $metadata.sources) {
        if (!$membership.Contains((Local-Path $relative))) { $issues.Add("Source/header is absent from explicit CMake target membership: $relative") }
    }
    $declared = @($metadata.sources | ForEach-Object { (Local-Path $_).ToLowerInvariant() })
    foreach ($file in $allFiles) {
        if ($file.Extension -match '^\.(cpp|cc|cxx|h|hpp)$') {
            $content = [IO.File]::ReadAllText($file.FullName)
            if ($content -match '(?m)^[ \t]*(?:(?:const|static|inline|virtual)\s+)*(?:[\w:<>*&]+\s+)?(?:Forsetti::)?(ForsettiRuntime|ModuleManager|ManifestLoader|CompatibilityChecker|ForsettiHostBootstrap|ForsettiHostController|ForsettiHostOverlayRouter|ForsettiSurfaceStateAdapter|ServiceContainer|CapabilityScopedServiceProvider|UISurfaceManager|WinHttpNetworkingService|RegistryStorageService|DpapiSecureStorageService)\s*::\s*[A-Za-z_]\w*\s*\(' -or
                $content -match '\bclass\s+(ForsettiRuntime|ModuleManager|ManifestLoader|CompatibilityChecker|ForsettiHostBootstrap|ForsettiHostController|ForsettiHostOverlayRouter|ForsettiSurfaceStateAdapter|ServiceContainer|UISurfaceManager)\s+(final\s*)?[:{]' -or
                $content -match '#\s*include\s*[<"](?:[^>"\r\n]*[/\\])?(ForsettiExample[^/\\>"\r\n]*|src[/\\]Forsetti)[/\\]') {
                $issues.Add("Framework-owned implementation/interface or example content found in app source: $($file.FullName)")
            }
            if ($declared -notcontains $file.FullName.ToLowerInvariant()) { $issues.Add("Source/header is outside the explicit integration inventory: $($file.FullName)") }
            if ($content -match 'com\.forsetti\.(template|example)\.|\bForsettiExample\w*|\bForsettiDemo\b') { $issues.Add("Unreplaced scaffold/example identity in source: $($file.FullName)") }
        }
        if ($file.Name -in @('framework-policy.json','implementation-policy.json','sdk-manifest.json') -or
            $file.FullName -match '[\\/]\.github[\\/]' -or $file.Extension -eq '.lib') { $issues.Add("Framework/SDK development payload in app source: $($file.FullName)") }
        if ($file.Name -eq '.gitmodules' -and [IO.File]::ReadAllText($file.FullName) -match 'Forsetti-Framework') { $issues.Add('Framework repository submodule integration is forbidden.') }
    }
    $ids = @{}
    $entries = @{}
    foreach ($manifestRecord in $metadata.manifests) {
        $path = Local-Path $manifestRecord.path
        if (!(Test-Path -LiteralPath $path)) { $issues.Add("Declared manifest is missing: $path"); continue }
        $manifest = [IO.File]::ReadAllText($path) | ConvertFrom-Json
        if ($ids.ContainsKey($manifest.moduleID) -or $entries.ContainsKey($manifest.entryPoint)) { $issues.Add("Duplicate module identity or entry point: $path") }
        $ids[$manifest.moduleID] = $true
        $entries[$manifest.entryPoint] = $true
        if ($manifest.moduleID -cne $manifestRecord.moduleID -or $manifest.entryPoint -cne $manifestRecord.entryPoint -or $manifest.moduleType -cne $manifestRecord.type) { $issues.Add("Manifest/inventory identity mismatch: $path") }
        if ($manifest.moduleID -match '^com\.forsetti\.(template|example)\.' -or $manifest.entryPoint -match '^Template(App|UI|Service|SharedDatabase)Module$|^ForsettiExample') { $issues.Add("Unreplaced scaffold/example manifest identity: $path") }
        if ($manifest.schemaVersion -cne '1.1' -or $manifest.manifestTemplateVersion -cne '1.1' -or
            @($manifest.supportedPlatforms).Count -ne 1 -or $manifest.supportedPlatforms[0] -cne 'Windows' -or
            !($manifest.moduleVersion.PSObject.Properties['major']) -or !($manifest.minForsettiVersion.PSObject.Properties['major'])) { $issues.Add("Manifest schema, platform or object-encoded semantic version is invalid: $path") }
        $capabilities = @('networking','storage','secure_storage','file_export','crypto_utilities','telemetry','routing_overlay','toolbar_items','view_injection','ui_theme_mask','event_publishing','shared_database','authentication','diagnostics','api','security')
        foreach ($capability in $manifest.capabilitiesRequested) { if ($capability -cnotin $capabilities) { $issues.Add("Unknown capability casing/value: $path / $capability") } }
        foreach ($requirement in $manifest.runtimeRequirements.io) {
            if ($requirement.kind -cnotin $manifest.capabilitiesRequested) { $issues.Add("I/O requirement lacks matching capability: $path / $($requirement.kind)") }
        }
        if ($manifestRecord.implemented -and $metadata.kind -eq 'host') {
            $mainPath = Local-Path 'src/Main.cpp'
            $main = if (Test-Path -LiteralPath $mainPath) { [IO.File]::ReadAllText($mainPath) } else { '' }
            if ($main -notmatch ('registerModule\s*\(\s*"' + [regex]::Escape($manifest.entryPoint) + '"')) { $issues.Add("Host registry factory is absent: $($manifest.entryPoint)") }
        }
    }
    # Find extra manifests even when an inventory deliberately omits them.
    foreach ($file in $allFiles | Where-Object { $_.Extension -eq '.json' -and $_.Directory.Name -eq 'ForsettiManifests' }) {
        $relative = $file.FullName.Substring($root.Length + 1).Replace('\','/')
        if (@($metadata.manifests.path) -notcontains $relative) { $issues.Add("Manifest is outside the integration inventory: $relative") }
    }
    foreach ($resource in $metadata.resources) {
        $path = Local-Path $resource.source
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { $issues.Add("Resource is missing: $($resource.source)"); continue }
        if ($implemented.Count -gt 0 -and !$membership.Contains($path)) { $issues.Add("Resource is absent from explicit CMake target membership: $($resource.source)") }
        if ($ShippingRoot -and $resource.shipping) {
            if ([IO.Path]::IsPathRooted($resource.shipping) -or $resource.shipping -match '(^|[\\/])\.\.([\\/]|$)') { throw 'Shipping resource path escapes deployment.' }
            $shipped = Join-Path $ShippingRoot $resource.shipping
            if (!(Test-Path -LiteralPath $shipped -PathType Leaf)) { $issues.Add("Shipping resource is missing: $($resource.shipping)") }
            elseif ((Get-FileHash -LiteralPath $path).Hash -ne (Get-FileHash -LiteralPath $shipped).Hash) { $issues.Add("Shipping resource differs from app-owned source: $($resource.shipping)") }
        }
    }
    if ($ShippingRoot) {
        foreach ($entry in Get-ChildItem -LiteralPath $ShippingRoot -Recurse) {
            if (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { $issues.Add("Reparse point in shipping payload: $($entry.FullName)") }
        }
        foreach ($file in Get-ChildItem -LiteralPath $ShippingRoot -Recurse -File) {
            if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
                $file.Extension -match '^\.(cpp|cc|cxx|h|hpp|ps1|cmake|lib)$' -or
                $file.Name -in @('sdk-manifest.json','framework-policy.json','implementation-policy.json','CMakeLists.txt') -or
                $file.FullName -match '[\\/](\.git|\.github|vcpkg_installed)[\\/]') { $issues.Add("Development/SDK content in shipping payload: $($file.FullName)") }
            if ($file.Directory.Name -eq 'ForsettiManifests' -and $file.Extension -eq '.json') {
                $shippedManifest = [IO.File]::ReadAllText($file.FullName) | ConvertFrom-Json
                if (!$ids.ContainsKey($shippedManifest.moduleID)) { $issues.Add("Unexpected shipping module manifest: $($file.FullName)") }
            }
        }
    }
    $report = [ordered]@{ passed = $issues.Count -eq 0; projectRoot = $root; filesChecked = $allFiles.Count; issues = @($issues.ToArray()); detectionLimits = 'Static inventory/content/CMake checks do not prove arbitrary copied code absence or resolve arbitrary CMake metaprogramming. Native configure/build/run and archive validation are separate required checks.' }
    if ($Json) { $report | ConvertTo-Json -Depth 8 }
    else { if ($issues.Count) { $issues | ForEach-Object { Write-Output "ERROR: $_" } } else { Write-Output "PASS: $($allFiles.Count) application files, installed dependencies, source/resource membership and manifest identities." } }
    if ($issues.Count) { exit 1 }
} catch { Write-Error $_ -ErrorAction Continue; exit 1 }
