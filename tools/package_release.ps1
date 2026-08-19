<#
.SYNOPSIS
    Builds a Release exe and stages a standalone, shareable copy in dist/.

.DESCRIPTION
    Not part of the shipped game -- a dev-only convenience for handing a
    playable build to someone who doesn't have this source tree. Builds the
    Release config, copies the exe plus the data it needs (as populated next
    to the exe by CMakeLists.txt's post-build step) into dist/AnsalonRPG/,
    and zips that folder to dist/AnsalonRPG-v<N>.zip. See CLAUDE.md's "Build
    process" and docs/GOTCHAS.md for why the exe can locate its own data
    wherever it's unzipped (render::Console::executableDirectory()).

    The recipient just unzips and runs ansalon_rpg.exe -- the MSVC runtime
    is statically linked (CMakeLists.txt's CMAKE_MSVC_RUNTIME_LIBRARY), so
    no separate Visual C++ Redistributable install is needed. They do need
    a VT100-capable terminal (Windows 10+ Terminal or cmd/PowerShell both
    qualify) at least 80x24.

    Each run bumps a build counter in tools/release_version.txt (checked
    into git, so it persists and the increment shows up in `git diff`) and
    stamps it into the zip name (dist/AnsalonRPG-v<N>.zip) and a VERSION.txt
    dropped inside the package, so successive builds handed to the same
    person are distinguishable.

.EXAMPLE
    powershell -File tools\package_release.ps1
#>

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build"
$releaseDir = Join-Path $buildDir "Release"
$stageDir = Join-Path $repoRoot "dist\AnsalonRPG"

$versionFile = Join-Path $PSScriptRoot "release_version.txt"
$version = 0
if (Test-Path $versionFile) {
    $version = [int](Get-Content $versionFile -Raw).Trim()
}
$version++
Set-Content -Path $versionFile -Value $version -NoNewline

$zipPath = Join-Path $repoRoot "dist\AnsalonRPG-v$version.zip"

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

Set-Content -Path (Join-Path $stageDir "VERSION.txt") -Value "AnsalonRPG build $version"

Write-Host "Zipping $zipPath ..."
Get-ChildItem (Join-Path $repoRoot "dist") -Filter "AnsalonRPG-v*.zip" | Remove-Item -Force
Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipPath

Write-Host "Done: $zipPath (build $version)"
