#pragma once
#include <filesystem>
#include <optional>
#include <memory>
#include <string_view>
#include <string>

namespace jbkv {

struct Value {
  int data;
};

/*struct NodeId {
  uint32_t id = 0;
};*/

// todo: docs, exception specs
// todo non-copyable, non-movable
class Node {
 public:
  using Key = std::string_view;
  using Ptr = std::shared_ptr<Node>;
  using Id = uint32_t;
  using Path = std::vector<uint32_t>;

 public:
  virtual ~Node() = default;
  // virtual Node* FindChild(Id id) const = 0;

  virtual Value ReadValue(Key key) const = 0;
  virtual void WriteValue(Key key, Value&& value) = 0;
  // virtual bool RemoveValue(Key key) = 0;
  // virtual void Flush() = 0;

  // can be VolumeNode (OverFs), or StorageNode (Virtual)
};

Node::Ptr NewNode(const std::filesystem::path& path);
// Node::Ptr MountNode(Node::Ptr&& src);

/*class Volume {
 public:
  virtual ~Volume() = default;
  virtual Node& GetRoot() const = 0;
  virtual Node* FindNode(std::string_view path) const = 0;
  virtual void Save() = 0;
};*/
}  // namespace jbkv
