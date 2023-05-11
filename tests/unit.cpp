
#include "lib/jbkv.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace jbkv;

TEST(VolumeNodeData, ReadWrite) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("bool", true);
  d->Write("char", 'y');
  d->Write("unsigned char", (unsigned char)'h');
  d->Write("int16", (int16_t)-32);
  d->Write("uint16", (uint16_t)48);
  d->Write("int32", (int32_t)-35000);
  d->Write("uint32", (uint32_t)10004);
  d->Write("int64", (int64_t)-10000000);
  d->Write("uin64", (uint64_t)1000456);
  d->Write("float", 23.567f);
  d->Write("double", 1234.567678);
  d->Write("string", "Ю");
  d->Write("blob", Value::Blob{1, 2, 3, 4});

  EXPECT_EQ(d->Read<bool>("bool"), true);
  EXPECT_EQ(d->Read<char>("char"), 'y');
  EXPECT_EQ(d->Read<unsigned char>("unsigned char"), 'h');
  EXPECT_EQ(d->Read<int16_t>("int16"), -32);
  EXPECT_EQ(d->Read<uint16_t>("uint16"), 48);
  EXPECT_EQ(d->Read<int32_t>("int32"), -35000);
  EXPECT_EQ(d->Read<uint32_t>("uint32"), 10004u);
  EXPECT_EQ(d->Read<int64_t>("int64"), -10000000l);
  EXPECT_EQ(d->Read<uint64_t>("uin64"), 1000456ul);
  EXPECT_DOUBLE_EQ(*d->Read<float>("float"), 23.567f);
  EXPECT_DOUBLE_EQ(*d->Read<double>("double"), 1234.567678);
  EXPECT_EQ(d->Read<Value::String>("string"), "Ю");
  EXPECT_EQ(d->Read<Value::Blob>("blob"), (Value::Blob{1, 2, 3, 4}));
  EXPECT_EQ(d->Read<uint32_t>("uint32"), 10004u);
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
  ASSERT_EQ(fields.size(), size_t(2));
  EXPECT_EQ(fields[0].first, "good");
  EXPECT_EQ(*fields[0].second.Try<Value::String>(), "buy");
  EXPECT_EQ(fields[1].first, "hello");
  EXPECT_EQ(*fields[1].second.Try<Value::String>(), "world");
}

TEST(VolumeNodeData, EnumerateExcludeRemoved) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("good", "buy");
  d->Write("hello", "world");
  d->Remove("hello");
  auto fields = d->Enumerate();

  ASSERT_EQ(fields.size(), size_t(1));
  EXPECT_EQ(fields[0].first, "good");
}

TEST(VolumeNodeData, ValueCheck) {
  auto v = CreateVolume();
  auto d = v->Open();
  EXPECT_FALSE(d->Read("hello"));
  d->Write("hello", "world");
  const auto str_value = d->Read<Value::String>("hello");
  ASSERT_TRUE(str_value.has_value());
  EXPECT_EQ(*str_value, "world");
  const auto int_value = d->Read<int>("hello");
  EXPECT_FALSE(int_value.has_value());
}

TEST(VolumeNodeData, Remove) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("hello", "world");
  EXPECT_TRUE(d->Read("hello"));
  EXPECT_TRUE(d->Remove("hello"));
  EXPECT_FALSE(d->Read("hello"));
  EXPECT_FALSE(d->Remove("hello"));
}

TEST(VolumeNodeData, ReadUnknown) {
  auto v = CreateVolume();
  auto d = v->Open();
  auto uk = d->Read("unknown");
  EXPECT_FALSE(uk.has_value());
}

TEST(VolumeNodeData, Accept) {
  auto d = CreateVolume()->Open();
  d->Write("number", 42);
  int num = 0;
  d->Read("number")->Accept([&num](const auto& data) {
    using Type = std::decay_t<decltype(data)>;
    if constexpr (std::is_same_v<Type, int>) {
      num = data;
    }
  });

  EXPECT_EQ(num, 42);
  EXPECT_FALSE(d->Read("unknown").has_value());
}

TEST(VolumeNodeData, Rewrites) {
  auto d = CreateVolume()->Open();
  d->Write("number", 42);
  EXPECT_EQ(d->Read<int>("number"), 42);
  d->Write("number", 54);
  EXPECT_EQ(d->Read<int>("number"), 54);
}

TEST(VolumeNodeData, ChangeType) {
  auto d = CreateVolume()->Open();
  d->Write("number", 42);
  EXPECT_EQ(d->Read<int>("number"), 42);
  d->Write("number", "string number");
  EXPECT_EQ(d->Read<Value::String>("number"), "string number");
}

TEST(VolumeNodeData, Update) {
  auto d = CreateVolume()->Open();
  d->Write("number", 42);
  EXPECT_EQ(d->Read<int>("number"), 42);

  d->Update("number", 50);
  EXPECT_EQ(d->Read<int>("number"), 50);

  d->Update("other", 34);
  EXPECT_FALSE(d->Read<int>("other"));
}

TEST(VolumeNodeData, ReadNull) {
  auto d = CreateVolume()->Open();
  EXPECT_FALSE(d->Read<int>("number").has_value());
  d->Write("number", 42);
  EXPECT_FALSE(d->Read<double>("number").has_value());
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
  EXPECT_TRUE(c1->Find("child11")->IsValid());
  EXPECT_TRUE(v->Find("child2")->IsValid());
}

TEST(VolumeNode, FindInvalid) {
  auto v = CreateVolume();
  auto c = v->Find("child");
  EXPECT_FALSE(c->IsValid());
  EXPECT_THROW(c->Find("smth"), std::exception);
  EXPECT_THROW(c->Create("smth"), std::exception);
  EXPECT_THROW(c->Enumerate(), std::exception);
  EXPECT_THROW(c->GetName(), std::exception);
  EXPECT_THROW(c->Open(), std::exception);
  EXPECT_THROW(c->Unlink("smth"), std::exception);
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
  ASSERT_EQ(children.size(), size_t(3));
  EXPECT_EQ(children[0]->GetName(), "c1");
  EXPECT_EQ(children[0]->Open()->Read<Value::String>("text"), "t1");
  EXPECT_EQ(children[1]->GetName(), "c2");
  EXPECT_EQ(children[1]->Open()->Read<Value::String>("text"), "t2");
  EXPECT_EQ(children[2]->GetName(), "c3");
  EXPECT_EQ(children[2]->Open()->Read<Value::String>("text"), "t3");
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
  ASSERT_EQ(children.size(), size_t(2));
  EXPECT_EQ(children[0]->GetName(), "c1");
  EXPECT_EQ(children[1]->GetName(), "c3");
}

TEST(VolumeNode, ChildrenUnlink) {
  auto v = CreateVolume();
  auto c1 = v->Create("c1");
  c1->Create("c2");
  c1->Open()->Write("num", 32);

  EXPECT_TRUE(v->Find("c1")->IsValid());
  v->Unlink("c1");
  EXPECT_FALSE(v->Find("c1")->IsValid());
  EXPECT_TRUE(c1->Find("c2")->IsValid());
  EXPECT_EQ(c1->Open()->Read<int>("num"), 32);
}

TEST(VolumeNode, CreateExisting) {
  auto v = CreateVolume();
  v->Create("c1")->Create("c2");
  v->Create("c1")->Open()->Write("num", 38);

  auto c1 = v->Find("c1");
  EXPECT_TRUE(c1->Find("c2")->IsValid());
  EXPECT_EQ(c1->Open()->Read<int>("num"), 38);
}

TEST(VolumeNode, SaveLoadThrowsOnTruncated) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("string", Value::String(std::string(100, 'a')));

  std::stringstream stream;
  Save(v, stream);

  // truncate
  auto data = stream.str();
  std::stringstream trunc;
  trunc.write(data.data(), 50);

  auto v2 = CreateVolume();
  EXPECT_THROW(Load(v2, trunc), std::exception);
}

TEST(VolumeNode, SaveLoadDataInMemory) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("bool", true);
  d->Write("char", 'y');
  d->Write("unsigned char", (unsigned char)'h');
  d->Write("int16", (int16_t)-32);
  d->Write("uint16", (uint16_t)48);
  d->Write("int32", (int32_t)-35000);
  d->Write("uint32", (uint32_t)10004);
  d->Write("int64", (int64_t)-10000000);
  d->Write("uin64", (uint64_t)1000456);
  d->Write("float", 23.567f);
  d->Write("double", 1234.567678);
  d->Write("string", "Ю");
  d->Write("blob", Value::Blob{1, 2, 3, 4});

  std::stringstream stream;
  Save(v, stream);

  auto v2 = CreateVolume();
  Load(v2, stream);
  auto d2 = v2->Open();
  EXPECT_EQ(d2->Read<bool>("bool"), true);
  EXPECT_EQ(d2->Read<char>("char"), 'y');
  EXPECT_EQ(d2->Read<unsigned char>("unsigned char"), 'h');
  EXPECT_EQ(d2->Read<int16_t>("int16"), -32);
  EXPECT_EQ(d2->Read<uint16_t>("uint16"), 48);
  EXPECT_EQ(d2->Read<int32_t>("int32"), -35000);
  EXPECT_EQ(d2->Read<uint32_t>("uint32"), 10004);
  EXPECT_EQ(d2->Read<int64_t>("int64"), -10000000);
  EXPECT_EQ(d2->Read<uint64_t>("uin64"), 1000456);
  EXPECT_DOUBLE_EQ(*d2->Read<float>("float"), 23.567f);
  EXPECT_DOUBLE_EQ(*d2->Read<double>("double"), 1234.567678);
  EXPECT_EQ(d2->Read<Value::String>("string"), "Ю");
  EXPECT_EQ(d2->Read<Value::Blob>("blob"), (Value::Blob{1, 2, 3, 4}));
  EXPECT_EQ(d2->Read<uint32_t>("uint32"), 10004u);
}

TEST(VolumeNode, SaveLoadHierarchyInMemory) {
  auto v1 = CreateVolume();
  v1->Create("c1")->Create("c11")->Create("c111");
  auto c22 = v1->Create("c2")->Create("c22");
  auto c12 = v1->Find("c1")->Create("c12");
  auto c1 = v1->Find("c1");

  c1->Open()->Write("name", 1);
  c22->Open()->Write("name", 22);
  c12->Open()->Write("name", 12);

  std::stringstream stream;
  Save(v1, stream);

  auto v2 = CreateVolume();
  Load(v2, stream);

  ASSERT_TRUE(v2->Find("c1"));
  ASSERT_TRUE(v2->Find("c1")->Find("c11"));
  ASSERT_TRUE(v2->Find("c1")->Find("c11")->Find("c111"));
  ASSERT_TRUE(v2->Find("c1")->Find("c12"));
  ASSERT_TRUE(v2->Find("c2"));
  ASSERT_TRUE(v2->Find("c2")->Find("c22"));

  EXPECT_EQ(v2->Find("c1")->Open()->Read<int>("name"), 1);
  EXPECT_EQ(v2->Find("c1")->Find("c12")->Open()->Read<int>("name"), 12);
  EXPECT_EQ(v2->Find("c2")->Find("c22")->Open()->Read<int>("name"), 22);
}

TEST(VolumeNode, SaveLoadNullThrows) {
  std::stringstream stream;
  EXPECT_THROW(Save(nullptr, stream), std::exception);
  EXPECT_THROW(Load(nullptr, stream), std::exception);
}

TEST(StorageNode, MountsVolumeNodes) {
  auto v1 = CreateVolume();
  v1->Create("first");

  auto v2 = CreateVolume();
  v2->Create("second");

  auto s1 = MountStorage(v1);
  EXPECT_TRUE(s1->Find("first")->IsValid());
  EXPECT_FALSE(s1->Find("second")->IsValid());

  auto s2 = s1->Mount(v2);
  EXPECT_TRUE(s1->Find("first")->IsValid());
  EXPECT_FALSE(s1->Find("second")->IsValid());

  EXPECT_TRUE(s2->Find("first")->IsValid());
  EXPECT_TRUE(s2->Find("second")->IsValid());
}

TEST(StorageNode, MountNullThrows) {
  EXPECT_THROW(MountStorage(nullptr), std::exception);

  auto v = CreateVolume();
  auto s = MountStorage(v);
  EXPECT_THROW({ auto m = s->Mount(nullptr); }, std::exception);
}

TEST(StorageNode, MountEmptryThrows) {
  EXPECT_THROW(MountStorage(VolumeNode::List{}), std::exception);

  auto s = MountStorage(CreateVolume());
  EXPECT_THROW({ auto m = s->Mount(VolumeNode::List{}); }, std::exception);
}

TEST(StorageNode, CreateUseExisting) {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  v1->Create("first")->Open()->Write("num", 42);
  v2->Create("second");

  auto n1 = v1->Find("first");
  ASSERT_TRUE(n1);

  auto d1 = n1->Open();
  EXPECT_EQ(d1->Read<int>("num"), 42);

  auto s = MountStorage(v1)->Mount(v2);
  auto sd1 = s->Create("first")->Open();
  EXPECT_EQ(sd1->Read<int>("num"), 42);
}

TEST(StorageNode, LiveAfterUnlink) {
  auto v = CreateVolume();
  v->Create("child")->Open()->Write("num", 33);

  auto s = MountStorage(v->Find("child"));
  v->Unlink("child");

  EXPECT_FALSE(v->Find("child")->IsValid());
  EXPECT_EQ(s->Open()->Read<int>("num"), 33);
}

TEST(StorageNode, MountSubtree) {
  auto v = CreateVolume();
  v->Create("c1");

  auto s = MountStorage(v);
  auto m = s->Find("c1")->Mount(v);
  EXPECT_TRUE(s->Find("c1")->Find("c1")->IsValid());
  m.reset();  // unmount here

  EXPECT_FALSE(s->Find("c1")->Find("c1")->IsValid());
}

TEST(StorageNode, CreatesOnTopLayer) {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  auto s = MountStorage(v1)->Mount(v2);
  s->Create("c1");
  EXPECT_TRUE(s->Find("c1")->IsValid());
  EXPECT_TRUE(v2->Find("c1")->IsValid());
  EXPECT_FALSE(v1->Find("c1")->IsValid());
}

TEST(StorageNode, LayerRead) {
  auto v1 = CreateVolume();
  v1->Create("i")->Create("c1")->Open()->Write("from", "v1");

  auto v2 = CreateVolume();
  v2->Create("c1")->Open()->Write("from", "v2");

  /// single layer
  auto s = MountStorage(v1);
  auto d1 = s->Find("i")->Find("c1")->Open();
  EXPECT_EQ(d1->Read<Value::String>("from"), "v1");

  // second layer
  auto m = s->Find("i")->Mount(v2);
  auto d2 = m->Find("c1")->Open();
  EXPECT_EQ(d2->Read<Value::String>("from"), "v2");

  // global effect
  auto d3 = s->Find("i")->Find("c1")->Open();
  EXPECT_EQ(d3->Read<Value::String>("from"), "v2");

  m.reset();  // unmount
  auto d4 = s->Find("i")->Find("c1")->Open();
  EXPECT_EQ(d4->Read<Value::String>("from"), "v1");

  // no side-effects
  EXPECT_EQ(d2->Read<Value::String>("from"), "v2");
}

TEST(StorageNode, UnlinkAll) {
  auto v1 = CreateVolume();
  v1->Create("a");

  auto v2 = CreateVolume();
  v2->Create("a");

  auto s = MountStorage(v1)->Mount(v2);
  EXPECT_TRUE(s->Find("a")->IsValid());
  EXPECT_TRUE(s->Unlink("a"));
  EXPECT_FALSE(s->Find("a")->IsValid());
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
  ASSERT_EQ(children.size(), size_t(3));
  EXPECT_EQ(children[0]->GetName(), "a");
  EXPECT_EQ(children[1]->GetName(), "b");
  EXPECT_EQ(children[2]->GetName(), "c");

  auto d = children[2]->Open();
  EXPECT_EQ(d->Read<int>("num1"), 1);
  EXPECT_EQ(d->Read<int>("num2"), 2);
}

TEST(StorageNode, FindInvalid) {
  auto v = CreateVolume();
  auto s = MountStorage(v);
  auto c = s->Find("child");
  EXPECT_FALSE(c->IsValid());
  EXPECT_THROW(c->Find("smth"), std::exception);
  EXPECT_THROW(c->Create("smth"), std::exception);
  EXPECT_THROW(c->Enumerate(), std::exception);
  EXPECT_THROW(c->GetName(), std::exception);
  EXPECT_THROW(c->Open(), std::exception);
  EXPECT_THROW(c->Unlink("smth"), std::exception);
  EXPECT_THROW({ auto m = c->Mount(v); }, std::exception);
}

TEST(StorageNodeData, ValueSideEffects) {
  auto v = CreateVolume();
  v->Open()->Write("num", 34);

  auto s = MountStorage(v);
  s->Open()->Write("num", 35);

  EXPECT_EQ(v->Open()->Read<int>("num"), 35);
  EXPECT_EQ(s->Open()->Read<int>("num"), 35);
}

TEST(StorageNodeData, WriteNewToTopLayer) {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  auto s = MountStorage({v1, v2});
  s->Open()->Write("num", 35);

  EXPECT_FALSE(v1->Open()->Read("num").has_value());
  EXPECT_EQ(v2->Open()->Read<int>("num"), 35);
}

TEST(StorageNodeData, Update) {
  auto v1 = CreateVolume();
  auto v2 = CreateVolume();

  v1->Open()->Write("num1", 42);
  v2->Open()->Write("num2", 23);

  auto s = MountStorage({v1, v2});

  EXPECT_EQ(s->Open()->Read<int>("num1"), 42);
  EXPECT_EQ(s->Open()->Read<int>("num2"), 23);

  EXPECT_TRUE(s->Open()->Update("num1", 24));
  EXPECT_TRUE(s->Open()->Update("num2", 32));
  EXPECT_FALSE(s->Open()->Update("num3", 77));

  EXPECT_EQ(s->Open()->Read<int>("num1"), 24);
  EXPECT_EQ(s->Open()->Read<int>("num2"), 32);
  EXPECT_FALSE(s->Open()->Read("num3"));
  EXPECT_EQ(v1->Open()->Read<int>("num1"), 24);
  EXPECT_EQ(v2->Open()->Read<int>("num2"), 32);
}

TEST(StorageNodeData, RemoveFromAllLayers) {
  auto v1 = CreateVolume();
  v1->Open()->Write("num", 1);
  auto v2 = CreateVolume();
  v2->Open()->Write("num", 2);

  auto s = MountStorage({v1, v2});
  EXPECT_EQ(s->Open()->Read<int>("num"), 2);
  EXPECT_TRUE(s->Open()->Remove("num"));
  EXPECT_FALSE(s->Open()->Read("num").has_value());
  EXPECT_FALSE(s->Open()->Remove("num"));
}

TEST(StorageNodeData, EnumerateValues) {
  auto v1 = CreateVolume();
  v1->Open()->Write("num1", 1);
  v1->Open()->Write("num2", 3);

  auto v2 = CreateVolume();
  v2->Open()->Write("num2", 2);

  auto v3 = CreateVolume();

  auto s = MountStorage({v1, v2, v3});
  auto d = s->Open();
  auto kv = d->Enumerate();

  auto by_key = [](auto lhs, auto rhs) {
    return lhs.first < rhs.first;
  };

  std::sort(kv.begin(), kv.end(), by_key);
  ASSERT_EQ(kv.size(), size_t(2));
  EXPECT_EQ(kv[0].first, "num1");
  EXPECT_EQ(*kv[0].second.Try<int>(), 1);
  EXPECT_EQ(kv[1].first, "num2");
  EXPECT_EQ(*kv[1].second.Try<int>(), 2);
}
