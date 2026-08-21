# Build portable zip for Nebbie Editor (Windows).
param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Dist = Join-Path $Root "dist"
$Staging = Join-Path $Dist "windows-staging"

function Get-ProjectVersion {
    $line = Select-String -Path (Join-Path $Root "CMakeLists.txt") -Pattern 'project\(nebbie-editor VERSION ' | Select-Object -First 1
    if ($line -match 'VERSION ([0-9.]+)') { return $Matches[1] }
    return "0.0.0"
}

if ($PSVersionTable.PSEdition -ne "Desktop" -and $IsWindows -ne $true -and $env:OS -notlike "*Windows*") {
    Write-Error "package-windows-portable.ps1 must run on Windows."
}

$prepareArgs = @()
if ($NoBuild) { $prepareArgs += "-NoBuild" }
& (Join-Path $Root "scripts\prepare-windows-package.ps1") @prepareArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$Version = Get-ProjectVersion
$Zip = Join-Path $Dist "nebbie-editor_${Version}_windows_portable.zip"
Remove-Item $Zip -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $Staging "*") -DestinationPath $Zip
Write-Host "Portable zip created: $Zip"
