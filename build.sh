#/bin/sh

set -e
rm -rf build
mkdir build
cd build
cmake ..
make
ctest .
cd -
echo "SUCCESS! Find your binaries in ./build/bin"
