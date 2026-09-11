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

    Each run bumps a version counter in tools/release_version.txt (checked
    into git, so it persists and the increment shows up in `git diff`,
    format "<major>.<minor>") and stamps it into the zip name
    (dist/AnsalonRPG-v<major>[.<minor>].zip) and a VERSION.txt dropped
    inside the package, so successive builds handed to the same person are
    distinguishable. The caller must pass exactly one of -Major/-Minor to
    classify the release -- see CLAUDE.md's "Release versioning" for the
    classification rule; this script refuses to guess.

.PARAMETER Major
    This release adds or completes a player-visible system, or changes the
    save format. Bumps the major number and resets minor to 0 (v6 -> v7).

.PARAMETER Minor
    This release is a bug fix, a doc-only change, a parity/cleanup pass, or
    a tweak with no new player-facing capability and no save-format change.
    Bumps the minor number, keeping major (v6 -> v6.1).

.EXAMPLE
    powershell -File tools\package_release.ps1 -Major
.EXAMPLE
    powershell -File tools\package_release.ps1 -Minor
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

Example: powershell -File tools\package_release.ps1 -Major
"@
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build"
$releaseDir = Join-Path $buildDir "Release"
$stageDir = Join-Path $repoRoot "dist\AnsalonRPG"

$versionFile = Join-Path $PSScriptRoot "release_version.txt"
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
