@echo off
mkdir build
cd build
cmake ..
rem %MSBUILD_DIR%\msbuild jbkv.sln /p:Platform=x64
%MSBUILD_DIR%\msbuild jbkv.sln /p:Platform=Win32
cd ..
echo "SUCCESS! Find your binaries are in build\bin\Release"
