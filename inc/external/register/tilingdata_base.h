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

#ifndef __INC_REGISTER_TIK2_TILINGDATA_BASE_HEADER__
#define __INC_REGISTER_TIK2_TILINGDATA_BASE_HEADER__

#include "graph/ascend_string.h"
#include <vector>
#include <map>
#include <memory>
#include <securec.h>

namespace optiling {
class FieldInfo {
public:
  FieldInfo(const ge::AscendString &dtype, const ge::AscendString &name) : dtype_(dtype), name_(name) {}

public:
  ge::AscendString dtype_;
  ge::AscendString name_;
};

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
  public:                                                                                                              \
    class FieldHandler {                                                                                               \
    public:                                                                                                            \
      FieldHandler(class_name *pinstance, const ge::AscendString &dtype, const ge::AscendString &name, size_t len) {   \
        pinstance->field_info_.push_back(FieldInfo(dtype, name));                                                      \
        pinstance->field_offset_map_[name] = pinstance->data_size_;                                                    \
        pinstance->data_size_ += len;                                                                                  \
        pinstance->InitData();                                                                                         \
      }                                                                                                                \
    };                                                                                                                 \
    friend class FieldHandler;                                                                                         \
                                                                                                                       \
  public:                                                                                                              \
    class_name() {                                                                                                     \
      class_name_ = #class_name;                                                                                       \
    };

#define TILING_DATA_FIELD_DEF(data_type, field_name)                                                                   \
 public:                                                                                                               \
  void set_##field_name(data_type field_name) {                                                                        \
    field_name##_ = field_name;                                                                                        \
    auto offset = field_offset_map_[#field_name];                                                                      \
    *((data_type *) (data_ptr_ + offset)) = field_name;                                                                \
  }                                                                                                                    \
  data_type get_##field_name() {                                                                                       \
    return field_name##_;                                                                                              \
  }                                                                                                                    \
                                                                                                                       \
 private:                                                                                                              \
  data_type field_name##_;                                                                                             \
  FieldHandler field_name##_handler_ = FieldHandler(this, #data_type, #field_name, sizeof(data_type));

#define END_TILING_DATA_DEF                                                                                            \
  }                                                                                                                    \
  ;

class TilingDef {
public:
  ~TilingDef() {
    if (data_ptr_ != nullptr) {
      delete data_ptr_;
      data_ptr_ = nullptr;
    }
  }
  void SaveToBuffer(void *pdata, size_t capacity) {
    memcpy_s(pdata, capacity, data_ptr_, data_size_);
  }
  std::vector<FieldInfo> GetFieldInfo() {
    return field_info_;
  }
  ge::AscendString GetClassName() {
    return class_name_;
  }
  size_t GetDataSize() {
    return data_size_;
  }

  void InitData() {
    if (data_ptr_ != nullptr) {
      delete data_ptr_;
      data_ptr_ = nullptr;
    }
    if (data_size_ > 0) {
      data_ptr_ = new uint8_t[data_size_];
    }
  }

protected:
  // dtype, name
  std::vector<FieldInfo> field_info_;
  std::map<ge::AscendString, size_t> field_offset_map_;
  uint8_t *data_ptr_ = nullptr;
  size_t data_size_ = 0;
  ge::AscendString class_name_;
};

typedef std::shared_ptr<TilingDef> (*TilingDataConstructor)();

class CTilingDataClassFactory {
public:
  static void RegisterTilingData(ge::AscendString op_type, TilingDataConstructor constructor) {
    instance_[op_type] = constructor;
  }

  static std::shared_ptr<TilingDef> CreateTilingDataInstance(const ge::AscendString &op_type) {
    auto it = instance_.find(op_type);
    if (it == instance_.end()) {
      return nullptr;
    }
    TilingDataConstructor constructor = it->second;

    if (constructor == nullptr) {
      return nullptr;
    }

    return (*constructor)();
  }

private:
  static std::map<ge::AscendString, TilingDataConstructor> instance_;
};

#define REGISTER_TILING_DATA_CLASS(op_type, class_name)                                                                \
  class op_type##class_name##Helper {                                                                                  \
  public:                                                                                                              \
    op_type##class_name##Helper() {                                                                                    \
      CTilingDataClassFactory::RegisterTilingData(#op_type, op_type##class_name##Helper::CreateTilingDataInstance);    \
    }                                                                                                                  \
    static std::shared_ptr<TilingDef> CreateTilingDataInstance() {                                                     \
      return std::make_shared<class_name>();                                                                           \
    }                                                                                                                  \
  };                                                                                                                   \
  op_type##class_name##Helper g_tilingdata_##op_type##class_name##helper;
} // end of namespace optiling
#endif  // __INC_REGISTER_TIK2_TILINGDATA_BASE_HEADER__
