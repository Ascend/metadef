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
#ifndef AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_TILING_CONTEXT_H_
#define AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_TILING_CONTEXT_H_
#include "storage_shape.h"
#include "tensor.h"
#include "continuous_vector.h"
#include "extended_kernel_context.h"
#include "tiling_data.h"
namespace gert {
class TilingContext : public ExtendedKernelContext {
 public:
  /*
   * todo 有一种更高效的排布是把固定个数的放到前面，可以节省计算输入输出个数的时间。但是这么做的缺点是，如果写tiling的人没注意，
   *   使用`KernelRunContextCpp::GetInputPointer`来获取输入shape，会出错
   * inputs, tiling的inputs以如下顺序排列：
   * inputs[0~n): 所有输入的storage-shape
   * inputs[n~n+m): 所有输出的storage-shape
   * inputs[n+m]: CompileInfo
   * inputs[n+m+1): tiling_func
   */
  const void *GetCompileInfo() {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }

    size_t index = compute_node_info->GetInputsNum() + compute_node_info->GetOutputsNum();
    auto av = GetInput(index);
    if (av == nullptr) {
      return nullptr;
    }
    return av->GetValue<void *>();
  }
  const StorageShape *GetInputShape(size_t index) {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }

    if (index >= compute_node_info->GetInputsNum()) {
      return nullptr;
    }
    return GetInputPointer<StorageShape>(index);
  }
  const Tensor *GetInputTensor(size_t index) {
    return GetInputPointer<Tensor>(index);
  }
  const StorageShape *GetOptionalInputShape(size_t ir_index) {
    return GetDynamicInputPointer<StorageShape>(ir_index, 0);
  }
  const StorageShape *GetDynamicInputShape(size_t ir_index, size_t relative_index) {
    return GetDynamicInputPointer<StorageShape>(ir_index, relative_index);
  }
  const StorageShape *GetOutputShape(size_t index) {
    auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return nullptr;
    }

    if (index >= compute_node_info->GetOutputsNum()) {
      return nullptr;
    }

    size_t offset = compute_node_info->GetInputsNum();
    return GetInputPointer<StorageShape>(offset + index);
  }

  /*
   * outputs, tiling的outputs以如下顺序排列：
   * outputs[0]: tiling-key
   * outputs[1]: block-dim
   * outputs[2]: atomic-clean-flag
   * outputs[3]: tiling-data
   * outputs[4]: workspace sizes
   */
  enum TilingOutputIndex {
    kOutputTilingKey,
    kOutputBlockDim,
    kOutputAtomicCleanFlag,
    kOutputTilingData,
    kOutputWorkspace,

    // add new output definitions here
    kOutputNum
  };

  ge::graphStatus SetTilingKey(uint64_t tiling_key) {
    auto p = GetOutputPointer<uint64_t>(kOutputTilingKey);
    if (p == nullptr) {
      return ge::GRAPH_FAILED;
    }
    *p = tiling_key;
    return ge::GRAPH_SUCCESS;
  }
  ge::graphStatus SetBlockDim(uint32_t block_dim) {
    auto p = GetOutputPointer<uint32_t>(kOutputBlockDim);
    if (p == nullptr) {
      return ge::GRAPH_FAILED;
    }
    *p = block_dim;
    return ge::GRAPH_SUCCESS;
  }
  ge::graphStatus SetNeedAtomic(bool atomic) {
    auto p = GetOutputPointer<bool>(kOutputAtomicCleanFlag);
    if (p == nullptr) {
      return ge::GRAPH_FAILED;
    }
    *p = atomic;
    return ge::GRAPH_SUCCESS;
  }
  template<typename T>
  T *GetTilingData() {
    auto tiling_data = GetRawTilingData();
    if (tiling_data == nullptr) {
      return nullptr;
    }
    if (tiling_data->GetCapacity() < sizeof(T)) {
      return nullptr;
    }
    tiling_data->SetDataSize(sizeof(T));
    return reinterpret_cast<T *>(tiling_data->GetData());
  }

  TilingData *GetRawTilingData() {
    return *GetOutputPointer<TilingData *>(kOutputTilingData);
  }
  size_t *GetWorkspaceSizes(size_t workspace_count) {
    auto workspace = GetOutputPointer<ContinuousVector>(kOutputWorkspace);
    if (workspace == nullptr) {
      return nullptr;
    }
    if (workspace_count > workspace->GetCapacity()) {
      return nullptr;
    }
    workspace->SetSize(workspace_count);
    return reinterpret_cast<size_t *>(workspace->MutableData());
  }
};
static_assert(std::is_standard_layout<TilingContext>::value, "The class TilingContext must be a POD");
}  // namespace gert
#endif  //AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_TILING_CONTEXT_H_
