<#
.SYNOPSIS
    Builds a Release exe and stages a standalone, shareable copy in dist/.

.DESCRIPTION
    Not part of the shipped game -- a dev-only convenience for handing a
    playable build to someone who doesn't have this source tree. Builds the
    Release config, copies the exe plus the data it needs (as populated next
    to the exe by CMakeLists.txt's post-build step) into dist/AnsalonRPG/,
    and zips that folder to dist/AnsalonRPG.zip. See CLAUDE.md's "Build
    process" and docs/GOTCHAS.md for why the exe can locate its own data
    wherever it's unzipped (render::Console::executableDirectory()).

    The recipient just unzips and runs ansalon_rpg.exe -- the MSVC runtime
    is statically linked (CMakeLists.txt's CMAKE_MSVC_RUNTIME_LIBRARY), so
    no separate Visual C++ Redistributable install is needed. They do need
    a VT100-capable terminal (Windows 10+ Terminal or cmd/PowerShell both
    qualify) at least 80x24.

.EXAMPLE
    powershell -File tools\package_release.ps1
#>

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build"
$releaseDir = Join-Path $buildDir "Release"
$stageDir = Join-Path $repoRoot "dist\AnsalonRPG"
$zipPath = Join-Path $repoRoot "dist\AnsalonRPG.zip"

Write-Host "Building Release..."
$cmake = "C:\Program Files\CMake\bin\cmake.exe"
& $cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) {
    throw "Release build failed (exit code $LASTEXITCODE)"
}

$exePath = Join-Path $releaseDir "ansalon_rpg.exe"
$dataSrc = Join-Path $releaseDir "data"
if (-not (Test-Path $exePath)) { throw "Build succeeded but $exePath is missing" }
if (-not (Test-Path $dataSrc)) { throw "Build succeeded but $dataSrc is missing" }

Write-Host "Staging $stageDir ..."
if (Test-Path $stageDir) { Remove-Item -Recurse -Force $stageDir }
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Copy-Item $exePath -Destination $stageDir

$dataDst = Join-Path $stageDir "data"
Copy-Item $dataSrc -Destination $dataDst -Recurse
# Inspection-only image from tools/generate_overworld.py, not read at
# runtime, and derived from the copyrighted reference map -- see
# docs/MAP_NOTES.md. Never share it.
$preview = Join-Path $dataDst "overworld_preview.png"
if (Test-Path $preview) { Remove-Item -Force $preview }

Write-Host "Zipping $zipPath ..."
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipPath

Write-Host "Done: $zipPath"
