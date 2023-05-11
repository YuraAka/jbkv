#pragma once
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>
#include "noncopyable.h"
#include "node_data.h"

namespace jbkv {

template <typename NodeFamily>
class Node : NonCopyableNonMovable {
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
  /// @note return non-null ptr
  virtual Node::Ptr Find(const Name& name) const = 0;

  /// @brief Removes link to child node by name
  /// @return false if no child is found by given name, otherwise true
  /// @note node remains alive until last strong link on it
  virtual bool Unlink(const Name& name) = 0;

  /// @brief List children nodes
  /// @return return non-null ptr list
  virtual List Enumerate() const = 0;

  /// @return name of current node
  virtual const Name& GetName() const = 0;

  /// @brief Retrieves data for current node
  /// @return return non-null ptr
  virtual NodeData::Ptr Open() const = 0;

  /// Returns false if node not exist, otherwise true
  virtual bool IsValid() const = 0;
};

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
}  // namespace jbkv
