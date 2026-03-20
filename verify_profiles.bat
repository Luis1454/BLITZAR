@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 exit /b %errorlevel%
cmake -B build
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build build --target gravityPhysicsGTests
if %errorlevel% neq 0 exit /b %errorlevel%
.\build\gravityPhysicsGTests.exe
