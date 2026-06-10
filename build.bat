@rem Copyright (c) 2026 Alibaba Cloud Computing Ltd.
@rem SPDX-License-Identifier: BSD-3-Clause
@echo off
setlocal

:: Find MSBuild via vswhere (works with any VS edition)
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do set "MSBUILD=%%i"
if "%MSBUILD%"=="" (
    echo ERROR: MSBuild not found. Install Visual Studio with C++ workload.
    exit /b 1
)

:: Default to Win10 Release, allow override via first argument
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Win10 Release"

if "%CONFIG%"=="Win10 Release" (
    set "INSTALLDIR=%~dp0Install\Win10\amd64"
) else if "%CONFIG%"=="Win11 Release" (
    set "INSTALLDIR=%~dp0Install\Win11\amd64"
) else (
    echo ERROR: Unknown configuration "%CONFIG%". Use "Win10 Release" or "Win11 Release".
    exit /b 1
)

echo === Building driver [%CONFIG%] ===
"%MSBUILD%" "%~dp0stdvga.sln" /p:Configuration="%CONFIG%" /p:Platform=x64 /t:Rebuild /verbosity:minimal
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

echo === Done ===
dir "%INSTALLDIR%\stdvga.cat" "%INSTALLDIR%\stdvga.sys" "%INSTALLDIR%\stdvga.inf"
