#/bin/sh

rm -rf build
mkdir build; cd build;cmake ..; make; cd -;
echo "SUCCESS! Find your binaries in ./build/bin"
