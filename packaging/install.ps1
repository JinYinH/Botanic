# Installs the Botanic VST3 bundle for the current user or all users.
# Usage: powershell -ExecutionPolicy Bypass -File install.ps1 [-User]

param([switch]$User)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$bundle = Join-Path $here 'Botanic.vst3'

if (-not (Test-Path $bundle)) {
    Write-Host "Botanic.vst3 not found next to this script." -ForegroundColor Red
    exit 1
}

$targets = @()
if (-not $User) {
    $targets += 'C:\Program Files\Common Files\VST3'
}
$targets += (Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3')

$installedTo = $null

foreach ($target in $targets) {
    try {
        if (-not (Test-Path $target)) {
            New-Item -ItemType Directory -Path $target -Force | Out-Null
        }

        $dest = Join-Path $target 'Botanic.vst3'
        if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }

        Copy-Item -Path $bundle -Destination $target -Recurse -Force
        $installedTo = $target
        break
    }
    catch {
        Write-Host "Cannot write to $target : $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

if ($installedTo -eq $null) {
    Write-Host ""
    Write-Host "Installation failed. Please copy the Botanic.vst3 folder manually" -ForegroundColor Red
    Write-Host "into a VST3 folder that your DAW scans." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Botanic installed to:" -ForegroundColor Green
Write-Host "  $installedTo" -ForegroundColor Green
Write-Host ""
Write-Host "FL Studio: Options > Manage plugins > Find more plugins, then look for 'Botanic'." -ForegroundColor Cyan

if ($installedTo -notmatch 'Program Files') {
    Write-Host ""
    Write-Host "Note: this is a per-user VST3 folder. Add it to FL Studio's plugin" -ForegroundColor Yellow
    Write-Host "search paths if the plugin does not show up after scanning:" -ForegroundColor Yellow
    Write-Host "  $installedTo" -ForegroundColor Yellow
}
