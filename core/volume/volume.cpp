#include "volume.h"
#include "node.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message_lite.h>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <format>
#include <type_traits>

namespace {
using namespace jbkv;
using namespace std::string_literals;

/// concurent adding kv to list
/// concurent changing value

class FsNode final : public Node {
 public:
  explicit FsNode(const std::filesystem::path& path)
      : path_(path) {
    /*if (create_on_absense && !std::filesystem::exists(path)) {
      return;
    }

    std::ifstream input(path_.c_str(), std::ios::binary);
    if (!input) {
      throw std::runtime_error("File is corrupted: "s + path.c_str());
    }

    jbkv_abi::Node data;
    if (!data.ParseFromIstream(&input)) {
      throw std::runtime_error("File is corrupted: "s + path.c_str());
    }

    /// copy to internal data
    // todo checksum check
  }

 public:
  Node* FindChild(Id id) const override {
    (void)id;
    return nullptr;
  }

  std::optional<Value> ReadValue(Key key) const override {
    (void)key;
    return {};
  }

  bool WriteValue(Key key, Value&& value) override {
    /// how non-locking read/write strings?
    (void)key;
    (void)value;
    return false;
  }

  bool RemoveValue(Key key) override {
    // write tombstone
    (void)key;
    return false;
  }

  void Flush() override {
  }

 private:
  std::filesystem::path path_;
};
}  // namespace

Node::Ptr jbkv::NewNode(const std::filesystem::path& path) {
  return std::make_shared<FsNode>(path);
}
