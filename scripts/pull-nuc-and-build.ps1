# Sync izanagi sources from the NUC over SSH, then build (and optionally
# package) on this Windows PC — no GitHub push required.
#
# Prerequisites on Windows:
#   - OpenSSH client (ssh, scp) — optional if you only extract a local .tgz
#   - Visual Studio 2022 C++ workload, CMake, Qt 6 MSVC kit
#
# Examples:
#   # Full pipeline (pack on NUC -> download -> extract -> build -> portable zip)
#   .\scripts\pull-nuc-and-build.ps1 -NucHost nebbie@192.168.1.50
#
#   # Only sync sources, then you run build yourself
#   .\scripts\pull-nuc-and-build.ps1 -NucHost nebbie@nucbuntu -FetchOnly
#
#   # You already copied dist\izanagi-wip-src.tgz manually
#   .\scripts\pull-nuc-and-build.ps1 -LocalTarball C:\Temp\izanagi-wip-src.tgz
#
#   # Custom Qt kit
#   .\scripts\pull-nuc-and-build.ps1 -NucHost nebbie@nuc -QtPrefix C:\Qt\6.5.3\msvc2019_64

param(
    [string]$NucHost = "",
    [string]$NucRepo = "~/NebbieArcane/izanagi",
    [string]$LocalRoot = "",
    [string]$LocalTarball = "",
    [string]$QtPrefix = $env:CMAKE_PREFIX_PATH,
    [switch]$FetchOnly,
    [switch]$NoPackage,
    [switch]$Test,
    [switch]$SkipFetch
)

$ErrorActionPreference = "Stop"

if ($IsWindows -eq $false) {
    Write-Error "pull-nuc-and-build.ps1 must run on Windows."
}

$ScriptDir = $PSScriptRoot
$DefaultRoot = Split-Path -Parent $ScriptDir
if (-not $LocalRoot) {
    $LocalRoot = $DefaultRoot
}
$LocalRoot = [System.IO.Path]::GetFullPath($LocalRoot)
$StagingParent = Split-Path -Parent $LocalRoot
$RepoName = Split-Path -Leaf $LocalRoot
$DownloadDir = Join-Path $env:TEMP "izanagi-nuc-sync"
$RemoteTarball = "/tmp/izanagi-wip-src.tgz"
$LocalDownload = Join-Path $DownloadDir "izanagi-wip-src.tgz"

function Require-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        Write-Error "Required command not found: $Name"
    }
}

function Invoke-NucPack {
    param([string]$HostSpec, [string]$RepoPath)
    Require-Command ssh
    Write-Host "==> packing on NUC ($HostSpec): $RepoPath"
    $remoteCmd = "cd $RepoPath && ./scripts/pack-src-for-windows.sh $RemoteTarball"
    & ssh $HostSpec $remoteCmd
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Invoke-NucDownload {
    param([string]$HostSpec)
    Require-Command scp
    New-Item -ItemType Directory -Force -Path $DownloadDir | Out-Null
    Write-Host "==> scp ${HostSpec}:${RemoteTarball} -> $LocalDownload"
    & scp "${HostSpec}:${RemoteTarball}" $LocalDownload
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Expand-SrcTarball {
    param([string]$Tarball, [string]$DestParent, [string]$ExpectedLeaf)
    Require-Command tar
    if (-not (Test-Path $Tarball)) {
        Write-Error "Tarball not found: $Tarball"
    }
    New-Item -ItemType Directory -Force -Path $DestParent | Out-Null
    Write-Host "==> extracting $Tarball into $DestParent (folder $ExpectedLeaf)"
    # tar creates izanagi/ under DestParent
    & tar -xzf $Tarball -C $DestParent
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $extracted = Join-Path $DestParent $ExpectedLeaf
    if (-not (Test-Path (Join-Path $extracted "scripts\build.ps1"))) {
        Write-Error "Extracted tree looks wrong (missing scripts\build.ps1): $extracted"
    }
}

function Invoke-WindowsBuild {
    param([string]$Root, [string]$Qt, [switch]$RunTest, [switch]$DoPackage)
    $build = Join-Path $Root "scripts\build.ps1"
    $pack = Join-Path $Root "scripts\package-windows-portable.ps1"
    if ($Qt) {
        $env:CMAKE_PREFIX_PATH = $Qt
    }
    if (-not $env:CMAKE_PREFIX_PATH) {
        Write-Warning "CMAKE_PREFIX_PATH / -QtPrefix not set. CMake must find Qt 6 MSVC by itself."
    }
    Push-Location $Root
    try {
        # Named args only: an array splat like @("-Test") binds positionally to -Generator.
        if ($RunTest) {
            Write-Host "==> .\scripts\build.ps1 -Test"
            & $build -Test
        } else {
            Write-Host "==> .\scripts\build.ps1"
            & $build
        }
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

        if ($DoPackage) {
            Write-Host "==> .\scripts\package-windows-portable.ps1"
            & $pack
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            $dist = Join-Path $Root "dist"
            Write-Host ""
            Write-Host "Portable zip(s) in: $dist"
            Get-ChildItem $dist -Filter "izanagi_*_windows_portable.zip" |
                Sort-Object LastWriteTime -Descending |
                Select-Object -First 3 |
                ForEach-Object { Write-Host "  $($_.FullName)" }
        } else {
            $exe = Join-Path $Root "build\nebbie-qt\Release\izanagi.exe"
            if (-not (Test-Path $exe)) {
                $exe = Join-Path $Root "build\nebbie-qt\izanagi.exe"
            }
            Write-Host ""
            Write-Host "Run: $exe"
        }
    } finally {
        Pop-Location
    }
}

Write-Host "Local root: $LocalRoot"

if (-not $SkipFetch) {
    if ($LocalTarball) {
        Expand-SrcTarball -Tarball $LocalTarball -DestParent $StagingParent -ExpectedLeaf $RepoName
    } elseif ($NucHost) {
        Invoke-NucPack -HostSpec $NucHost -RepoPath $NucRepo
        Invoke-NucDownload -HostSpec $NucHost
        Expand-SrcTarball -Tarball $LocalDownload -DestParent $StagingParent -ExpectedLeaf $RepoName
    } else {
        Write-Error @"
Specify how to get sources:
  -NucHost nebbie@IP_OR_HOSTNAME
  -LocalTarball C:\path\izanagi-wip-src.tgz
  -SkipFetch   (tree already at -LocalRoot)
"@
    }
} else {
    Write-Host "==> SkipFetch: using existing tree at $LocalRoot"
}

if ($FetchOnly) {
    Write-Host "FetchOnly: done. Next:"
    Write-Host "  cd $LocalRoot"
    Write-Host "  `$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.x.x\msvc2019_64'"
    Write-Host "  .\scripts\build.ps1 -Test"
    Write-Host "  .\scripts\package-windows-portable.ps1"
    exit 0
}

$doPackage = -not $NoPackage
Invoke-WindowsBuild -Root $LocalRoot -Qt $QtPrefix -RunTest:$Test -DoPackage:$doPackage
