/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef METADEF_CXX_INC_EXE_GRAPH_RUNTIME_EXTENDED_KERNEL_CONTEXT_H_
#define METADEF_CXX_INC_EXE_GRAPH_RUNTIME_EXTENDED_KERNEL_CONTEXT_H_
#include <type_traits>
#include "kernel_context.h"
#include "context_extend.h"

namespace gert {
class ExtendedKernelContext : protected KernelContext {
 public:
  /**
   * 根据index获取节点输入的Tensor description
   * @param index 输入index
   * @return 输入TensorDesc的指针，index非法时，返回空指针
   */
  const CompileTimeTensorDesc *GetInputDesc(size_t index) const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetInputTdInfo(index);
  }
  /**
   * 根据index获取节点输出的Tensor description
   * @param index 输出index
   * @return 输出TensorDesc的指针，index非法时，返回空指针
   */
  const CompileTimeTensorDesc *GetOutputDesc(size_t index) const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetOutputTdInfo(index);
  }
  /**
   * 根据IR原型中的index，获取在节点上的实例化信息
   * @param ir_index IR原型中定义的index
   * @return 实例化信息
   */
  const AnchorInstanceInfo *GetIrInputInstanceInfo(size_t ir_index) const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetInputInstanceInfo(ir_index);
  }
  /**
   * 获取计算节点的输入数量
   * @return 计算节点的输入数量
   */
  size_t GetComputeNodeInputNum() const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return 0;
    }
    return compute_node_info->GetInputsNum();
  }
  /**
   * 获取计算节点的属性，仅IR原型中定义的属性可被获取到
   * @return 计算节点的属性
   */
  const RuntimeAttrs *GetAttrs() const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetAttrs();
  }
  /**
   * 获取计算节点的类型
   * @return 计算节点的类型
   */
  const char *GetNodeType() const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetNodeType();
  }
  /**
   * 获取计算节点的name
   * @return 计算节点的name
   */
  const char *GetNodeName() const {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }
    return compute_node_info->GetNodeName();
  }
  /**
   * 获取本kernel对应的计算节点的信息
   * @return 计算节点的信息
   */
  const ComputeNodeInfo *GetComputeNodeInfo() const {
    return reinterpret_cast<const ComputeNodeInfo *>(GetComputeNodeExtend());
  }
  /**
   * 获取本kernel的name
   * @return 本kernel的name
   */
  const char *GetKernelName() const {
    auto extend_info = GetExtendInfo();
    if (extend_info == nullptr) {
      return nullptr;
    }
    return extend_info->GetKernelName();
  }
  /**
   * 获取本kernel的type
   * @return 本kernel的type
   */
  const char *GetKernelType() const {
    auto extend_info = GetExtendInfo();
    if (extend_info == nullptr) {
      return nullptr;
    }
    return extend_info->GetKernelType();
  }
  /**
   * 获取本kernel的扩展信息
   * @return 本kernel的扩展信息
   */
  const KernelExtendInfo *GetExtendInfo() const {
    return reinterpret_cast<const KernelExtendInfo *>(GetKernelExtend());
  }

 protected:
  template<typename T, size_t Offset = 0>
  const T *GetDynamicInputPointer(size_t ir_index, size_t relative_ins_index) const {
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
#endif  // METADEF_CXX_INC_EXE_GRAPH_RUNTIME_EXTENDED_KERNEL_CONTEXT_H_
