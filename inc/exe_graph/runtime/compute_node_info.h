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

#ifndef AIR_CXX_RUNTIME_V2_KERNEL_CONTEXT_EXTEND_COMPUTE_NODE_INFO_H_
#define AIR_CXX_RUNTIME_V2_KERNEL_CONTEXT_EXTEND_COMPUTE_NODE_INFO_H_
#include <type_traits>
#include <cstdint>
#include <cstddef>
#include "graph/types.h"
#include "storage_format.h"
#include "runtime_attrs.h"
namespace gert {
class AnchorInstanceInfo {
 public:
  AnchorInstanceInfo() = default;
  AnchorInstanceInfo(uint32_t instance_start, uint32_t instantiation_num)
      : instance_start_(instance_start), instantiation_num_(instantiation_num) {}
  uint32_t GetInstanceNum() const {
    return instantiation_num_;
  }
  uint32_t GetInstanceStart() const {
    return instance_start_;
  }
  void SetInstanceStart(uint32_t instance_start) {
    instance_start_ = instance_start;
  }
  void SetInstantiationNum(uint32_t instantiation_num) {
    instantiation_num_ = instantiation_num;
  }

 private:
  uint32_t instance_start_;
  uint32_t instantiation_num_;
};
static_assert(std::is_standard_layout<AnchorInstanceInfo>::value, "The class AnchorInstanceInfo must be a POD");

class CompileTimeTensorDesc {
 public:
  ge::DataType GetDataType() const {
    return data_type_;
  }
  const StorageFormat &GetFormat() const {
    return storage_format_;
  }
  ge::Format GetOriginFormat() const {
    return storage_format_.GetOriginFormat();
  }
  ge::Format GetStorageFormat() const {
    return storage_format_.GetStorageFormat();
  }
  ExpandDimsType GetExpandDimsType() const {
    return storage_format_.GetExpandDimsType();
  }
  void SetDataType(ge::DataType data_type) {
    data_type_ = data_type;
  }
  void SetStorageFormat(ge::Format format) {
    storage_format_.SetStorageFormat(format);
  }
  void SetOriginFormat(ge::Format format) {
    storage_format_.SetOriginFormat(format);
  }
  void SetExpandDimsType(ExpandDimsType expand_dims_type) {
    storage_format_.SetExpandDimsType(expand_dims_type);
  }

 private:
  ge::DataType data_type_;
  StorageFormat storage_format_;
};
static_assert(std::is_standard_layout<CompileTimeTensorDesc>::value, "The class CompileTimeTensorDesc must be a POD");

class ComputeNodeInfo {
 public:
  const char *GetNodeType() const {
    return node_type_;
  }
  const char *GetNodeName() const {
    return node_name_;
  }
  size_t GetIrInputsNum() const {
    return ir_inputs_num_;
  }
  size_t GetInputsNum() const {
    return inputs_num_;
  }
  size_t GetOutputsNum() const {
    return outputs_num_;
  }
  const AnchorInstanceInfo *GetInputInstanceInfo(size_t ir_index) const {
    if (ir_index >= ir_inputs_num_) {
      return nullptr;
    }
    auto inputs = reinterpret_cast<const AnchorInstanceInfo *>(&place_holder);
    return inputs + ir_index;
  }
  const CompileTimeTensorDesc *GetInputTdInfo(size_t index) const {
    if (index >= inputs_num_) {
      return nullptr;
    }
    auto inputs = reinterpret_cast<const CompileTimeTensorDesc *>(reinterpret_cast<const uint8_t *>(&place_holder) +
                                                                  sizeof(AnchorInstanceInfo) * ir_inputs_num_);
    return inputs + index;
  }
  const CompileTimeTensorDesc *GetOutputTdInfo(size_t index) const {
    if (index >= outputs_num_) {
      return nullptr;
    }
    auto outputs = reinterpret_cast<const CompileTimeTensorDesc *>(reinterpret_cast<const uint8_t *>(&place_holder) +
                                                                   sizeof(AnchorInstanceInfo) * ir_inputs_num_ +
                                                                   sizeof(CompileTimeTensorDesc) * inputs_num_);
    return outputs + index;
  }

  const RuntimeAttrs *GetAttrs() const {
    return reinterpret_cast<const RuntimeAttrs *>(reinterpret_cast<const uint8_t *>(&place_holder) +
                                                  sizeof(AnchorInstanceInfo) * ir_inputs_num_ +
                                                  sizeof(CompileTimeTensorDesc) * (inputs_num_ + outputs_num_));
  }
  void SetNodeType(const char *node_type) {
    node_type_ = node_type;
  }
  void SetNodeName(const char *node_name) {
    node_name_ = node_name;
  }
  AnchorInstanceInfo *MutableInputInstanceInfo(size_t ir_index);
  CompileTimeTensorDesc *MutableInputTdInfo(size_t index);
  CompileTimeTensorDesc *MutableOutputTdInfo(size_t index);
  RuntimeAttrs *MutableAttrs();
  static ge::graphStatus CalcSize(size_t ir_inputs_num, size_t inputs_num, size_t outputs_num, size_t &total_size);
  void Init(size_t ir_inputs_num, size_t inputs_num, size_t outputs_num, const char *node_name, const char *node_type);

  ComputeNodeInfo() = delete;
  ComputeNodeInfo(const ComputeNodeInfo &) = delete;
  ComputeNodeInfo(ComputeNodeInfo &&) = delete;
  ComputeNodeInfo &operator=(const ComputeNodeInfo &) = delete;
  ComputeNodeInfo &operator=(ComputeNodeInfo &&) = delete;

 private:
  const char *node_type_;
  const char *node_name_;
  size_t ir_inputs_num_;
  size_t inputs_num_;
  size_t outputs_num_;

  // following by AnchorInstanceInfo, inputs-outputs-CompileTimeTensorDesc, RuntimeAttrs
  uint64_t place_holder;
};
static_assert(std::is_standard_layout<ComputeNodeInfo>::value, "The class ComputeNodeInfo must be a POD");
}  // namespace gert

#endif  //AIR_CXX_RUNTIME_V2_KERNEL_CONTEXT_EXTEND_COMPUTE_NODE_INFO_H_
