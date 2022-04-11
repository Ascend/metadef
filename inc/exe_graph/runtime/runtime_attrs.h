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

#ifndef AIR_CXX_RUNTIME_V2_KERNEL_RUNTIME_ATTRS_H_
#define AIR_CXX_RUNTIME_V2_KERNEL_RUNTIME_ATTRS_H_
#include <cstdint>
#include <type_traits>

#include "runtime_attrs_def.h"

namespace gert {
class RuntimeAttrs {
 public:
  template<typename T>
  const T *GetAttrPointer(size_t index) const {
    return reinterpret_cast<const T *>(GetPointerByIndex(index));
  }

  size_t GetAttrNum() const;

  RuntimeAttrs() = delete;
  RuntimeAttrs(const RuntimeAttrs &) = delete;
  RuntimeAttrs(RuntimeAttrs &&) =  delete;
  RuntimeAttrs &operator=(const RuntimeAttrs &) = delete;
  RuntimeAttrs &operator=(RuntimeAttrs &&) = delete;

 private:
  const void *GetPointerByIndex(size_t index) const;

 private:
  uint64_t placeholder_;
};
static_assert(std::is_standard_layout<RuntimeAttrs>::value, "This class must be a POD");
}  // namespace gert
#endif  // AIR_CXX_RUNTIME_V2_KERNEL_RUNTIME_ATTRS_H_
