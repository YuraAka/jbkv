# jbkv
### How to build
Prerequisites:
- [CMake](https://cmake.org) installed and added to PATH

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
