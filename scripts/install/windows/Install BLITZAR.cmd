@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-BLITZAR.ps1" %*
exit /b %ERRORLEVEL%
