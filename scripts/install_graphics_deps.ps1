param(
    [string]$Triplet = "x64-windows",
    [string]$QtVersion = "6.8.2",
    [string]$QtArch = "win64_msvc2022_64",
    [string]$QtOutputDir = "C:/Qt",
    [switch]$BuildQtWithVcpkg,
    [switch]$ForceQtReinstall
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[deps] $Message"
}

function Resolve-AqtPath {
    $wingetAqt = Join-Path $env:LOCALAPPDATA "Microsoft/WinGet/Packages/miurahr.aqtinstall_Microsoft.Winget.Source_8wekyb3d8bbwe/aqt.exe"
    if (Test-Path $wingetAqt) {
        return $wingetAqt
    }

    $command = Get-Command aqt -ErrorAction SilentlyContinue
    if ($command -and $command.Source) {
        return $command.Source
    }
    return $null
}

$vcpkgRoot = "C:/vcpkg"
$vcpkgExe = Join-Path $vcpkgRoot "vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    Write-Step "vcpkg absent, clonage et bootstrap"
    git clone https://github.com/microsoft/vcpkg $vcpkgRoot
    & (Join-Path $vcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics
}

Write-Step "Installation SFML ($Triplet) via vcpkg"
& $vcpkgExe install "sfml:$Triplet" --recurse

if ($BuildQtWithVcpkg) {
    Write-Step "Installation Qt via vcpkg (qtbase:$Triplet)"
    & $vcpkgExe install "qtbase:$Triplet" --recurse
    Write-Step "Termine: SFML + Qt (vcpkg) installes."
    exit 0
}

$qtMsvcDir = Join-Path $QtOutputDir "$QtVersion/msvc2022_64"
$qtConfig = Join-Path $qtMsvcDir "lib/cmake/Qt6/Qt6Config.cmake"

if ($ForceQtReinstall -or -not (Test-Path $qtConfig)) {
    $aqtExe = Resolve-AqtPath
    if (-not $aqtExe) {
        Write-Step "aqt non trouve, installation via winget"
        winget install --id miurahr.aqtinstall --exact --accept-package-agreements --accept-source-agreements
        $aqtExe = Resolve-AqtPath
    }

    if (-not $aqtExe) {
        throw "aqt introuvable apres installation."
    }

    Write-Step "Installation Qt prebuilt ($QtVersion / $QtArch) dans $QtOutputDir"
    & $aqtExe install-qt windows desktop $QtVersion $QtArch --outputdir $QtOutputDir
} else {
    Write-Step "Qt deja present: $qtMsvcDir"
}

Write-Step "Termine: SFML (vcpkg) + Qt prebuilt installes."
