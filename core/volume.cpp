#include "volume.h"
#include <exception>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <format>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <list>
#include <shared_mutex>
// todo clear includes

namespace {
using namespace jbkv;

const std::string kRootName = "/";

class VolumeNodeData final : public NodeData {
 public:
  Value Read(const Key& key) const {
    std::shared_lock lock(mutex_);
    const auto it = data_.find(key);
    if (it == data_.end()) {
      return {};
    }

    return it->second;
  }

  void Write(const Key& key, const Value& value) {
    std::lock_guard lock(mutex_);
    data_[key] = value;
  }

  bool Remove(const Key& key) {
    std::shared_lock lock(mutex_);
    const auto it = data_.find(key);
    if (it == data_.end() || !it->second.Has()) {
      return false;
    }

    it->second = Value();
    return true;
  }

  void Accept(const Reader& reader) const {
    std::shared_lock lock(mutex_);
    for (const auto& [key, value] : data_) {
      if (value.Has()) {
        reader(key, value);
      }
    }
  }

 private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<Key, Value> data_;
};

class StorageNodeData final : public NodeData {
 public:
  explicit StorageNodeData(NodeData::List&& links)
      : links_(std::move(links)),
        key_map_(CreateKeyMap(links_)) {
    assert(!links_.empty());
  }

 public:
  Value Read(const Key& key) const override {
    auto it = key_map_.find(key);
    if (it != key_map_.end()) {
      return it->second->Read(key);
    }

    return PriorityLink().Read(key);
  }

  void Write(const Key& key, const Value& value) override {
    auto it = key_map_.find(key);
    if (it != key_map_.end()) {
      it->second->Write(key, value);
    } else {
      // new key scenario -- write to last node
      PriorityLink().Write(key, value);
    }
  }

  bool Remove(const Key& key) override {
    auto it = key_map_.find(key);
    if (it != key_map_.end()) {
      return it->second->Remove(key);
    }

    return PriorityLink().Remove(key);
  }

  void Accept(const Reader& reader) const override {
    for (const auto& [key, node] : key_map_) {
      const auto value = node->Read(key);
      if (value.Has()) {
        reader(key, value);
      }
    }
  }

 private:
  using KeyNodeMap = std::unordered_map<Key, NodeData*>;

  NodeData& PriorityLink() const {
    return *links_.back();
  }

  static KeyNodeMap CreateKeyMap(const NodeData::List& links) {
    KeyNodeMap result;
    for (auto it = links.rbegin(); it != links.rend(); ++it) {
      const auto& link = *it;
      link->Accept([&result, &link](const auto& key, const auto& value) {
        auto& node = result[key];
        if (!node) {
          node = link.get();
        }
      });
    }

    return result;
  }

 private:
  const NodeData::List links_;
  const KeyNodeMap key_map_;
};

class VolumeNodeImpl final : public VolumeNode {
 public:
  explicit VolumeNodeImpl(const Name& name)
      : name_(name),
        data_(std::make_shared<VolumeNodeData>()) {
  }

  const Name& GetName() const override {
    return name_;
  }

  Ptr Create(const Name& name) override {
    std::lock_guard lock(mutex_);
    auto& child = children_[name];
    if (child) {
      return child;
    }

    child = std::make_shared<VolumeNodeImpl>(name);
    return child;
  }

  Ptr Find(const Name& name) const override {
    std::shared_lock lock(mutex_);
    auto it = children_.find(name);
    if (it == children_.end()) {
      return nullptr;
    }

    return it->second;
  }

  bool Unlink(const Name& name) override {
    std::lock_guard lock(mutex_);
    return children_.erase(name) == 1u;
  }

  NodeData::Ptr Open() const override {
    return data_;
  }

 private:
  const Name name_;
  const NodeData::Ptr data_;

  mutable std::shared_mutex mutex_;
  std::unordered_map<Name, Node::Ptr> children_;
};

class StorageNodeImpl final : public StorageNode {
 public:
  StorageNodeImpl(const Name& name, VolumeNode::List&& links)
      : name_(name),
        links_(std::move(links)) {
  }

  /// StorageNode interface
 public:
  StorageNode::Ptr Mount(const VolumeNode::Ptr& node) override {
    auto links = links_;
    links.push_back(node);
    const auto& child_name = PriorityLink().GetName();
    return std::make_shared<StorageNodeImpl>(child_name, std::move(links));
  }

  /// Node interface
 public:
  const Name& GetName() const override {
    return name_;
  }

  Ptr Create(const Name& name) override {
    auto child = Find(name);
    if (child) {
      return child;
    }

    auto link = PriorityLink().Create(name);
    VolumeNode::List children_links = {std::move(link)};
    return std::make_shared<StorageNodeImpl>(name, std::move(children_links));
  }

  Ptr Find(const Name& name) const override {
    VolumeNode::List children_links;
    children_links.reserve(links_.size());
    for (const auto& link : links_) {
      auto child = link->Find(name);
      if (child) {
        children_links.push_back(std::move(child));
      }
    }

    if (children_links.empty()) {
      return nullptr;
    }

    return std::make_shared<StorageNodeImpl>(name, std::move(children_links));
  }

  bool Unlink(const Name& name) override {
    bool result = false;
    for (const auto& link : links_) {
      result = link->Unlink(name) || result;
    }

    return result;
  }

  NodeData::Ptr Open() const override {
    NodeData::List link_data;
    link_data.reserve(links_.size());
    for (const auto& link : links_) {
      link_data.push_back(link->Open());
    }

    return std::make_shared<StorageNodeData>(std::move(link_data));
  }

 private:
  VolumeNode& PriorityLink() const {
    return *links_.back();
  }

 private:
  const Name name_;
  const VolumeNode::List links_;
};

}  // namespace

VolumeNode::Ptr jbkv::CreateVolume() {
  return std::make_shared<VolumeNodeImpl>(kRootName);
}

StorageNode::Ptr jbkv::MountStorage(const VolumeNode::Ptr& node) {
  VolumeNode::List nodes{node};
  return std::make_shared<StorageNodeImpl>(kRootName, std::move(nodes));
}
