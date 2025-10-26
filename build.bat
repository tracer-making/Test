@echo off
title Building Tracer Project

REM 保存当前目录
set ORIGINAL_DIR=%cd%

REM 检查Tracer.sln是否存在
cd /d "%~dp0"
if not exist "Tracer.sln" (
    echo Error: Tracer.sln not found in the current directory.
    echo Please run this script from the project root directory.
    pause
    exit /b 1
)

REM 检查MSBuild是否在PATH中
where msbuild >nul 2>&1
if %errorlevel% == 0 (
    echo Using MSBuild from PATH
    msbuild Tracer.sln /p:Configuration=Debug /p:Platform=x64
) else (
    REM 尝试常见的Visual Studio安装路径
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
        echo Using MSBuild from Visual Studio 2022 Community
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Tracer.sln /p:Configuration=Debug /p:Platform=x64
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" (
        echo Using MSBuild from Visual Studio 2022 Professional
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" Tracer.sln /p:Configuration=Debug /p:Platform=x64
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
        echo Using MSBuild from Visual Studio 2022 Enterprise
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" Tracer.sln /p:Configuration=Debug /p:Platform=x64
    ) else (
        echo Error: MSBuild not found. Please install Visual Studio or add MSBuild to your PATH.
        echo Download Visual Studio Community from https://visualstudio.microsoft.com/vs/community/
        pause
        exit /b 1
    )
)

REM 恢复原始目录
cd /d "%ORIGINAL_DIR%"

echo.
echo Build completed.
pause
