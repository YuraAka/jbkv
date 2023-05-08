
#include "lib/volume.h"
#include <gtest/gtest.h>

using namespace jbkv;

TEST(VolumeNodeData, ReadWrite) {
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
  d->Write("string", "Ю");
  d->Write("blob", Value::Blob{1, 2, 3, 4});

  EXPECT_EQ(d->Read("bool").As<bool>(), true);
  EXPECT_EQ(d->Read("char").As<char>(), 'y');
  EXPECT_EQ(d->Read("unsigned char").As<unsigned char>(), 'h');
  EXPECT_EQ(d->Read("int16").As<int16_t>(), -32);
  EXPECT_EQ(d->Read("uint16").As<uint16_t>(), 48);
  EXPECT_EQ(d->Read("int32").As<int32_t>(), -35000);
  EXPECT_EQ(d->Read("uint32").As<uint32_t>(), 10004u);
  EXPECT_EQ(d->Read("int64").As<int64_t>(), -10000000l);
  EXPECT_EQ(d->Read("uin64").As<uint64_t>(), 1000456ul);
  EXPECT_DOUBLE_EQ(d->Read("float").As<float>(), 23.567f);
  EXPECT_DOUBLE_EQ(d->Read("double").As<double>(), 1234.567678);
  EXPECT_EQ(d->Read("string").As<std::string>(), "Ю");
  EXPECT_EQ(d->Read("blob").As<Value::Blob>(), (Value::Blob{1, 2, 3, 4}));
  EXPECT_EQ(d->Read("uint32").As<uint32_t>(), 10004u);
}

TEST(VolumeNodeData, Enumerate) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("good", "buy");
  d->Write("hello", "world");
  auto fields = d->Enumerate();
  auto by_key = [](const auto& lhs, const auto& rhs) {
    return lhs.first < rhs.first;
  };

  std::sort(fields.begin(), fields.end(), by_key);
  ASSERT_EQ(fields.size(), 2);
  EXPECT_EQ(fields[0].first, "good");
  EXPECT_EQ(fields[0].second.As<std::string>(), "buy");
  EXPECT_EQ(fields[1].first, "hello");
  EXPECT_EQ(fields[1].second.As<std::string>(), "world");
}

TEST(VolumeNodeData, EnumerateExcludeRemoved) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("good", "buy");
  d->Write("hello", "world");
  d->Remove("hello");
  auto fields = d->Enumerate();

  ASSERT_EQ(fields.size(), 1);
  EXPECT_EQ(fields[0].first, "good");
}

TEST(VolumeNodeData, ValueCheck) {
  auto v = CreateVolume();
  auto d = v->Open();
  EXPECT_FALSE(d->Read("hello").Has());
  d->Write("hello", "world");
  const auto* str_value = d->Read("hello").Try<std::string>();
  ASSERT_TRUE(str_value);
  EXPECT_EQ(*str_value, "world");
  const auto* int_value = d->Read("hello").Try<int>();
  EXPECT_EQ(int_value, nullptr);
}

TEST(VolumeNodeData, Remove) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("hello", "world");
  EXPECT_TRUE(d->Read("hello").Has());
  EXPECT_TRUE(d->Remove("hello"));
  EXPECT_FALSE(d->Read("hello").Has());
  EXPECT_FALSE(d->Remove("hello"));
}

TEST(VolumeNodeData, ReadUnknown) {
  auto v = CreateVolume();
  auto d = v->Open();
  auto uk = d->Read("unknown");
  EXPECT_FALSE(uk.Has());
  EXPECT_THROW(uk.As<std::string>(), std::exception);
}

TEST(VolumeNodeData, Accept) {
  auto d = CreateVolume()->Open();
  d->Write("number", 42);
  int num = 0;
  d->Read("number").Accept([&num](const auto& data) {
    using Type = std::decay_t<decltype(data)>;
    if constexpr (std::is_same_v<Type, int>) {
      num = data;
    }
  });

  EXPECT_EQ(num, 42);

  bool checked = false;
  d->Read("unknown").Accept([&checked](const auto& data) {
    using Type = std::decay_t<decltype(data)>;
    if constexpr (std::is_same_v<Type, std::monostate>) {
      checked = true;
    }
  });

  EXPECT_TRUE(checked);
}

TEST(VolumeNodeData, ChangeType) {
  auto d = CreateVolume()->Open();
  d->Write("number", 42);
  EXPECT_EQ(d->Read("number").As<int>(), 42);
  d->Write("number", "string number");
  EXPECT_EQ(d->Read("number").As<std::string>(), "string number");
}

TEST(VolumeNodeData, ValueOut) {
  std::stringstream str;
  str << Value(true) << Value('a') << Value((unsigned char)'b')
      << Value((int16_t)-5) << Value((uint16_t)45) << Value((int32_t)-345)
      << Value((uint32_t)554545) << Value((int64_t)-111)
      << Value((uint64_t)556677) << Value("hello")
      << Value(Value::Blob{1, 2, 3, 4});
  EXPECT_EQ(str.str(), "1ab-545-345554545-111556677hello\x1\x2\x3\x4");
}

TEST(VolumeNode, ChildrenAddFind) {
  auto v = CreateVolume();
  v->Create("child1")->Create("child11");
  v->Create("child2");

  auto c1 = v->Find("child1");
  ASSERT_TRUE(c1);
  EXPECT_TRUE(c1->Find("child11"));
  EXPECT_TRUE(v->Find("child2"));
}

TEST(VolumeNode, ChildrenEnumerate) {
  auto v = CreateVolume();
  v->Create("c1")->Open()->Write("text", "t1");
  v->Create("c2")->Open()->Write("text", "t2");
  v->Create("c3")->Open()->Write("text", "t3");
  auto children = v->Enumerate();
  auto by_name = [](const auto& lhs, const auto& rhs) {
    return lhs->GetName() < rhs->GetName();
  };

  std::sort(children.begin(), children.end(), by_name);
  ASSERT_EQ(children.size(), 3);
  EXPECT_EQ(children[0]->GetName(), "c1");
  EXPECT_EQ(children[0]->Open()->Read("text").As<std::string>(), "t1");
  EXPECT_EQ(children[1]->GetName(), "c2");
  EXPECT_EQ(children[1]->Open()->Read("text").As<std::string>(), "t2");
  EXPECT_EQ(children[2]->GetName(), "c3");
  EXPECT_EQ(children[2]->Open()->Read("text").As<std::string>(), "t3");
}

TEST(VolumeNode, ChildrenEnumerateExludeUnlinked) {
  auto v = CreateVolume();
  v->Create("c1");
  v->Create("c2");
  v->Create("c3");
  v->Unlink("c2");

  auto children = v->Enumerate();
  auto by_name = [](const auto& lhs, const auto& rhs) {
    return lhs->GetName() < rhs->GetName();
  };

  std::sort(children.begin(), children.end(), by_name);
  ASSERT_EQ(children.size(), 2);
  EXPECT_EQ(children[0]->GetName(), "c1");
  EXPECT_EQ(children[1]->GetName(), "c3");
}

TEST(VolumeNode, ChildrenUnlink) {
  auto v = CreateVolume();
  auto c1 = v->Create("c1");
  c1->Create("c2");
  c1->Open()->Write("num", 32);

  EXPECT_TRUE(v->Find("c1"));
  v->Unlink("c1");
  EXPECT_FALSE(v->Find("c1"));
  EXPECT_TRUE(c1->Find("c2"));
  EXPECT_EQ(c1->Open()->Read("num").As<int>(), 32);
}

TEST(VolumeNode, CreateExisting) {
  auto v = CreateVolume();
  v->Create("c1")->Create("c2");
  v->Create("c1")->Open()->Write("num", 38);

  auto c1 = v->Find("c1");
  EXPECT_TRUE(c1->Find("c2"));
  EXPECT_EQ(c1->Open()->Read("num").As<int>(), 38);
}

TEST(StorageNode, MountsVolumeNodes) {
  auto v1 = CreateVolume();
  v1->Create("first");

  auto v2 = CreateVolume();
  v2->Create("second");

  auto s1 = MountStorage(v1);
  EXPECT_TRUE(s1->Find("first"));
  EXPECT_FALSE(s1->Find("second"));

  auto s2 = s1->Mount(v2);
  EXPECT_TRUE(s1->Find("first"));
  EXPECT_FALSE(s1->Find("second"));

  EXPECT_TRUE(s2->Find("first"));
  EXPECT_TRUE(s2->Find("second"));
}

TEST(StorageNode, EnumeratesSuperposition) {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  v1->Create("first");
  v2->Create("second");

  auto s = MountStorage(v1)->Mount(v2);
  auto children = s->Enumerate();
  auto by_name = [](const auto& lhs, const auto& rhs) {
    return lhs->GetName() < rhs->GetName();
  };

  std::sort(children.begin(), children.end(), by_name);
  ASSERT_EQ(children.size(), 2);
  EXPECT_EQ(children[0]->GetName(), "first");
  EXPECT_EQ(children[1]->GetName(), "second");
}

TEST(StorageNode, CreateUseExisting) {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  v1->Create("first")->Open()->Write("num", 42);
  v2->Create("second");

  auto n1 = v1->Find("first");
  ASSERT_TRUE(n1);

  auto d1 = n1->Open();
  EXPECT_EQ(d1->Read("num").As<int>(), 42);

  auto s = MountStorage(v1)->Mount(v2);
  auto sd1 = s->Create("first")->Open();
  EXPECT_EQ(sd1->Read("num").As<int>(), 42);
}

TEST(StorageNode, LiveAfterUnlink) {
  auto v = CreateVolume();
  v->Create("child")->Open()->Write("num", 33);

  auto s = MountStorage(v->Find("child"));
  v->Unlink("child");

  EXPECT_FALSE(v->Find("child"));
  EXPECT_EQ(s->Open()->Read("num").As<int>(), 33);
}

TEST(StorageNode, MountSubtree) {
  auto v = CreateVolume();
  v->Create("c1");

  auto s = MountStorage(v);
  auto m = s->Find("c1")->Mount(v);
  EXPECT_TRUE(s->Find("c1")->Find("c1"));
  m.reset();  // unmount here

  EXPECT_FALSE(s->Find("c1")->Find("c1"));
}

TEST(StorageNode, LayerRead) {
  auto v1 = CreateVolume();
  v1->Create("i")->Create("c1")->Open()->Write("from", "v1");

  auto v2 = CreateVolume();
  v2->Create("c1")->Open()->Write("from", "v2");

  /// single layer
  auto s = MountStorage(v1);
  auto d1 = s->Find("i")->Find("c1")->Open();
  EXPECT_EQ(d1->Read("from").As<std::string>(), "v1");

  // second layer
  auto m = s->Find("i")->Mount(v2);
  auto d2 = m->Find("c1")->Open();
  EXPECT_EQ(d2->Read("from").As<std::string>(), "v2");

  // global effect
  auto d3 = s->Find("i")->Find("c1")->Open();
  EXPECT_EQ(d3->Read("from").As<std::string>(), "v2");

  m.reset();  // unmount
  auto d4 = s->Find("i")->Find("c1")->Open();
  EXPECT_EQ(d4->Read("from").As<std::string>(), "v1");

  // no side-effects
  EXPECT_EQ(d2->Read("from").As<std::string>(), "v2");
}

TEST(StorageNode, UnlinkAll) {
  auto v1 = CreateVolume();
  v1->Create("a");

  auto v2 = CreateVolume();
  v2->Create("a");

  auto s = MountStorage(v1)->Mount(v2);
  EXPECT_TRUE(s->Find("a"));
  EXPECT_TRUE(s->Unlink("a"));
  EXPECT_FALSE(s->Find("a"));
  EXPECT_FALSE(s->Unlink("a"));
}

TEST(StorageNode, EnumerateAggregation) {
  auto v1 = CreateVolume();
  v1->Create("a");
  v1->Create("c")->Open()->Write("num1", 1);

  auto v2 = CreateVolume();
  v2->Create("b");
  v1->Create("c")->Open()->Write("num2", 2);

  auto s = MountStorage(v1)->Mount(v2);
  auto children = s->Enumerate();

  auto by_name = [](auto lhs, auto rhs) {
    return lhs->GetName() < rhs->GetName();
  };

  std::sort(children.begin(), children.end(), by_name);
  ASSERT_EQ(children.size(), 3);
  EXPECT_EQ(children[0]->GetName(), "a");
  EXPECT_EQ(children[1]->GetName(), "b");
  EXPECT_EQ(children[2]->GetName(), "c");

  auto d = children[2]->Open();
  EXPECT_EQ(d->Read("num1").As<int>(), 1);
  EXPECT_EQ(d->Read("num2").As<int>(), 2);
}

TEST(StorageNodeData, ValueSideEffects) {
  auto v = CreateVolume();
  v->Open()->Write("num", 34);

  auto s = MountStorage(v);
  s->Open()->Write("num", 35);

  EXPECT_EQ(v->Open()->Read("num").As<int>(), 35);
  EXPECT_EQ(s->Open()->Read("num").As<int>(), 35);
}

TEST(StorageNodeData, WriteNewToTopLayer) {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  auto s = MountStorage(v1)->Mount(v2);
  s->Open()->Write("num", 35);

  EXPECT_FALSE(v1->Open()->Read("num").Has());
  EXPECT_EQ(v2->Open()->Read("num").As<int>(), 35);
}

TEST(StorageNodeData, RemoveFromAllLayers) {
  auto v1 = CreateVolume();
  v1->Open()->Write("num", 1);
  auto v2 = CreateVolume();
  v2->Open()->Write("num", 2);

  auto s = MountStorage(v1)->Mount(v2);
  EXPECT_EQ(s->Open()->Read("num").As<int>(), 2);
  EXPECT_TRUE(s->Open()->Remove("num"));
  EXPECT_FALSE(s->Open()->Read("num").Has());
  EXPECT_FALSE(s->Open()->Remove("num"));
}

TEST(StorageNodeData, EnumerateValues) {
  auto v1 = CreateVolume();
  v1->Open()->Write("num1", 1);
  v1->Open()->Write("num2", 3);

  auto v2 = CreateVolume();
  v1->Open()->Write("num2", 2);

  auto v3 = CreateVolume();

  auto s = MountStorage(v1)->Mount(v2)->Mount(v3);
  auto d = s->Open();
  auto kv = d->Enumerate();

  auto by_key = [](auto lhs, auto rhs) {
    return lhs.first < rhs.first;
  };

  std::sort(kv.begin(), kv.end(), by_key);
  ASSERT_EQ(kv.size(), 2);
  EXPECT_EQ(kv[0].first, "num1");
  EXPECT_EQ(kv[0].second.As<int>(), 1);
  EXPECT_EQ(kv[1].first, "num2");
  EXPECT_EQ(kv[1].second.As<int>(), 2);
}
