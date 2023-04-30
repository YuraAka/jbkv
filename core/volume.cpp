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
#include <unordered_set>
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

class StNode final : public VolumeNode {
 public:
  explicit StNode(const std::string& name)
      : name_(name) {
  }

  Node::Ptr AddNode(const std::string& name) override {
    if (nodes_.contains(name)) {
      throw std::runtime_error("Node with name '" + name + "' already exists");
    }

    auto& child = nodes_[name];
    ++node_version_;
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

  bool RemoveNode(const Name& name) override {
    if (nodes_.erase(name) == 1u) {
      ++node_version_;
      return true;
    }

    return false;
  }

  const Name& GetName() const override {
    return name_;
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

  Version GetDataVersion() const override {
    return data_version_;
  }

  Version GetNodeVersion() const override {
    return node_version_;
  }

  void Write(const Key& key, const Value& value) override {
    ++data_version_;
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

    ++data_version_;
    it->second = Value();
    return true;
  }

  void Accept(Reader& reader) const override {
    for (const auto& [name, _] : nodes_) {
      reader.OnLink(name_, name);
    }

    for (const auto& [key, value] : kv_) {
      if (value.Has()) {
        reader.OnData(name_, key, value);
      }
    }
  }

  void Accept(Writer& writer) override {
    writer.OnLink(name_, [this](auto&& child_name) {
      auto& child = nodes_[child_name];
      child = std::make_shared<StNode>(std::move(child_name));
    });

    writer.OnData(name_, [this](auto&& key, auto&& value) {
      kv_[std::move(key)] = std::move(value);
    });

    ++data_version_;
    ++node_version_;
  }

 private:
  std::string name_;
  std::unordered_map<Name, Node::Ptr> nodes_;
  std::unordered_map<Key, Value> kv_;
  Version data_version_ = 0;
  Version node_version_ = 0;
};

class StVolume final : public Volume {
 public:
  StVolume()
      : root_(std::make_shared<StNode>("root")) {
  }

 public:
  VolumeNode::Ptr Root() const override {
    return root_;
  }

 private:
  VolumeNode::Ptr root_;
};

class FsSaver final : public VolumeNode::Reader {
 public:
  explicit FsSaver(const std::filesystem::path& path) {
    std::cout << "Saving to " << path << std::endl;
  }

  void OnLink(const NodeName& parent, const NodeName& child) override {
    std::cout << "Saving parent-child: " << parent << "->" << child
              << std::endl;
  }

  void OnData(const NodeName& name, const NodeKey& key,
              const Value& value) override {
    std::cout << "- saving node '" << name << "', " << key << " = " << value
              << std::endl;
  }
};

class FsLoader final : public VolumeNode::Writer {
 public:
  explicit FsLoader(const std::filesystem::path& path)
      : hierarchy_{{"root", {"d1", "d2"}}, {"d1", {"d3", "d4"}}},
        data_{{"d1", {{"k1", "v1"}, {"k2", "v2"}}},
              {"d4", {{"k3", "v3"}, {"k4", "v4"}}}} {
    std::cout << "Loading from " << path << std::endl;
  }

  void OnData(const NodeName& self, const DataCallback& cb) {
    auto dit = data_.find(self);
    if (dit != data_.end()) {
      for (auto&& [key, value] : dit->second) {
        Value val(std::move(value));
        cb(std::move(key), std::move(val));
      }
    }
  }

  void OnLink(const NodeName& self, const LinkCallback& cb) {
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

template <typename NodeType, typename U>
void Accept(const std::shared_ptr<NodeType>& root, U& visitor) {
  std::deque<typename NodeType::Ptr> nodes{root};
  while (!nodes.empty()) {
    auto node = nodes.front();
    nodes.pop_front();
    node->Accept(visitor);
    for (auto&& child : node->ListNodes()) {
      nodes.push_back(std::move(child));
    }
  }
}

class StStorageNode final : public StorageNode {
 public:
  explicit StStorageNode(const VolumeNode::Ptr& node)
      : mount_points_{node} {
  }

 public:
  void Mount(const VolumeNode::Ptr& node) override {
    mount_points_.push_back(node);
    // todo fill in children
    // warning: recursive calls can be!!!
  }

 public:
  const Name& GetName() const override {
    return Primary().GetName();
  }

  StorageNode::Ptr AddNode(const Name& name) override {
    if (nodes_.contains(name)) {
      throw std::runtime_error("node already exists");
    }

    /// case: volume has been changes from another storage
    auto mount_point = Primary().AddNode(name);
    auto& child = nodes_[name];
    child = std::make_shared<StStorageNode>(mount_point);
    return child;
  }

  StorageNode::Ptr GetNode(const Name& name) const override {
    auto it = nodes_.find(name);
    if (it == nodes_.end()) {
      return nullptr;
    }

    return it->second;
  }

  bool RemoveNode(const Name& name) override {
    // remove only from storage
    return nodes_.erase(name) == 1u;
  }

  StorageNode::List ListNodes() const override {
    StorageNode::List result;
    result.reserve(nodes_.size());
    for (const auto& [_, node] : nodes_) {
      result.push_back(node);
    }

    return result;
  }

  void Write(const Key& key, const Value& value) override {
    // search where to write?
    // make key -> node mapping at the moment of mount
    // OR search key linearly by all nodes, if not found, write to primary
  }

  Value Read(const Key& key) const override {
    // same as write OR read until found
    return {};
  }

  bool Remove(const Key& key) override {
    // todo
    return false;
  }

  void Accept(Reader&) const override {
  }

  void Accept(Writer&) override {
  }

 private:
  void LazyInit() const {
  }

  VolumeNode& Primary() const {
    assert(!mount_points_.empty());
    return *mount_points_.front();
  }

 private:
  VolumeNode::List mount_points_;
  mutable std::unordered_map<StorageNode::Name, StorageNode::Ptr> nodes_;
  std::unordered_set<VolumeNode::Name> removed_;
};

class StStorage final : public Storage {
 public:
  StorageNode::Ptr MountRoot(const VolumeNode::Ptr& node) override {
    if (!root_) {
      root_ = std::make_shared<StStorageNode>(node);
    } else {
      root_->Mount(node);
    }

    return root_;
  }

 private:
  StorageNode::Ptr root_;
};

}  // namespace

Volume::Ptr Volume::CreateSingleThreaded() {
  return std::make_shared<StVolume>();
}

void jbkv::Save(const Volume& volume, const std::filesystem::path& path) {
  FsSaver saver(path);
  Accept(volume.Root(), saver);
}

void jbkv::Load(Volume& volume, const std::filesystem::path& path) {
  FsLoader loader(path);
  Accept(volume.Root(), loader);
}

Storage::Ptr Storage::CreateSingleThreaded() {
  return std::make_shared<StStorage>();
}
