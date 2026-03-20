@echo off
setlocal enabledelayedexpansion
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake --build build --target gravityPhysicsGTests > build_output.txt 2>&1
if %errorlevel% neq 0 (
    echo BUILD_FAILED
    exit /b %errorlevel%
)
.\build\gravityPhysicsGTests.exe > test_output.txt 2>&1
if %errorlevel% neq 0 (
    echo TESTS_FAILED
    exit /b %errorlevel%
)
echo ALL_PASSED
