@echo off
rmdir /s /q build
mkdir build
cd build
cmake ..
%MSBUILD_DIR%\msbuild jbkv.sln /p:Platform=x64
cd ..
echo "SUCCESS! Find your binaries in build\bin\Release"
