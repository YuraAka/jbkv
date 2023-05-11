#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <ostream>

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
  void CheckLimits() const;

 private:
  Data data_;
};

std::ostream& operator<<(std::ostream& os, const Value& value);
}  // namespace jbkv
