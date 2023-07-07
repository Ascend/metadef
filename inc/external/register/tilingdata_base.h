/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INC_REGISTER_ASCENDC_TILINGDATA_BASE_HEADER__
#define __INC_REGISTER_ASCENDC_TILINGDATA_BASE_HEADER__

#include <vector>
#include <map>
#include <memory>
#include <cstring>
#include <securec.h>
#include "graph/ascend_string.h"

namespace optiling {
struct CharPtrCmp {
  bool operator()(const char *strLeft, const char *strRight) const
  {
    return strcmp(strLeft, strRight) < 0;
  }
};

class StructSizeInfoBase {
public:
  static StructSizeInfoBase &GetInstance();
  void SetStructSize(const char *structType, const size_t structSize)
  {
    structSizeInfo[structType] = structSize;
  }
  size_t GetStructSize(const char *structType)
  {
    return structSizeInfo.at(structType);
  }
private:
  StructSizeInfoBase() { };
  ~StructSizeInfoBase() { };
  StructSizeInfoBase(const StructSizeInfoBase &);
  StructSizeInfoBase &operator=(const StructSizeInfoBase &);
  std::map<const char *, size_t, CharPtrCmp> structSizeInfo;
};

class FieldInfo {
public:
  FieldInfo(const char *dtype, const char *name)
      : dtype_(dtype), name_(name), classType_("0") {}
  FieldInfo(const char *dtype, const char *name, size_t arrSize)
      : dtype_(dtype), name_(name), arrSize_(arrSize), classType_("1") {}
  FieldInfo(const char *dtype, const char *name, const char *structType,
            size_t structSize)
      : dtype_(dtype), name_(name), structType_(structType), structSize_(structSize), classType_("2") {}

public:
  const char *dtype_;
  const char *name_;
  size_t arrSize_;
  const char *structType_;
  size_t structSize_;
  const char *classType_;
};

class TilingDef {
public:
  ~TilingDef()
  {
    if (data_ptr_ != nullptr) {
      delete[] data_ptr_;
      data_ptr_ = nullptr;
    }
  }
  void SaveToBuffer(void *pdata, size_t capacity);
  std::vector<FieldInfo> GetFieldInfo() const;
  const char *GetTilingClassName() const;
  size_t GetDataSize() const;
  void InitData();
  void GeLogError(const std::string& str) const;

protected:
  // dtype, name
  std::vector<FieldInfo> field_info_;
  std::map<const char *, size_t, CharPtrCmp> field_offset_map_;
  uint8_t *data_ptr_ = nullptr;
  size_t data_size_ = 0;
  const char *class_name_;
  std::map<const char *, void *, CharPtrCmp> saveBufferPtr;
  size_t struct_size_ = 0;
};

using TilingDataConstructor = std::shared_ptr<TilingDef> (*)();

class CTilingDataClassFactory {
public:
  static CTilingDataClassFactory &GetInstance();
  void RegisterTilingData(const char *op_type, const TilingDataConstructor constructor);
  std::shared_ptr<TilingDef> CreateTilingDataInstance(const char *op_type);

private:
  CTilingDataClassFactory() { };
  ~CTilingDataClassFactory() { };
  CTilingDataClassFactory(const CTilingDataClassFactory &);
  CTilingDataClassFactory &operator=(const CTilingDataClassFactory &);
  std::map<const char *, TilingDataConstructor, CharPtrCmp> instance_;
};
}  // end of namespace optiling

/*
example:
// supported data_type: int8_t/uint8_t/int16_t/uint16_t/int32_t/uint32_t/int64_t/uint64_t
BEGIN_TILING_DATA_DEF(MaxPoolTilingData)
    // format: TILING_DATA_FIELD_DEF(data_type, field_name);
    TILING_DATA_FIELD_DEF(int32_t, dim_0);
    TILING_DATA_FIELD_DEF(uint8_t, var_1);
    TILING_DATA_FIELD_DEF(int64_t, factor_1);
END_TILING_DATA_DEF
REGISTER_TILING_DATA_CLASS(MaxPool, MaxPoolTilingData)
*/
#define BEGIN_TILING_DATA_DEF(class_name)                                                                              \
  class class_name : public TilingDef {                                                                                \
   public:                                                                                                             \
    class FieldHandler {                                                                                               \
     public:                                                                                                           \
      FieldHandler(class_name *pinstance, const char *dtype, const char *name, size_t len) {                           \
        pinstance->field_info_.push_back( { dtype, name });                                                            \
        pinstance->field_offset_map_[name] = pinstance->data_size_;                                                    \
        pinstance->data_size_ += len;                                                                                  \
        pinstance->InitData();                                                                                         \
        StructSizeInfoBase::GetInstance().SetStructSize(#class_name, pinstance->data_size_);                           \
      }                                                                                                                \
      FieldHandler(class_name *pinstance, const char *dtype, const char *name, size_t len,                             \
                   size_t arrSize) {                                                                                   \
        pinstance->field_info_.push_back(FieldInfo(dtype, name, arrSize));                                             \
        pinstance->field_offset_map_[name] = pinstance->data_size_;                                                    \
        pinstance->data_size_ += len * arrSize;                                                                        \
        pinstance->InitData();                                                                                         \
        StructSizeInfoBase::GetInstance().SetStructSize(#class_name, pinstance->data_size_);                           \
      }                                                                                                                \
      FieldHandler(class_name *pinstance, const char *dtype, const char *name,                                         \
                   const char *structType, size_t structSize, void *ptr) {                                             \
        pinstance->field_info_.push_back(FieldInfo(dtype, name, structType, structSize));                              \
        pinstance->field_offset_map_[name] = pinstance->data_size_;                                                    \
        pinstance->data_size_ += structSize;                                                                           \
        pinstance->InitData();                                                                                         \
        StructSizeInfoBase::GetInstance().SetStructSize(#class_name, pinstance->data_size_);                           \
        pinstance->saveBufferPtr[name] = ptr;                                                                          \
        pinstance->struct_size_ += structSize;                                                                         \
      }                                                                                                                \
    };                                                                                                                 \
    friend class FieldHandler;                                                                                         \
                                                                                                                       \
   public:                                                                                                             \
    class_name() { class_name_ = #class_name; }

#define TILING_DATA_FIELD_DEF(data_type, field_name)                                                                   \
 public:                                                                                                               \
  void set_##field_name(data_type field_name) {                                                                        \
    field_name##_ = field_name;                                                                                        \
    auto offset = field_offset_map_[#field_name];                                                                      \
    *((data_type *) (data_ptr_ + offset)) = field_name;                                                                \
  }                                                                                                                    \
  data_type get_##field_name() { return field_name##_; }                                                               \
                                                                                                                       \
 private:                                                                                                              \
  data_type field_name##_ = 0;                                                                                         \
  FieldHandler field_name##_handler_ = FieldHandler(this, #data_type, #field_name, sizeof(data_type));

#define TILING_DATA_FIELD_DEF_ARR(arr_type, arr_size, field_name)                                                      \
 public:                                                                                                               \
  void set_##field_name(arr_type *field_name) {                                                                        \
    field_name##_ = field_name;                                                                                        \
    auto offset = field_offset_map_[#field_name];                                                                      \
    if (data_ptr_ + offset == (uint8_t *)field_name) {                                                                 \
      return;                                                                                                          \
    }                                                                                                                  \
    const auto err_t = memcpy_s(data_ptr_ + offset, data_size_ - offset, field_name, arr_size * sizeof(arr_type));     \
    if (err_t != EOK) {                                                                                                \
        GeLogError("tilingdata_base.h TILING_DATA_FIELD_DEF_ARR memcpy is failed !");                                  \
    }                                                                                                                  \
  }                                                                                                                    \
  arr_type *get_##field_name() {                                                                                       \
    auto offset = field_offset_map_[#field_name];                                                                      \
    return (arr_type *)(data_ptr_ + offset);                                                                           \
  }                                                                                                                    \
                                                                                                                       \
 private:                                                                                                              \
  arr_type *field_name##_ = nullptr;                                                                                   \
  FieldHandler field_name##_handler_ = FieldHandler(this, #arr_type, #field_name, sizeof(arr_type), arr_size);

#define TILING_DATA_FIELD_DEF_STRUCT(struct_type, field_name)                                                          \
 public:                                                                                                               \
  struct_type field_name;                                                                                              \
                                                                                                                       \
 private:                                                                                                              \
  FieldHandler field_name##_handler_ =                                                                                 \
      FieldHandler(this, "struct", #field_name, #struct_type,                                                          \
                   StructSizeInfoBase::GetInstance().GetStructSize(#struct_type), (void *) &field_name)

#define END_TILING_DATA_DEF                                                                                            \
  }                                                                                                                    \
  ;

#define REGISTER_TILING_DATA_CLASS(op_type, class_name)                                                                \
  class op_type##class_name##Helper {                                                                                  \
   public:                                                                                                             \
    op_type##class_name##Helper() {                                                                                    \
      CTilingDataClassFactory::GetInstance().RegisterTilingData(#op_type,                                              \
                   op_type##class_name##Helper::CreateTilingDataInstance);                                             \
    }                                                                                                                  \
    static std::shared_ptr<TilingDef> CreateTilingDataInstance() { return std::make_shared<class_name>(); }            \
  };                                                                                                                   \
  static op_type##class_name##Helper g_tilingdata_##op_type##class_name##helper;

#endif  // __INC_REGISTER_ASCENDC_TILINGDATA_BASE_HEADER__
