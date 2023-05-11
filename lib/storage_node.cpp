#include "storage_node.h"
#include <list>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

namespace {
using namespace jbkv;

constexpr auto kRootName = "/";

class NullStorageNode final : public InvalidNode<StorageNode> {
 public:
  Ptr Mount(VolumeNode::Ptr) override {
    throw std::runtime_error(kError);
  }

 public:
  static StorageNode::Ptr Instance() {
    return std::make_shared<NullStorageNode>();
  }
};

class StorageNodeData final : public NodeData {
 public:
  explicit StorageNodeData(NodeData::List&& layers)
      : layers_(std::move(layers)) {
    assert(!layers_.empty());
  }

 public:
  std::optional<Value> Read(const Key& key) const override {
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
      const auto& layer = *it;
      auto value = layer->Read(key);
      if (value) {
        return value;
      }
    }

    return std::nullopt;
  }

  void Write(const Key& key, Value&& value) override {
    if (!Update(key, std::move(value))) {
      TopLayer().Write(key, std::move(value));
    }
  }

  bool Update(const Key& key, Value&& value) override {
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
      const auto& layer = *it;
      if (layer->Update(key, std::move(value))) {
        return true;
      }
    }

    return false;
  }

  bool Remove(const Key& key) override {
    bool result = false;
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
      const auto& layer = *it;
      result = layer->Remove(key) || result;
    }

    return result;
  }

  KeyValueList Enumerate() const override {
    KeyValueList result;
    std::unordered_set<NodeData::Key> used;
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
      const auto& layer = *it;
      for (auto&& [key, value] : layer->Enumerate()) {
        if (!used.contains(key)) {
          used.insert(key);
          result.push_back({std::move(key), std::move(value)});
        }
      }
    }

    return result;
  }

 private:
  NodeData& TopLayer() const {
    return *layers_.back();
  }

 private:
  const NodeData::List layers_;
};

struct MountPoint : NonCopyableNonMovable {
 public:
  using StrongPtr = std::shared_ptr<MountPoint>;
  using WeakPtr = std::weak_ptr<MountPoint>;

  explicit MountPoint(VolumeNode::Ptr n)
      : node(std::move(n)) {
  }

  VolumeNode::Ptr node;
};

class StorageNodeMetadata : NonCopyableNonMovable {
 public:
  using Ptr = std::shared_ptr<StorageNodeMetadata>;
  using List = std::vector<Ptr>;

 public:
  explicit StorageNodeMetadata(const StorageNode::Name& name)
      : name_(name) {
  }

 public:
  template <typename... Args>
  static StorageNodeMetadata::Ptr Create(Args&&... args) {
    return std::make_shared<StorageNodeMetadata>(std::forward<Args>(args)...);
  }

  const StorageNode::Name& Name() const {
    return name_;
  }

  StorageNodeMetadata::Ptr GetAddChild(const StorageNode::Name& name) {
    std::shared_lock rlock(mutex_);
    auto it = children_.find(name);
    if (it != children_.end()) {
      return it->second;
    }
    rlock.unlock();

    std::lock_guard wlock(mutex_);
    auto& child = children_[name];
    if (!child) {
      child = StorageNodeMetadata::Create(name);
    }

    return child;
  }

  void RemoveChild(const StorageNode::Name& name) {
    std::lock_guard lock(mutex_);
    children_.erase(name);
  }

  MountPoint::StrongPtr AddMountPoint(VolumeNode::Ptr&& node) {
    std::lock_guard lock(mutex_);
    CleanUpExpired();
    auto mount = std::make_shared<MountPoint>(std::move(node));
    mounts_.insert(mounts_.end(), mount);
    return mount;
  }

  void ListMountPoints(VolumeNode::List& mounts) const {
    std::shared_lock lock(mutex_);
    for (const auto& mount_ref : mounts_) {
      auto mount = mount_ref.lock();
      if (mount) {
        mounts.push_back(mount->node);
      }
    }
  }

 private:
  void CleanUpExpired() {
    auto it = mounts_.begin();
    while (it != mounts_.end()) {
      auto locked = it->lock();
      auto prev_it = it++;
      if (!locked) {
        mounts_.erase(prev_it);
      }
    }
  }

 private:
  const StorageNode::Name name_;

  mutable std::shared_mutex mutex_;
  std::unordered_map<StorageNode::Name, StorageNodeMetadata::Ptr> children_;
  std::list<MountPoint::WeakPtr> mounts_;
};

class StorageNodeImpl final : public StorageNode {
 public:
  explicit StorageNodeImpl(const StorageNodeMetadata::Ptr& meta,
                           VolumeNode::List&& layers = {},
                           MountPoint::StrongPtr&& mount = nullptr)
      : meta_(meta),
        layers_(std::move(layers)),
        mount_(std::move(mount)) {
  }

  /// StorageNode interface
 public:
  Ptr Mount(VolumeNode::Ptr node) override {
    if (!node) {
      throw std::runtime_error("Unable to mount null volume node");
    }

    auto layers = layers_;
    layers.push_back(node);
    auto mount = meta_->AddMountPoint(std::move(node));
    return std::make_shared<StorageNodeImpl>(meta_, std::move(layers),
                                             std::move(mount));
  }

  /// Node interface
 public:
  const Name& GetName() const override {
    return meta_->Name();
  }

  Ptr Create(const Name& name) override {
    auto child = Find(name);
    if (child->IsValid()) {
      return child;
    }

    auto layer = TopLayer().Create(name);
    VolumeNode::List child_layers = {std::move(layer)};
    auto child_meta = meta_->GetAddChild(name);
    /// do not search for mount points, we've already done this on Find
    return std::make_shared<StorageNodeImpl>(std::move(child_meta),
                                             std::move(child_layers));
  }

  StorageNode::Ptr Find(const Name& name) const override {
    VolumeNode::List child_layers;
    child_layers.reserve(layers_.size());
    for (const auto& layer : layers_) {
      auto child = layer->Find(name);
      if (child->IsValid()) {
        child_layers.push_back(std::move(child));
      }
    }
    auto child_meta = meta_->GetAddChild(name);
    child_meta->ListMountPoints(child_layers);
    if (child_layers.empty()) {
      meta_->RemoveChild(name);
      return NullStorageNode::Instance();
    }

    return std::make_shared<StorageNodeImpl>(std::move(child_meta),
                                             std::move(child_layers));
  }

  bool Unlink(const Name& name) override {
    bool result = false;
    for (const auto& layer : layers_) {
      result = layer->Unlink(name) || result;
    }

    meta_->RemoveChild(name);
    return result;
  }

  StorageNode::List Enumerate() const override {
    std::unordered_map<Name, VolumeNode::List> name_groups;
    for (const auto& layer : layers_) {
      for (auto&& child : layer->Enumerate()) {
        const auto& name = child->GetName();
        name_groups[name].push_back(std::move(child));
      }
    }

    StorageNode::List result;
    result.reserve(name_groups.size());
    for (auto&& [name, layers] : name_groups) {
      auto meta = meta_->GetAddChild(name);
      meta->ListMountPoints(layers);
      auto child =
          std::make_shared<StorageNodeImpl>(std::move(meta), std::move(layers));
      result.push_back(std::move(child));
    }

    return result;
  }

  NodeData::Ptr Open() const override {
    NodeData::List layer_data;
    layer_data.reserve(layers_.size());
    for (const auto& layer : layers_) {
      layer_data.push_back(layer->Open());
    }

    return std::make_shared<StorageNodeData>(std::move(layer_data));
  }

  bool IsValid() const override {
    return true;
  }

 private:
  VolumeNode& TopLayer() const {
    return *layers_.back();
  }

 private:
  const StorageNodeMetadata::Ptr meta_;
  const VolumeNode::List layers_;
  const MountPoint::StrongPtr mount_;
};
}  // namespace

StorageNode::Ptr jbkv::MountStorage(VolumeNode::Ptr node) {
  return MountStorage(VolumeNode::List{std::move(node)});
}

StorageNode::Ptr jbkv::MountStorage(VolumeNode::List nodes) {
  auto meta = StorageNodeMetadata::Create(kRootName);
  StorageNode::Ptr root = std::make_shared<StorageNodeImpl>(std::move(meta));
  return root->Mount(std::move(nodes));
}
