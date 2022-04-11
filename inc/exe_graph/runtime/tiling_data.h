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
#ifndef INC_145790231B1246659C38D0701EABDFB1_H
#define INC_145790231B1246659C38D0701EABDFB1_H
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "graph/utils/math_util.h"
#include "framework/common/debug/ge_log.h"
#include "graph/ge_error_codes.h"

namespace gert {
struct TilingData {
 public:
  size_t GetCapacity() const {
    return capacity_;
  }
  size_t GetDataSize() const {
    return data_size_;
  }
  void SetDataSize(size_t size) {
    data_size_ = size;
  }
  void *GetData() {
    return data_;
  }
  const void *GetData() const {
    return data_;
  }

  template<typename T, typename std::enable_if<std::is_standard_layout<T>::value, int>::type = 0>
  ge::graphStatus Append(const T &data) {
    size_t after_size;
    if (ge::AddOverflow(data_size_, sizeof(data), after_size)) {
      return ge::GRAPH_FAILED;
    }
    if (after_size > capacity_) {
      return ge::GRAPH_FAILED;
    }
    *reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(data_) + GetDataSize()) = data;
    data_size_ = after_size;
    return ge::GRAPH_SUCCESS;
  }

  static std::unique_ptr<uint8_t[]> CreateFixed(size_t fix_size) {
    auto buf = CreateCap(fix_size);
    if (buf != nullptr) {
      reinterpret_cast<TilingData *>(buf.get())->data_size_ = fix_size;
    }
    return buf;
  }
  static std::unique_ptr<uint8_t[]> CreateCap(size_t cap_size) {
    size_t total_size;
    if (ge::AddOverflow(sizeof(TilingData), cap_size, total_size)) {
      return nullptr;
    }
    auto td_buf = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_size]());
    if (td_buf == nullptr) {
      return nullptr;
    }
    auto td = reinterpret_cast<TilingData *>(td_buf.get());
    td->capacity_ = cap_size;
    td->data_ = td_buf.get() + sizeof(TilingData);
    return td_buf;
  }

  TilingData(const TilingData &) = delete;
  TilingData(TilingData &&) = delete;
  TilingData operator=(const TilingData &) = delete;
  TilingData operator=(TilingData &&) = delete;

 private:
  TilingData() = default;
  size_t capacity_;
  size_t data_size_;
  void *data_;
};

template<typename T>
TilingData &operator<<(TilingData &out, const T &data) {
  out.Append(data);  // we can not throw exception, so callers can not get the error information
  return out;
}
static_assert(std::is_standard_layout<TilingData>::value, "The class TilingData must be a POD");
}  // namespace gert

#endif
