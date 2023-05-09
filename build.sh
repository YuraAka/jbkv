#/bin/sh

mkdir build; cd build;cmake ..; make; cd -;
echo "SUCCESS! Find your binaries in ./build/bin"
