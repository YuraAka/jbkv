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

namespace jbkv {

class Value {
 public:
  using Blob = std::vector<uint8_t>;
  using Data = std::variant<std::monostate, bool, char, unsigned char, uint16_t,
                            int16_t, uint32_t, int32_t, uint64_t, int64_t,
                            float, double, std::string, Blob>;

  Value() = default;
  explicit Value(Data&& data)
      : data_(std::move(data)) {
  }

  explicit Value(const char* data)
      : data_(std::string(data)) {
  }

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
    return data_.index() != 0;
  }

  void Accept(const auto& visitor) const {
    std::visit(visitor, data_);
  }

  static Value NotSet() {
    return {};
  }

 private:
  Data data_;
};

inline std::ostream& operator<<(std::ostream& os, const Value& value) {
  value.Accept([&os](const auto& data) {
    using T = std::remove_cvref_t<decltype(data)>;
    if constexpr (std::is_same_v<T, std::monostate>) {
      os << "<not-set>";
    } else if constexpr (std::is_same_v<T, Value::Blob>) {
      os << std::hex;
      for (const auto byte : data) {
        os << byte;
      }

      os << std::resetiosflags(std::ios_base::basefield);
    } else {
      os << data;
    }
  });
  return os;
}

// todo: docs, exception specs
// todo non-copyable, non-movable
class NodeData {
 public:
  using Key = std::string;
  using Value = Value;
  using KeyValueList = std::vector<std::pair<Key, Value>>;
  using Ptr = std::shared_ptr<NodeData>;
  using List = std::vector<Ptr>;

 public:
  virtual ~NodeData() = default;

  virtual Value Read(const Key& key) const = 0;
  virtual void Write(const Key& key, Value&& value) = 0;
  virtual bool Remove(const Key& key) = 0;

  virtual KeyValueList Enumerate() const = 0;

  /// helpers
 public:
  template <typename T>
  void Write(const Key& key, const T& value) {
    Write(key, Value(value));
  }

  template <typename T>
  std::optional<T> Read(const Key& key) {
    const auto value = Read(key);
    const auto* value_ptr = value.template Try<T>();
    if (!value_ptr) {
      return std::nullopt;
    }

    return *value_ptr;
  }
};

template <typename NodeFamily>
class Node {
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
  /// @return nullptr if node is not found, otherwise pointer to node
  /// todo wrap to optional, do not spread nullptrs on code
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

/// todo GoTo(path)

VolumeNode::Ptr CreateVolume();
StorageNode::Ptr MountStorage(const VolumeNode::Ptr& node);

void Save(const VolumeNode::Ptr& root, const std::filesystem::path& path);
void Load(const VolumeNode::Ptr& root, const std::filesystem::path& path);

}  // namespace jbkv
