
#include "lib/volume.h"
#include <gtest/gtest.h>

using namespace jbkv;

TEST(VolumeNode, DataReadWrite) {
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

TEST(VolumeNode, DataEnumerate) {
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

TEST(VolumeNode, DataValueCheck) {
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

TEST(VolumeNode, DataRemove) {
  auto v = CreateVolume();
  auto d = v->Open();
  d->Write("hello", "world");
  EXPECT_TRUE(d->Read("hello").Has());
  EXPECT_TRUE(d->Remove("hello"));
  EXPECT_FALSE(d->Read("hello").Has());
  EXPECT_FALSE(d->Remove("hello"));
}

TEST(VolumeNode, DataReadUnknown) {
  auto v = CreateVolume();
  auto d = v->Open();
  auto uk = d->Read("unknown");
  EXPECT_FALSE(uk.Has());
  EXPECT_THROW(uk.As<std::string>(), std::exception);
}

TEST(VolumeNode, DataAccept) {
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

TEST(VolumeNode, ValueChangeType) {
  auto d = CreateVolume()->Open();
  d->Write("number", 42);
  EXPECT_EQ(d->Read("number").As<int>(), 42);
  d->Write("number", "string number");
  EXPECT_EQ(d->Read("number").As<std::string>(), "string number");
}

TEST(VolumeNode, ValueOut) {
  std::stringstream str;
  str << Value(true) << Value('a') << Value((unsigned char)'b')
      << Value((int16_t)-5) << Value((uint16_t)45) << Value((int32_t)-345)
      << Value((uint32_t)554545) << Value((int64_t)-111)
      << Value((uint64_t)556677) << Value("hello")
      << Value(Value::Blob{1, 2, 3, 4});
  EXPECT_EQ(str.str(), "1ab-545-345554545-111556677hello\x1\x2\x3\x4");
}
