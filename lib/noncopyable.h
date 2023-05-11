#pragma once

namespace jbkv {

class NonCopyableNonMovable {
 public:
  NonCopyableNonMovable() = default;
  NonCopyableNonMovable(const NonCopyableNonMovable&) = delete;
  NonCopyableNonMovable(NonCopyableNonMovable&&) = delete;
  NonCopyableNonMovable& operator=(NonCopyableNonMovable&&) = delete;
  NonCopyableNonMovable& operator=(const NonCopyableNonMovable&) = delete;
};
}  // namespace jbkv
