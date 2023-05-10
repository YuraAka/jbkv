#include "volume.h"
#include <assert.h>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
using namespace jbkv;

const std::string kRootName = "/";

template <typename Parent>
class InvalidNode : public Parent {
 public:
  using typename Parent::List;
  using typename Parent::Name;
  using typename Parent::Ptr;
  static constexpr auto kError = "Node is not valid";

 public:
  Ptr Create(const Name&) override {
    throw std::runtime_error(kError);
  }
  Ptr Find(const Name&) const override {
    throw std::runtime_error(kError);
  }
  bool Unlink(const Name&) override {
    throw std::runtime_error(kError);
  }
  List Enumerate() const override {
    throw std::runtime_error(kError);
  }
  const Name& GetName() const override {
    throw std::runtime_error(kError);
  }
  NodeData::Ptr Open() const override {
    throw std::runtime_error(kError);
  }
  bool IsValid() const override {
    return false;
  }
};

class NullVolumeNode final : public InvalidNode<VolumeNode> {
 public:
  static VolumeNode::Ptr Instance() {
    static auto instance = std::make_shared<NullVolumeNode>();
    return instance;
  }
};

class NullStorageNode final : public InvalidNode<StorageNode> {
 public:
  Ptr Mount(const VolumeNode::Ptr&) const override {
    throw std::runtime_error(kError);
  }

 public:
  static StorageNode::Ptr Instance() {
    static auto instance = std::make_shared<NullStorageNode>();
    return instance;
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

class MountPoint : NonCopyableMovable {
 public:
  using StrongPtr = std::shared_ptr<MountPoint>;
  using WeakPtr = std::weak_ptr<MountPoint>;
  using Unmounter = std::function<void()>;

  MountPoint(const VolumeNode::Ptr& node, const MountPoint::StrongPtr& next)
      : node_(node),
        next_(next) {
  }

  void SetUnmounter(const Unmounter& unmounter) {
    on_unmount_ = unmounter;
  }

  VolumeNode::Ptr GetNode() const {
    return node_;
  }

  ~MountPoint() {
    if (on_unmount_) {
      on_unmount_();
    }
  }

 private:
  VolumeNode::Ptr node_;
  StrongPtr next_;
  Unmounter on_unmount_;
};

class StorageNodeMetadata
    : NonCopyableMovable,
      public std::enable_shared_from_this<StorageNodeMetadata> {
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

  void AddMountPoint(const MountPoint::StrongPtr& mount) {
    std::lock_guard lock(mutex_);
    auto it = mounts_.insert(mounts_.end(), mount);
    auto self = shared_from_this();
    mount->SetUnmounter([self, it]() {
      std::lock_guard lock(self->mutex_);
      self->mounts_.erase(it);
    });
  }

  void GetMountPoints(VolumeNode::List& layers) {
    std::shared_lock lock(mutex_);
    for (const auto& mount_ref : mounts_) {
      auto mount = mount_ref.lock();
      if (mount) {
        layers.push_back(std::move(mount->GetNode()));
      }
    }
  }

 private:
  const StorageNode::Name name_;

  std::shared_mutex mutex_;
  std::unordered_map<StorageNode::Name, StorageNodeMetadata::Ptr> children_;
  std::list<MountPoint::WeakPtr> mounts_;
};

class StorageNodeImpl final : public StorageNode {
 public:
  StorageNodeImpl(const StorageNodeMetadata::Ptr& meta,
                  VolumeNode::List&& layers,
                  MountPoint::StrongPtr&& mount = nullptr)
      : meta_(meta),
        layers_(std::move(layers)),
        mount_(std::move(mount)) {
    if (mount_) {
      meta_->AddMountPoint(mount_);
    }
  }

  /// StorageNode interface
 public:
  Ptr Mount(const VolumeNode::Ptr& node) const override {
    if (!node) {
      throw std::runtime_error("Unable to mount: node is nullptr");
    }

    auto mount = std::make_shared<MountPoint>(node, mount_);
    auto layers = layers_;
    layers.push_back(node);
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
    child_meta->GetMountPoints(child_layers);
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
      meta->GetMountPoints(layers);
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

enum class FormatMarker : uint8_t {
  Double = 0,
  String = 1,
  Blob = 2,
  Bool = 3,
  Char = 4,
  UChar = 5,
  UInt16 = 6,
  Int16 = 7,
  UInt32 = 8,
  Int32 = 9,
  UInt64 = 10,
  Int64 = 11,
  Float = 12
};

constexpr uint8_t kFormatVersion = 1;
constexpr std::string_view kMagic = "jbkv";

void Check(std::ios& stream) {
  if (!stream.good()) {
    throw std::runtime_error("Bad stream");
  }
}

size_t ConvertSize(uint64_t size) {
  if constexpr (sizeof(size_t) != sizeof(size)) {
    if (size > std::numeric_limits<size_t>::max()) {
      throw std::runtime_error("Size is too big");
    }
  }

  return static_cast<size_t>(size);
}

void SerializeHeader(std::ostream& out) {
  Check(out.write(kMagic.data(), kMagic.size()));
  Check(out.write(reinterpret_cast<const char*>(&kFormatVersion),
                  sizeof(kFormatVersion)));
}

void DeserializeHeader(std::string& magic, uint8_t& version, std::istream& in) {
  magic.resize(kMagic.size());
  Check(in.read(&magic[0], magic.size()));
  Check(in.read(reinterpret_cast<char*>(&version), sizeof(version)));
}

void Serialize(const std::string& value, std::ostream& out) {
  uint64_t size = value.size();
  Check(out.write(reinterpret_cast<const char*>(&size), sizeof(size)));
  Check(out.write(value.data(), size));
}

void Deserialize(std::string& value, std::istream& in) {
  uint64_t size = 0;
  Check(in.read(reinterpret_cast<char*>(&size), sizeof(size)));

  const auto native_size = ConvertSize(size);
  value.resize(native_size);
  Check(in.read(&value[0], native_size));
}

void Serialize(const Value::String& value, std::ostream& out) {
  Serialize(value.Ref(), out);
}

void Deserialize(Value::String& value, std::istream& in) {
  Deserialize(value.Ref(), in);
}

void Serialize(const Value::Blob& value, std::ostream& out) {
  uint64_t size = value.Ref().size();
  Check(out.write(reinterpret_cast<const char*>(&size), sizeof(size)));
  Check(out.write(reinterpret_cast<const char*>(&value.Ref()[0]), size));
}

void Deserialize(Value::Blob& value, std::istream& in) {
  uint64_t size = 0;
  Check(in.read(reinterpret_cast<char*>(&size), sizeof(size)));

  const auto native_size = ConvertSize(size);
  value.Ref().resize(native_size);
  Check(in.read(reinterpret_cast<char*>(&value.Ref()[0]), native_size));
}

template <typename T>
  requires std::is_scalar_v<T>
void Serialize(T value, std::ostream& out) {
  Check(out.write(reinterpret_cast<char*>(&value), sizeof(value)));
}

template <typename T>
  requires std::is_scalar_v<T>
void Deserialize(T& value, std::istream& in) {
  Check(in.read(reinterpret_cast<char*>(&value), sizeof(value)));
}

void Serialize(const Value& value, std::ostream& out) {
  value.Accept([&out](const auto& data) {
    using Type = std::remove_cvref_t<decltype(data)>;
    if constexpr (std::is_same_v<Type, bool>) {
      Serialize(FormatMarker::Bool, out);
    } else if constexpr (std::is_same_v<Type, char>) {
      Serialize(FormatMarker::Char, out);
    } else if constexpr (std::is_same_v<Type, unsigned char>) {
      Serialize(FormatMarker::UChar, out);
    } else if constexpr (std::is_same_v<Type, uint16_t>) {
      Serialize(FormatMarker::UInt16, out);
    } else if constexpr (std::is_same_v<Type, int16_t>) {
      Serialize(FormatMarker::Int16, out);
    } else if constexpr (std::is_same_v<Type, uint32_t>) {
      Serialize(FormatMarker::UInt32, out);
    } else if constexpr (std::is_same_v<Type, int32_t>) {
      Serialize(FormatMarker::Int32, out);
    } else if constexpr (std::is_same_v<Type, uint64_t>) {
      Serialize(FormatMarker::UInt64, out);
    } else if constexpr (std::is_same_v<Type, int64_t>) {
      Serialize(FormatMarker::Int64, out);
    } else if constexpr (std::is_same_v<Type, float>) {
      Serialize(FormatMarker::Float, out);
    } else if constexpr (std::is_same_v<Type, double>) {
      Serialize(FormatMarker::Double, out);
    } else if constexpr (std::is_same_v<Type, Value::String>) {
      Serialize(FormatMarker::String, out);
    } else if constexpr (std::is_same_v<Type, Value::Blob>) {
      Serialize(FormatMarker::Blob, out);
    } else {
      static_assert(sizeof(Type) != sizeof(Type), "unknown type");
    }

    Serialize(data, out);
  });
}

void Deserialize(std::optional<Value>& value, std::istream& in) {
  FormatMarker marker;
  Deserialize(marker, in);
  switch (marker) {
    case FormatMarker::Bool: {
      bool data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Char: {
      char data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::UChar: {
      unsigned char data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::UInt16: {
      uint16_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Int16: {
      int16_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::UInt32: {
      uint32_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Int32: {
      int32_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::UInt64: {
      uint64_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Int64: {
      int64_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Float: {
      float data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Double: {
      double data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::String: {
      Value::String data;
      Deserialize(data, in);
      value.emplace(std::move(data));
      break;
    }
    case FormatMarker::Blob: {
      Value::Blob data;
      Deserialize(data, in);
      value.emplace(std::move(data));
      break;
    }
    default:
      throw std::runtime_error("Unknown type marker: " +
                               std::to_string(static_cast<uint8_t>(marker)));
  };
}

template <typename T>
  requires(std::is_same_v<T, Value::String> || std::is_same_v<T, Value::Blob>)
void CheckSum(const T& value, uint8_t& checksum) {
  for (const auto c : value.Ref()) {
    checksum ^= c;
  }
}

template <typename T>
  requires(std::is_same_v<T, std::string>)
void CheckSum(const T& value, uint8_t& checksum) {
  for (const auto c : value) {
    checksum ^= c;
  }
}

template <typename T>
  requires std::is_scalar_v<T>
void CheckSum(T value, uint8_t& checksum) {
  for (size_t i = 0; i < sizeof(value); ++i) {
    checksum ^= *(reinterpret_cast<uint8_t*>(&value) + i);
  }
}

void CheckSum(const Value& value, uint8_t checksum) {
  value.Accept([&checksum](const auto& data) {
    CheckSum(data, checksum);
  });
}

class VolumeSaver {
 public:
  explicit VolumeSaver(std::ostream& stream)
      : stream_(stream) {
    SerializeHeader(stream_);
  }

 public:
  void OnNode(const VolumeNode& node, auto& descendants) {
    auto children = node.Enumerate();
    uint8_t checksum = 0;
    Serialize(children.size(), stream_);
    for (auto&& child : std::move(children)) {
      const auto& name = child->GetName();
      Serialize(name, stream_);
      CheckSum(name, checksum);
      descendants.push_back(std::move(child));
    }

    const auto data = node.Open();
    const auto kv_list = data->Enumerate();
    Serialize(kv_list.size(), stream_);
    for (const auto& [key, value] : kv_list) {
      Serialize(key, stream_);
      Serialize(value, stream_);

      CheckSum(key, checksum);
      CheckSum(value, checksum);
    }

    Serialize(checksum, stream_);
  }

 private:
  std::ostream& stream_;
};

class VolumeLoader {
 public:
  explicit VolumeLoader(std::istream& stream)
      : stream_(stream) {
    std::string magic;
    uint8_t version = 0;
    DeserializeHeader(magic, version, stream_);
    if (magic != kMagic) {
      throw std::runtime_error("Bad file format, magic mismatch: " + magic);
    }

    if (version > kFormatVersion) {
      throw std::runtime_error("File version is too new. Update program!");
    }
  }

  void OnNode(VolumeNode& node, auto& descendants) {
    uint8_t checksum = 0;
    size_t children_count = 0;
    Deserialize(children_count, stream_);
    for (size_t i = 0; i < children_count; ++i) {
      VolumeNode::Name name;
      Deserialize(name, stream_);
      CheckSum(name, checksum);
      auto child = node.Create(name);
      descendants.push_back(std::move(child));
    }

    auto data = node.Open();
    size_t kv_size = 0;
    Deserialize(kv_size, stream_);
    for (size_t i = 0; i < kv_size; ++i) {
      NodeData::Key key;
      std::optional<Value> value;
      Deserialize(key, stream_);
      Deserialize(value, stream_);
      CheckSum(key, checksum);
      CheckSum(*value, checksum);
      data->Write(key, std::move(*value));
    }

    uint8_t stream_checksum = 0;
    Deserialize(stream_checksum, stream_);
    if (checksum != stream_checksum) {
      throw std::runtime_error("Data corrupted");
    }
  }

 private:
  std::istream& stream_;
};

template <typename Visitor>
void Traverse(const VolumeNode::Ptr& root, Visitor&& visitor) {
  std::deque<VolumeNode::Ptr> nodes{root};
  while (!nodes.empty()) {
    auto node = nodes.front();
    nodes.pop_front();
    visitor.OnNode(*node, nodes);
  }
}

}  // namespace

VolumeNode::Ptr jbkv::CreateVolume() {
  return std::make_shared<VolumeNodeImpl>(kRootName);
}

StorageNode::Ptr jbkv::MountStorage(const VolumeNode::Ptr& node) {
  if (!node) {
    throw std::runtime_error("Unable to mount: node is nullptr");
  }

  VolumeNode::List nodes{node};
  const auto meta = StorageNodeMetadata::Create(kRootName);
  return std::make_shared<StorageNodeImpl>(meta, std::move(nodes));
}

void jbkv::Save(const VolumeNode::Ptr& root,
                const std::filesystem::path& path) {
  std::ofstream stream(path, std::ios_base::binary);
  if (!stream.is_open()) {
    throw std::runtime_error("Cannot open file for writing: " + path.string());
  }

  Save(root, stream);
}

void jbkv::Load(const VolumeNode::Ptr& root,
                const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios_base::binary);
  if (!stream.is_open()) {
    throw std::runtime_error("Cannot open file for reading: " + path.string());
  }

  Load(root, stream);
}

void jbkv::Save(const VolumeNode::Ptr& root, std::ostream& stream) {
  if (!root) {
    throw std::runtime_error("Unable to save: root is nullptr");
  }

  VolumeSaver saver(stream);
  Traverse(root, saver);
}

void jbkv::Load(const VolumeNode::Ptr& root, std::istream& stream) {
  if (!root) {
    throw std::runtime_error("Unable to load: root is nullptr");
  }

  VolumeLoader loader(stream);
  Traverse(root, loader);
}
