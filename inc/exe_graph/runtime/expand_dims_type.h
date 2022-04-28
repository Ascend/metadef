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
#ifndef B70061AA26BC43BB8673CA2DC8CBAEF5_H
#define B70061AA26BC43BB8673CA2DC8CBAEF5_H
#include <cstdint>
#include <initializer_list>
#include <cstddef>
#include "graph/ge_error_codes.h"
#include "shape.h"

namespace gert {
/**
 * |-|-|-|-|-|---|
 * |1|1|0|1|0|...|
 * |-|-|-|-|-|---|
 * 如上bits所示，表达了expand后的shape信息，其中的1代表需要补1，0代表保留原有shape中的dim值(反着看)
 */
struct ExpandDimsType {
  using AxisIndex = uint64_t;
  static constexpr size_t kMaxExpandSize = 56;

  ExpandDimsType() = default;

  /*
   * specify expand dims type by string:
   * 1: means expand dims here
   * 0: means use origin dim value here
   */
  explicit ExpandDimsType(const char *expand_dims_type) : size_(0), mask_(0) {
    if (expand_dims_type == nullptr) {
      return;
    }
    auto size = strlen(expand_dims_type);
    if (size > kMaxExpandSize) {
      // log error
      return;
    }

    size_ = size;
    for (AxisIndex i = 0; i < size; ++i) {
      if (expand_dims_type[i] == '1') {
        SetExpandIndex(i);
      }
    }
  }

  explicit ExpandDimsType(const int64_t &reshape_type_mask) : size_(0), mask_(0) {
    if (reshape_type_mask == 0) {
      return;
    }
    size_ = static_cast<size_t>(reshape_type_mask >> kMaxExpandSize);
    if (size_ > kMaxExpandSize) {
      return;
    }
    mask_ = reshape_type_mask & 0xff;
  }

  bool operator==(const ExpandDimsType &other) const {
    return size_ == other.size_ && mask_ == other.mask_;
  }

  AxisIndex GetFullSize() const {
    return size_;
  }

  void SetExpandIndex(AxisIndex index) {
    mask_ |= (1UL << index);
  }

  bool IsExpandIndex(AxisIndex index) const {
    return (1UL << index) & mask_;
  }

  ge::graphStatus Expand(const Shape &shape, Shape &out_shape) const {
    size_t shape_pos = 0;
    out_shape.SetDimNum(0);
    for (size_t out_shape_pos = 0; out_shape_pos < size_; ++out_shape_pos) {
      if (!IsExpandIndex(out_shape_pos)) {
        if (shape_pos >= shape.GetDimNum()) {
          return ge::GRAPH_FAILED;
        }
        out_shape.AppendDim(shape.GetDim(shape_pos++));
      } else {
        out_shape.AppendDim(1);
      }
    }

    for (; shape_pos < shape.GetDimNum(); ++shape_pos) {
      out_shape.AppendDim(shape.GetDim(shape_pos));
    }
    return ge::GRAPH_SUCCESS;
  }

  ge::graphStatus Expand(Shape &shape) const {
    // full_size:4, shape:[A,B], reshape_type:1010
    // shape:[A,B] + full_size:4 -> [A,B,1,1]
    size_t dim_size = shape.GetDimNum();
    for (size_t i = dim_size; i < size_; i++) {
      shape.AppendDim(1);
    }

    // shape:[A,B,1,1] + 1010 -> [A,1,B,1]
    for (int32_t i = static_cast<int32_t>(size_ - 1); i >= 0; --i) {
      if (!IsExpandIndex(i)) {
        if (dim_size > 0 && dim_size -1 < static_cast<size_t>(i)) {
          shape.SetDim(i, shape.GetDim(dim_size - 1));
          shape.SetDim(dim_size - 1, 1);
          dim_size--;
        }
      }
    }
    return ge::GRAPH_SUCCESS;
  }

 private:
  uint64_t size_ : 8;
  uint64_t mask_ : kMaxExpandSize;
};
}

#endif
