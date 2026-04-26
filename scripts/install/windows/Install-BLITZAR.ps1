param(
    [string]$InstallDir = "",
    [switch]$NoDesktopShortcut
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($InstallDir)) {
    $InstallDir = Join-Path $env:LOCALAPPDATA "Programs\BLITZAR"
}

$sourceDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$resolvedInstallDir = [System.IO.Path]::GetFullPath($InstallDir)

New-Item -ItemType Directory -Force -Path $resolvedInstallDir | Out-Null

Get-ChildItem -LiteralPath $sourceDir -Force | ForEach-Object {
    $target = Join-Path $resolvedInstallDir $_.Name
    Copy-Item -LiteralPath $_.FullName -Destination $target -Recurse -Force
}

$launcher = Join-Path $resolvedInstallDir "Launch BLITZAR GUI.cmd"
if (-not (Test-Path -LiteralPath $launcher)) {
    throw "Missing launcher: $launcher"
}

function New-BlitzarShortcut {
    param(
        [string]$ShortcutPath,
        [string]$TargetPath,
        [string]$WorkingDirectory
    )
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($ShortcutPath)
    $shortcut.TargetPath = $TargetPath
    $shortcut.WorkingDirectory = $WorkingDirectory
    $icon = Join-Path $WorkingDirectory "blitzar.exe"
    if (Test-Path -LiteralPath $icon) {
        $shortcut.IconLocation = $icon
    }
    $shortcut.Save()
}

$programsDir = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\BLITZAR"
New-Item -ItemType Directory -Force -Path $programsDir | Out-Null
New-BlitzarShortcut -ShortcutPath (Join-Path $programsDir "BLITZAR GUI.lnk") -TargetPath $launcher -WorkingDirectory $resolvedInstallDir

$uninstaller = Join-Path $resolvedInstallDir "Uninstall BLITZAR.cmd"
if (Test-Path -LiteralPath $uninstaller) {
    New-BlitzarShortcut -ShortcutPath (Join-Path $programsDir "Uninstall BLITZAR.lnk") -TargetPath $uninstaller -WorkingDirectory $resolvedInstallDir
}

if (-not $NoDesktopShortcut) {
    $desktop = [Environment]::GetFolderPath("Desktop")
    New-BlitzarShortcut -ShortcutPath (Join-Path $desktop "BLITZAR GUI.lnk") -TargetPath $launcher -WorkingDirectory $resolvedInstallDir
}

Write-Host "BLITZAR installed to $resolvedInstallDir"
Write-Host "Use the Start Menu shortcut or Launch BLITZAR GUI.cmd to start the GUI."
