#include "volume.h"
#include "node.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message_lite.h>
#include <exception>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <format>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <list>

namespace {
using namespace jbkv;
using namespace std::string_literals;

class StVolumeNode final : public VolumeNode {
 public:
  explicit StVolumeNode(const std::string& name)
      : name_(name) {
  }

  const UniqueId& GetId() const override {
    /// TODO: todo change to unique id
    return name_;
  }

  const Name& GetName() const override {
    return name_;
  }

  Ptr ObtainChild(const std::string& name) override {
    auto& child = children_[name];
    if (child) {
      return child;
    }

    child = std::make_shared<StVolumeNode>(name);
    Notify([this, &child](auto& subscriber) {
      return subscriber.OnChildAdd(*this, child);
    });

    return child;
  }

  Ptr TryChild(const Name& name) const override {
    auto it = children_.find(name);
    if (it == children_.end()) {
      return nullptr;
    }

    return it->second;
  }

  bool DropChild(const Name& name) override {
    auto child = TryChild(name);
    if (child) {
      children_.erase(name);

      Notify([this, &child](auto& subscriber) {
        return subscriber.OnChildDrop(*this, child);
      });

      return true;
    }

    return false;
  }

  void AcceptChildren(const ChildReader& reader) const override {
    for (const auto& [name, node] : children_) {
      reader(name, node);
    }
  }

  void Write(const Key& key, const Value& value) override {
    data_[key] = value;
  }

  Value Read(const Key& key) const override {
    const auto it = data_.find(key);
    if (it == data_.end()) {
      return {};
    }

    return it->second;
  }

  bool Remove(const Key& key) override {
    const auto it = data_.find(key);
    if (it == data_.end() || !it->second.Has()) {
      return false;
    }

    it->second = Value();
    return true;
  }

  void Accept(Reader& reader) const override {
    for (const auto& [name, _] : children_) {
      reader.OnLink(name_, name);
    }

    for (const auto& [key, value] : data_) {
      if (value.Has()) {
        reader.OnData(name_, key, value);
      }
    }
  }

  void Accept(Writer& writer) override {
    writer.OnLink(name_, [this](auto&& child_name) {
      auto& child = children_[child_name];
      child = std::make_shared<StVolumeNode>(std::move(child_name));
    });

    /// if changed, call subscriber
    writer.OnData(name_, [this](auto&& key, auto&& value) {
      data_[std::move(key)] = std::move(value);
    });
  }

  void Subscribe(const SubscriberWeakPtr& subscriber) override {
    subscribers_.insert(subscribers_.end(), subscriber);
  }

 private:
  using Change = std::function<bool(VolumeNodeSubscriber&)>;

  void Notify(const Change& on_change) {
    auto it = subscribers_.begin();
    while (it != subscribers_.end()) {
      auto subscriber = it->lock();
      if (!subscriber || !on_change(*subscriber)) {
        subscribers_.erase(it++);
      }
    }
  }

 private:
  std::string name_;
  std::unordered_map<Name, Node::Ptr> children_;
  std::unordered_map<Key, Value> data_;
  std::list<SubscriberWeakPtr> subscribers_;
};

class StVolume final : public Volume {
 public:
  StVolume()
      : root_(std::make_shared<StVolumeNode>("root")) {
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

    node->AcceptChildren([&nodes](const auto& name, const auto& node) {
      nodes.push_back(node);
    });
  }
}

// todo: where inherit from to prevent misuse?
class StStorageNode final : public StorageNode,
                            public VolumeNodeSubscriber,
                            public std::enable_shared_from_this<StStorageNode> {
 public:
  explicit StStorageNode(const VolumeNode::Ptr& node)
      : name_(node->GetName()) {
    Mount(node);
  }

  /// StorageNode interface
 public:
  void Mount(const VolumeNode::Ptr& node) override {
    /**
     Mount behaves lazily: in that call it only fills up mount-structures to
     register links to volume nodes. Further work will be done on actual access
     to concrete node. It lets optimize time and space for this operation
     because volume tree can be too big and actual accesses to its nodes thru
     storage hierarchy can be too small
     */
    if (mount_map_.contains(node->GetId())) {
      return;
    }

    auto it = mount_links_.insert(mount_links_.end(), node);
    mount_map_[node->GetId()] = it;
  }

  void Unmount(const VolumeNode::Ptr& node) override {
    /**
      Unmount is called in two cases:
      1. client explicitly unmounts previously mounted node
      2. there was a deletion in volume hierarchy that notifies storage
      hierarchy about that

      In either cases Unmount has to recursively remove mount-links to volume
      hierarchy down to unmounted node and that storage nodes which don't
      contain any links.
     */

    UnmountRecursively(this, node);
  }

  /// Node interface
 public:
  StorageNode::Ptr TryChild(const Name& name) override {
    if (mount_links_.empty()) {
      throw std::runtime_error("node is not mounted with any volume");
    }

    auto& child = children_[name];
    if (child) {
      return child;
    }

    for (const auto& link : mount_links_) {
      link->Subscribe(weak_from_this());
    }

    for (const auto& link : mount_links_) {
      auto child_link = link->TryChild(name);
      if (!child_link) {
        continue;
      }

      if (!child) {
        child = std::make_shared<StStorageNode>(child_link);
      } else {
        child->Mount(child_link);
      }
    }

    return child;
  }

  const Name& GetName() const {
    return name_;
  }

  void AcceptChildren(const ChildReader& reader) const override {
    for (const auto& [name, child] : children_) {
      reader(name, child);
    }
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

  /// VolumeNodeSubscriber
 public:
  bool OnChildAdd(const VolumeNode& vparent,
                  const VolumeNode::Ptr& vchild) override {
    if (!mount_map_.contains(vparent.GetId())) {
      return false;
    }

    auto& schild = children_[vchild->GetName()];
    if (!schild) {
      schild = std::make_shared<StStorageNode>(vchild);
    }

    return true;
  }

  bool OnChildDrop(const VolumeNode& vparent,
                   const VolumeNode::Ptr& vchild) override {
    if (!mount_map_.contains(vparent.GetId())) {
      return false;
    }

    auto it = children_.find(vchild->GetName());
    if (it == children_.end()) {
      return true;
    }

    struct PairedNode {
      StorageNode::Ptr snode;
      VolumeNode::Ptr vnode;
    };

    std::deque<PairedNode> nodes{{it->second, vchild}};
    while (!nodes.empty()) {
      auto paired = nodes.front();
      nodes.pop_front();
      paired.snode->Unmount(paired.vnode);
      auto collect = [&paired, &nodes](const auto& name, const auto& schild) {
        auto vchild = paired.vnode->TryChild(name);
        if (vchild) {
          nodes.push_back({schild, vchild});
        }
      };

      paired.snode->AcceptChildren(collect);
    }

    return true;
  }

  bool OnKeyAdd(const VolumeNode& owner, const VolumeNode::Key& key) override {
    return mount_map_.contains(owner.GetId());

    /// todo
  }

  bool OnKeyRemove(const VolumeNode& owner,
                   const VolumeNode::Key& key) override {
    return mount_map_.contains(owner.GetId());
    // todo
  }

 private:
  /// @return true if no mounts is remained
  bool UnmountSelf(const VolumeNode::Ptr& node) {
    auto it = mount_map_.find(node->GetId());
    if (it != mount_map_.end()) {
      mount_links_.erase(it->second);
      mount_map_.erase(it);
    }

    return mount_links_.empty();
  }

  void UnmountRecursively(StStorageNode* sroot, const VolumeNode::Ptr& vroot) {
    struct NodePair {
      StStorageNode* snode = nullptr;
      VolumeNode::Ptr vnode;
    };

    std::deque<NodePair> nodes{{sroot, vroot}};
    while (!nodes.empty()) {
      auto paired = nodes.front();
      nodes.pop_front();
      for (const auto& [name, schild] : paired.snode->children_) {
        auto vchild = paired.vnode->TryChild(name);
        if (vchild) {
          nodes.push_back({schild.get(), vchild});
        }
      }

      if (paired.snode->UnmountSelf(paired.vnode)) {
        /// TODO: remove from parent
      }
    }
  }

 private:
  using VolumeNodeList = std::list<VolumeNode::Ptr>;
  using StStorageNodePtr = std::shared_ptr<StStorageNode>;

  const Name name_;
  VolumeNodeList mount_links_;
  std::unordered_map<VolumeNode::UniqueId, VolumeNodeList::iterator> mount_map_;
  std::unordered_map<StorageNode::Name, StStorageNodePtr> children_;
};

class StStorage final : public Storage {
 public:
  StorageNode::Ptr MountRoot(const VolumeNode::Ptr& node) override {
    // todo in ctor
    if (!root_) {
      root_ = std::make_shared<StStorageNode>(node);
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
