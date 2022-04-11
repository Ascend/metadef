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
#ifndef INC_6C162F64C3674AE1962192667C4E217E_H
#define INC_6C162F64C3674AE1962192667C4E217E_H

#include <array>
#include <vector>
#include <iostream>
#include <cstring>
#include <type_traits>
#include "graph/utils/math_util.h"

namespace gert {
struct Shape {
  // todo 禁用构造函数(防止使用者直接在栈上创建实例，导致ABI不兼容）
  Shape() = default;
  Shape(std::initializer_list<int64_t> args) : size_(args.size()) {
    int i = 0;
    for (auto it = args.begin(); it != args.end(); ++it) {
      data_[i++] = (*it);
    }
  }

  bool operator==(const Shape &rht) const {
    if (this->size_ != rht.size_) {
      return false;
    }
    // TODO use vector cmp;
    for (size_t i = 0; i < this->size_; i++) {
      if (this->data_[i] != rht.data_[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const Shape &rht) const {
    return !(*this == rht);
  }

  int64_t GetShapeSize() const {
    int64_t shape_size = 1;
    for (size_t i = 0; i < size_; ++i) {
      if (ge::MulOverflow(shape_size, data_[i], shape_size)) {
        return -1;
      }
    }
    return shape_size;
  }
  bool IsScalar() const {
    return size_ == 0L;
  }
  size_t GetDimNum() const {
    return size_;
  }
  void SetDimNum(size_t size) {
    this->size_ = size;
  }

  int64_t GetDim(size_t idx) const {
    return data_[idx];
  }

  int64_t operator[](size_t idx) const {
    return GetDim(idx);
  }

  int64_t GetDimNotLine(size_t idx) const;

  void SetDim(size_t idx, int64_t value) {
    if (idx >= 8) {
      return;
    }
    data_[idx] = value;
    this->size_ = (this->size_ < idx) ? idx : this->size_;
  }

  Shape& AppendDim(int64_t value) {
    if (this->size_ >= 8) {
      return *this;
    }
    data_[this->size_++] = value;
    return *this;
  }

 private:
  size_t size_;
  std::array<int64_t, 8> data_;
};
static_assert(std::is_standard_layout<Shape>::value, "The class must be a POD");
}  // namespace gert

#endif
