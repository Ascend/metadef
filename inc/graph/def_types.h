/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#ifndef INC_GRAPH_DEF_TYPES_H_
#define INC_GRAPH_DEF_TYPES_H_

#include <atomic>
#include <memory>
#include <vector>
#include "graph/attr_value_serializable.h"
#include "graph/buffer.h"
namespace ge {
// used for data type of DT_STRING
struct StringHead {
  uint64_t addr;  // the addr of string
  uint64_t len;   // the length of string
};
}  // namespace ge

inline uint64_t PtrToValue(const void *ptr) {
  return static_cast<const uint64_t>(reinterpret_cast<const uintptr_t>(ptr));
}

inline void *ValueToPtr(const uint64_t value) {
  return reinterpret_cast<void *>(static_cast<const uintptr_t>(value));
}

template<typename TI, typename TO>
inline TO *PtrToPtr(TI *ptr) {
  return reinterpret_cast<TO *>(ptr);
}

template<typename T>
inline T *PtrAdd(T *ptr, const size_t max_buf_len, const size_t idx) {
  if ((ptr != nullptr) && (idx < max_buf_len)) {
    return reinterpret_cast<T *>(ptr + idx);
  }
  return nullptr;
}

#endif  // INC_GRAPH_DEF_TYPES_H_
