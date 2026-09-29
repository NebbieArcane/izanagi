# Prepare dist/windows-translate-staging with portable cypher and Qt runtime DLLs.
param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root "build"
$Dist = Join-Path $Root "dist"
$Staging = Join-Path $Dist "windows-translate-staging"

. (Join-Path $PSScriptRoot "nebbie-version.ps1")

function Find-BuiltExe {
    param(
        [string[]]$Candidates
    )
    return $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
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

$Translate = Find-BuiltExe @(
    (Join-Path $Build "nebbie-translator\Release\cypher.exe"),
    (Join-Path $Build "nebbie-translator\cypher.exe"),
    (Join-Path $Build "nebbie-translator\Release\nebbie-translate.exe"),
    (Join-Path $Build "nebbie-translator\nebbie-translate.exe")
)

if (-not $Translate) {
    Write-Error "cypher.exe not found. Run .\scripts\build.ps1 first."
}

Remove-Item -Recurse -Force $Staging -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $Staging | Out-Null
Copy-Item $Translate (Join-Path $Staging "cypher.exe")

$Icon = Join-Path $Root "nebbie-translator\icons\nebbie-translate.ico"
if (Test-Path $Icon) {
    Copy-Item $Icon (Join-Path $Staging "cypher.ico")
}

$Sample = Join-Path $Dist "sample-mudroot"
if (Test-Path $Sample) {
    Copy-Item -Recurse $Sample (Join-Path $Staging "sample-mudroot")
}

$Readme = @"
Cypher (portable)
=================

1. Estrai tutto lo zip in una cartella (es. C:\Cypher)
2. Avvia cypher.exe
3. File -> Apri libreria -> seleziona mudroot o mudroot\lib

Puoi trascinare la cartella lib sull'eseguibile, oppure:
  cypher.exe D:\percorso\mudroot\lib

Mondo di prova incluso: sample-mudroot\lib

Config salvata in: %APPDATA%\Nebbie\nebbie-translate.conf

Requisito: Windows 10/11 64-bit + VC++ Redistributable (di solito gia' installato).
"@
Set-Content -Path (Join-Path $Staging "README.txt") -Value $Readme -Encoding UTF8

$Windeploy = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($Windeploy) {
    & windeployqt (Join-Path $Staging "cypher.exe")
} else {
    Write-Warning "windeployqt not in PATH; package may miss Qt DLLs."
}

Write-Host "Windows translate package staging ready: $Staging"
Write-Host "Version: $(Get-ProjectVersion)"
