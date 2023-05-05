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

enum class FormatMarker : uint8_t {
  Name = 0,
  Data = 1,
  Child = 2,
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
  Blob = 15
};

/// todo catch error on compilation
template <typename Type>
FormatMarker TypeToMarker() {
  if constexpr (std::is_same_v<Type, bool>) {
    return FormatMarker::Bool;
  } else if constexpr (std::is_same_v<Type, char>) {
    return FormatMarker::Char;
  } else if constexpr (std::is_same_v<Type, unsigned char>) {
    return FormatMarker::UChar;
  } else if constexpr (std::is_same_v<Type, uint16_t>) {
    return FormatMarker::UInt16;
  } else if constexpr (std::is_same_v<Type, int16_t>) {
    return FormatMarker::Int16;
  } else if constexpr (std::is_same_v<Type, uint32_t>) {
    return FormatMarker::UInt32;
  } else if constexpr (std::is_same_v<Type, int32_t>) {
    return FormatMarker::Int32;
  } else if constexpr (std::is_same_v<Type, uint64_t>) {
    return FormatMarker::UInt64;
  } else if constexpr (std::is_same_v<Type, int64_t>) {
    return FormatMarker::Int64;
  } else if constexpr (std::is_same_v<Type, float>) {
    return FormatMarker::Float;
  } else if constexpr (std::is_same_v<Type, double>) {
    return FormatMarker::Double;
  } else if constexpr (std::is_same_v<Type, std::string>) {
    return FormatMarker::String;
  } else if constexpr (std::is_same_v<Type, Value::Blob>) {
    return FormatMarker::Blob;
  } else {
    throw std::runtime_error("Unknown type" + std::string(typeid(Type).name()));
  }
}

/// todo make limitations for len of string & blob

constexpr uint8_t kFormatVersion = 1;
constexpr std::string_view kMagic = "jbkv";

void SerializeHeader(std::ostream& out) {
  out.write(kMagic.data(), kMagic.size());
  out.write(reinterpret_cast<const char*>(&kFormatVersion),
            sizeof(kFormatVersion));
}

void DeserializeHeader(std::string& value, std::istream& in) {
  /// todo
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

class VolumeSaver {
 public:
  explicit VolumeSaver(const std::filesystem::path& path)
      : stream_(path, std::ios_base::binary) {
    if (!stream_) {
      throw std::runtime_error("Cannot open file '{}'" + std::string(path));
    }

    SerializeHeader(stream_);
  }

  ~VolumeSaver() {
    stream_.close();
  }

  void OnNode(const VolumeNode::Name& name) {
    Serialize(FormatMarker::Name, stream_);
    Serialize(name, stream_);
    Serialize(FormatMarker::Data, stream_);
  }

  void OnData(const NodeData::Key& key, const NodeData::Value& value) {
    Serialize(key, stream_);
    value.Accept([this](const auto& typed_value) {
      using Type = std::remove_cvref_t<decltype(typed_value)>;
      if constexpr (!std::is_same_v<Type, std::monostate>) {
        const auto marker = TypeToMarker<Type>();
        Serialize(marker, stream_);
        Serialize(typed_value, stream_);
      }
    });
  }

  void OnChild(const VolumeNode::Name& name) {
    (void)name;
  }

 private:
  std::ofstream stream_;
};

class VolumeLoader {
 public:
  explicit VolumeLoader(const std::filesystem::path& path)
      : stream_(path, std::ios_base::binary) {
    std::string magic;
    Deserialize(magic, stream_);
    if (magic != kMagic) {
      throw std::runtime_error("Unknown file format");
    }

    uint16_t version = 0;
    Deserialize(version, stream_);
    if (version != kFormatVersion) {
      throw std::runtime_error("File version is not supported: " +
                               std::to_string(version));
    }
  }

 private:
  std::ifstream stream_;
};

template <typename Visitor>
void Accept(const VolumeNode::Ptr& root, Visitor&& visitor) {
  std::deque<VolumeNode::Ptr> nodes{root};
  while (!nodes.empty()) {
    auto node = nodes.front();
    nodes.pop_front();
    visitor.OnNode(node->GetName());
    node->Open()->Accept([&visitor](const auto& key, const auto& value) {
      visitor.OnData(key, value);
    });

    /*node->Accept([&nodes, &visitor](const auto& node) {
      visitor.OnChild(node->GetName());
      nodes.push_back(node);
    });*/
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
  Accept(root, saver);
}

/*void jbkv::Load(VolumeNode& volume, const std::filesystem::path& path) {
  FsLoader loader(path);
  Accept(volume.Root(), loader);
}*/
