param(
    [ValidateSet("dev", "run", "ci")]
    [string]$Profile = "run",
    [string]$BuildDir = "",
    [switch]$Clean,
    [string]$Generator = "Ninja",
    [int]$Jobs = 6,
    [string]$CudaArch = "native",
    [string]$VsDevCmdPath = "",
    [switch]$ProfileLogs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Info([string]$Message)
{
    Write-Host "[build] $Message"
}

function Get-VsDevCmdPath([string]$OverridePath)
{
    if (-not [string]::IsNullOrWhiteSpace($OverridePath)) {
        if (-not (Test-Path -LiteralPath $OverridePath)) {
            throw "VsDevCmd introuvable: $OverridePath"
        }
        return (Resolve-Path -LiteralPath $OverridePath).Path
    }

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
    throw "VsDevCmd.bat introuvable. Installe/repare Visual Studio Build Tools (C++)."
}

function Invoke-InMsvcEnv([string]$VsDevCmd, [string[]]$Commands)
{
    if ($Commands.Count -eq 0) {
        return
    }
    $tmpFile = Join-Path $env:TEMP ("gravity_build_{0}.cmd" -f [Guid]::NewGuid().ToString("N"))
    try {
        $lines = New-Object System.Collections.Generic.List[string]
        $lines.Add("@echo off")
        $lines.Add("call `"$VsDevCmd`" -arch=x64")
        $lines.Add("if errorlevel 1 exit /b 1")
        $index = 2
        foreach ($command in $Commands) {
            $lines.Add($command)
            $lines.Add("if errorlevel 1 exit /b $index")
            $index++
        }
        [System.IO.File]::WriteAllLines($tmpFile, $lines)
        Write-Info ("exec: " + ($Commands -join " && "))
        & cmd /c $tmpFile
        if ($LASTEXITCODE -ne 0) {
            throw "Build echoue (exit code $LASTEXITCODE)."
        }
    } finally {
        Remove-Item -LiteralPath $tmpFile -Force -ErrorAction SilentlyContinue
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Push-Location $repoRoot
try {
    $resolvedVsDevCmd = Get-VsDevCmdPath -OverridePath $VsDevCmdPath

    $buildType = "Release"
    $buildTests = "OFF"
    switch ($Profile) {
        "dev" {
            $buildType = "Debug"
            $buildTests = "ON"
            if ([string]::IsNullOrWhiteSpace($BuildDir)) {
                $BuildDir = "build-dev"
            }
        }
        "run" {
            $buildType = "Release"
            $buildTests = "OFF"
            if ([string]::IsNullOrWhiteSpace($BuildDir)) {
                $BuildDir = "build-run"
            }
        }
        "ci" {
            $buildType = "Release"
            $buildTests = "ON"
            if ([string]::IsNullOrWhiteSpace($BuildDir)) {
                $BuildDir = "build-ci"
            }
        }
    }

    if ($Clean -and (Test-Path -LiteralPath $BuildDir)) {
        Write-Info "clean: $BuildDir"
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
    }

    $profileLogsValue = if ($ProfileLogs) { "ON" } else { "OFF" }

    $configureCmd = @(
        "cmake -S . -B `"$BuildDir`" -G `"$Generator`"",
        "-DCMAKE_BUILD_TYPE=$buildType",
        "-DCMAKE_CUDA_ARCHITECTURES=$CudaArch",
        "-DGRAVITY_BUILD_BACKEND_DAEMON=ON",
        "-DGRAVITY_BUILD_HEADLESS_BINARY=ON",
        "-DGRAVITY_BUILD_FRONTEND_MODULE_HOST=ON",
        "-DGRAVITY_BUILD_FRONTEND_MODULES=ON",
        "-DGRAVITY_BUILD_TESTS=$buildTests",
        "-DGRAVITY_PROFILE_LOGS=$profileLogsValue"
    ) -join " "

    $buildCmd = if ($Jobs -gt 0) {
        "cmake --build `"$BuildDir`" --parallel $Jobs"
    } else {
        "cmake --build `"$BuildDir`" --parallel"
    }

    Write-Info "profile=$Profile build_dir=$BuildDir build_type=$buildType tests=$buildTests"
    Invoke-InMsvcEnv -VsDevCmd $resolvedVsDevCmd -Commands @($configureCmd, $buildCmd)
    Write-Info "done"
} finally {
    Pop-Location
}
