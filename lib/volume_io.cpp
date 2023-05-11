#include "volume_io.h"
#include <fstream>

namespace {
using namespace jbkv;

enum class FormatMarker : uint8_t {
  Double = 0,
  String = 1,
  Blob = 2,
  Bool = 3,
  Char = 4,
  UChar = 5,
  UInt16 = 6,
  Int16 = 7,
  UInt32 = 8,
  Int32 = 9,
  UInt64 = 10,
  Int64 = 11,
  Float = 12
};

constexpr uint8_t kFormatVersion = 1;
constexpr std::string_view kMagic = "jbkv";

void Check(std::ios& stream) {
  if (!stream.good()) {
    throw std::runtime_error("Bad stream");
  }
}

std::streamsize ConvertSize(uint64_t size) {
  if constexpr (sizeof(std::streamsize) < sizeof(size)) {
    const uint64_t native_max = std::numeric_limits<std::streamsize>::max();
    if (size > native_max) {
      throw std::runtime_error("Size is too big");
    }
  }

  return static_cast<std::streamsize>(size);
}

void SerializeHeader(std::ostream& out) {
  Check(out.write(kMagic.data(), kMagic.size()));
  Check(out.write(reinterpret_cast<const char*>(&kFormatVersion),
                  sizeof(kFormatVersion)));
}

void DeserializeHeader(std::string& magic, uint8_t& version, std::istream& in) {
  magic.resize(kMagic.size());
  Check(in.read(&magic[0], ConvertSize(magic.size())));
  Check(in.read(reinterpret_cast<char*>(&version), sizeof(version)));
}

void Serialize(const std::string& value, std::ostream& out) {
  uint64_t size = value.size();
  Check(out.write(reinterpret_cast<const char*>(&size), sizeof(size)));
  Check(out.write(value.data(), ConvertSize(size)));
}

void Deserialize(std::string& value, std::istream& in) {
  uint64_t size = 0;
  Check(in.read(reinterpret_cast<char*>(&size), sizeof(size)));

  const auto native_size = ConvertSize(size);
  value.resize(static_cast<size_t>(native_size));
  Check(in.read(&value[0], native_size));
}

void Serialize(const Value::String& value, std::ostream& out) {
  Serialize(value.Ref(), out);
}

void Deserialize(Value::String& value, std::istream& in) {
  Deserialize(value.Ref(), in);
}

void Serialize(const Value::Blob& value, std::ostream& out) {
  uint64_t size = value.Ref().size();
  Check(out.write(reinterpret_cast<const char*>(&size), sizeof(size)));
  Check(out.write(reinterpret_cast<const char*>(&value.Ref()[0]),
                  ConvertSize(size)));
}

void Deserialize(Value::Blob& value, std::istream& in) {
  uint64_t size = 0;
  Check(in.read(reinterpret_cast<char*>(&size), sizeof(size)));

  const auto native_size = ConvertSize(size);
  value.Ref().resize(static_cast<size_t>(native_size));
  Check(in.read(reinterpret_cast<char*>(&value.Ref()[0]), native_size));
}

template <typename T>
  requires std::is_scalar_v<T>
void Serialize(T value, std::ostream& out) {
  Check(out.write(reinterpret_cast<char*>(&value), sizeof(value)));
}

template <typename T>
  requires std::is_scalar_v<T>
void Deserialize(T& value, std::istream& in) {
  Check(in.read(reinterpret_cast<char*>(&value), sizeof(value)));
}

void Serialize(const Value& value, std::ostream& out) {
  value.Accept([&out](const auto& data) {
    using Type = std::remove_cvref_t<decltype(data)>;
    if constexpr (std::is_same_v<Type, bool>) {
      Serialize(FormatMarker::Bool, out);
    } else if constexpr (std::is_same_v<Type, char>) {
      Serialize(FormatMarker::Char, out);
    } else if constexpr (std::is_same_v<Type, unsigned char>) {
      Serialize(FormatMarker::UChar, out);
    } else if constexpr (std::is_same_v<Type, uint16_t>) {
      Serialize(FormatMarker::UInt16, out);
    } else if constexpr (std::is_same_v<Type, int16_t>) {
      Serialize(FormatMarker::Int16, out);
    } else if constexpr (std::is_same_v<Type, uint32_t>) {
      Serialize(FormatMarker::UInt32, out);
    } else if constexpr (std::is_same_v<Type, int32_t>) {
      Serialize(FormatMarker::Int32, out);
    } else if constexpr (std::is_same_v<Type, uint64_t>) {
      Serialize(FormatMarker::UInt64, out);
    } else if constexpr (std::is_same_v<Type, int64_t>) {
      Serialize(FormatMarker::Int64, out);
    } else if constexpr (std::is_same_v<Type, float>) {
      Serialize(FormatMarker::Float, out);
    } else if constexpr (std::is_same_v<Type, double>) {
      Serialize(FormatMarker::Double, out);
    } else if constexpr (std::is_same_v<Type, Value::String>) {
      Serialize(FormatMarker::String, out);
    } else if constexpr (std::is_same_v<Type, Value::Blob>) {
      Serialize(FormatMarker::Blob, out);
    } else {
      static_assert(sizeof(Type) != sizeof(Type), "unknown type");
    }

    Serialize(data, out);
  });
}

void Deserialize(std::optional<Value>& value, std::istream& in) {
  FormatMarker marker;
  Deserialize(marker, in);
  switch (marker) {
    case FormatMarker::Bool: {
      bool data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Char: {
      char data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::UChar: {
      unsigned char data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::UInt16: {
      uint16_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Int16: {
      int16_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::UInt32: {
      uint32_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Int32: {
      int32_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::UInt64: {
      uint64_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Int64: {
      int64_t data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Float: {
      float data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::Double: {
      double data = {};
      Deserialize(data, in);
      value.emplace(data);
      break;
    }
    case FormatMarker::String: {
      Value::String data;
      Deserialize(data, in);
      value.emplace(std::move(data));
      break;
    }
    case FormatMarker::Blob: {
      Value::Blob data;
      Deserialize(data, in);
      value.emplace(std::move(data));
      break;
    }
  };
}

template <typename T>
  requires(std::is_same_v<T, Value::String> || std::is_same_v<T, Value::Blob>)
void CheckSum(const T& value, uint8_t& checksum) {
  for (const auto c : value.Ref()) {
    checksum ^= c;
  }
}

template <typename T>
  requires(std::is_same_v<T, std::string>)
void CheckSum(const T& value, uint8_t& checksum) {
  for (const auto c : value) {
    checksum ^= c;
  }
}

template <typename T>
  requires std::is_scalar_v<T>
void CheckSum(T value, uint8_t& checksum) {
  for (size_t i = 0; i < sizeof(value); ++i) {
    checksum ^= *(reinterpret_cast<uint8_t*>(&value) + i);
  }
}

void CheckSum(const Value& value, uint8_t checksum) {
  value.Accept([&checksum](const auto& data) {
    CheckSum(data, checksum);
  });
}

class VolumeSaver {
 public:
  explicit VolumeSaver(std::ostream& stream)
      : stream_(stream) {
    SerializeHeader(stream_);
  }

 public:
  void OnNode(const VolumeNode& node, auto& descendants) {
    auto children = node.Enumerate();
    uint8_t checksum = 0;
    Serialize(children.size(), stream_);
    for (auto&& child : std::move(children)) {
      const auto& name = child->GetName();
      Serialize(name, stream_);
      CheckSum(name, checksum);
      descendants.push_back(std::move(child));
    }

    const auto data = node.Open();
    const auto kv_list = data->Enumerate();
    Serialize(kv_list.size(), stream_);
    for (const auto& [key, value] : kv_list) {
      Serialize(key, stream_);
      Serialize(value, stream_);

      CheckSum(key, checksum);
      CheckSum(value, checksum);
    }

    Serialize(checksum, stream_);
  }

 private:
  std::ostream& stream_;
};

class VolumeLoader {
 public:
  explicit VolumeLoader(std::istream& stream)
      : stream_(stream) {
    std::string magic;
    uint8_t version = 0;
    DeserializeHeader(magic, version, stream_);
    if (magic != kMagic) {
      throw std::runtime_error("Bad file format, magic mismatch: " + magic);
    }

    if (version > kFormatVersion) {
      throw std::runtime_error("File version is too new. Update program!");
    }
  }

  void OnNode(VolumeNode& node, auto& descendants) {
    uint8_t checksum = 0;
    size_t children_count = 0;
    Deserialize(children_count, stream_);
    for (size_t i = 0; i < children_count; ++i) {
      VolumeNode::Name name;
      Deserialize(name, stream_);
      CheckSum(name, checksum);
      auto child = node.Create(name);
      descendants.push_back(std::move(child));
    }

    auto data = node.Open();
    size_t kv_size = 0;
    Deserialize(kv_size, stream_);
    for (size_t i = 0; i < kv_size; ++i) {
      NodeData::Key key;
      std::optional<Value> value;
      Deserialize(key, stream_);
      Deserialize(value, stream_);
      CheckSum(key, checksum);
      CheckSum(*value, checksum);
      data->Write(key, std::move(*value));
    }

    uint8_t stream_checksum = 0;
    Deserialize(stream_checksum, stream_);
    if (checksum != stream_checksum) {
      throw std::runtime_error("Data corrupted");
    }
  }

 private:
  std::istream& stream_;
};

template <typename Visitor>
void Traverse(const VolumeNode::Ptr& root, Visitor&& visitor) {
  std::deque<VolumeNode::Ptr> nodes{root};
  while (!nodes.empty()) {
    auto node = nodes.front();
    nodes.pop_front();
    visitor.OnNode(*node, nodes);
  }
}

}  // namespace

void jbkv::Save(const VolumeNode::Ptr& root,
                const std::filesystem::path& path) {
  std::ofstream stream(path, std::ios_base::binary);
  if (!stream.is_open()) {
    throw std::runtime_error("Cannot open file for writing: " + path.string());
  }

  Save(root, stream);
}

void jbkv::Load(const VolumeNode::Ptr& root,
                const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios_base::binary);
  if (!stream.is_open()) {
    throw std::runtime_error("Cannot open file for reading: " + path.string());
  }

  Load(root, stream);
}

void jbkv::Save(const VolumeNode::Ptr& root, std::ostream& stream) {
  if (!root) {
    throw std::runtime_error("Unable to save: root is nullptr");
  }

  VolumeSaver saver(stream);
  Traverse(root, saver);
}

void jbkv::Load(const VolumeNode::Ptr& root, std::istream& stream) {
  if (!root) {
    throw std::runtime_error("Unable to load: root is nullptr");
  }

  VolumeLoader loader(stream);
  Traverse(root, loader);
}
