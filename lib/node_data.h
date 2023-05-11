#pragma once
#include <memory>
#include <optional>
#include "noncopyable.h"
#include "value.h"

namespace jbkv {
class NodeData : NonCopyableNonMovable {
 public:
  using Key = std::string;
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
}  // namespace jbkv
