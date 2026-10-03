@echo off
setlocal enabledelayedexpansion

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=all"

:: Locate Visual Studio vcvars64.bat
set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if "%VCVARS%"=="" (
    for /f "usebackq tokens=*" %%i in (`powershell -NoProfile -Command "(Get-ChildItem 'C:\Program Files*\Microsoft Visual Studio' -Recurse -Filter 'vcvars64.bat' -ErrorAction SilentlyContinue | Select-Object -First 1).FullName"`) do set "VCVARS=%%i"
)

if "%VCVARS%"=="" (
    echo [ERROR] Could not find Visual Studio 2022 vcvars64.bat.
    exit /b 1
)

call "%VCVARS%"
nmake %TARGET%
