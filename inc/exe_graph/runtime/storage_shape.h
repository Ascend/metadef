/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#ifndef INC_42D3114E59BD47AE94C2A91A9F0A6D82_H
#define INC_42D3114E59BD47AE94C2A91A9F0A6D82_H
#include <type_traits>
#include "graph/ge_error_codes.h"
#include "graph/types.h"
#include "shape.h"

namespace gert {
// todo 删除构造函数
struct StorageShape {
 public:
  StorageShape() = default;
  StorageShape(std::initializer_list<int64_t> origin_shape, std::initializer_list<int64_t> storage_shape)
      : origin_shape_(origin_shape), storage_shape_(storage_shape) {}
  const Shape &GetOriginShape() const {
    return origin_shape_;
  }

  const Shape &GetStorageShape() const {
    return storage_shape_;
  }

  Shape &MutableOriginShape() {
    return origin_shape_;
  }

  Shape &MutableStorageShape() {
    return storage_shape_;
  }

 private:
  Shape origin_shape_;
  Shape storage_shape_;
};
static_assert(std::is_standard_layout<StorageShape>::value, "The class must be a POD");
}  // namespace gert

#endif
