#include "lib/jbkv.h"
#include <gtest/gtest.h>

using namespace jbkv;

TEST(Volume, SaveLoadData) {
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

  Save(v, "data.bin");

  auto v2 = CreateVolume();
  Load(v2, "data.bin");
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

TEST(Volume, SaveLoadHierarchy) {
  auto v1 = CreateVolume();
  v1->Create("c1")->Create("c11")->Create("c111");
  auto c22 = v1->Create("c2")->Create("c22");
  auto c12 = v1->Find("c1")->Create("c12");
  auto c1 = v1->Find("c1");

  c1->Open()->Write("name", 1);
  c22->Open()->Write("name", 22);
  c12->Open()->Write("name", 12);
  Save(v1, "hierarchy.bin");

  auto v2 = CreateVolume();
  Load(v2, "hierarchy.bin");

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

TEST(Volume, SaveLoadEmpty) {
  auto v = CreateVolume();
  EXPECT_TRUE(v->Enumerate().empty());
  EXPECT_TRUE(v->Open()->Enumerate().empty());
  Save(v, "empty.bin");

  auto v1 = CreateVolume();
  Load(v1, "empty.bin");
  EXPECT_TRUE(v1->Enumerate().empty());
  EXPECT_TRUE(v1->Open()->Enumerate().empty());
}

TEST(Volume, LoadUnexistingThrows) {
  auto v = CreateVolume();
  EXPECT_THROW(Load(v, "some/unexsiting/path"), std::exception);
}

TEST(Volume, SaveUnexistingThrows) {
  auto v = CreateVolume();
  EXPECT_THROW(Save(v, "some/unexsiting/path"), std::exception);
}

/// todo checksum test
