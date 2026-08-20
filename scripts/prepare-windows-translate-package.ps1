# Prepare dist/windows-translate-staging with portable nebbie-translate and Qt runtime DLLs.
param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root "build"
$Dist = Join-Path $Root "dist"
$Staging = Join-Path $Dist "windows-translate-staging"

function Get-ProjectVersion {
    $line = Select-String -Path (Join-Path $Root "CMakeLists.txt") -Pattern 'project\(nebbie-editor VERSION ' | Select-Object -First 1
    if ($line -match 'VERSION ([0-9.]+)') { return $Matches[1] }
    return "0.0.0"
}

if ($PSVersionTable.PSEdition -ne "Desktop" -and $IsWindows -ne $true -and $env:OS -notlike "*Windows*") {
    Write-Error "prepare-windows-translate-package.ps1 must run on Windows."
}

if (-not $NoBuild) {
    & (Join-Path $Root "scripts\build.ps1") -NoQt
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$bash = Get-Command bash -ErrorAction SilentlyContinue
if ($bash) {
    Write-Host "==> Preparing bundled sample lib (getworldlocal)"
    & bash (Join-Path $Root "scripts/prepare-sample-lib.sh")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} else {
    Write-Warning "bash not found; package will not include sample-mudroot"
}

$Translate = @(
    (Join-Path $Build "nebbie-translator\Release\nebbie-translate.exe"),
    (Join-Path $Build "nebbie-translator\nebbie-translate.exe")
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $Translate) {
    Write-Error "nebbie-translate.exe not found. Run .\scripts\build.ps1 -NoQt first."
}

Remove-Item -Recurse -Force $Staging -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $Staging | Out-Null
Copy-Item $Translate (Join-Path $Staging "nebbie-translate.exe")

$Icon = Join-Path $Root "nebbie-qt\icons\nebbieedit.ico"
if (Test-Path $Icon) {
    Copy-Item $Icon (Join-Path $Staging "nebbie-translate.ico")
}

$Sample = Join-Path $Dist "sample-mudroot"
if (Test-Path $Sample) {
    Copy-Item -Recurse $Sample (Join-Path $Staging "sample-mudroot")
}

$Readme = @"
Nebbie Translate (portable)
===========================

Run nebbie-translate.exe and open your mudroot/lib folder.
Sample world: sample-mudroot/lib

Config: %APPDATA%\Nebbie\nebbie-translate.conf
"@
Set-Content -Path (Join-Path $Staging "README.txt") -Value $Readme -Encoding UTF8

$Windeploy = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($Windeploy) {
    & windeployqt (Join-Path $Staging "nebbie-translate.exe")
} else {
    Write-Warning "windeployqt not in PATH; package may miss Qt DLLs."
}

Write-Host "Windows translate package staging ready: $Staging"
Write-Host "Version: $(Get-ProjectVersion)"
