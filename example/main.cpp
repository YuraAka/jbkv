#include <iostream>
#include "../core/volume.h"

using namespace jbkv;

void StorageExample() {
  auto v1 = Volume::CreateLockBased();
  auto v2 = Volume::CreateLockBased();

  auto c1 = v1->Create("a")->Create("b")->Create("c");
  auto c2 = v2->Create("a")->Create("b")->Create("c");

  auto s = Storage::CreateLockBased();
  auto s1 = s->Mount(v1);
  auto s2 = s->Mount(v2);
  /// s1 links to only v1
  /// s2 links to only v2

  s2.reset();
  /// s2 is unmounted

  s1 = s1->Mount(v2);
  /// s1 links to v1 & v2

  v1->Unlink("a");
  auto s1b = s1->Find("a")->Find("b");
  /// unlink does not affect on mountpoint

  auto data = s1b->Open();
  data->Write("hello", "from yuraaka");
  std::cout << data->Read("hello") << std::endl;
  data->Remove("hello");

  /*
  // FIND -- CONST, because it does not store anything
  auto sc2 = sr->Find("moscow");

  sc2->Open();  // return StorageNodeData -- as join of VolumeNodeData, it can
                // subscribe

  auto sr2 = sr->Mount(tyc);
  // sr == sr2
  sr.reset();
  sr2.reset();
  // tyc is unmounted
  */
  // DESIGN DECISION:
  // 1. if link to storage node is obtained -- its hierarchy is not changed
  // on upper mounts but can be changed on bottom mounts, if these mounts
  // relate to existing volume, the same is for adding children for this
  // storage node So, when storage node is hold by client, it represents
  // snapshot of volume nodes hiearchy
  //  on the moment of building
  // 2. to view mount effect path needs to be renavigated from the root
  // 3. All nodes created or found via storage hierarchy automatically become
  // mounted
  // 4. storage nodes are unmounted when no hard-link on it (storage does not
  // hold hardlinks on mounts)

  // PROFITS:
  // - speed of mount/unmount
  // - speed of add/remove nodes
  // - no unexpected behavior for client on occasionally added or deleted
  // nodes
  // - simplicity
}

int main() {
  /*
  /// volume ops
  auto v1 = CreateVolume();
  auto n1 = v1->AddNode("dir1");
  auto n2 = n1->AddNode("dir2");
  auto* pn = v1->FindNode(path);
  for (const auto name : v1->ListNodes()) {
    v1->GetNode(name);
  }

  v1->Save("/tmp/my.vol");
  auto v2 = LoadVolume("/tmp/my.vol");

  /// value ops
  for (const auto& key : n1->ListKeys()) {
    auto value = n1->ReadValue(key);
    n1->WriteValue(key, value);

    if (!n1->RemoveValue(key)) {
      std::cout << "already removed" << std::endl;
    }
  }

  /// storage ops
  auto s1 = CreateStorage();
  auto m1 = s1->Mount(v1, {});
  auto m2 = s1->Mount(n2, {"dir1", "dir2"});

  for (const auto& key : m2.ListKeys()) {
    const auto value = m2->ReadValue(key);
    m2->RemoveValue(key);
  }
  */

  /*auto v1 = Volume::CreateSingleThreaded();

  auto d3 = v1->Root()->ObtainChild("d1")->ObtainChild("d2")->ObtainChild("d3");

  d3->Write("f1", "v1");
  d3->Write("f2", 35u);
  std::cout << d3->Read("f1").As<std::string>() << std::endl;
  std::cout << d3->Read("f2").As<uint32_t>() << std::endl;
  std::cout << *d3->Read<uint32_t>("f2") << std::endl;
  std::cout << d3->Read<std::string>("f2").value_or("34.56") << std::endl;

  d3->Write("f2", 39u);
  std::cout << d3->Read("f2").As<uint32_t>() << std::endl;

  d3->Remove("f2");
  std::cout << d3->Read("f2").Has() << std::endl;
  std::cout << d3->Remove("f2") << std::endl;

  d3->Write("f2", "hello");
  std::cout << d3->Read("f2").As<std::string>() << std::endl;

  std::cout << "remove: " << v1->Root()->ObtainChild("d1")->DropChild("d2")
            << std::endl;

  Save(*v1, "/tmp/yuraaka");

  auto v2 = Volume::CreateSingleThreaded();
  Load(*v2, "/tmp/yuraaka2");
  Save(*v2, "/tmp/yuraaka3");

  auto s1 = Storage::CreateSingleThreaded();
  auto mp = s1->MountRoot(v1->Root());
  // vn->GetOrAddChild()
  auto sd1 = mp->TryChild("d1");*/

  storage_example();

  return 0;
}
