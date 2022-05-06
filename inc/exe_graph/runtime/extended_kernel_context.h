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

#ifndef AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_EXTENDED_KERNEL_CONTEXT_H_
#define AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_EXTENDED_KERNEL_CONTEXT_H_
#include <type_traits>
#include "kernel_context.h"
#include "context_extend.h"

namespace gert {
class ExtendedKernelContext : protected KernelContext {
 public:
  const CompileTimeTensorDesc *GetInputDesc(size_t index) const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetInputTdInfo(index);
  }
  const CompileTimeTensorDesc *GetOutputDesc(size_t index) const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetOutputTdInfo(index);
  }
  const AnchorInstanceInfo *GetIrInputInstanceInfo(size_t ir_index) const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetInputInstanceInfo(ir_index);
  }
  size_t GetComputeNodeInputNum() const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return 0;
    }
    return compute_node_info->GetInputsNum();
  }
  const RuntimeAttrs *GetAttrs() const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetAttrs();
  }

  const char *GetNodeType() const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetNodeType();
  }
  const char *GetNodeName() const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetNodeName();
  }
  const ComputeNodeInfo *GetComputeNodeInfo() const {
    return reinterpret_cast<const ComputeNodeInfo *>(GetComputeNodeExtend());
  }

  const char *GetKernelName() const {
    auto extend_info = GetExtendInfo();
    if (extend_info == nullptr) {
      return nullptr;
    }
    return extend_info->GetKernelName();
  }
  const char *GetKernelType() const {
    auto extend_info = GetExtendInfo();
    if (extend_info == nullptr) {
      return nullptr;
    }
    return extend_info->GetKernelType();
  }
  const KernelExtendInfo *GetExtendInfo() const {
    return reinterpret_cast<const KernelExtendInfo *>(GetKernelExtend());
  }

 protected:
  template<typename T, size_t Offset = 0>
  const T *GetDynamicInputPointer(size_t ir_index, size_t relative_ins_index) {
    auto ins_info = GetIrInputInstanceInfo(ir_index);
    if (ins_info == nullptr) {
      return nullptr;
    }
    if (ins_info->GetInstanceNum() <= relative_ins_index) {
      return nullptr;
    }
    return GetInputPointer<T>(Offset + ins_info->GetInstanceStart() + relative_ins_index);
  }
};
static_assert(std::is_standard_layout<ExtendedKernelContext>::value,
              "The class ExtendedKernelRunContext must be a POD");
}  // namespace gert
#endif  //AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_EXTENDED_KERNEL_CONTEXT_H_