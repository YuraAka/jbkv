#include <iostream>
#include "lib/volume.h"

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

void Example2() {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  auto c1 = v1->Create("a")->Create("b")->Create("c");
  auto c2 = v2->Create("a")->Create("b")->Create("c");

  auto s1 = MountStorage(v1);
  auto s2 = MountStorage(v2);
  s1->Find("a")->Open()->Write("yura", false);
  s2->Find("a")->Open()->Write("yura", true);
  /// s1 links to only v1
  /// s2 links to only v2

  s2.reset();
  /// s2 is unmounted

  std::cout << "yura: " << s1->Find("a")->Open()->Read("yura") << std::endl;
  s1 = s1->Mount(v2);
  std::cout << "yura: " << s1->Find("a")->Open()->Read("yura") << std::endl;
  /// s1 links to v1 & v2

  auto s1b = s1->Find("a")->Find("b");

  v1->Unlink("a");
  /// unlink does not affect on mountpoint

  auto data = s1b->Open();
  data->Write("hello", "from yuraaka");
  std::cout << data->Read("hello") << std::endl;
  data->Remove("hello");
  std::cout << data->Read("hello") << std::endl;
}

void Example3() {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("bool", true);
  d->Write("char", 'y');
  d->Write("unsigned char", (unsigned char)'h');
  d->Write("int16", (int16_t)-32);
  d->Write("uint16", (uint16_t)48);
  d->Write("int32", -35000);
  d->Write("uint32", 10004u);
  d->Write("int64", -10000000l);
  d->Write("uin64", 1000456ul);
  d->Write("float", 23.567f);
  d->Write("double", 1234.567678);
  d->Write("string", "hello world");
  d->Write("blob", Value::Blob{1, 2, 3, 4});
  d->Write("nop", 123);
  d->Remove("nop");

  auto c1 = v->Create("child1");
  c1->Open()->Write("my name", "child-1");
  auto c2 = c1->Create("child2");
  c2->Open()->Write("my name", "child-2");
  Save(v, "saved.bin");
}

void Example4() {
  auto v2 = CreateVolume();
  Load(v2, "saved.bin");

  std::cout << "string: " << v2->Open()->Read("string") << std::endl;
  std::cout << "c2 name: "
            << v2->Find("child1")->Find("child2")->Open()->Read("my name")
            << std::endl;
}

int main() {
  Example1();
  Example2();
  Example3();
  Example4();

  return 0;
}
