#include "volume.h"
#include "node.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message_lite.h>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <format>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {
using namespace jbkv;
using namespace std::string_literals;

/// concurent adding kv to list
// - write value by key
// - key is not exist
// - ? get strong-ptr on table ?
// - ? copy table value by value ?
//    - problem with scalars

/// concurent changing value (string)
// - add to intrusive linked list (lock free)
// - switch raw pointer in table
// - unlink old (it dies when all clients finish working with it)
/// concurrent changing value (scalar)
// - just atomic store
/// string value remove: change current value to special tombstone value
// - do not delete tombstone in case of rewrite
// - do not store tombstone in dump
/// scalar value remove
// - additional flag inside value "tombstone" ??? how update?
//  - 1. given: key with tombstone flag
//  - 2. store value (maybe another thread also stored, or deleted)
//  - 3. exchange tombstone

class StNode final : public Node {
 public:
  explicit StNode(const std::string& name)
      : name_(name) {
  }

  Node::Ptr AddNode(const std::string& name) override {
    auto& child = nodes_[name];
    if (child) {
      throw std::runtime_error("Node with name '" + name + "' already exists");
    }

    child = std::make_shared<StNode>(name);
    return child;
  }

  Node::Ptr GetNode(const std::string& name) const override {
    const auto it = nodes_.find(name);
    if (it == nodes_.end()) {
      return nullptr;
    }

    return it->second;
  }

  void Write(const Key& key, const Value& value) override {
    kv_[key] = value;
  }

  Value Read(const Key& key) const override {
    const auto it = kv_.find(key);
    if (it == kv_.end()) {
      return {};
    }

    return it->second;
  }

 private:
  std::string name_;
  std::unordered_map<Name, Node::Ptr> nodes_;
  std::unordered_map<Key, Value> kv_;
};

class StVolume final : public Volume {
 public:
  StVolume()
      : root_(std::make_shared<StNode>("")) {
  }

 public:
  Node::Ptr Root() const override {
    return root_;
  }

 private:
  Node::Ptr root_;
};

}  // namespace

Node::Ptr Node::FindByPath(const Node::Ptr& from, const Path& path) {
  auto cur = from;
  for (const auto& name : path) {
    if (!cur) {
      return nullptr;
    }

    cur = cur->GetNode(name);
  }

  return cur;
}

Volume::Ptr Volume::CreateSingleThreaded() {
  return std::make_shared<StVolume>();
}
