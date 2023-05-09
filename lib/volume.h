#pragma once
#include <filesystem>
#include <optional>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <string>
#include <vector>
#include <variant>
#include <list>
#include <iomanip>
#include <iostream>

namespace jbkv {

template <typename T>
class Referenced {
 public:
  Referenced()
      : storage_(std::make_shared<T>()) {
  }

  Referenced(T&& storage)
      : storage_(std::make_shared<T>(std::move(storage))) {
  }

  Referenced(std::initializer_list<typename T::value_type> args)
      : storage_(std::make_shared<T>(std::move(args))) {
  }

  bool operator==(const Referenced<T>& other) const {
    return Ref() == other.Ref();
  }

  bool operator==(const T& other) const {
    return Ref() == other;
  }

  T& Ref() {
    return *storage_;
  }

  const T& Ref() const {
    return *storage_;
  }

 private:
  std::shared_ptr<T> storage_;
};

class Value {
 public:
  using Blob = Referenced<std::vector<uint8_t>>;
  using String = Referenced<std::string>;
  using Data =
      std::variant<bool, char, unsigned char, uint16_t, int16_t, uint32_t,
                   int32_t, uint64_t, int64_t, float, double, String, Blob>;

  explicit Value(Data&& data)
      : data_(std::move(data)) {
    CheckLimits();
  }

  explicit Value(const char* data)
      : data_(String(data)) {
    CheckLimits();
  }

  template <typename T>
  const T* Try() const {
    return std::get_if<T>(&data_);
  }

  void Accept(const auto& visitor) const {
    std::visit(visitor, data_);
  }

 private:
  void CheckLimits() const {
    Accept([](const auto& data) {
      using T = std::remove_cvref_t<decltype(data)>;
      if constexpr (std::is_same_v<T, Value::Blob> ||
                    std::is_same_v<T, Value::String>) {
        if (data.Ref().size() > std::numeric_limits<uint32_t>::max()) {
          throw std::runtime_error(
              "Value is too big which drops x86 platform support: " +
              std::to_string(data.Ref().size()));
        }
      }
    });
  }

 private:
  Data data_;
};

inline std::ostream& operator<<(std::ostream& os, const Value& value) {
  value.Accept([&os](const auto& data) {
    using T = std::remove_cvref_t<decltype(data)>;
    if constexpr (std::is_same_v<T, Value::Blob>) {
      os << std::hex;
      for (const auto byte : data.Ref()) {
        os << byte;
      }

      os << std::resetiosflags(std::ios_base::basefield);
    } else if constexpr (std::is_same_v<T, Value::String>) {
      os << data.Ref();
    } else {
      os << data;
    }
  });
  return os;
}

class NonCopyableMovable {
 public:
  NonCopyableMovable() = default;
  NonCopyableMovable(const NonCopyableMovable&) = delete;
  NonCopyableMovable(NonCopyableMovable&&) = delete;
  NonCopyableMovable& operator=(NonCopyableMovable&&) = delete;
  NonCopyableMovable& operator=(const NonCopyableMovable&) = delete;
};

class NodeData : NonCopyableMovable {
 public:
  using Key = std::string;
  using Value = Value;
  using KeyValueList = std::vector<std::pair<Key, Value>>;
  using Ptr = std::shared_ptr<NodeData>;
  using List = std::vector<Ptr>;

 public:
  virtual ~NodeData() = default;

  /// Reads value by key
  /// @return nullopt if key is not exist, otherwise corresponding value
  virtual std::optional<Value> Read(const Key& key) const = 0;

  /// Writes value by key
  /// @note if key does not exist new entry is created, otherwise value is
  /// updated
  virtual void Write(const Key& key, Value&& value) = 0;

  /// Updates value by key if key exists, otherwise do nothing
  /// @return true if value was updated, otherwise false
  /// @note value is not moved in case Update returned false
  virtual bool Update(const Key& key, Value&& value) = 0;

  /// Removes value by key
  /// @return true if value was deleted, false if valus isn't found
  virtual bool Remove(const Key& key) = 0;

  /// List data entries (key=value)
  virtual KeyValueList Enumerate() const = 0;

  /// helpers
 public:
  template <typename T>
  void Write(const Key& key, const T& value) {
    Write(key, Value(value));
  }

  template <typename T>
  bool Update(const Key& key, const T& value) {
    return Update(key, Value(value));
  }

  /// monadic Read
  template <typename T>
  std::optional<T> Read(const Key& key) {
    const auto value = Read(key);
    if (!value) {
      return std::nullopt;
    }

    const auto* mb_data = value->Try<T>();
    if (!mb_data) {
      return std::nullopt;
    }

    return *mb_data;
  }
};

template <typename NodeFamily>
class Node : NonCopyableMovable {
 public:
  using Name = std::string;
  using Path = std::vector<Name>;
  using Ptr = std::shared_ptr<NodeFamily>;
  using List = std::vector<Ptr>;

 public:
  virtual ~Node() = default;

  /// @brief Creates new or returns existing one
  /// @return non-null pointer to node
  virtual Node::Ptr Create(const Name& name) = 0;

  /// @brief Searches node by name amoung children
  /// @return invalid node (IsValid returns false) if node is not found,
  /// otherwise pointer to node
  virtual Node::Ptr Find(const Name& name) const = 0;

  /// @brief Removes link to child node by name
  /// @return false if no child is found by given name, otherwise true
  /// @note node remains alive until last strong link on it
  virtual bool Unlink(const Name& name) = 0;

  /// @brief List children nodes
  virtual List Enumerate() const = 0;

  /// @return name of current node
  virtual const Name& GetName() const = 0;

  /// @brief Retrieves data for current node
  virtual NodeData::Ptr Open() const = 0;

  /// Returns false if node not exist, otherwise true
  virtual bool IsValid() const = 0;
};

class VolumeNode : public Node<VolumeNode> {};

/// Represents a virtual node that maps on some Volume nodes
/// Mapping is done either by mounting some volume nodes or by linking children
/// of mounted node
/// Structure of node has layered structure where bottom layers consists of
/// co-named children, and upper layers built from mount-points to current node
/// Layers are fixed at the moment of node materialization and stays constant
/// for all node life
/// Materialization is happening on node Create/Find/Mount methods
/// Mounts lives until explicit unmount (see below) and affect for future
/// materialization by the same path
class StorageNode : public Node<StorageNode> {
 public:
  /// @brief Mounts current node with subtree with root in given node
  /// @param node root of subtree to link
  /// @return new node with top-layer as given node
  /// @note mount will be valid and globally visible for all result life
  /// @note mount does not make side-effects to current node
  [[nodiscard]] virtual Ptr Mount(const VolumeNode::Ptr& node) const = 0;
};

VolumeNode::Ptr CreateVolume();
StorageNode::Ptr MountStorage(const VolumeNode::Ptr& node);

void Save(const VolumeNode::Ptr& root, const std::filesystem::path& path);
void Load(const VolumeNode::Ptr& root, const std::filesystem::path& path);

}  // namespace jbkv
