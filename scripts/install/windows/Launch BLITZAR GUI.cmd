@echo off
setlocal
cd /d "%~dp0"

if exist "%~dp0blitzar-client.exe" (
    start "BLITZAR GUI" "%~dp0blitzar-client.exe" --config "%~dp0simulation.ini" --module qt
    exit /b %ERRORLEVEL%
)

if exist "%~dp0blitzar.exe" (
    start "BLITZAR GUI" "%~dp0blitzar.exe" --mode client --module qt -- --config "%~dp0simulation.ini"
    exit /b %ERRORLEVEL%
)

echo BLITZAR GUI binaries are missing from this directory.
echo Expected blitzar-client.exe or blitzar.exe next to this launcher.
pause
exit /b 1
