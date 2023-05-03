#include "volume.h"
#include "node.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message_lite.h>
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
using namespace std::string_literals;

const auto kRootName = "/"s;

class VolumeNodeData final : public NodeData {
 public:
  Value Read(const Key& key) const {
    const auto it = data_.find(key);
    if (it == data_.end()) {
      return {};
    }

    return it->second;
  }

  void Write(const Key& key, const Value& value) {
    data_[key] = value;
  }

  bool Remove(const Key& key) {
    const auto it = data_.find(key);
    if (it == data_.end() || !it->second.Has()) {
      return false;
    }

    it->second = Value();
    return true;
  }

  void Accept(const Reader& reader) const {
    for (const auto& [key, value] : data_) {
      if (value.Has()) {
        reader(key, value);
      }
    }
  }

 private:
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

    return links_.back()->Read(key);
  }

  void Write(const Key& key, const Value& value) override {
    auto it = key_map_.find(key);
    if (it != key_map_.end()) {
      it->second->Write(key, value);
    } else {
      // new key scenario -- write to last node
      links_.back()->Write(key, value);
    }
  }

  bool Remove(const Key& key) override {
    auto it = key_map_.find(key);
    if (it != key_map_.end()) {
      return it->second->Remove(key);
    }

    return false;
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

  static KeyNodeMap CreateKeyMap(const NodeData::List& links) {
    /// greedy cache keys for not locking subsequent reads/writes
    KeyNodeMap result;
    for (const auto& link : links) {
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

class ThreadSafeData final : public NodeData {
 public:
  explicit ThreadSafeData(NodeData::Ptr&& impl)
      : impl_(std::move(impl)) {
  }

  Value Read(const Key& key) const {
    std::shared_lock lock(mutex_);
    return impl_->Read(key);
  }

  void Write(const Key& key, const Value& value) {
    std::lock_guard lock(mutex_);
    return impl_->Write(key, value);
  }

  bool Remove(const Key& key) {
    std::lock_guard lock(mutex_);
    return impl_->Remove(key);
  }

  void Accept(const Reader& reader) const {
    std::shared_lock lock(mutex_);
    impl_->Accept(reader);
  }

 public:
  static NodeData::Ptr Wrap(NodeData::Ptr&& data) {
    return std::make_shared<ThreadSafeData>(std::move(data));
  }

 private:
  mutable std::shared_mutex mutex_;
  NodeData::Ptr impl_;
};

class ThreadSafeNode final : public VolumeNode {
 public:
  explicit ThreadSafeNode(VolumeNode::Ptr&& impl)
      : impl_(std::move(impl)) {
  }

  Node::Ptr Create(const Name& name) {
    std::lock_guard lock(mutex_);
    return Wrap(impl_->Create(name));
  }

  Node::Ptr Find(const Name& name) const {
    std::shared_lock lock(mutex_);
    return Wrap(impl_->Find(name));
  }

  bool Unlink(const Name& name) {
    std::lock_guard lock(mutex_);
    return impl_->Unlink(name);
  }

  const Name& GetName() const {
    return impl_->GetName();
  }

  NodeData::Ptr Open() const {
    std::shared_lock lock(mutex_);
    return ThreadSafeData::Wrap(impl_->Open());
  }

 public:
  static VolumeNode::Ptr Wrap(VolumeNode::Ptr&& node) {
    if (!node) {
      return nullptr;
    }

    return std::make_shared<ThreadSafeNode>(std::move(node));
  }

 private:
  mutable std::shared_mutex mutex_;
  VolumeNode::Ptr impl_;
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
    auto& child = children_[name];
    if (child) {
      return child;
    }

    child = std::make_shared<VolumeNodeImpl>(name);
    return child;
  }

  Ptr Find(const Name& name) const override {
    auto it = children_.find(name);
    if (it == children_.end()) {
      return nullptr;
    }

    return it->second;
  }

  bool Unlink(const Name& name) override {
    return children_.erase(name) == 1u;
  }

  NodeData::Ptr Open() const override {
    return data_;
  }

 private:
  const Name name_;
  const NodeData::Ptr data_;

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
    const auto& child_name = links.back()->GetName();
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

    auto link = links_.front()->Create(name);
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
  const Name name_;
  const VolumeNode::List links_;
};

}  // namespace

VolumeNode::Ptr jbkv::CreateVolume() {
  auto root = std::make_shared<VolumeNodeImpl>(kRootName);
  return ThreadSafeNode::Wrap(std::move(root));
}

StorageNode::Ptr jbkv::MountStorage(const VolumeNode::Ptr& node) {
  VolumeNode::List nodes{node};
  return std::make_shared<StorageNodeImpl>(kRootName, std::move(nodes));
}
