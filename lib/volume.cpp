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
#include <iostream>
// todo clear includes

namespace {
using namespace jbkv;

const std::string kRootName = "/";

class VolumeNodeData final : public NodeData {
 public:
  Value Read(const Key& key) const override {
    std::shared_lock lock(mutex_);
    const auto it = data_.find(key);
    if (it == data_.end()) {
      return {};
    }

    return it->second;
  }

  void Write(const Key& key, Value&& value) override {
    std::lock_guard lock(mutex_);
    data_[key] = std::move(value);
  }

  bool Remove(const Key& key) override {
    std::shared_lock lock(mutex_);
    const auto it = data_.find(key);
    if (it == data_.end() || !it->second.Has()) {
      return false;
    }

    it->second = Value();
    return true;
  }

  KeyValueList Enumerate() const override {
    std::shared_lock lock(mutex_);
    KeyValueList result;
    result.reserve(data_.size());
    for (const auto& [key, value] : data_) {
      if (value.Has()) {
        result.push_back({key, value});
      }
    }

    return result;
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

  void Write(const Key& key, Value&& value) override {
    auto it = key_map_.find(key);
    if (it != key_map_.end()) {
      it->second->Write(key, std::move(value));
    } else {
      // new key scenario -- write to last node
      PriorityLink().Write(key, std::move(value));
    }
  }

  bool Remove(const Key& key) override {
    auto it = key_map_.find(key);
    if (it != key_map_.end()) {
      return it->second->Remove(key);
    }

    return PriorityLink().Remove(key);
  }

  KeyValueList Enumerate() const override {
    /// todo
    return {};
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
      for (auto&& [key, value] : link->Enumerate()) {
        auto& node = result[key];
        if (node == nullptr) {
          node = link.get();
        }
      }
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

  List Enumerate() const override {
    std::shared_lock lock(mutex_);
    List result;
    result.reserve(children_.size());
    for (const auto& [_, child] : children_) {
      result.push_back(child);
    }

    return result;
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

  List Enumerate() const override {
    /// todo support
    return {};
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

enum class FormatMarker : uint8_t {
  Bool = 3,
  Char = 4,
  UChar = 5,
  UInt16 = 6,
  Int16 = 7,
  UInt32 = 8,
  Int32 = 9,
  UInt64 = 10,
  Int64 = 11,
  Float = 12,
  Double = 13,
  String = 14,
  Blob = 15,
};

/// todo catch error on compilation

constexpr uint8_t kFormatVersion = 1;
constexpr std::string_view kMagic = "jbkv";

void SerializeHeader(std::ostream& out) {
  out.write(kMagic.data(), kMagic.size());
  out.write(reinterpret_cast<const char*>(&kFormatVersion),
            sizeof(kFormatVersion));
}

void DeserializeHeader(std::string& magic, uint8_t& version, std::istream& in) {
  magic.resize(kMagic.size());
  in.read(&magic[0], magic.size());
  in.read(reinterpret_cast<char*>(&version), sizeof(version));
}

void Serialize(const std::string& value, std::ostream& out) {
  /// todo ensure string is utf-8 decoded for all platforms (Windows
  /// especially)
  uint64_t size = value.size();
  out.write(reinterpret_cast<const char*>(&size), sizeof(size));
  out.write(value.data(), size);
}

void Deserialize(std::string& value, std::istream& in) {
  uint64_t size = 0;
  in.read(reinterpret_cast<char*>(&size), sizeof(size));
  value.resize(size);
  in.read(&value[0], size);
}

void Serialize(const Value::Blob& value, std::ostream& out) {
  uint64_t size = value.size();
  out.write(reinterpret_cast<const char*>(&size), sizeof(size));
  out.write(reinterpret_cast<const char*>(&value[0]), size);
}

void Deserialize(Value::Blob& value, std::istream& in) {
  uint64_t size = 0;
  in.read(reinterpret_cast<char*>(&size), sizeof(size));
  value.resize(size);
  in.read(reinterpret_cast<char*>(&value[0]), size);
}

template <typename T>
  requires std::is_scalar_v<T>
void Serialize(T value, std::ostream& out) {
  out.write(reinterpret_cast<char*>(&value), sizeof(value));
}

template <typename T>
  requires std::is_scalar_v<T>
void Deserialize(T& value, std::istream& in) {
  in.read(reinterpret_cast<char*>(&value), sizeof(value));
}

void Serialize(std::monostate, std::ostream&) {
  assert(false && "unreachable");
}

template <typename T>
constexpr bool kAlwaysFalse = false;

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
    } else if constexpr (std::is_same_v<Type, std::string>) {
      Serialize(FormatMarker::String, out);
    } else if constexpr (std::is_same_v<Type, Value::Blob>) {
      Serialize(FormatMarker::Blob, out);
    } else if constexpr (std::is_same_v<Type, std::monostate>) {
      return;
    } else {
      static_assert(kAlwaysFalse<Type>, "unknown type");
    }

    Serialize(data, out);
  });
}

void Deserialize(Value& value, std::istream& in) {
  FormatMarker marker;
  Deserialize(marker, in);
  switch (marker) {
    case FormatMarker::Bool: {
      bool data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::Char: {
      char data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::UChar: {
      unsigned char data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::UInt16: {
      uint16_t data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::Int16: {
      int16_t data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::UInt32: {
      uint32_t data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::Int32: {
      int32_t data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::UInt64: {
      uint64_t data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::Int64: {
      int64_t data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::Float: {
      float data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::Double: {
      double data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::String: {
      std::string data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    case FormatMarker::Blob: {
      Value::Blob data = {};
      Deserialize(data, in);
      value = Value(data);
      break;
    }
    default:
      throw std::runtime_error("Unknown type marker: " +
                               std::to_string(static_cast<uint8_t>(marker)));
  };
}

template <typename T>
  requires(std::is_same_v<T, std::string> || std::is_same_v<T, Value::Blob>)
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

/// todo think about how to avoid monostate handling?
void CheckSum(std::monostate, uint8_t&) {
  assert(false && "unreachable");
}

void CheckSum(const Value& value, uint8_t checksum) {
  value.Accept([&checksum](const auto& data) {
    CheckSum(data, checksum);
  });
}

class VolumeSaver {
 public:
  explicit VolumeSaver(const std::filesystem::path& path)
      : stream_(path, std::ios_base::binary) {
    if (!stream_) {
      throw std::runtime_error("Cannot open file '{}'" + std::string(path));
    }

    SerializeHeader(stream_);
    /// todo calc check sum after each node
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
  std::ofstream stream_;
};

class VolumeLoader {
 public:
  explicit VolumeLoader(const std::filesystem::path& path)
      : stream_(path, std::ios_base::binary) {
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
      NodeData::Value value;
      Deserialize(key, stream_);
      Deserialize(value, stream_);
      data->Write(key, std::move(value));
      CheckSum(key, checksum);
      CheckSum(value, checksum);
    }

    uint8_t actual_checksum = 0;
    Deserialize(actual_checksum, stream_);
    if (checksum != actual_checksum) {
      throw std::runtime_error("File corrupted");
    }
  }

 private:
  std::ifstream stream_;
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
  VolumeNode::List nodes{node};
  return std::make_shared<StorageNodeImpl>(kRootName, std::move(nodes));
}

void jbkv::Save(const VolumeNode::Ptr& root,
                const std::filesystem::path& path) {
  VolumeSaver saver(path);
  Traverse(root, saver);
}

void jbkv::Load(const VolumeNode::Ptr& root,
                const std::filesystem::path& path) {
  VolumeLoader loader(path);
  Traverse(root, loader);
}
