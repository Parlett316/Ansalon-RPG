<#
.SYNOPSIS
    Builds Release ansalon_sfml_phase1.exe and stages a standalone,
    shareable package in dist/ that lets a recipient create their own
    character and play it, entirely in the graphical window.

.DESCRIPTION
    Dev-only convenience, not part of the shipped game -- sibling to
    tools/package_release.ps1 (which packages the older console build,
    ansalon_rpg, instead). ansalon_sfml_phase1 now has its own native
    save-slot menu and character creation wizard (see
    sfml_phase1/main.cpp's runPhase1, run with no save-path argument) --
    a single exe handles everything a recipient needs, so this package no
    longer bundles ansalon_rpg.exe or a separate "create in the console,
    then switch to the graphical build" step the way it used to.

    ansalon_sfml_phase1 resolves data/*.txt and
    References/dragonlancemap2.png relative to process CWD -- no batch-file
    launcher needed for that (unlike the old package_sfml_demo.ps1, which
    needed RunDemo.bat for two reasons that are both gone now: supplying
    the save-path argument, no longer required since character creation
    is native, and setting CWD, which Explorer already sets to the exe's
    own folder on an ordinary double-click with no shortcut involved).
    The recipient just unzips and double-clicks ansalon_sfml_phase1.exe
    directly -- a real exe, not a wrapper script.

    No Visual C++ Redistributable is needed (CMAKE_MSVC_RUNTIME_LIBRARY
    statically links the MSVC runtime) and no SFML DLLs are needed
    (BUILD_SHARED_LIBS is unset, so SFML's FetchContent build defaults to
    static libs). They do need a discrete GPU driver capable of whatever
    OpenGL version SFML 3's graphics module requires.

    Each run bumps a version counter in tools/playable_release_version.txt
    (checked into git so the increment shows up in `git diff`, format
    "<major>.<minor>") and stamps it into the zip name
    (dist/AnsalonRPG-Playable-v<major>[.<minor>].zip) and a VERSION.txt
    dropped inside the package. The caller must pass exactly one of
    -Major/-Minor to classify the release -- see CLAUDE.md's "Release
    versioning" for the classification rule; this script refuses to guess.

.PARAMETER Major
    This release adds or completes a player-visible system, or changes the
    save format. Bumps the major number and resets minor to 0 (v6 -> v7).

.PARAMETER Minor
    This release is a bug fix, a doc-only change, a parity/cleanup pass, or
    a tweak with no new player-facing capability and no save-format change.
    Bumps the minor number, keeping major (v6 -> v6.1).

.EXAMPLE
    powershell -File tools\package_playable_release.ps1 -Major
.EXAMPLE
    powershell -File tools\package_playable_release.ps1 -Minor
#>

param(
    [switch]$Major,
    [switch]$Minor
)

$ErrorActionPreference = "Stop"

if ($Major -and $Minor) {
    throw "Pass exactly one of -Major or -Minor, not both."
}
if (-not $Major -and -not $Minor) {
    throw @"
Pass -Major or -Minor to classify this release (see CLAUDE.md's "Release versioning"):

  A major increment (v6 -> v7) is a new milestone that adds or completes a
  player-visible system or changes the save format. A minor increment
  (v6 -> v6.1) is a bug fix, a doc-only change, a parity/cleanup pass, or a
  tweak that doesn't add new player-facing capability or touch save
  compatibility. When unsure, ask before tagging.

Example: powershell -File tools\package_playable_release.ps1 -Major
"@
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build"
$releaseDir = Join-Path $buildDir "Release"
$stageDir = Join-Path $repoRoot "dist\AnsalonRPG-Playable"

$versionFile = Join-Path $PSScriptRoot "playable_release_version.txt"
$majorVersion = 0
$minorVersion = 0
if (Test-Path $versionFile) {
    $raw = (Get-Content $versionFile -Raw).Trim()
    if ($raw -match '^(\d+)\.(\d+)$') {
        $majorVersion = [int]$Matches[1]
        $minorVersion = [int]$Matches[2]
    } elseif ($raw -match '^\d+$') {
        # Pre-major.minor bare integer (every build before this convention) --
        # read as major.0, matching how that number was always displayed.
        $majorVersion = [int]$raw
    } else {
        throw "Unrecognized content in ${versionFile}: '$raw'"
    }
}
if ($Major) {
    $majorVersion++
    $minorVersion = 0
} else {
    $minorVersion++
}
Set-Content -Path $versionFile -Value "$majorVersion.$minorVersion" -NoNewline

$version = if ($minorVersion -eq 0) { "$majorVersion" } else { "$majorVersion.$minorVersion" }
$zipPath = Join-Path $repoRoot "dist\AnsalonRPG-Playable-v$version.zip"

Write-Host "Building Release ansalon_sfml_phase1..."
$cmake = "C:\Program Files\CMake\bin\cmake.exe"
& $cmake --build $buildDir --config Release --target ansalon_sfml_phase1
if ($LASTEXITCODE -ne 0) {
    throw "Release build failed (exit code $LASTEXITCODE)"
}

$sfmlExePath = Join-Path $releaseDir "ansalon_sfml_phase1.exe"
$dataSrc = Join-Path $repoRoot "data"
$mapSrc = Join-Path $repoRoot "References\dragonlancemap2.png"
if (-not (Test-Path $sfmlExePath)) { throw "Build succeeded but $sfmlExePath is missing" }
if (-not (Test-Path $mapSrc)) { throw "$mapSrc is missing" }

Write-Host "Staging $stageDir ..."
if (Test-Path $stageDir) { Remove-Item -Recurse -Force $stageDir }
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Copy-Item $sfmlExePath -Destination $stageDir

$dataDst = Join-Path $stageDir "data"
Copy-Item $dataSrc -Destination $dataDst -Recurse
# Inspection-only image from tools/generate_overworld.py, not read at
# runtime, and derived from the copyrighted reference map -- see
# docs/MAP_NOTES.md. Never share it.
$preview = Join-Path $dataDst "overworld_preview.png"
if (Test-Path $preview) { Remove-Item -Force $preview }

New-Item -ItemType Directory -Force -Path (Join-Path $stageDir "References") | Out-Null
Copy-Item $mapSrc -Destination (Join-Path $stageDir "References\dragonlancemap2.png")

Set-Content -Path (Join-Path $stageDir "VERSION.txt") -Value "AnsalonRPG-Playable build $version"

Set-Content -Path (Join-Path $stageDir "README.txt") -Value @"
Ansalon: Age of Despair -- build $version
========================================================

This is a fan project, still in active development. Most systems are
implemented and playable; a few things aren't wired up yet and will say
so on screen instead of silently doing nothing -- quest tracking is the
main one (shops/dialogue that reference a quest will tell you it isn't
tracked in this build yet). That's expected, not a bug.

HOW TO PLAY: double-click ansalon_sfml_phase1.exe. You'll see a save-slot
menu -- pick an empty slot to create a new character (name, race, class,
ability scores, etc., all in this same window), or a slot with an
existing character to continue it. Runs straight into the game from
there.

CONTROLS:
  WASD / arrow keys   move
  Enter               interact (enter a zone/portal, talk, confirm)
  C                   character sheet (any key closes it; press s for
                      the spellbook if you can cast spells)
  I                   inventory (or buy/sell toggle inside a shop)
  T                   talk to whoever's on your tile
  L                   look around
  R / Z               rest / bed rest
  V                   full message log
  O                   world map
  G                   journal
  /                   help screen
  Q / Escape          quit (or back out of whatever's open)
  In combat: on-screen prompts show your options each round (attack,
  cast, use item, flee, etc.)

This is a fan project built for fun, not for profit, and is not
affiliated with or endorsed by Wizards of the Coast / the Dragonlance IP
holders.

The overworld map (References/dragonlancemap2.png) is used with the
permission of its author, paercebal (www.paercebal.org/HtmlKrynnMaps),
built on the original map by AtenOkke. Both are credited here per the
terms of that permission.
"@

Write-Host "Zipping $zipPath ..."
Get-ChildItem (Join-Path $repoRoot "dist") -Filter "AnsalonRPG-Playable-v*.zip" -ErrorAction SilentlyContinue | Remove-Item -Force
Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipPath

Write-Host "Done: $zipPath (build $version)"
