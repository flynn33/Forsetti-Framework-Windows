# Forsetti Framework — Windows Guardrails Verification Script
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0
#
# Usage: .\Scripts\verify-forsetti-guardrails.ps1
# Runs from the repository root directory.

$ErrorActionPreference = "Stop"

Write-Host "=== Forsetti Guardrails Verification ===" -ForegroundColor Cyan

# 1. Configure
Write-Host "`n[1/9] Configuring CMake..." -ForegroundColor Yellow
cmake --preset debug
if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed"; exit 1 }

# 2. Build
Write-Host "`n[2/9] Building..." -ForegroundColor Yellow
cmake --build --preset debug
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed"; exit 1 }

# 3. Test
Write-Host "`n[3/9] Running tests..." -ForegroundColor Yellow
ctest --preset debug --output-on-failure --no-tests=error
if ($LASTEXITCODE -ne 0) { Write-Error "Tests failed"; exit 1 }

# 4. Architecture enforcement
Write-Host "`n[4/9] Checking architecture..." -ForegroundColor Yellow
& "$PSScriptRoot\check-architecture.ps1"
if ($LASTEXITCODE -ne 0) { Write-Error "Architecture check failed"; exit 1 }

# 5. Dependency enforcement
Write-Host "`n[5/9] Checking dependencies..." -ForegroundColor Yellow
& "$PSScriptRoot\check-dependencies.ps1"
if ($LASTEXITCODE -ne 0) { Write-Error "Dependency check failed"; exit 1 }

# 6. Manifest enforcement
Write-Host "`n[6/9] Checking manifests..." -ForegroundColor Yellow
& "$PSScriptRoot\check-manifests.ps1"
if ($LASTEXITCODE -ne 0) { Write-Error "Manifest check failed"; exit 1 }

# 7. Pull request compatibility enforcement
Write-Host "`n[7/9] Checking pull request compatibility..." -ForegroundColor Yellow
& "$PSScriptRoot\check-pr-compatibility.ps1"
if ($LASTEXITCODE -ne 0) { Write-Error "Compatibility check failed"; exit 1 }

# 8. Script regression tests
Write-Host "`n[8/9] Checking guardrail scripts..." -ForegroundColor Yellow
& "$PSScriptRoot\test-guardrail-scripts.ps1"
if ($LASTEXITCODE -ne 0) { Write-Error "Guardrail script regression check failed"; exit 1 }

# SDK consumer-tool regressions are separate from the three native CTest suites.
Write-Host "`n[9/9] Checking SDK consumer tools..." -ForegroundColor Yellow
& "$PSScriptRoot\test-sdk-tools.ps1"
if ($LASTEXITCODE -ne 0) { Write-Error "SDK consumer-tool checks failed"; exit 1 }
& "$PSScriptRoot\test-sdk-tools.ps1" -PowerShellExe powershell
if ($LASTEXITCODE -ne 0) { Write-Error "Windows PowerShell SDK consumer-tool checks failed"; exit 1 }

Write-Host "`n=== All guardrails passed ===" -ForegroundColor Green
