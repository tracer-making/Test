@echo off
REM 尝试使用系统PATH中的MSBuild
title Compiling Tracer Project

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

echo.
echo Compilation completed.
pause
