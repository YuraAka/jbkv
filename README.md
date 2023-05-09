# jbkv
### How to build
Prerequisites:
- [CMake](https://cmake.org) installed and added to PATH

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
  $ set MSBUILD_DIR=<where-msbuild-located>
  $ build.bat
  $ build\bin\Release\unittest.exe
  $ build\bin\Release\functest.exe
  $ build\bin\Release\stresstest.exe
```


