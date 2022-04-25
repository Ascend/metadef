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
#include "tensor_data.h"

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
  Tensor() = default;
  Tensor(const StorageShape &storage_shape, const StorageFormat &storage_format, TensorPlacement placement,
         ge::DataType data_type, TensorAddress addr)
      : storage_shape_(storage_shape), storage_format_(storage_format), placement_(placement), data_type_(data_type),
        tensor_data_(addr) {}

  int64_t GetShapeSize() const {
    return storage_shape_.GetStorageShape().GetShapeSize();
  }

  template<class T>
  const T *GetData() const {
    return static_cast<const T *>(GetAddr());
  }

  template<class T>
  T *GetData() {
    return static_cast<T *>(GetAddr());
  }

  void SetData(const TensorData& data) {
    tensor_data_ = data;
  }

  const void *GetAddr() const {
    if (placement_ == kFollowing) {
      return reinterpret_cast<const void *>(reinterpret_cast<const uint8_t *>(this) + sizeof(*this));
    } else {
      return tensor_data_.GetAddr();
    }
  }

  void *GetAddr() {
    if (placement_ == kFollowing) {
      return reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(this) + sizeof(*this));
    } else {
      return tensor_data_.GetAddr();
    }
  }

  ge::DataType GetDataType() const {
    return data_type_;
  }

  void SetDataType(ge::DataType data_type) {
    data_type_ = data_type;
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
    tensor->placement_ = kFollowing;
    tensor->data_type_ = dt;
    tensor->tensor_data_ = TensorData{};
    return holder;
  }

  const Shape &GetStorageShape() const {
    return storage_shape_.GetStorageShape();
  }

  Shape &MutableStorageShape() {
    return storage_shape_.MutableStorageShape();
  }

  const Shape &GetOriginShape() const {
    return storage_shape_.GetOriginShape();
  }

  Shape &MutableOriginShape() {
    return storage_shape_.MutableOriginShape();
  }

  const StorageShape &GetShape() const {
    return storage_shape_;
  }

  StorageShape &GetShape() {
    return storage_shape_;
  }

  ge::Format GetStorageFormat() const {
    return storage_format_.GetStorageFormat();
  }

  void SetStorageFormat(ge::Format storage_format) {
    storage_format_.SetStorageFormat(storage_format);
  }

  ge::Format GetOriginFormat() const {
    return storage_format_.GetOriginFormat();
  }

  void SetOriginFormat(ge::Format storage_format) {
    storage_format_.SetOriginFormat(storage_format);
  }

  const StorageFormat &GetFormat() const {
    return storage_format_;
  }
  StorageFormat &MutableFormat() {
    return storage_format_;
  }

  ExpandDimsType GetExpandDimsType() const {
    return storage_format_.GetExpandDimsType();
  }
  void SetExpandDimsType(ExpandDimsType expand_dims_type) {
    storage_format_.SetExpandDimsType(expand_dims_type);
  }

  TensorPlacement GetPlacement() const {
    return placement_;
  }

  void SetPlacement(TensorPlacement placement) {
    placement_ = placement;
  }

  const TensorData& GetTensorData() const {
    return tensor_data_;
  }

 private:
  StorageShape storage_shape_;
  StorageFormat storage_format_;
  TensorPlacement placement_;
  ge::DataType data_type_;
  TensorData tensor_data_;
};
static_assert(std::is_standard_layout<Tensor>::value, "The class Tensor must be a POD");
}  // namespace gert

#endif
