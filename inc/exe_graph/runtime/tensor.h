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
#ifndef INC_1492C31AD0B645108093A808E7C9A85B_H
#define INC_1492C31AD0B645108093A808E7C9A85B_H

#include "graph/ge_error_codes.h"
#include "storage_shape.h"
#include "storage_format.h"

namespace gert {
enum TensorPlacement {
  kOnDeviceHbm,
  kOnHost,
  kFollowing,  // 数据紧跟在结构体后面
  kTensorPlacementEnd
};

using TensorAddress = void *;
using ConstTensorAddress = void *const;

struct Tensor {
  int64_t GetShapeSize() const {
    return storage_shape.GetStorageShape().GetShapeSize();
  }

  template<class T>
  const T *GetData() const {
    return static_cast<const T *>(GetAddr());
  }

  template<class T>
  T *GetData() {
    return static_cast<T *>(GetAddr());
  }

  const void *GetAddr() const {
    if (placement == kFollowing) {
      return reinterpret_cast<const void *>(reinterpret_cast<const uint8_t *>(this) + sizeof(*this));
    } else {
      return addr;
    }
  }

  void *GetAddr() {
    if (placement == kFollowing) {
      return reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(this) + sizeof(*this));
    } else {
      return addr;
    }
  }

  ge::DataType GetDataType() const {
    return data_type;
  }

  static std::unique_ptr<uint8_t[]> CreateFollowing(int64_t shape_size, ge::DataType dt, size_t &total_size) {
    total_size = ge::GetSizeInBytes(shape_size, dt);
    if (ge::AddOverflow(total_size, sizeof(Tensor), total_size)) {
      return nullptr;
    }
    auto holder = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_size]);
    if (holder == nullptr) {
      return nullptr;
    }

    auto tensor = reinterpret_cast<Tensor *>(holder.get());
    tensor->placement = kFollowing;
    tensor->addr = nullptr;
    tensor->data_type = dt;

    return holder;
  }

  const Shape &GetStorageShape() const {
    return storage_shape.GetStorageShape();
  }

  const Shape &GetOriginShape() const {
    return storage_shape.GetOriginShape();
  }

  ge::Format GetStorageFormat() const {
    return storage_format.GetStorageFormat();
  }

  ge::Format GetOriginFormat() const {
    return storage_format.GetOriginFormat();
  }

  ExpandDimsType GetExpandDimsType() const {
    return storage_format.GetExpandDimsType();
  }

  TensorPlacement GetPlacement() const {
    return placement;
  }

  StorageShape storage_shape;
  StorageFormat storage_format;
  TensorPlacement placement;
  ge::DataType data_type;
  TensorAddress addr;
};
static_assert(std::is_standard_layout<Tensor>::value, "The class Tensor must be a POD");
}  // namespace gert

#endif
