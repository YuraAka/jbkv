#include "lib/volume.h"
#include <gtest/gtest.h>
#include <thread>

using namespace jbkv;

TEST(VolumeNodeData, ReadWriteConcurrently) {
  const size_t concurrency = 100;
  const size_t iterations = 1000;
  const size_t value_size = 100;

  auto v = CreateVolume();
  auto d = v->Open();
  auto val = Value::String(std::string(value_size, 'H'));
  std::vector<std::thread> threads;
  threads.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    threads.emplace_back([=]() {
      for (size_t j = 0; j < iterations; ++j) {
        if (i % 3 == 0) {
          d->Write(std::to_string(j), val);
        } else if (i % 3 == 1) {
          d->Read(std::to_string(j));
        } else {
          d->Remove(std::to_string(j));
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

TEST(StorageNodeData, ReadWriteConcurrently) {
  const size_t volume_count = 100;
  const size_t concurrency = 100;
  const size_t iterations = 1000;
  const size_t keys = 1000;
  const size_t value_size = 100;

  VolumeNode::List volumes;
  auto val = Value::String(std::string(value_size, 'A'));
  for (size_t vol = 0; vol < volume_count; ++vol) {
    volumes.push_back(CreateVolume());
    auto d = volumes[vol]->Open();
    for (size_t key = 0; key < keys; ++key) {
      if ((vol + key) % 3 == 0) {
        d->Write(std::to_string(key), val);
      }
    }
  }

  StorageNode::List mount_points;
  mount_points.reserve(volume_count);
  auto s = MountStorage(volumes[0]);
  for (size_t i = 1; i < volume_count; ++i) {
    mount_points.push_back(s->Mount(volumes[i]));
  }

  std::vector<std::thread> threads;
  threads.reserve(concurrency);
  auto d = s->Open();
  for (size_t i = 0; i < concurrency; ++i) {
    threads.emplace_back([&d, i]() {
      for (size_t j = 0; j < iterations; ++j) {
        if (i % 3 == 0) {
          d->Write(std::to_string(j), j);
        } else if (i % 3 == 1) {
          d->Read(std::to_string(j));
        } else {
          d->Remove(std::to_string(j));
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

/// volume hierarchy
/// storage data
/// storage hier
