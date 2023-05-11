@echo off
rmdir /s /q build || exit /b
mkdir build
cd build
cmake .. || exit /b
%MSBUILD_DIR%\msbuild jbkv.sln /p:Platform=x64 || exit /b
cd ..
echo "SUCCESS! Find your binaries in build\bin\Release"
