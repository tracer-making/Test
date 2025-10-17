@echo off
cd /d "D:\C++Project\新建文件夹"
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Tracer.sln /p:Configuration=Debug /p:Platform=x64
pause
