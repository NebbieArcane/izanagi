# Pacchetto Windows portable (.zip) con izanagi.exe + Qt via windeployqt.
param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root "build"
$Dist = Join-Path $Root "dist"
$Staging = Join-Path $Dist "windows-staging"
$Version = if ($env:NEBBIE_VERSION) { $env:NEBBIE_VERSION } else { "0.1.0" }

if (-not $NoBuild) {
    cmake -S $Root -B $Build -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build $Build --config Release -j $env:NUMBER_OF_PROCESSORS
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    ctest --test-dir $Build -C Release --output-on-failure
}

$ExeCandidates = @(
    (Join-Path $Build "app\Release\izanagi.exe"),
    (Join-Path $Build "app\izanagi.exe")
)
$Exe = $ExeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Exe) {
    Write-Error "izanagi.exe non trovato sotto build/app"
}

Remove-Item -Recurse -Force $Staging -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $Staging | Out-Null
New-Item -ItemType Directory -Force -Path $Dist | Out-Null

Copy-Item $Exe (Join-Path $Staging "izanagi.exe")
Copy-Item -Recurse (Join-Path $Root "sample") (Join-Path $Staging "sample")
Copy-Item -Recurse (Join-Path $Root "catalogs") (Join-Path $Staging "catalogs")
Copy-Item (Join-Path $Root "README.md") (Join-Path $Staging "README.md") -ErrorAction SilentlyContinue

$Readme = @"
Izanagi / Izanagi (restyle) — Windows portable
=================================================

1. Estrai tutto lo zip in una cartella
2. Avvia izanagi.exe
3. File -> Apri workspace / area

Mondo di esempio: sample\B1
Versione: $Version

Requisito: Windows 10/11 64-bit (+ VC++ Redistributable se manca).
"@
Set-Content -Path (Join-Path $Staging "LEGGIMI.txt") -Value $Readme -Encoding UTF8

$Windeploy = Get-Command windeployqt -ErrorAction SilentlyContinue
if (-not $Windeploy) {
    $QtBin = Join-Path $env:QT_ROOT_DIR "bin\windeployqt.exe"
    if (Test-Path $QtBin) {
        $Windeploy = Get-Command $QtBin
    }
}
if ($Windeploy) {
    & $Windeploy.Source (Join-Path $Staging "izanagi.exe") --release --no-translations
} else {
    Write-Warning "windeployqt non trovato: lo zip potrebbe non avere le DLL Qt"
}

$Zip = Join-Path $Dist "izanagi_${Version}_windows_portable.zip"
Remove-Item $Zip -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $Staging "*") -DestinationPath $Zip
Write-Host "Creato: $Zip"
Get-Item $Zip | Format-List FullName, Length
