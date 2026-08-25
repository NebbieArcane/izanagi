# Prepare dist/windows-staging with izanagi (GUI), nebbiedit (CLI), and Qt runtime DLLs.
param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root "build"
$Dist = Join-Path $Root "dist"
$Staging = Join-Path $Dist "windows-staging"

function Get-ProjectVersion {
    $line = Select-String -Path (Join-Path $Root "CMakeLists.txt") -Pattern 'project\(nebbie-editor VERSION ' | Select-Object -First 1
    if ($line -match 'VERSION ([0-9.]+)') { return $Matches[1] }
    return "0.0.0"
}

function Find-BuiltExe {
    param(
        [string[]]$Candidates
    )
    return $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if ($PSVersionTable.PSEdition -ne "Desktop" -and $IsWindows -ne $true -and $env:OS -notlike "*Windows*") {
    Write-Error "prepare-windows-package.ps1 must run on Windows."
}

if (-not $NoBuild) {
    & (Join-Path $Root "scripts\build.ps1")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$bash = Get-Command bash -ErrorAction SilentlyContinue
if ($bash) {
    Write-Host "==> Preparing bundled sample lib (getworldlocal)"
    & bash (Join-Path $Root "scripts/prepare-sample-lib.sh")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} else {
    Write-Warning "bash not found; Windows package will not include sample-mudroot"
}

$Cli = Find-BuiltExe @(
    (Join-Path $Build "nebbiedit\Release\nebbiedit.exe"),
    (Join-Path $Build "nebbiedit\nebbiedit.exe")
)

$Gui = Find-BuiltExe @(
    (Join-Path $Build "nebbie-qt\Release\izanagi.exe"),
    (Join-Path $Build "nebbie-qt\izanagi.exe"),
    (Join-Path $Build "nebbie-qt\Release\nebbieedit.exe"),
    (Join-Path $Build "nebbie-qt\nebbieedit.exe")
)

if (-not $Cli -or -not $Gui) {
    Write-Error "Build binaries not found. Run .\scripts\build.ps1 first."
}

Remove-Item -Recurse -Force $Staging -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $Staging | Out-Null
Copy-Item $Cli (Join-Path $Staging "nebbiedit.exe")
Copy-Item $Gui (Join-Path $Staging "izanagi.exe")

$Icon = Join-Path $Root "nebbie-qt\icons\nebbieedit.ico"
if (Test-Path $Icon) {
    Copy-Item $Icon (Join-Path $Staging "izanagi.ico")
}

$Sample = Join-Path $Dist "sample-mudroot"
if (Test-Path $Sample) {
    Copy-Item -Recurse $Sample (Join-Path $Staging "sample-mudroot")
}

$Readme = @"
Izanagi (portable)
==================

1. Estrai tutto lo zip in una cartella (es. C:\Izanagi)
2. Avvia izanagi.exe
3. File -> Apri libreria -> seleziona mudroot o mudroot\lib

Puoi trascinare la cartella lib sull'eseguibile, oppure:
  izanagi.exe D:\percorso\mudroot\lib

Mondo di prova incluso: sample-mudroot\lib

Config salvata in: %APPDATA%\Nebbie\nebbieedit.conf

Opzionale: nebbiedit.exe e' la CLI da terminale (comandi room/mob/obj).
Per l'editor grafico usa sempre izanagi.exe.

Requisito: Windows 10/11 64-bit + VC++ Redistributable (di solito gia' installato).
"@
Set-Content -Path (Join-Path $Staging "README.txt") -Value $Readme -Encoding UTF8

$Windeploy = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($Windeploy) {
    & windeployqt (Join-Path $Staging "izanagi.exe")
} else {
    Write-Warning "windeployqt not in PATH; package may miss Qt DLLs. Add Qt kit\bin to PATH."
}

Write-Host "Windows package staging ready: $Staging"
Write-Host "Version: $(Get-ProjectVersion)"
