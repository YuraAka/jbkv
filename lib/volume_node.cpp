#include "volume_node.h"
#include <shared_mutex>
#include <unordered_map>

namespace {
using namespace jbkv;

constexpr auto kRootName = "/";

class NullVolumeNode final : public InvalidNode<VolumeNode> {
 public:
  static VolumeNode::Ptr Instance() {
    return std::make_shared<NullVolumeNode>();
  }
};

class VolumeNodeData final : public NodeData {
 public:
  std::optional<Value> Read(const Key& key) const override {
    std::shared_lock lock(mutex_);
    const auto it = data_.find(key);
    if (it == data_.end()) {
      return std::nullopt;
    }

    return it->second;
  }

  void Write(const Key& key, Value&& value) override {
    std::lock_guard lock(mutex_);
    data_.insert_or_assign(key, std::move(value));
  }

  bool Update(const Key& key, Value&& value) override {
    std::lock_guard lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) {
      return false;
    }

    it->second = std::move(value);
    return true;
  }

  bool Remove(const Key& key) override {
    std::lock_guard lock(mutex_);
    return data_.erase(key) == 1;
  }

  KeyValueList Enumerate() const override {
    std::shared_lock lock(mutex_);
    KeyValueList result;
    result.reserve(data_.size());
    for (const auto& [key, value] : data_) {
      result.push_back({key, value});
    }

    return result;
  }

 private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<Key, Value> data_;
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

  VolumeNode::Ptr Create(const Name& name) override {
    std::lock_guard lock(mutex_);
    auto& child = children_[name];
    if (child) {
      return child;
    }

    child.reset(new VolumeNodeImpl(name));
    return child;
  }

  VolumeNode::Ptr Find(const Name& name) const override {
    std::shared_lock lock(mutex_);
    auto it = children_.find(name);
    if (it == children_.end()) {
      return NullVolumeNode::Instance();
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

  VolumeNode::List Enumerate() const override {
    std::shared_lock lock(mutex_);
    VolumeNode::List result;
    result.reserve(children_.size());
    for (const auto& [_, child] : children_) {
      result.push_back(child);
    }

    return result;
  }

  bool IsValid() const override {
    return true;
  }

 private:
  const Name name_;
  const NodeData::Ptr data_;

  mutable std::shared_mutex mutex_;
  std::unordered_map<Name, Node::Ptr> children_;
};

}  // namespace

VolumeNode::Ptr jbkv::CreateVolume() {
  return std::make_shared<VolumeNodeImpl>(kRootName);
}
