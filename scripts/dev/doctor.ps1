param(
    [string]$QtDir = "C:/Qt/6.8.2/msvc2022_64",
    [string]$BuildDir = "build"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

enum CheckLevel {
    Pass
    Warn
    Fail
}

class DoctorCheck {
    [string]$Name
    [CheckLevel]$Level
    [string]$Message
    [string]$FixHint
}

$checks = New-Object System.Collections.Generic.List[DoctorCheck]

function Add-Check(
    [string]$Name,
    [CheckLevel]$Level,
    [string]$Message,
    [string]$FixHint = ""
)
{
    $check = [DoctorCheck]::new()
    $check.Name = $Name
    $check.Level = $Level
    $check.Message = $Message
    $check.FixHint = $FixHint
    $checks.Add($check)
}

function Test-Command([string]$Name)
{
    return $null -ne (Get-Command -Name $Name -ErrorAction SilentlyContinue)
}

function Get-VsDevCmdPath()
{
    $candidates = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }
    return ""
}

function First-Line([string]$Text)
{
    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }
    $parts = $Text -split "`r?`n"
    if ($parts.Count -eq 0) {
        return ""
    }
    return $parts[0]
}

if (Test-Command "cmake") {
    $cmakeVersion = First-Line ((cmake --version | Out-String))
    Add-Check -Name "cmake" -Level Pass -Message $cmakeVersion
} else {
    Add-Check -Name "cmake" -Level Fail -Message "cmake not found in PATH" -FixHint "Install CMake and restart shell."
}

if (Test-Command "ninja") {
    $ninjaVersion = First-Line ((ninja --version | Out-String))
    Add-Check -Name "ninja" -Level Pass -Message "ninja $ninjaVersion"
} else {
    Add-Check -Name "ninja" -Level Warn -Message "ninja not found in PATH" -FixHint "Use -Generator \"Ninja\" only if Ninja is installed."
}

if (Test-Command "nvcc") {
    $nvccVersion = First-Line ((nvcc --version | Out-String))
    Add-Check -Name "cuda" -Level Pass -Message $nvccVersion
} else {
    Add-Check -Name "cuda" -Level Fail -Message "nvcc not found in PATH" -FixHint "Install CUDA Toolkit and/or add it to PATH."
}

if (Test-Command "cl") {
    Add-Check -Name "msvc-cl" -Level Pass -Message "cl.exe available in current shell"
} else {
    $vsDevCmd = Get-VsDevCmdPath
    if ([string]::IsNullOrWhiteSpace($vsDevCmd)) {
        Add-Check -Name "msvc-cl" -Level Fail -Message "cl.exe not in PATH and VsDevCmd.bat not found" -FixHint "Install/repair Visual Studio Build Tools C++."
    } else {
        Add-Check -Name "msvc-cl" -Level Warn -Message "cl.exe not in PATH; VsDevCmd found at $vsDevCmd" -FixHint "Build scripts can still bootstrap MSVC env."
    }
}

$qtDeploy = Join-Path -Path $QtDir -ChildPath "bin/windeployqt.exe"
if (Test-Path -LiteralPath $qtDeploy) {
    Add-Check -Name "qt-deploy" -Level Pass -Message "windeployqt found at $qtDeploy"
} else {
    Add-Check -Name "qt-deploy" -Level Warn -Message "windeployqt not found at $qtDeploy" -FixHint "Set QT_DIR or install Qt desktop toolchain."
}

$moduleHostExe = Join-Path -Path $BuildDir -ChildPath "myAppModuleHost.exe"
if (Test-Path -LiteralPath $moduleHostExe) {
    Add-Check -Name "module-host" -Level Pass -Message "found $moduleHostExe"
} else {
    Add-Check -Name "module-host" -Level Warn -Message "missing $moduleHostExe" -FixHint "Run make all first."
}

$qtModuleDll = Join-Path -Path $BuildDir -ChildPath "gravityFrontendModuleQtInProc.dll"
if (Test-Path -LiteralPath $qtModuleDll) {
    Add-Check -Name "qt-module" -Level Pass -Message "found $qtModuleDll"
} else {
    Add-Check -Name "qt-module" -Level Warn -Message "missing $qtModuleDll" -FixHint "Build frontend modules (make all)."
}

$failCount = 0
$warnCount = 0

Write-Host "[doctor] environment checks"
foreach ($check in $checks) {
    switch ($check.Level) {
        Pass {
            Write-Host ("[PASS] {0}: {1}" -f $check.Name, $check.Message)
        }
        Warn {
            $warnCount++
            Write-Host ("[WARN] {0}: {1}" -f $check.Name, $check.Message)
            if (-not [string]::IsNullOrWhiteSpace($check.FixHint)) {
                Write-Host ("       hint: {0}" -f $check.FixHint)
            }
        }
        Fail {
            $failCount++
            Write-Host ("[FAIL] {0}: {1}" -f $check.Name, $check.Message)
            if (-not [string]::IsNullOrWhiteSpace($check.FixHint)) {
                Write-Host ("       hint: {0}" -f $check.FixHint)
            }
        }
    }
}

Write-Host ("[doctor] summary: fail={0} warn={1} pass={2}" -f $failCount, $warnCount, ($checks.Count - $failCount - $warnCount))
if ($failCount -gt 0) {
    exit 1
}
exit 0
