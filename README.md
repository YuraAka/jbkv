# jbkv
[![Linux Build Status](https://github.com/YuraAka/jbkv/actions/workflows/cmake.yml/badge.svg)](https://github.com/YuraAka/jbkv/actions/workflows/cmake.yml)
### How to build
Prerequisites:
- [CMake](https://cmake.org) installed and added to PATH
- C++ compiler with C++20 standard support

Tested on:
- Visual Studio 17 2022
- Clang 14.0.0, x86_64-apple-darwin21.6.0
- Clang 14.0.6, x86_64-pc-linux-gnu

#### Darwin/Linux
```
  $ ./build.sh
  $ ./build/bin/unittest
  $ ./build/bin/functest
  $ ./build/bin/stresstest
```

#### Windows
```
  $ set MSBUILD_DIR=<where-msbuild-located>, ex.: "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin"
  $ build.bat
  $ build\bin\Release\unittest.exe
  $ build\bin\Release\functest.exe
  $ build\bin\Release\stresstest.exe
```


