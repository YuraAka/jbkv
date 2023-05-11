#include "value.h"
#include <iomanip>
#include <stdexcept>
#include <limits>

using namespace jbkv;

void Value::CheckLimits() const {
  Accept([](const auto& data) {
    using T = std::remove_cvref_t<decltype(data)>;
    if constexpr (std::is_same_v<T, Value::Blob> ||
                  std::is_same_v<T, Value::String>) {
      /// int32_t because of the least of std::streamsize in x86
      constexpr size_t kNativeLeastMax = std::numeric_limits<int32_t>::max();
      if (data.Ref().size() > kNativeLeastMax) {
        throw std::runtime_error(
            "Value is too big which drops x86 platform support: " +
            std::to_string(data.Ref().size()));
      }
    }
  });
}

std::ostream& jbkv::operator<<(std::ostream& os, const Value& value) {
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
