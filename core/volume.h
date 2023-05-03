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
#include <list>

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

// todo remove as base class, maybe transform to DataNode

class NodeData {
 public:
  using Key = std::string;
  using Value = Value;
  using Ptr = std::shared_ptr<NodeData>;

 public:
  virtual ~NodeData() = default;

  virtual Value Read(const Key& key) const = 0;
  virtual void Write(const Key& key, const Value& value) = 0;
  virtual bool Remove(const Key& key) = 0;

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
};

template <typename NodeType>
class Node {
 public:
  using Name = std::string;
  using Path = std::vector<Name>;
  using Ptr = std::shared_ptr<NodeType>;
  using List = std::vector<Ptr>;

 public:
  virtual ~Node() = default;

  /// @brief Creates new or returns existing one
  /// @return non-null pointer to node
  virtual Node::Ptr Create(const Name& name) = 0;

  /// @brief Searches node by name amoung children
  /// @return nullptr if node is not found, otherwise pointer to node
  virtual Node::Ptr Find(const Name& name) = 0;

  /// @brief Removes link to child node by name
  /// @return false if no child is found by given name, otherwise true
  /// @note node remains alive until last strong link on it
  virtual bool Unlink(const Name& name) = 0;

  /// @return name of current node
  virtual const Name& GetName() = 0;

  /// @brief Retrieves data for current node
  virtual NodeData::Ptr Open() const = 0;
};

class VolumeNode : public Node<VolumeNode> {};

class Volume : public VolumeNode {
 public:
  using Ptr = std::shared_ptr<Volume>;

 public:
  virtual ~Volume() = default;

 public:
  static Volume::Ptr CreateLockBased();
};

/// =================================== Storage ================================

class StorageNode : public Node<StorageNode> {
 public:
  /// @brief Links current node with subtree with root in given node
  /// @param node root of subtree to link
  /// @return pointer to new storage node mounted to this and all previously
  /// given subtrees
  /// @note
  /// - resulting node hierarchy is fixed on the moment of mount and does not
  /// change on external cirtumstances
  /// - node will be automatically unmounted on last strong reference
  /// deletion
  /// - name of node is inherited from first mounted volume node
  /// - child creation makes a new Storage node mounted with all co-named
  /// children nodes of mounted trees if any, or force create new child node in
  /// some mounted volume node
  /// - node data joins all node data of corresponding Volume nodes, key
  /// priority is on last mounted nodes
  /// - key binding fixed on the moment of operation (write/read/remove)
  virtual Ptr Mount(const VolumeNode::Ptr& node) = 0;
};

class Storage : public StorageNode {
 public:
  using Ptr = std::shared_ptr<Storage>;

 public:
  virtual ~Storage() = default;

 public:
  static Storage::Ptr CreateLockBased();
};

/// =================================== Save & load ===========================
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

void Save(const Volume& volume, const std::filesystem::path& path);
void Load(Volume& volume, const std::filesystem::path& path);
}  // namespace jbkv
