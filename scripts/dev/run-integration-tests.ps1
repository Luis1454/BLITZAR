param(
    [string]$BuildDir = "build-integration",
    [string]$BuildType = "Release",
    [string]$Generator = "Ninja",
    [string]$BackendExe = "",
    [string]$CTestRegex = "FrontendBackendBridgeRegression|FrontendRuntimeContractRegression",
    [switch]$SkipConfigure,
    [switch]$SkipTests,
    [switch]$BuildBackendIfMissing,
    [string]$BackendBuildDir = "build",
    [string]$BackendBuildType = "Release"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Info([string]$Message)
{
    Write-Host "[integration] $Message"
}

function Get-Vcvars64Path()
{
    $candidatePaths = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $candidatePaths) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "vcvars64.bat introuvable. Installe/repair Visual Studio Build Tools (C++ workload)."
}

function Invoke-InMsvcEnv(
    [string]$VcvarsPath,
    [string[]]$Commands,
    [hashtable]$EnvVars = @{}
)
{
    if ($Commands.Count -eq 0) {
        return
    }

    $tmpCmd = Join-Path (Get-Location).Path ("tmp_integration_{0}.cmd" -f [System.Guid]::NewGuid().ToString("N"))
    try {
        $lines = New-Object System.Collections.Generic.List[string]
        $lines.Add("@echo off")
        $lines.Add("call `"$VcvarsPath`"")
        $lines.Add("if errorlevel 1 exit /b 1")
        foreach ($entry in $EnvVars.GetEnumerator()) {
            $safeValue = [string]$entry.Value
            $lines.Add("set `"$($entry.Key)=$safeValue`"")
        }
        $commandIndex = 2
        foreach ($command in $Commands) {
            $lines.Add($command)
            $lines.Add("if errorlevel 1 exit /b $commandIndex")
            $commandIndex++
        }
        [System.IO.File]::WriteAllLines($tmpCmd, $lines)

        Write-Info ("exec: {0}" -f ($Commands -join " && "))
        & cmd /c $tmpCmd
        if ($LASTEXITCODE -ne 0) {
            throw "Commande MSVC echouee (exit code $LASTEXITCODE)."
        }
    } finally {
        Remove-Item $tmpCmd -ErrorAction SilentlyContinue
    }
}

function Resolve-BackendExePath([string]$ExplicitPath)
{
    if (-not [string]::IsNullOrWhiteSpace($ExplicitPath)) {
        if (-not (Test-Path $ExplicitPath)) {
            throw "Backend executable introuvable: $ExplicitPath"
        }
        return (Resolve-Path $ExplicitPath).Path
    }

    if ($env:GRAVITY_BACKEND_EXE -and (Test-Path $env:GRAVITY_BACKEND_EXE)) {
        return (Resolve-Path $env:GRAVITY_BACKEND_EXE).Path
    }

    $candidates = @(
        "build/myAppBackend.exe",
        "build/Release/myAppBackend.exe",
        "build-clean-arch/myAppBackend.exe",
        "build-verify/myAppBackend.exe"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $discovered = Get-ChildItem -Path "build*" -Recurse -Filter "myAppBackend.exe" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($null -ne $discovered) {
        return $discovered.FullName
    }

    return ""
}

$vcvars = Get-Vcvars64Path
Write-Info "vcvars: $vcvars"

$backendExePath = Resolve-BackendExePath -ExplicitPath $BackendExe
if ([string]::IsNullOrWhiteSpace($backendExePath) -and $BuildBackendIfMissing) {
    Write-Info "backend executable absent, build target myAppBackend..."
    Invoke-InMsvcEnv -VcvarsPath $vcvars -Commands @(
        "cmake -S . -B `"$BackendBuildDir`" -G `"$Generator`" -DCMAKE_BUILD_TYPE=$BackendBuildType",
        "cmake --build `"$BackendBuildDir`" --target myAppBackend --parallel"
    )
    $backendExePath = Resolve-BackendExePath -ExplicitPath ""
}

if ([string]::IsNullOrWhiteSpace($backendExePath)) {
    throw "Impossible de resoudre myAppBackend.exe. Passe -BackendExe <path> ou active -BuildBackendIfMissing."
}

$backendExePath = (Resolve-Path $backendExePath).Path
Write-Info "GRAVITY_BACKEND_EXE=$backendExePath"

if (-not $SkipConfigure) {
    Invoke-InMsvcEnv -VcvarsPath $vcvars -Commands @(
        "cmake -S tests/integration -B `"$BuildDir`" -G `"$Generator`" -DCMAKE_BUILD_TYPE=$BuildType"
    )
}

Invoke-InMsvcEnv -VcvarsPath $vcvars -Commands @(
    "cmake --build `"$BuildDir`" --parallel"
)

if (-not $SkipTests) {
    $ctestCmd = if ([string]::IsNullOrWhiteSpace($CTestRegex)) {
        "ctest --test-dir `"$BuildDir`" --output-on-failure --timeout 180"
    } else {
        "ctest --test-dir `"$BuildDir`" -R `"$CTestRegex`" --output-on-failure --timeout 180"
    }
    Invoke-InMsvcEnv -VcvarsPath $vcvars -Commands @($ctestCmd) -EnvVars @{
        "GRAVITY_BACKEND_EXE" = $backendExePath
    }
}

Write-Info "done"
