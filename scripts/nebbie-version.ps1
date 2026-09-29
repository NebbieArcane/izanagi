function Get-ProjectVersion {
    param(
        [string]$Root = (Split-Path -Parent $PSScriptRoot),
        [string]$BuildDir = ""
    )

    if ($env:NEBBIE_VERSION) {
        return $env:NEBBIE_VERSION
    }

    if (-not $BuildDir) {
        $BuildDir = Join-Path $Root "build"
    }

    $header = Join-Path $BuildDir "generated/version.hpp"
    if (Test-Path $header) {
        $match = Select-String -Path $header -Pattern '#define NEBBIE_VERSION "(.+)"' | Select-Object -First 1
        if ($match) {
            return $match.Matches[0].Groups[1].Value
        }
    }

    $line = Select-String -Path (Join-Path $Root "CMakeLists.txt") -Pattern 'project\(nebbie-editor VERSION ' | Select-Object -First 1
    if ($line -match 'VERSION ([0-9.]+)') {
        return $Matches[1]
    }

    return "0.0.0"
}
