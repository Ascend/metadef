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
#include "shape.h"
#include "graph/ge_error_codes.h"

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
      if (expand_dims_type[i] == '0') {
        SetUseOriginDim(i);
      }
    }
  }

  AxisIndex GetFullSize() const {
    return size_;
  }

  void SetUseOriginDim(AxisIndex index) {
    mask_ |= (1UL << index);
  }

  bool UseOriginDim(AxisIndex index) const {
    return (1UL << index) & mask_;
  }

  bool IsExpandIndex(AxisIndex index) const {
    return !UseOriginDim(index);
  }

  ge::graphStatus Expand(const Shape &shape, Shape &out_shape) const {
    size_t shape_pos = 0;
    out_shape.SetDimNum(0);
    for (size_t out_shape_pos = 0; out_shape_pos < size_; ++out_shape_pos) {
      if (UseOriginDim(out_shape_pos)) {
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

 private:
  uint64_t size_ : 8;
  uint64_t mask_ : kMaxExpandSize;
};
}

#endif
