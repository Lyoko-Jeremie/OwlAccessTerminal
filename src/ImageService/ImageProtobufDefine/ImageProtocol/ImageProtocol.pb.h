// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ImageProtocol.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_ImageProtocol_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_ImageProtocol_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021011 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_ImageProtocol_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_ImageProtocol_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_ImageProtocol_2eproto;
class ImageRequest;
struct ImageRequestDefaultTypeInternal;
extern ImageRequestDefaultTypeInternal _ImageRequest_default_instance_;
class ImageResponse;
struct ImageResponseDefaultTypeInternal;
extern ImageResponseDefaultTypeInternal _ImageResponse_default_instance_;
PROTOBUF_NAMESPACE_OPEN
template<> ::ImageRequest* Arena::CreateMaybeMessage<::ImageRequest>(Arena*);
template<> ::ImageResponse* Arena::CreateMaybeMessage<::ImageResponse>(Arena*);
PROTOBUF_NAMESPACE_CLOSE

enum ImageFormat : int {
  IMAGE_FORMAT_NONE = 0,
  IMAGE_FORMAT_JPG = 1,
  IMAGE_FORMAT_BMP = 2,
  IMAGE_FORMAT_PNG = 3,
  IMAGE_FORMAT_GIF = 4,
  IMAGE_FORMAT_BYTE_MAT = 5,
  ImageFormat_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<int32_t>::min(),
  ImageFormat_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<int32_t>::max()
};
bool ImageFormat_IsValid(int value);
constexpr ImageFormat ImageFormat_MIN = IMAGE_FORMAT_NONE;
constexpr ImageFormat ImageFormat_MAX = IMAGE_FORMAT_BYTE_MAT;
constexpr int ImageFormat_ARRAYSIZE = ImageFormat_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* ImageFormat_descriptor();
template<typename T>
inline const std::string& ImageFormat_Name(T enum_t_value) {
  static_assert(::std::is_same<T, ImageFormat>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function ImageFormat_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    ImageFormat_descriptor(), enum_t_value);
}
inline bool ImageFormat_Parse(
    ::PROTOBUF_NAMESPACE_ID::ConstStringParam name, ImageFormat* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<ImageFormat>(
    ImageFormat_descriptor(), name, value);
}
// ===================================================================

class ImageRequest final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:ImageRequest) */ {
 public:
  inline ImageRequest() : ImageRequest(nullptr) {}
  ~ImageRequest() override;
  explicit PROTOBUF_CONSTEXPR ImageRequest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ImageRequest(const ImageRequest& from);
  ImageRequest(ImageRequest&& from) noexcept
    : ImageRequest() {
    *this = ::std::move(from);
  }

  inline ImageRequest& operator=(const ImageRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline ImageRequest& operator=(ImageRequest&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ImageRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const ImageRequest* internal_default_instance() {
    return reinterpret_cast<const ImageRequest*>(
               &_ImageRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(ImageRequest& a, ImageRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(ImageRequest* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ImageRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ImageRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ImageRequest>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const ImageRequest& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const ImageRequest& from) {
    ImageRequest::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ImageRequest* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "ImageRequest";
  }
  protected:
  explicit ImageRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kCmdIdFieldNumber = 1,
    kPackageIdFieldNumber = 2,
    kCameraIdFieldNumber = 3,
    kImageWidthFieldNumber = 11,
    kImageHeightFieldNumber = 12,
  };
  // int32 cmd_id = 1;
  void clear_cmd_id();
  int32_t cmd_id() const;
  void set_cmd_id(int32_t value);
  private:
  int32_t _internal_cmd_id() const;
  void _internal_set_cmd_id(int32_t value);
  public:

  // optional int32 package_id = 2;
  bool has_package_id() const;
  private:
  bool _internal_has_package_id() const;
  public:
  void clear_package_id();
  int32_t package_id() const;
  void set_package_id(int32_t value);
  private:
  int32_t _internal_package_id() const;
  void _internal_set_package_id(int32_t value);
  public:

  // optional int32 camera_id = 3;
  bool has_camera_id() const;
  private:
  bool _internal_has_camera_id() const;
  public:
  void clear_camera_id();
  int32_t camera_id() const;
  void set_camera_id(int32_t value);
  private:
  int32_t _internal_camera_id() const;
  void _internal_set_camera_id(int32_t value);
  public:

  // optional int32 image_width = 11;
  bool has_image_width() const;
  private:
  bool _internal_has_image_width() const;
  public:
  void clear_image_width();
  int32_t image_width() const;
  void set_image_width(int32_t value);
  private:
  int32_t _internal_image_width() const;
  void _internal_set_image_width(int32_t value);
  public:

  // optional int32 image_height = 12;
  bool has_image_height() const;
  private:
  bool _internal_has_image_height() const;
  public:
  void clear_image_height();
  int32_t image_height() const;
  void set_image_height(int32_t value);
  private:
  int32_t _internal_image_height() const;
  void _internal_set_image_height(int32_t value);
  public:

  // @@protoc_insertion_point(class_scope:ImageRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
    int32_t cmd_id_;
    int32_t package_id_;
    int32_t camera_id_;
    int32_t image_width_;
    int32_t image_height_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ImageProtocol_2eproto;
};
// -------------------------------------------------------------------

class ImageResponse final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:ImageResponse) */ {
 public:
  inline ImageResponse() : ImageResponse(nullptr) {}
  ~ImageResponse() override;
  explicit PROTOBUF_CONSTEXPR ImageResponse(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ImageResponse(const ImageResponse& from);
  ImageResponse(ImageResponse&& from) noexcept
    : ImageResponse() {
    *this = ::std::move(from);
  }

  inline ImageResponse& operator=(const ImageResponse& from) {
    CopyFrom(from);
    return *this;
  }
  inline ImageResponse& operator=(ImageResponse&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ImageResponse& default_instance() {
    return *internal_default_instance();
  }
  static inline const ImageResponse* internal_default_instance() {
    return reinterpret_cast<const ImageResponse*>(
               &_ImageResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(ImageResponse& a, ImageResponse& b) {
    a.Swap(&b);
  }
  inline void Swap(ImageResponse* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ImageResponse* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ImageResponse* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ImageResponse>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const ImageResponse& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const ImageResponse& from) {
    ImageResponse::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ImageResponse* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "ImageResponse";
  }
  protected:
  explicit ImageResponse(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kImageDataFieldNumber = 60,
    kCmdIdFieldNumber = 1,
    kPackageIdFieldNumber = 2,
    kCameraIdFieldNumber = 3,
    kImageWidthFieldNumber = 11,
    kImageHeightFieldNumber = 12,
    kImagePixelChannelFieldNumber = 51,
    kImageDataSizeFieldNumber = 50,
    kImageFormatFieldNumber = 52,
  };
  // optional bytes image_data = 60;
  bool has_image_data() const;
  private:
  bool _internal_has_image_data() const;
  public:
  void clear_image_data();
  const std::string& image_data() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_image_data(ArgT0&& arg0, ArgT... args);
  std::string* mutable_image_data();
  PROTOBUF_NODISCARD std::string* release_image_data();
  void set_allocated_image_data(std::string* image_data);
  private:
  const std::string& _internal_image_data() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_image_data(const std::string& value);
  std::string* _internal_mutable_image_data();
  public:

  // int32 cmd_id = 1;
  void clear_cmd_id();
  int32_t cmd_id() const;
  void set_cmd_id(int32_t value);
  private:
  int32_t _internal_cmd_id() const;
  void _internal_set_cmd_id(int32_t value);
  public:

  // optional int32 package_id = 2;
  bool has_package_id() const;
  private:
  bool _internal_has_package_id() const;
  public:
  void clear_package_id();
  int32_t package_id() const;
  void set_package_id(int32_t value);
  private:
  int32_t _internal_package_id() const;
  void _internal_set_package_id(int32_t value);
  public:

  // optional int32 camera_id = 3;
  bool has_camera_id() const;
  private:
  bool _internal_has_camera_id() const;
  public:
  void clear_camera_id();
  int32_t camera_id() const;
  void set_camera_id(int32_t value);
  private:
  int32_t _internal_camera_id() const;
  void _internal_set_camera_id(int32_t value);
  public:

  // optional int32 image_width = 11;
  bool has_image_width() const;
  private:
  bool _internal_has_image_width() const;
  public:
  void clear_image_width();
  int32_t image_width() const;
  void set_image_width(int32_t value);
  private:
  int32_t _internal_image_width() const;
  void _internal_set_image_width(int32_t value);
  public:

  // optional int32 image_height = 12;
  bool has_image_height() const;
  private:
  bool _internal_has_image_height() const;
  public:
  void clear_image_height();
  int32_t image_height() const;
  void set_image_height(int32_t value);
  private:
  int32_t _internal_image_height() const;
  void _internal_set_image_height(int32_t value);
  public:

  // optional int32 image_pixel_channel = 51;
  bool has_image_pixel_channel() const;
  private:
  bool _internal_has_image_pixel_channel() const;
  public:
  void clear_image_pixel_channel();
  int32_t image_pixel_channel() const;
  void set_image_pixel_channel(int32_t value);
  private:
  int32_t _internal_image_pixel_channel() const;
  void _internal_set_image_pixel_channel(int32_t value);
  public:

  // optional uint64 image_data_size = 50;
  bool has_image_data_size() const;
  private:
  bool _internal_has_image_data_size() const;
  public:
  void clear_image_data_size();
  uint64_t image_data_size() const;
  void set_image_data_size(uint64_t value);
  private:
  uint64_t _internal_image_data_size() const;
  void _internal_set_image_data_size(uint64_t value);
  public:

  // optional .ImageFormat image_format = 52;
  bool has_image_format() const;
  private:
  bool _internal_has_image_format() const;
  public:
  void clear_image_format();
  ::ImageFormat image_format() const;
  void set_image_format(::ImageFormat value);
  private:
  ::ImageFormat _internal_image_format() const;
  void _internal_set_image_format(::ImageFormat value);
  public:

  // @@protoc_insertion_point(class_scope:ImageResponse)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr image_data_;
    int32_t cmd_id_;
    int32_t package_id_;
    int32_t camera_id_;
    int32_t image_width_;
    int32_t image_height_;
    int32_t image_pixel_channel_;
    uint64_t image_data_size_;
    int image_format_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ImageProtocol_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// ImageRequest

// int32 cmd_id = 1;
inline void ImageRequest::clear_cmd_id() {
  _impl_.cmd_id_ = 0;
}
inline int32_t ImageRequest::_internal_cmd_id() const {
  return _impl_.cmd_id_;
}
inline int32_t ImageRequest::cmd_id() const {
  // @@protoc_insertion_point(field_get:ImageRequest.cmd_id)
  return _internal_cmd_id();
}
inline void ImageRequest::_internal_set_cmd_id(int32_t value) {
  
  _impl_.cmd_id_ = value;
}
inline void ImageRequest::set_cmd_id(int32_t value) {
  _internal_set_cmd_id(value);
  // @@protoc_insertion_point(field_set:ImageRequest.cmd_id)
}

// optional int32 package_id = 2;
inline bool ImageRequest::_internal_has_package_id() const {
  bool value = (_impl_._has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool ImageRequest::has_package_id() const {
  return _internal_has_package_id();
}
inline void ImageRequest::clear_package_id() {
  _impl_.package_id_ = 0;
  _impl_._has_bits_[0] &= ~0x00000001u;
}
inline int32_t ImageRequest::_internal_package_id() const {
  return _impl_.package_id_;
}
inline int32_t ImageRequest::package_id() const {
  // @@protoc_insertion_point(field_get:ImageRequest.package_id)
  return _internal_package_id();
}
inline void ImageRequest::_internal_set_package_id(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000001u;
  _impl_.package_id_ = value;
}
inline void ImageRequest::set_package_id(int32_t value) {
  _internal_set_package_id(value);
  // @@protoc_insertion_point(field_set:ImageRequest.package_id)
}

// optional int32 camera_id = 3;
inline bool ImageRequest::_internal_has_camera_id() const {
  bool value = (_impl_._has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline bool ImageRequest::has_camera_id() const {
  return _internal_has_camera_id();
}
inline void ImageRequest::clear_camera_id() {
  _impl_.camera_id_ = 0;
  _impl_._has_bits_[0] &= ~0x00000002u;
}
inline int32_t ImageRequest::_internal_camera_id() const {
  return _impl_.camera_id_;
}
inline int32_t ImageRequest::camera_id() const {
  // @@protoc_insertion_point(field_get:ImageRequest.camera_id)
  return _internal_camera_id();
}
inline void ImageRequest::_internal_set_camera_id(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000002u;
  _impl_.camera_id_ = value;
}
inline void ImageRequest::set_camera_id(int32_t value) {
  _internal_set_camera_id(value);
  // @@protoc_insertion_point(field_set:ImageRequest.camera_id)
}

// optional int32 image_width = 11;
inline bool ImageRequest::_internal_has_image_width() const {
  bool value = (_impl_._has_bits_[0] & 0x00000004u) != 0;
  return value;
}
inline bool ImageRequest::has_image_width() const {
  return _internal_has_image_width();
}
inline void ImageRequest::clear_image_width() {
  _impl_.image_width_ = 0;
  _impl_._has_bits_[0] &= ~0x00000004u;
}
inline int32_t ImageRequest::_internal_image_width() const {
  return _impl_.image_width_;
}
inline int32_t ImageRequest::image_width() const {
  // @@protoc_insertion_point(field_get:ImageRequest.image_width)
  return _internal_image_width();
}
inline void ImageRequest::_internal_set_image_width(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000004u;
  _impl_.image_width_ = value;
}
inline void ImageRequest::set_image_width(int32_t value) {
  _internal_set_image_width(value);
  // @@protoc_insertion_point(field_set:ImageRequest.image_width)
}

// optional int32 image_height = 12;
inline bool ImageRequest::_internal_has_image_height() const {
  bool value = (_impl_._has_bits_[0] & 0x00000008u) != 0;
  return value;
}
inline bool ImageRequest::has_image_height() const {
  return _internal_has_image_height();
}
inline void ImageRequest::clear_image_height() {
  _impl_.image_height_ = 0;
  _impl_._has_bits_[0] &= ~0x00000008u;
}
inline int32_t ImageRequest::_internal_image_height() const {
  return _impl_.image_height_;
}
inline int32_t ImageRequest::image_height() const {
  // @@protoc_insertion_point(field_get:ImageRequest.image_height)
  return _internal_image_height();
}
inline void ImageRequest::_internal_set_image_height(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000008u;
  _impl_.image_height_ = value;
}
inline void ImageRequest::set_image_height(int32_t value) {
  _internal_set_image_height(value);
  // @@protoc_insertion_point(field_set:ImageRequest.image_height)
}

// -------------------------------------------------------------------

// ImageResponse

// int32 cmd_id = 1;
inline void ImageResponse::clear_cmd_id() {
  _impl_.cmd_id_ = 0;
}
inline int32_t ImageResponse::_internal_cmd_id() const {
  return _impl_.cmd_id_;
}
inline int32_t ImageResponse::cmd_id() const {
  // @@protoc_insertion_point(field_get:ImageResponse.cmd_id)
  return _internal_cmd_id();
}
inline void ImageResponse::_internal_set_cmd_id(int32_t value) {
  
  _impl_.cmd_id_ = value;
}
inline void ImageResponse::set_cmd_id(int32_t value) {
  _internal_set_cmd_id(value);
  // @@protoc_insertion_point(field_set:ImageResponse.cmd_id)
}

// optional int32 package_id = 2;
inline bool ImageResponse::_internal_has_package_id() const {
  bool value = (_impl_._has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline bool ImageResponse::has_package_id() const {
  return _internal_has_package_id();
}
inline void ImageResponse::clear_package_id() {
  _impl_.package_id_ = 0;
  _impl_._has_bits_[0] &= ~0x00000002u;
}
inline int32_t ImageResponse::_internal_package_id() const {
  return _impl_.package_id_;
}
inline int32_t ImageResponse::package_id() const {
  // @@protoc_insertion_point(field_get:ImageResponse.package_id)
  return _internal_package_id();
}
inline void ImageResponse::_internal_set_package_id(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000002u;
  _impl_.package_id_ = value;
}
inline void ImageResponse::set_package_id(int32_t value) {
  _internal_set_package_id(value);
  // @@protoc_insertion_point(field_set:ImageResponse.package_id)
}

// optional int32 camera_id = 3;
inline bool ImageResponse::_internal_has_camera_id() const {
  bool value = (_impl_._has_bits_[0] & 0x00000004u) != 0;
  return value;
}
inline bool ImageResponse::has_camera_id() const {
  return _internal_has_camera_id();
}
inline void ImageResponse::clear_camera_id() {
  _impl_.camera_id_ = 0;
  _impl_._has_bits_[0] &= ~0x00000004u;
}
inline int32_t ImageResponse::_internal_camera_id() const {
  return _impl_.camera_id_;
}
inline int32_t ImageResponse::camera_id() const {
  // @@protoc_insertion_point(field_get:ImageResponse.camera_id)
  return _internal_camera_id();
}
inline void ImageResponse::_internal_set_camera_id(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000004u;
  _impl_.camera_id_ = value;
}
inline void ImageResponse::set_camera_id(int32_t value) {
  _internal_set_camera_id(value);
  // @@protoc_insertion_point(field_set:ImageResponse.camera_id)
}

// optional int32 image_width = 11;
inline bool ImageResponse::_internal_has_image_width() const {
  bool value = (_impl_._has_bits_[0] & 0x00000008u) != 0;
  return value;
}
inline bool ImageResponse::has_image_width() const {
  return _internal_has_image_width();
}
inline void ImageResponse::clear_image_width() {
  _impl_.image_width_ = 0;
  _impl_._has_bits_[0] &= ~0x00000008u;
}
inline int32_t ImageResponse::_internal_image_width() const {
  return _impl_.image_width_;
}
inline int32_t ImageResponse::image_width() const {
  // @@protoc_insertion_point(field_get:ImageResponse.image_width)
  return _internal_image_width();
}
inline void ImageResponse::_internal_set_image_width(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000008u;
  _impl_.image_width_ = value;
}
inline void ImageResponse::set_image_width(int32_t value) {
  _internal_set_image_width(value);
  // @@protoc_insertion_point(field_set:ImageResponse.image_width)
}

// optional int32 image_height = 12;
inline bool ImageResponse::_internal_has_image_height() const {
  bool value = (_impl_._has_bits_[0] & 0x00000010u) != 0;
  return value;
}
inline bool ImageResponse::has_image_height() const {
  return _internal_has_image_height();
}
inline void ImageResponse::clear_image_height() {
  _impl_.image_height_ = 0;
  _impl_._has_bits_[0] &= ~0x00000010u;
}
inline int32_t ImageResponse::_internal_image_height() const {
  return _impl_.image_height_;
}
inline int32_t ImageResponse::image_height() const {
  // @@protoc_insertion_point(field_get:ImageResponse.image_height)
  return _internal_image_height();
}
inline void ImageResponse::_internal_set_image_height(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000010u;
  _impl_.image_height_ = value;
}
inline void ImageResponse::set_image_height(int32_t value) {
  _internal_set_image_height(value);
  // @@protoc_insertion_point(field_set:ImageResponse.image_height)
}

// optional uint64 image_data_size = 50;
inline bool ImageResponse::_internal_has_image_data_size() const {
  bool value = (_impl_._has_bits_[0] & 0x00000040u) != 0;
  return value;
}
inline bool ImageResponse::has_image_data_size() const {
  return _internal_has_image_data_size();
}
inline void ImageResponse::clear_image_data_size() {
  _impl_.image_data_size_ = uint64_t{0u};
  _impl_._has_bits_[0] &= ~0x00000040u;
}
inline uint64_t ImageResponse::_internal_image_data_size() const {
  return _impl_.image_data_size_;
}
inline uint64_t ImageResponse::image_data_size() const {
  // @@protoc_insertion_point(field_get:ImageResponse.image_data_size)
  return _internal_image_data_size();
}
inline void ImageResponse::_internal_set_image_data_size(uint64_t value) {
  _impl_._has_bits_[0] |= 0x00000040u;
  _impl_.image_data_size_ = value;
}
inline void ImageResponse::set_image_data_size(uint64_t value) {
  _internal_set_image_data_size(value);
  // @@protoc_insertion_point(field_set:ImageResponse.image_data_size)
}

// optional int32 image_pixel_channel = 51;
inline bool ImageResponse::_internal_has_image_pixel_channel() const {
  bool value = (_impl_._has_bits_[0] & 0x00000020u) != 0;
  return value;
}
inline bool ImageResponse::has_image_pixel_channel() const {
  return _internal_has_image_pixel_channel();
}
inline void ImageResponse::clear_image_pixel_channel() {
  _impl_.image_pixel_channel_ = 0;
  _impl_._has_bits_[0] &= ~0x00000020u;
}
inline int32_t ImageResponse::_internal_image_pixel_channel() const {
  return _impl_.image_pixel_channel_;
}
inline int32_t ImageResponse::image_pixel_channel() const {
  // @@protoc_insertion_point(field_get:ImageResponse.image_pixel_channel)
  return _internal_image_pixel_channel();
}
inline void ImageResponse::_internal_set_image_pixel_channel(int32_t value) {
  _impl_._has_bits_[0] |= 0x00000020u;
  _impl_.image_pixel_channel_ = value;
}
inline void ImageResponse::set_image_pixel_channel(int32_t value) {
  _internal_set_image_pixel_channel(value);
  // @@protoc_insertion_point(field_set:ImageResponse.image_pixel_channel)
}

// optional .ImageFormat image_format = 52;
inline bool ImageResponse::_internal_has_image_format() const {
  bool value = (_impl_._has_bits_[0] & 0x00000080u) != 0;
  return value;
}
inline bool ImageResponse::has_image_format() const {
  return _internal_has_image_format();
}
inline void ImageResponse::clear_image_format() {
  _impl_.image_format_ = 0;
  _impl_._has_bits_[0] &= ~0x00000080u;
}
inline ::ImageFormat ImageResponse::_internal_image_format() const {
  return static_cast< ::ImageFormat >(_impl_.image_format_);
}
inline ::ImageFormat ImageResponse::image_format() const {
  // @@protoc_insertion_point(field_get:ImageResponse.image_format)
  return _internal_image_format();
}
inline void ImageResponse::_internal_set_image_format(::ImageFormat value) {
  _impl_._has_bits_[0] |= 0x00000080u;
  _impl_.image_format_ = value;
}
inline void ImageResponse::set_image_format(::ImageFormat value) {
  _internal_set_image_format(value);
  // @@protoc_insertion_point(field_set:ImageResponse.image_format)
}

// optional bytes image_data = 60;
inline bool ImageResponse::_internal_has_image_data() const {
  bool value = (_impl_._has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool ImageResponse::has_image_data() const {
  return _internal_has_image_data();
}
inline void ImageResponse::clear_image_data() {
  _impl_.image_data_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000001u;
}
inline const std::string& ImageResponse::image_data() const {
  // @@protoc_insertion_point(field_get:ImageResponse.image_data)
  return _internal_image_data();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void ImageResponse::set_image_data(ArgT0&& arg0, ArgT... args) {
 _impl_._has_bits_[0] |= 0x00000001u;
 _impl_.image_data_.SetBytes(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:ImageResponse.image_data)
}
inline std::string* ImageResponse::mutable_image_data() {
  std::string* _s = _internal_mutable_image_data();
  // @@protoc_insertion_point(field_mutable:ImageResponse.image_data)
  return _s;
}
inline const std::string& ImageResponse::_internal_image_data() const {
  return _impl_.image_data_.Get();
}
inline void ImageResponse::_internal_set_image_data(const std::string& value) {
  _impl_._has_bits_[0] |= 0x00000001u;
  _impl_.image_data_.Set(value, GetArenaForAllocation());
}
inline std::string* ImageResponse::_internal_mutable_image_data() {
  _impl_._has_bits_[0] |= 0x00000001u;
  return _impl_.image_data_.Mutable(GetArenaForAllocation());
}
inline std::string* ImageResponse::release_image_data() {
  // @@protoc_insertion_point(field_release:ImageResponse.image_data)
  if (!_internal_has_image_data()) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000001u;
  auto* p = _impl_.image_data_.Release();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.image_data_.IsDefault()) {
    _impl_.image_data_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void ImageResponse::set_allocated_image_data(std::string* image_data) {
  if (image_data != nullptr) {
    _impl_._has_bits_[0] |= 0x00000001u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000001u;
  }
  _impl_.image_data_.SetAllocated(image_data, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.image_data_.IsDefault()) {
    _impl_.image_data_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:ImageResponse.image_data)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)


PROTOBUF_NAMESPACE_OPEN

template <> struct is_proto_enum< ::ImageFormat> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::ImageFormat>() {
  return ::ImageFormat_descriptor();
}

PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_ImageProtocol_2eproto