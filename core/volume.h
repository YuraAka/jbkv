#pragma once
#include <_types/_uint32_t.h>
#include <filesystem>
#include <optional>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <string>
#include <vector>
#include <variant>

namespace jbkv {

class Value {
 public:
  /// todo fill another types
  using Data = std::variant<std::monostate, uint32_t, std::string>;

  Value()
      : data_(std::monostate()) {
  }

  explicit Value(Data&& data)
      : data_(std::move(data)) {
  }

  Value(Value&& data) = default;
  Value(const Value& data) = default;
  Value& operator=(Value&& value) = default;
  Value& operator=(const Value& value) = default;

  template <typename T>
  const T& As() const {
    auto data_ptr = Try<T>();
    if (!data_ptr) {
      throw std::runtime_error("cannot get value of given type");
    }

    return *data_ptr;
  }

  template <typename T>
  const T* Try() const {
    if (!Has()) {
      /// empty value
      return nullptr;
    }

    return std::get_if<T>(&data_);
  }

  bool Has() const {
    return std::get_if<0>(&data_) == nullptr;
  }

  void Accept(const auto& visitor) const {
    std::visit(visitor, data_);
  }

 private:
  Data data_;
};

inline std::ostream& operator<<(std::ostream& os, const Value& value) {
  value.Accept([&os](const auto& data) {
    using T = std::decay_t<decltype(data)>;
    if constexpr (std::is_same_v<T, std::monostate>) {
    } else {
      os << data;
    }
  });
  return os;
}

template <typename NodeType>
class NodeReader;

template <typename NodeType>
class NodeWriter;

// todo: docs, exception specs
// todo non-copyable, non-movable

// template <Tag tag>
template <typename NodeType>
class Node {
 public:
  using Name = std::string;
  using Path = std::vector<Name>;
  using Ptr = std::shared_ptr<NodeType>;
  using List = std::vector<Ptr>;
  using Key = std::string;
  using Value = Value;
  using Version = uint32_t;  // todo platform specific
  using Reader = NodeReader<NodeType>;
  using Writer = NodeWriter<NodeType>;

 public:
  virtual ~Node() = default;

  virtual Ptr AddNode(const Name& name) = 0;
  virtual Ptr GetNode(const Name& name) const = 0;
  virtual bool RemoveNode(const Name& name) = 0;
  virtual List ListNodes() const = 0;
  virtual const Name& GetName() const = 0;

  virtual void Write(const Key& key, const Value& value) = 0;
  virtual Value Read(const Key& key) const = 0;
  virtual bool Remove(const Key& key) = 0;

  virtual void Accept(Reader& reader) const = 0;
  virtual void Accept(Writer& writer) = 0;

  /// helpers
 public:
  template <typename T>
  void Write(const Key& key, const T& value) {
    Write(key, Value(value));
  }

  template <typename T>
  std::optional<T> Read(const Key& key) {
    const auto& value_ptr = Read(key).template Try<T>();
    if (!value_ptr) {
      return std::nullopt;
    }

    return *value_ptr;
  }

  static Ptr FindByPath(const Ptr& from, const Path& path) {
    auto cur = from;
    for (const auto& name : path) {
      if (!cur) {
        return nullptr;
      }

      cur = cur->GetNode(name);
    }

    return cur;
  }
};

class VolumeNode : public Node<VolumeNode> {
 public:
  virtual Version GetDataVersion() const = 0;
  virtual Version GetNodeVersion() const = 0;  // todo can be aba
};

template <typename NodeType>
class NodeReader {
 public:
  using NodeName = typename NodeType::Name;
  using NodeValue = typename NodeType::Value;
  using NodeKey = typename NodeType::Key;

 public:
  virtual ~NodeReader() = default;

  virtual void OnLink(const NodeName& self, const NodeName& child) = 0;
  virtual void OnData(const NodeName& self, const NodeName& key,
                      const NodeValue& value) = 0;
};

template <typename NodeType>
class NodeWriter {
 public:
  using NodeName = typename NodeType::Name;
  using NodeValue = typename NodeType::Value;
  using NodeKey = typename NodeType::Key;

  /// @todo to avoid dynamic allocation fu2::function_view can be used
  using LinkCallback = std::function<void(NodeName&&)>;
  using DataCallback = std::function<void(NodeKey&&, NodeValue&&)>;

 public:
  virtual ~NodeWriter() = default;
  virtual void OnLink(const NodeName& self, const LinkCallback& cb) = 0;
  virtual void OnData(const NodeName& self, const DataCallback& cb) = 0;
};

class Volume {
 public:
  using Ptr = std::shared_ptr<Volume>;

 public:
  virtual ~Volume() = default;
  virtual VolumeNode::Ptr Root() const = 0;

 public:
  static Volume::Ptr CreateSingleThreaded();
};

void Save(const Volume& volume, const std::filesystem::path& path);
void Load(Volume& volume, const std::filesystem::path& path);

class StorageNode : public Node<StorageNode> {
 public:
  using Ptr = std::shared_ptr<StorageNode>;

 public:
  virtual void Mount(const VolumeNode::Ptr& node) = 0;
};

class Storage {
 public:
  using Ptr = std::shared_ptr<Storage>;

 public:
  virtual ~Storage() = default;
  virtual StorageNode::Ptr MountRoot(const VolumeNode::Ptr& node) = 0;

 public:
  static Storage::Ptr CreateSingleThreaded();
};

}  // namespace jbkv
