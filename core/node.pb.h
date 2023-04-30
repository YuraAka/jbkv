// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: node.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_node_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_node_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3012000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3012003 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_node_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_node_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[2]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_node_2eproto;
namespace jbkv_abi {
class Node;
class NodeDefaultTypeInternal;
extern NodeDefaultTypeInternal _Node_default_instance_;
class Value;
class ValueDefaultTypeInternal;
extern ValueDefaultTypeInternal _Value_default_instance_;
}  // namespace jbkv_abi
PROTOBUF_NAMESPACE_OPEN
template<> ::jbkv_abi::Node* Arena::CreateMaybeMessage<::jbkv_abi::Node>(Arena*);
template<> ::jbkv_abi::Value* Arena::CreateMaybeMessage<::jbkv_abi::Value>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace jbkv_abi {

enum ValueType : int {
  VT_STRING = 0,
  VT_BYTES = 1,
  VT_UINT32 = 2,
  ValueType_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  ValueType_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool ValueType_IsValid(int value);
constexpr ValueType ValueType_MIN = VT_STRING;
constexpr ValueType ValueType_MAX = VT_UINT32;
constexpr int ValueType_ARRAYSIZE = ValueType_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* ValueType_descriptor();
template<typename T>
inline const std::string& ValueType_Name(T enum_t_value) {
  static_assert(::std::is_same<T, ValueType>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function ValueType_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    ValueType_descriptor(), enum_t_value);
}
inline bool ValueType_Parse(
    ::PROTOBUF_NAMESPACE_ID::ConstStringParam name, ValueType* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<ValueType>(
    ValueType_descriptor(), name, value);
}
// ===================================================================

class Value PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:jbkv_abi.Value) */ {
 public:
  inline Value() : Value(nullptr) {};
  virtual ~Value();

  Value(const Value& from);
  Value(Value&& from) noexcept
    : Value() {
    *this = ::std::move(from);
  }

  inline Value& operator=(const Value& from) {
    CopyFrom(from);
    return *this;
  }
  inline Value& operator=(Value&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const Value& default_instance();

  enum DataCase {
    kStr = 2,
    kB = 3,
    kUi32 = 4,
    DATA_NOT_SET = 0,
  };

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const Value* internal_default_instance() {
    return reinterpret_cast<const Value*>(
               &_Value_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(Value& a, Value& b) {
    a.Swap(&b);
  }
  inline void Swap(Value* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Value* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline Value* New() const final {
    return CreateMaybeMessage<Value>(nullptr);
  }

  Value* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<Value>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const Value& from);
  void MergeFrom(const Value& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Value* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "jbkv_abi.Value";
  }
  protected:
  explicit Value(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_node_2eproto);
    return ::descriptor_table_node_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kTypeFieldNumber = 1,
    kStrFieldNumber = 2,
    kBFieldNumber = 3,
    kUi32FieldNumber = 4,
  };
  // .jbkv_abi.ValueType type = 1;
  void clear_type();
  ::jbkv_abi::ValueType type() const;
  void set_type(::jbkv_abi::ValueType value);
  private:
  ::jbkv_abi::ValueType _internal_type() const;
  void _internal_set_type(::jbkv_abi::ValueType value);
  public:

  // string str = 2;
  private:
  bool _internal_has_str() const;
  public:
  void clear_str();
  const std::string& str() const;
  void set_str(const std::string& value);
  void set_str(std::string&& value);
  void set_str(const char* value);
  void set_str(const char* value, size_t size);
  std::string* mutable_str();
  std::string* release_str();
  void set_allocated_str(std::string* str);
  private:
  const std::string& _internal_str() const;
  void _internal_set_str(const std::string& value);
  std::string* _internal_mutable_str();
  public:

  // bytes b = 3;
  private:
  bool _internal_has_b() const;
  public:
  void clear_b();
  const std::string& b() const;
  void set_b(const std::string& value);
  void set_b(std::string&& value);
  void set_b(const char* value);
  void set_b(const void* value, size_t size);
  std::string* mutable_b();
  std::string* release_b();
  void set_allocated_b(std::string* b);
  private:
  const std::string& _internal_b() const;
  void _internal_set_b(const std::string& value);
  std::string* _internal_mutable_b();
  public:

  // uint32 ui32 = 4;
  private:
  bool _internal_has_ui32() const;
  public:
  void clear_ui32();
  ::PROTOBUF_NAMESPACE_ID::uint32 ui32() const;
  void set_ui32(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_ui32() const;
  void _internal_set_ui32(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  void clear_data();
  DataCase data_case() const;
  // @@protoc_insertion_point(class_scope:jbkv_abi.Value)
 private:
  class _Internal;
  void set_has_str();
  void set_has_b();
  void set_has_ui32();

  inline bool has_data() const;
  inline void clear_has_data();

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  int type_;
  union DataUnion {
    DataUnion() {}
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr str_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr b_;
    ::PROTOBUF_NAMESPACE_ID::uint32 ui32_;
  } data_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  ::PROTOBUF_NAMESPACE_ID::uint32 _oneof_case_[1];

  friend struct ::TableStruct_node_2eproto;
};
// -------------------------------------------------------------------

class Node PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:jbkv_abi.Node) */ {
 public:
  inline Node() : Node(nullptr) {};
  virtual ~Node();

  Node(const Node& from);
  Node(Node&& from) noexcept
    : Node() {
    *this = ::std::move(from);
  }

  inline Node& operator=(const Node& from) {
    CopyFrom(from);
    return *this;
  }
  inline Node& operator=(Node&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const Node& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const Node* internal_default_instance() {
    return reinterpret_cast<const Node*>(
               &_Node_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(Node& a, Node& b) {
    a.Swap(&b);
  }
  inline void Swap(Node* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Node* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline Node* New() const final {
    return CreateMaybeMessage<Node>(nullptr);
  }

  Node* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<Node>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const Node& from);
  void MergeFrom(const Node& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Node* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "jbkv_abi.Node";
  }
  protected:
  explicit Node(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_node_2eproto);
    return ::descriptor_table_node_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kKeysFieldNumber = 1,
    kValuesFieldNumber = 2,
  };
  // repeated string keys = 1;
  int keys_size() const;
  private:
  int _internal_keys_size() const;
  public:
  void clear_keys();
  const std::string& keys(int index) const;
  std::string* mutable_keys(int index);
  void set_keys(int index, const std::string& value);
  void set_keys(int index, std::string&& value);
  void set_keys(int index, const char* value);
  void set_keys(int index, const char* value, size_t size);
  std::string* add_keys();
  void add_keys(const std::string& value);
  void add_keys(std::string&& value);
  void add_keys(const char* value);
  void add_keys(const char* value, size_t size);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>& keys() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>* mutable_keys();
  private:
  const std::string& _internal_keys(int index) const;
  std::string* _internal_add_keys();
  public:

  // repeated .jbkv_abi.Value values = 2;
  int values_size() const;
  private:
  int _internal_values_size() const;
  public:
  void clear_values();
  ::jbkv_abi::Value* mutable_values(int index);
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::jbkv_abi::Value >*
      mutable_values();
  private:
  const ::jbkv_abi::Value& _internal_values(int index) const;
  ::jbkv_abi::Value* _internal_add_values();
  public:
  const ::jbkv_abi::Value& values(int index) const;
  ::jbkv_abi::Value* add_values();
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::jbkv_abi::Value >&
      values() const;

  // @@protoc_insertion_point(class_scope:jbkv_abi.Node)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string> keys_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::jbkv_abi::Value > values_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_node_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// Value

// .jbkv_abi.ValueType type = 1;
inline void Value::clear_type() {
  type_ = 0;
}
inline ::jbkv_abi::ValueType Value::_internal_type() const {
  return static_cast< ::jbkv_abi::ValueType >(type_);
}
inline ::jbkv_abi::ValueType Value::type() const {
  // @@protoc_insertion_point(field_get:jbkv_abi.Value.type)
  return _internal_type();
}
inline void Value::_internal_set_type(::jbkv_abi::ValueType value) {
  
  type_ = value;
}
inline void Value::set_type(::jbkv_abi::ValueType value) {
  _internal_set_type(value);
  // @@protoc_insertion_point(field_set:jbkv_abi.Value.type)
}

// string str = 2;
inline bool Value::_internal_has_str() const {
  return data_case() == kStr;
}
inline void Value::set_has_str() {
  _oneof_case_[0] = kStr;
}
inline void Value::clear_str() {
  if (_internal_has_str()) {
    data_.str_.Destroy(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
    clear_has_data();
  }
}
inline const std::string& Value::str() const {
  // @@protoc_insertion_point(field_get:jbkv_abi.Value.str)
  return _internal_str();
}
inline void Value::set_str(const std::string& value) {
  _internal_set_str(value);
  // @@protoc_insertion_point(field_set:jbkv_abi.Value.str)
}
inline std::string* Value::mutable_str() {
  // @@protoc_insertion_point(field_mutable:jbkv_abi.Value.str)
  return _internal_mutable_str();
}
inline const std::string& Value::_internal_str() const {
  if (_internal_has_str()) {
    return data_.str_.Get();
  }
  return *&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited();
}
inline void Value::_internal_set_str(const std::string& value) {
  if (!_internal_has_str()) {
    clear_data();
    set_has_str();
    data_.str_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  data_.str_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), value, GetArena());
}
inline void Value::set_str(std::string&& value) {
  // @@protoc_insertion_point(field_set:jbkv_abi.Value.str)
  if (!_internal_has_str()) {
    clear_data();
    set_has_str();
    data_.str_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  data_.str_.Set(
    &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::move(value), GetArena());
  // @@protoc_insertion_point(field_set_rvalue:jbkv_abi.Value.str)
}
inline void Value::set_str(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  if (!_internal_has_str()) {
    clear_data();
    set_has_str();
    data_.str_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  data_.str_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      ::std::string(value), GetArena());
  // @@protoc_insertion_point(field_set_char:jbkv_abi.Value.str)
}
inline void Value::set_str(const char* value,
                             size_t size) {
  if (!_internal_has_str()) {
    clear_data();
    set_has_str();
    data_.str_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  data_.str_.Set(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size),
      GetArena());
  // @@protoc_insertion_point(field_set_pointer:jbkv_abi.Value.str)
}
inline std::string* Value::_internal_mutable_str() {
  if (!_internal_has_str()) {
    clear_data();
    set_has_str();
    data_.str_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  return data_.str_.Mutable(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline std::string* Value::release_str() {
  // @@protoc_insertion_point(field_release:jbkv_abi.Value.str)
  if (_internal_has_str()) {
    clear_has_data();
    return data_.str_.Release(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
  } else {
    return nullptr;
  }
}
inline void Value::set_allocated_str(std::string* str) {
  if (has_data()) {
    clear_data();
  }
  if (str != nullptr) {
    set_has_str();
    data_.str_.UnsafeSetDefault(str);
    ::PROTOBUF_NAMESPACE_ID::Arena* arena = GetArena();
    if (arena != nullptr) {
      arena->Own(str);
    }
  }
  // @@protoc_insertion_point(field_set_allocated:jbkv_abi.Value.str)
}

// bytes b = 3;
inline bool Value::_internal_has_b() const {
  return data_case() == kB;
}
inline void Value::set_has_b() {
  _oneof_case_[0] = kB;
}
inline void Value::clear_b() {
  if (_internal_has_b()) {
    data_.b_.Destroy(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
    clear_has_data();
  }
}
inline const std::string& Value::b() const {
  // @@protoc_insertion_point(field_get:jbkv_abi.Value.b)
  return _internal_b();
}
inline void Value::set_b(const std::string& value) {
  _internal_set_b(value);
  // @@protoc_insertion_point(field_set:jbkv_abi.Value.b)
}
inline std::string* Value::mutable_b() {
  // @@protoc_insertion_point(field_mutable:jbkv_abi.Value.b)
  return _internal_mutable_b();
}
inline const std::string& Value::_internal_b() const {
  if (_internal_has_b()) {
    return data_.b_.Get();
  }
  return *&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited();
}
inline void Value::_internal_set_b(const std::string& value) {
  if (!_internal_has_b()) {
    clear_data();
    set_has_b();
    data_.b_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  data_.b_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), value, GetArena());
}
inline void Value::set_b(std::string&& value) {
  // @@protoc_insertion_point(field_set:jbkv_abi.Value.b)
  if (!_internal_has_b()) {
    clear_data();
    set_has_b();
    data_.b_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  data_.b_.Set(
    &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::move(value), GetArena());
  // @@protoc_insertion_point(field_set_rvalue:jbkv_abi.Value.b)
}
inline void Value::set_b(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  if (!_internal_has_b()) {
    clear_data();
    set_has_b();
    data_.b_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  data_.b_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      ::std::string(value), GetArena());
  // @@protoc_insertion_point(field_set_char:jbkv_abi.Value.b)
}
inline void Value::set_b(const void* value,
                             size_t size) {
  if (!_internal_has_b()) {
    clear_data();
    set_has_b();
    data_.b_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  data_.b_.Set(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size),
      GetArena());
  // @@protoc_insertion_point(field_set_pointer:jbkv_abi.Value.b)
}
inline std::string* Value::_internal_mutable_b() {
  if (!_internal_has_b()) {
    clear_data();
    set_has_b();
    data_.b_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  }
  return data_.b_.Mutable(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline std::string* Value::release_b() {
  // @@protoc_insertion_point(field_release:jbkv_abi.Value.b)
  if (_internal_has_b()) {
    clear_has_data();
    return data_.b_.Release(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
  } else {
    return nullptr;
  }
}
inline void Value::set_allocated_b(std::string* b) {
  if (has_data()) {
    clear_data();
  }
  if (b != nullptr) {
    set_has_b();
    data_.b_.UnsafeSetDefault(b);
    ::PROTOBUF_NAMESPACE_ID::Arena* arena = GetArena();
    if (arena != nullptr) {
      arena->Own(b);
    }
  }
  // @@protoc_insertion_point(field_set_allocated:jbkv_abi.Value.b)
}

// uint32 ui32 = 4;
inline bool Value::_internal_has_ui32() const {
  return data_case() == kUi32;
}
inline void Value::set_has_ui32() {
  _oneof_case_[0] = kUi32;
}
inline void Value::clear_ui32() {
  if (_internal_has_ui32()) {
    data_.ui32_ = 0u;
    clear_has_data();
  }
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 Value::_internal_ui32() const {
  if (_internal_has_ui32()) {
    return data_.ui32_;
  }
  return 0u;
}
inline void Value::_internal_set_ui32(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  if (!_internal_has_ui32()) {
    clear_data();
    set_has_ui32();
  }
  data_.ui32_ = value;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 Value::ui32() const {
  // @@protoc_insertion_point(field_get:jbkv_abi.Value.ui32)
  return _internal_ui32();
}
inline void Value::set_ui32(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_ui32(value);
  // @@protoc_insertion_point(field_set:jbkv_abi.Value.ui32)
}

inline bool Value::has_data() const {
  return data_case() != DATA_NOT_SET;
}
inline void Value::clear_has_data() {
  _oneof_case_[0] = DATA_NOT_SET;
}
inline Value::DataCase Value::data_case() const {
  return Value::DataCase(_oneof_case_[0]);
}
// -------------------------------------------------------------------

// Node

// repeated string keys = 1;
inline int Node::_internal_keys_size() const {
  return keys_.size();
}
inline int Node::keys_size() const {
  return _internal_keys_size();
}
inline void Node::clear_keys() {
  keys_.Clear();
}
inline std::string* Node::add_keys() {
  // @@protoc_insertion_point(field_add_mutable:jbkv_abi.Node.keys)
  return _internal_add_keys();
}
inline const std::string& Node::_internal_keys(int index) const {
  return keys_.Get(index);
}
inline const std::string& Node::keys(int index) const {
  // @@protoc_insertion_point(field_get:jbkv_abi.Node.keys)
  return _internal_keys(index);
}
inline std::string* Node::mutable_keys(int index) {
  // @@protoc_insertion_point(field_mutable:jbkv_abi.Node.keys)
  return keys_.Mutable(index);
}
inline void Node::set_keys(int index, const std::string& value) {
  // @@protoc_insertion_point(field_set:jbkv_abi.Node.keys)
  keys_.Mutable(index)->assign(value);
}
inline void Node::set_keys(int index, std::string&& value) {
  // @@protoc_insertion_point(field_set:jbkv_abi.Node.keys)
  keys_.Mutable(index)->assign(std::move(value));
}
inline void Node::set_keys(int index, const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  keys_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set_char:jbkv_abi.Node.keys)
}
inline void Node::set_keys(int index, const char* value, size_t size) {
  keys_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:jbkv_abi.Node.keys)
}
inline std::string* Node::_internal_add_keys() {
  return keys_.Add();
}
inline void Node::add_keys(const std::string& value) {
  keys_.Add()->assign(value);
  // @@protoc_insertion_point(field_add:jbkv_abi.Node.keys)
}
inline void Node::add_keys(std::string&& value) {
  keys_.Add(std::move(value));
  // @@protoc_insertion_point(field_add:jbkv_abi.Node.keys)
}
inline void Node::add_keys(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  keys_.Add()->assign(value);
  // @@protoc_insertion_point(field_add_char:jbkv_abi.Node.keys)
}
inline void Node::add_keys(const char* value, size_t size) {
  keys_.Add()->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_add_pointer:jbkv_abi.Node.keys)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>&
Node::keys() const {
  // @@protoc_insertion_point(field_list:jbkv_abi.Node.keys)
  return keys_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>*
Node::mutable_keys() {
  // @@protoc_insertion_point(field_mutable_list:jbkv_abi.Node.keys)
  return &keys_;
}

// repeated .jbkv_abi.Value values = 2;
inline int Node::_internal_values_size() const {
  return values_.size();
}
inline int Node::values_size() const {
  return _internal_values_size();
}
inline void Node::clear_values() {
  values_.Clear();
}
inline ::jbkv_abi::Value* Node::mutable_values(int index) {
  // @@protoc_insertion_point(field_mutable:jbkv_abi.Node.values)
  return values_.Mutable(index);
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::jbkv_abi::Value >*
Node::mutable_values() {
  // @@protoc_insertion_point(field_mutable_list:jbkv_abi.Node.values)
  return &values_;
}
inline const ::jbkv_abi::Value& Node::_internal_values(int index) const {
  return values_.Get(index);
}
inline const ::jbkv_abi::Value& Node::values(int index) const {
  // @@protoc_insertion_point(field_get:jbkv_abi.Node.values)
  return _internal_values(index);
}
inline ::jbkv_abi::Value* Node::_internal_add_values() {
  return values_.Add();
}
inline ::jbkv_abi::Value* Node::add_values() {
  // @@protoc_insertion_point(field_add:jbkv_abi.Node.values)
  return _internal_add_values();
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::jbkv_abi::Value >&
Node::values() const {
  // @@protoc_insertion_point(field_list:jbkv_abi.Node.values)
  return values_;
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace jbkv_abi

PROTOBUF_NAMESPACE_OPEN

template <> struct is_proto_enum< ::jbkv_abi::ValueType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::jbkv_abi::ValueType>() {
  return ::jbkv_abi::ValueType_descriptor();
}

PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_node_2eproto