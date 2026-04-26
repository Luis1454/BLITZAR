param(
    [string]$InstallDir = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($InstallDir)) {
    $InstallDir = Join-Path $env:LOCALAPPDATA "Programs\BLITZAR"
}

$resolvedInstallDir = [System.IO.Path]::GetFullPath($InstallDir)
$programsDir = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\BLITZAR"
$desktopShortcut = Join-Path ([Environment]::GetFolderPath("Desktop")) "BLITZAR GUI.lnk"

if (Test-Path -LiteralPath $desktopShortcut) {
    Remove-Item -LiteralPath $desktopShortcut -Force
}

if (Test-Path -LiteralPath $programsDir) {
    Remove-Item -LiteralPath $programsDir -Recurse -Force
}

if (Test-Path -LiteralPath $resolvedInstallDir) {
    Remove-Item -LiteralPath $resolvedInstallDir -Recurse -Force
}

Write-Host "BLITZAR removed from $resolvedInstallDir"
