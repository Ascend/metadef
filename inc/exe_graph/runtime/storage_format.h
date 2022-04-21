/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#ifndef AIR_CXX_RUNTIME_V2_KERNEL_STORAGE_FORMAT_H_
#define AIR_CXX_RUNTIME_V2_KERNEL_STORAGE_FORMAT_H_
#include <cstdint>
#include <memory>
#include "graph/types.h"
#include "expand_dims_type.h"
namespace gert {
// todo 删除构造函数
struct StorageFormat {
 public:
  StorageFormat() = default;
  StorageFormat(ge::Format origin_format, ge::Format storage_format, const ExpandDimsType &expand_dims_type)
      : origin_format_(origin_format), storage_format_(storage_format), expand_dims_type_(expand_dims_type) {}
  ge::Format GetOriginFormat() const {
    return origin_format_;
  }
  void SetOriginFormat(ge::Format origin_format) {
    origin_format_ = origin_format;
  }
  ge::Format GetStorageFormat() const {
    return storage_format_;
  }
  void SetStorageFormat(ge::Format storage_format) {
    storage_format_ = storage_format;
  }
  ExpandDimsType GetExpandDimsType() const {
    return expand_dims_type_;
  }
  void SetExpandDimsType(ExpandDimsType expand_dims_type) {
    expand_dims_type_ = expand_dims_type;
  }
  ExpandDimsType &MutableExpandDimsType() {
    return expand_dims_type_;
  }
 private:
  ge::Format origin_format_;
  ge::Format storage_format_;
  ExpandDimsType expand_dims_type_;
};
static_assert(std::is_standard_layout<StorageFormat>::value);
}
#endif  //AIR_CXX_RUNTIME_V2_KERNEL_STORAGE_FORMAT_H_
