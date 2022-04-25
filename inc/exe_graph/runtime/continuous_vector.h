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

#ifndef AIR_CXX_RUNTIME_V2_KERNEL_CONTINUOUS_VECTOR_H_
#define AIR_CXX_RUNTIME_V2_KERNEL_CONTINUOUS_VECTOR_H_
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include "graph/ge_error_codes.h"
#include "graph/utils/math_util.h"
#include "graph/debug/ge_log.h"
namespace gert {
class ContinuousVector {
 public:
  template<typename T>
  static std::unique_ptr<uint8_t[]> Create(size_t capacity, size_t &total_size) {
    if (ge::MulOverflow(capacity, sizeof(T), total_size)) {
      return nullptr;
    }
    if (ge::AddOverflow(total_size, sizeof(ContinuousVector), total_size)) {
      return nullptr;
    }
    auto holder = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_size]);
    if (holder == nullptr) {
      return nullptr;
    }
    reinterpret_cast<ContinuousVector *>(holder.get())->Init(capacity);
    return holder;
  }
  template<typename T>
  static std::unique_ptr<uint8_t[]> Create(size_t capacity) {
    size_t total_size;
    return Create<T>(capacity, total_size);
  }
  void Init(size_t capacity) {
    capacity_ = capacity;
    size_ = 0;
  }
  size_t GetSize() const {
    return size_;
  }
  ge::graphStatus SetSize(size_t size) {
    if (size > capacity_) {
      GE_LOGE("Failed to set size for ContinuousVector, size(%zu) > cap(%zu)", size, capacity_);
      return ge::GRAPH_FAILED;
    }
    size_ = size;
    return ge::GRAPH_SUCCESS;
  }
  size_t GetCapacity() const {
    return capacity_;
  }
  const void *GetData() const {
    return elements;
  }
  void *MutableData() {
    return elements;
  }
 private:
  size_t capacity_;
  size_t size_;
  uint8_t elements[8];
};
static_assert(std::is_standard_layout<ContinuousVector>::value, "The ContinuousVector must be a POD");
}  // namespace gert
#endif  //AIR_CXX_RUNTIME_V2_KERNEL_CONTINUOUS_VECTOR_H_
