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

  template <typename T>
  const T& As() const {
    auto data_ptr = Try<T>();
    if (!data_ptr) {
      throw std::runtime_error("data type mismatched with underlying");
    }

    return *data_ptr;
  }

  template <typename T>
  const T* Try() const {
    return std::get_if<T>(&data_);
  }

  bool Has() const {
    return std::get_if<0>(&data_) != nullptr;
  }

 private:
  Data data_;
};

// todo: docs, exception specs
// todo non-copyable, non-movable
class Node {
 public:
  using Name = std::string;
  using Path = std::vector<Name>;
  using Ptr = std::shared_ptr<Node>;
  using List = std::vector<Ptr>;
  using Key = std::string;

 public:
  virtual ~Node() = default;

  virtual Node::Ptr AddNode(const Name& name) = 0;
  virtual Node::Ptr GetNode(const Name& name) const = 0;

  virtual void Write(const Key& key, const Value& value) = 0;
  virtual Value Read(const Key& key) const = 0;

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
}  // namespace jbkv
