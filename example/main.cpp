#include <iostream>
#include "volume.h"

using namespace jbkv;

void Example1() {
  auto v1 = CreateVolume();
  v1->Create("a")->Create("b");
  auto s1 = MountStorage(v1);
  v1->Find("a")->Find("b")->Open()->Write("key", "another value");
  auto b = s1->Find("a")->Find("b");
  auto d = b->Open();
  d->Write("key", "value");
  std::cout << "key: " << d->Read("key") << std::endl;

  // do unit tests with coverage
}

void StorageExample() {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  auto c1 = v1->Create("a")->Create("b")->Create("c");
  auto c2 = v2->Create("a")->Create("b")->Create("c");

  auto s1 = MountStorage(v1);
  auto s2 = MountStorage(v2);
  /// s1 links to only v1
  /// s2 links to only v2

  s2.reset();
  /// s2 is unmounted

  s1 = s1->Mount(v2);
  /// s1 links to v1 & v2

  auto s1b = s1->Find("a")->Find("b");

  // v1->Unlink("a");
  /// unlink does not affect on mountpoint

  auto data = s1b->Open();
  data->Write("hello", "from yuraaka");
  std::cout << data->Read("hello") << std::endl;
  data->Remove("hello");
}

int main() {
  Example1();
  // StorageExample();

  return 0;
}
