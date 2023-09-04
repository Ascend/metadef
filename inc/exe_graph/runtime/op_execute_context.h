/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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

#ifndef METADEF_CXX_INC_EXE_GRAPH_RUNTIME_OP_EXECUTE_CONTEXT_H_
#define METADEF_CXX_INC_EXE_GRAPH_RUNTIME_OP_EXECUTE_CONTEXT_H_
#include <type_traits>
#include "exe_graph/runtime/shape.h"
#include "exe_graph/runtime/tensor.h"
#include "exe_graph/runtime/runtime_attrs.h"
#include "exe_graph/runtime/extended_kernel_context.h"
#include "ge_common/ge_allocator.h"
#include "graph/small_vector.h"
#include "register/kernel_registry.h"
namespace gert {
using rtStream = void *;
struct OpExecuteOptions {
  bool deterministic; // 确定性计算
  bool allow_hf32; // hf32
  int32_t precision_mode; // 精度模式
};

enum OpExecuteInputExtendIndex : uint32_t {
  kAllocate,
  kRtStream,
  kExecuteOption,
  kBlockMemory,
  // add new extend input here
  kNum
};
// 用来初始化SmallVector的N
const constexpr size_t kBlockMemoryInitNum = 10UL;
/**
 * Aclnn kernel的context
 */
class OpExecuteContext : public ExtendedKernelContext {
 public:
  /**
   * 根据输入index，获取输出tensor指针
   *
   * **注意：只有在`IMPL_OP`实现算子时， 将对应输入设置为数据依赖后，才可以调用此接口获取tensor，否则行为是未定义的。**
   * @param index 输入index
   * @return 输入tensor指针，index非法时，返回空指针
   */
  const Tensor *GetInputTensor(const size_t index) const {
    return GetInputPointer<Tensor>(index);
  }
  /**
   * 基于算子IR原型定义，获取`OPTIONAL_INPUT`类型的输入tensor指针
   * @param ir_index IR原型定义中的index
   * @return tensor指针，index非法，或该INPUT没有实例化时，返回空指针
   */
  const Tensor *GetOptionalInputTensor(const size_t ir_index) const {
    return GetDynamicInputPointer<Tensor>(ir_index, 0);
  }
  /**
   * 基于算子IR原型定义，获取`DYNAMIC_INPUT`类型的输入Tensor指针
   * @param ir_index IR原型定义中的index
   * @param relative_index 该输入实例化后的相对index，例如某个DYNAMIC_INPUT实例化了3个输入，那么relative_index的有效范围是[0,2]
   * @return tensor指针，index或relative_index非法时，返回空指针
   */
  const Tensor *GetDynamicInputTensor(const size_t ir_index, const size_t relative_index) const {
    return GetDynamicInputPointer<Tensor>(ir_index, relative_index);
  }

  /**
   * 根据输出index，获取输出tensor指针
   *
   * **注意：只有在`IMPL_OP`实现算子时， 将对应输入设置为数据依赖后，才可以调用此接口获取tensor，否则行为是未定义的。**
   * @param index 输出index
   * @return 输出tensor指针，index非法时，返回空指针
   */
  const Tensor *GetOutputTensor(const size_t index) const {
    const size_t input_num = GetComputeNodeInputNum();
    return GetInputPointer<Tensor>(input_num + index);
  }

  /**
   * 基于算子IR原型定义，获取`DYNAMIC_OUTPUT`类型的输入Tensor指针
   * @param ir_index IR原型定义中的index
   * @param relative_index 该输入实例化后的相对index，例如某个DYNAMIC_OUTPUT实例化了3个输入，那么relative_index的有效范围是[0,2]
   * @return tensor指针，index或relative_index非法时，返回空指针
   */
  const Tensor *GetDynamicOutputTensor(const size_t ir_index, const size_t relative_index) const {
    const size_t input_num = GetComputeNodeInputNum();
    return GetDynamicInputPointer<Tensor>(input_num + ir_index, relative_index);
  }

  /**
   * 获取stream
   * @return rtStream, aclnn算子下发的流
   */
  rtStream GetStream() const {
    const size_t input_num = GetComputeNodeInputNum();
    const size_t output_num = GetComputeNodeOutputNum();
    auto stream = GetInputPointer<rtStream>(input_num + output_num + kRtStream);
    if (stream == nullptr) {
      return nullptr;
    }
    return *stream;
  }

  /**
   * 申请workspace内存大小
   * @param size 申请内存的大小
   * @return void *，内存地址
   */
  void *MallocWorkspace(const size_t size) {
    const size_t input_num = GetComputeNodeInputNum();
    const size_t output_num = GetComputeNodeOutputNum();
    auto memory_vec =
        MutableInputPointer<ge::SmallVector<ge::MemBlock *, kBlockMemoryInitNum>>(
        input_num + output_num + kBlockMemory);
    if (memory_vec == nullptr) {
      return nullptr;
    }
    ge::Allocator *allocator = MutableInputPointer<ge::Allocator>(input_num + output_num + kAllocate);
    if (allocator == nullptr) {
      return nullptr;
    }
    auto mem_block = allocator->Malloc(size);
    if (mem_block == nullptr) {
      return nullptr;
    }
    memory_vec->emplace_back(mem_block);
    return mem_block->GetAddr();
  }

  /**
   * 释放workspace内存
   */
  void FreeWorkspace() {
    const size_t input_num = GetComputeNodeInputNum();
    const size_t output_num = GetComputeNodeOutputNum();
    auto memory_vec =
        MutableInputPointer<ge::SmallVector<ge::MemBlock *, kBlockMemoryInitNum>>(
        input_num + output_num + kBlockMemory);
    if (memory_vec == nullptr) {
      return;
    }
    for (size_t i = 0UL; i < memory_vec->size(); i++) {
      if (memory_vec->at(i) != nullptr) {
        memory_vec->at(i)->Free();
      }
    }
    memory_vec->clear();
  }

  /**
   * 获取确定性计算模式
   * @return bool，是否开启确定性计算
   */
  bool GetDeterministic() {
    const size_t input_num = GetComputeNodeInputNum();
    const size_t output_num = GetComputeNodeOutputNum();
    const OpExecuteOptions *options =
        GetInputPointer<OpExecuteOptions>(input_num + output_num + kExecuteOption);
    if (options == nullptr) {
      return false;
    }
    return options->deterministic;
  }

  /**
   * 获取allow_hf32
   * @return bool，是否开启hf32
   */
  bool GetAllowHf32() {
    const size_t input_num = GetComputeNodeInputNum();
    const size_t output_num = GetComputeNodeOutputNum();
    const OpExecuteOptions *options =
        GetInputPointer<OpExecuteOptions>(input_num + output_num + kExecuteOption);
    if (options == nullptr) {
      return false;
    }
    return options->allow_hf32;
  }
  /**
   * 获取精度模式
   * @return int32，精度模式
   */
  int32_t GetPrecisionMode() {
    const size_t input_num = GetComputeNodeInputNum();
    const size_t output_num = GetComputeNodeOutputNum();
    const OpExecuteOptions *options =
        GetInputPointer<OpExecuteOptions>(input_num + output_num + kExecuteOption);
    if (options == nullptr) {
      return 0UL;
    }
    return options->precision_mode;
  }
};
static_assert(std::is_standard_layout<OpExecuteContext>::value, "The class OpExecuteContext must be a POD");
}  // namespace gert
#endif  // METADEF_CXX_INC_EXE_GRAPH_RUNTIME_OP_EXECUTE_CONTEXT_H_
