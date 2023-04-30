#pragma once
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

class NodeReader;
class NodeWriter;

// todo: docs, exception specs
// todo non-copyable, non-movable
class Node {
 public:
  using Name = std::string;
  using Path = std::vector<Name>;
  using Ptr = std::shared_ptr<Node>;
  using List = std::vector<Ptr>;
  using Key = std::string;
  using Value = Value;

 public:
  virtual ~Node() = default;

  virtual Node::Ptr AddNode(const Name& name) = 0;
  virtual Node::Ptr GetNode(const Name& name) const = 0;
  virtual Node::List ListNodes() const = 0;

  virtual void Write(const Key& key, const Value& value) = 0;
  virtual Value Read(const Key& key) const = 0;
  virtual bool Remove(const Key& key) = 0;

  virtual void Accept(NodeReader& reader) const = 0;
  virtual void Accept(NodeWriter& writer) = 0;

 public:
  template <typename T>
  void Write(const Key& key, const T& value) {
    Write(key, Value(value));
  }

  static Node::Ptr FindByPath(const Node::Ptr& from, const Path& path);

  // virtual Node* FindChild(Id id) const = 0;

  // virtual Value ReadValue(Key key) const = 0;
  // virtual void WriteValue(Key key, Value&& value) = 0;
  // virtual bool RemoveValue(Key key) = 0;
  // virtual void Flush() = 0;

  // can be VolumeNode (OverFs), or StorageNode (Virtual)
};

class NodeReader {
 public:
  virtual ~NodeReader() = default;

  virtual void OnLink(const Node::Name& self, const Node::Name& child) = 0;
  virtual void OnData(const Node::Name& self, const Node::Key& key,
                      const Node::Value& value) = 0;
};

class NodeWriter {
 public:
  /// @todo to avoid dynamic allocation fu2::function_view can be used
  using LinkCallback = std::function<void(Node::Name&&)>;
  using DataCallback = std::function<void(Node::Key&&, Node::Value&&)>;

 public:
  virtual ~NodeWriter() = default;
  virtual void OnLink(const Node::Name& self, const LinkCallback& cb) = 0;
  virtual void OnData(const Node::Name& self, const DataCallback& cb) = 0;
};

// Node::Ptr MountNode(Node::Ptr&& src);

/*class Volume {
 public:
  virtual ~Volume() = default;
  virtual Node& GetRoot() const = 0;
  virtual Node* FindNode(std::string_view path) const = 0;
  virtual void Save() = 0;
};*/

class Volume {
 public:
  using Ptr = std::shared_ptr<Volume>;

 public:
  virtual ~Volume() = default;
  virtual Node::Ptr Root() const = 0;

  // todo move to separate utility
  // virtual void Load(const std::filesystem::path& path);
  // virtual void Save(const std::filesystem::path& path) const = 0;

 public:
  static Volume::Ptr CreateSingleThreaded();
};

void Save(const Volume& volume, const std::filesystem::path& path);
void Load(Volume& volume, const std::filesystem::path& path);

}  // namespace jbkv
