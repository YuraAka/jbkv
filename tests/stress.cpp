#include "lib/volume.h"
#include <gtest/gtest.h>
#include <thread>

using namespace jbkv;

TEST(VolumeNodeData, ReadWriteConcurrently) {
  const size_t concurrency = 50;
  const size_t iterations = 1000;
  const size_t value_size = 100;

  auto v = CreateVolume();
  auto val = Value::String(std::string(value_size, 'H'));
  std::vector<std::thread> threads;
  threads.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    threads.emplace_back([&val, &v]() {
      for (size_t j = 0; j < iterations; ++j) {
        const auto name = std::to_string(j % 5);
        auto d = v->Open();
        d->Write(name, val);
        d->Read(name);
        d->Enumerate();
        d->Update(name, val);
        d->Remove(name);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(v->Open()->Enumerate().empty());
}

TEST(VolumeNodeHierarchy, ModifiesConcurrently) {
  const size_t concurrency = 50;
  const size_t iterations = 1000;

  auto v = CreateVolume();
  std::vector<std::thread> threads;
  threads.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    threads.emplace_back([&v]() {
      for (size_t j = 0; j < iterations; ++j) {
        const auto name = std::to_string(j % 5);
        v->Create(name)->Create(name);
        auto node = v->Find(name);
        if (node->IsValid()) {
          node->GetName();
          node->Enumerate();
          node->Unlink(name);
        }

        v->Unlink(name);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(v->Enumerate().empty());
}

TEST(StorageNodeData, ReadWriteConcurrently) {
  const size_t concurrency = 50;
  const size_t iterations = 1000;
  const size_t value_size = 100;

  VolumeNode::List vs{3, CreateVolume()};
  auto s = MountStorage(vs);
  auto val = Value::String(std::string(value_size, 'H'));
  std::vector<std::thread> threads;
  threads.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    threads.emplace_back([i, &val, &s, &vs]() {
      for (size_t j = 0; j < iterations; ++j) {
        const auto name = std::to_string(j % 5);
        auto d = (i % 4 != 0) ? vs[i % 4 - 1]->Open() : s->Open();
        d->Write(name, val);
        d->Read(name);
        d->Enumerate();
        d->Update(name, val);
        d->Remove(name);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(s->Open()->Enumerate().empty());
}

TEST(StorageNodeHierarchy, ModifiesConcurrently) {
  const size_t concurrency = 50;
  const size_t iterations = 1000;

  VolumeNode::List vs{3, CreateVolume()};
  auto s = MountStorage(vs);
  std::vector<std::thread> threads;
  threads.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    threads.emplace_back([i, &s, &vs]() {
      for (size_t j = 0; j < iterations; ++j) {
        const auto name = std::to_string(j % 5);
        if (i % 4 == 0) {
          auto n1 = s->Create(name);
          n1->Create(name);
          s->Enumerate();
          n1->Enumerate();
          n1->GetName();
          n1->Unlink(name);
          s->Unlink(name);
        } else {
          auto v = vs[i % 4 - 1];
          auto n1 = v->Create(name);
          n1->Create(name);
          s->Enumerate();
          n1->Enumerate();
          n1->GetName();
          n1->Unlink(name);
          v->Unlink(name);
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(s->Enumerate().empty());
  EXPECT_TRUE(vs[0]->Enumerate().empty());
  EXPECT_TRUE(vs[1]->Enumerate().empty());
  EXPECT_TRUE(vs[2]->Enumerate().empty());
}

TEST(StorageNodeHierarchy, MountsConcurrently) {
  const size_t concurrency = 30;
  const size_t iterations = 1000;

  auto s = MountStorage(CreateVolume());
  std::vector<std::thread> threads;
  threads.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    threads.emplace_back([&s]() {
      for (size_t j = 0; j < iterations; ++j) {
        const auto name = std::to_string(j % 5);

        auto m1 = s->Create(name)->Mount(CreateVolume());
        s->Unlink(name);
        auto m2 = s->Mount(CreateVolume());
        m1->Create(name);
        m1->Unlink(name);
        m2->GetName();
        m2->Enumerate();
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(s->Enumerate().empty());
}
