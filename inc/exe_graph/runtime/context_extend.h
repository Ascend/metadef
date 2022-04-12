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

#ifndef AIR_CXX_RUNTIME_V2_KERNEL_CONTEXT_EXTEND_H_
#define AIR_CXX_RUNTIME_V2_KERNEL_CONTEXT_EXTEND_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <memory>
#include "compute_node_info.h"
#include "kernel_info.h"

namespace gert {
class KernelExtendInfo {
 public:
  const ComputeNodeInfo &GetComputeNodeInfo() const {
    return compute_node_info;
  }
  ComputeNodeInfo &MutableComputeNodeInfo() {
    return compute_node_info;
  }

  KernelExtendInfo() = delete;
  KernelExtendInfo(const KernelExtendInfo &) = delete;
  KernelExtendInfo(KernelExtendInfo &&) = delete;
  KernelExtendInfo &operator=(const KernelExtendInfo &) = delete;
  KernelExtendInfo &operator=(KernelExtendInfo &&) = delete;

 private:
  ComputeNodeInfo compute_node_info;
};
static_assert(std::is_standard_layout<KernelExtendInfo>::value, "The class KernelExtendInfo must be a POD");

}  // namespace gert
#endif  //AIR_CXX_RUNTIME_V2_KERNEL_CONTEXT_EXTEND_H_
