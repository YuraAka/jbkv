@echo off
mkdir build
cd build
cmake ..
%MSBUILD_DIR%\msbuild jbkv.sln
echo
echo "SUCCESS! Find your binaries are in ./build/bin"
