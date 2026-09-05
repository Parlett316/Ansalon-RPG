<#
.SYNOPSIS
    Builds a Release ansalon_sfml_phase1.exe and stages a standalone,
    shareable demo copy in dist/.

.DESCRIPTION
    Dev-only convenience, not part of the shipped game -- sibling to
    tools/package_release.ps1, which packages the older console target
    (ansalon_rpg) instead. ansalon_sfml_phase1 (see docs/CURRENT_WORK.md)
    is a WIP trial target that reads data/*.txt and
    References/dragonlancemap2.png as plain relative paths off the
    process's current directory, and takes a save-file path as its one
    command-line argument -- none of that is portable outside this repo
    as-is. This script doesn't touch that source; it just lays out a
    folder where those relative paths resolve correctly, and adds a
    RunDemo.bat that cd's there first and supplies the save argument, so
    a recipient can double-click instead of using a terminal.

    Ships a copy of save2.txt (Regan, level 20 Human Mage) as
    demo_save.txt -- picked because it's the save this project has
    already used to exercise the character sheet/spellcasting UI, not a
    placeholder. The build never writes back to it (game::SaveGame::load
    is read-only here), so the original save2.txt is untouched.

    The recipient just unzips and double-clicks RunDemo.bat. No Visual
    C++ Redistributable is needed (CMAKE_MSVC_RUNTIME_LIBRARY statically
    links the MSVC runtime) and no SFML DLLs are needed (BUILD_SHARED_LIBS
    is unset at the top level, so SFML's FetchContent build defaults to
    static libs -- verified by the absence of any SFML*.dll under build/).
    They do need a discrete GPU driver capable of whatever OpenGL version
    SFML 3's graphics module requires -- untested on anything but this
    dev machine.

    Each run bumps a build counter in tools/sfml_demo_version.txt (checked
    into git so the increment shows up in `git diff`) and stamps it into
    the zip name (dist/AnsalonSFMLDemo-v<N>.zip) and a VERSION.txt dropped
    inside the package.

.EXAMPLE
    powershell -File tools\package_sfml_demo.ps1
#>

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build"
$releaseDir = Join-Path $buildDir "Release"
$stageDir = Join-Path $repoRoot "dist\AnsalonSFMLDemo"

$versionFile = Join-Path $PSScriptRoot "sfml_demo_version.txt"
$version = 0
if (Test-Path $versionFile) {
    $version = [int](Get-Content $versionFile -Raw).Trim()
}
$version++
Set-Content -Path $versionFile -Value $version -NoNewline

$zipPath = Join-Path $repoRoot "dist\AnsalonSFMLDemo-v$version.zip"

Write-Host "Building Release ansalon_sfml_phase1..."
$cmake = "C:\Program Files\CMake\bin\cmake.exe"
& $cmake --build $buildDir --config Release --target ansalon_sfml_phase1
if ($LASTEXITCODE -ne 0) {
    throw "Release build failed (exit code $LASTEXITCODE)"
}

$exePath = Join-Path $releaseDir "ansalon_sfml_phase1.exe"
$dataSrc = Join-Path $repoRoot "data"
$mapSrc = Join-Path $repoRoot "References\dragonlancemap2.png"
$saveSrc = Join-Path $buildDir "Debug\save2.txt"
if (-not (Test-Path $exePath)) { throw "Build succeeded but $exePath is missing" }
if (-not (Test-Path $mapSrc)) { throw "$mapSrc is missing" }
if (-not (Test-Path $saveSrc)) { throw "$saveSrc is missing -- expected the dev save2.txt to copy as the demo save" }

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

New-Item -ItemType Directory -Force -Path (Join-Path $stageDir "References") | Out-Null
Copy-Item $mapSrc -Destination (Join-Path $stageDir "References\dragonlancemap2.png")

Copy-Item $saveSrc -Destination (Join-Path $stageDir "demo_save.txt")

Set-Content -Path (Join-Path $stageDir "VERSION.txt") -Value "AnsalonSFMLDemo build $version"

Set-Content -Path (Join-Path $stageDir "RunDemo.bat") -Value @'
@echo off
cd /d "%~dp0"
ansalon_sfml_phase1.exe demo_save.txt
if errorlevel 1 pause
'@

Set-Content -Path (Join-Path $stageDir "README.txt") -Value @"
Ansalon: Age of Despair -- SFML preview build $version
========================================================

This is a work-in-progress preview of the graphical (SFML) rewrite of an
in-development fan project, not a finished game. Several actions
(Talk/Shop/Inventory/Journal/Rest/etc.) are stubbed out and will print
"not yet in this build" -- that's expected, not a bug.

To run: double-click RunDemo.bat (or ansalon_sfml_phase1.exe directly --
either works, RunDemo.bat just makes sure it starts in this folder).

Loads a preset character (Regan, level 20 Human Mage) read-only -- nothing
you do is ever saved.

Controls:
  WASD / arrow keys   move
  Enter               interact (enter a zone/portal, confirm)
  C                   character sheet (any key closes it)
  Q / Escape          quit
  In combat only: number/letter prompts shown on screen select actions;
  Cast (M) and Item (I) are not implemented in this build yet.

This is a fan project built for fun, not for profit, and is not
affiliated with or endorsed by Wizards of the Coast / the Dragonlance IP
holders.

The overworld map (References/dragonlancemap2.png) is used with the
permission of its author, paercebal (www.paercebal.org/HtmlKrynnMaps),
built on the original map by AtenOkke. Both are credited here per the
terms of that permission.
"@

Write-Host "Zipping $zipPath ..."
Get-ChildItem (Join-Path $repoRoot "dist") -Filter "AnsalonSFMLDemo-v*.zip" | Remove-Item -Force
Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipPath

Write-Host "Done: $zipPath (build $version)"
