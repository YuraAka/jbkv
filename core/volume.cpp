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

  /// todo mb intrusive siblings?
  Node::List ListNodes() const override {
    Node::List result;
    result.reserve(nodes_.size());
    for (const auto& [_, node] : nodes_) {
      result.push_back(node);
    }

    return result;
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

  bool Remove(const Key& key) override {
    const auto it = kv_.find(key);
    if (it == kv_.end() || !it->second.Has()) {
      return false;
    }

    it->second = Value();
    return true;
  }

  void Accept(NodeReader& reader) const override {
    for (const auto& [name, _] : nodes_) {
      reader.OnLink(name_, name);
    }

    for (const auto& [key, value] : kv_) {
      if (value.Has()) {
        reader.OnData(name_, key, value);
      }
    }
  }

  void Accept(NodeWriter& writer) override {
    writer.OnLink(name_, [this](auto&& child_name) {
      auto& child = nodes_[child_name];
      child = std::make_shared<StNode>(std::move(child_name));
    });

    writer.OnData(name_, [this](auto&& key, auto&& value) {
      kv_[std::move(key)] = std::move(value);
    });
  }

 private:
  std::string name_;
  std::unordered_map<Name, Node::Ptr> nodes_;
  std::unordered_map<Key, Value> kv_;
};

class StVolume final : public Volume {
 public:
  StVolume()
      : root_(std::make_shared<StNode>("root")) {
  }

 public:
  Node::Ptr Root() const override {
    return root_;
  }

 private:
  Node::Ptr root_;
};

class FsSaver final : public NodeReader {
 public:
  explicit FsSaver(const std::filesystem::path& path) {
    std::cout << "Saving to " << path << std::endl;
  }

  void OnLink(const Node::Name& parent, const Node::Name& child) override {
    std::cout << "Saving parent-child: " << parent << "->" << child
              << std::endl;
  }

  void OnData(const Node::Name& name, const Node::Key& key,
              const Value& value) override {
    std::cout << "- saving node '" << name << "', " << key << " = " << value
              << std::endl;
  }
};

class FsLoader final : public NodeWriter {
 public:
  explicit FsLoader(const std::filesystem::path& path)
      : hierarchy_{{"root", {"d1", "d2"}}, {"d1", {"d3", "d4"}}},
        data_{{"d1", {{"k1", "v1"}, {"k2", "v2"}}},
              {"d4", {{"k3", "v3"}, {"k4", "v4"}}}} {
    std::cout << "Loading from " << path << std::endl;
  }

  void OnData(const Node::Name& self, const DataCallback& cb) {
    auto dit = data_.find(self);
    if (dit != data_.end()) {
      for (auto&& [key, value] : dit->second) {
        Value val(std::move(value));
        cb(std::move(key), std::move(val));
      }
    }
  }

  void OnLink(const Node::Name& self, const LinkCallback& cb) {
    auto hit = hierarchy_.find(self);
    if (hit != hierarchy_.end()) {
      for (auto&& child_name : hit->second) {
        cb(std::move(child_name));
      }
    }
  }

 private:
  using NodeHierarchy = std::vector<std::string>;
  using Hierarchy = std::unordered_map<std::string, NodeHierarchy>;
  using KvList = std::vector<std::pair<std::string, std::string>>;
  using Data = std::unordered_map<std::string, KvList>;

  Hierarchy hierarchy_;
  Data data_;
};

template <typename T, typename U>
void Accept(T&& volume, U& visitor) {
  std::deque<Node::Ptr> nodes{volume.Root()};
  while (!nodes.empty()) {
    auto node = nodes.front();
    nodes.pop_front();
    node->Accept(visitor);
    for (auto&& child : node->ListNodes()) {
      nodes.push_back(std::move(child));
    }
  }
}

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

void jbkv::Save(const Volume& volume, const std::filesystem::path& path) {
  FsSaver saver(path);
  Accept(volume, saver);
}

void jbkv::Load(Volume& volume, const std::filesystem::path& path) {
  FsLoader loader(path);
  Accept(volume, loader);
}
