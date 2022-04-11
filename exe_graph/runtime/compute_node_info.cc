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

#include "exe_graph/runtime/compute_node_info.h"
namespace gert {
ge::graphStatus ComputeNodeInfo::CalcSize(size_t ir_inputs_num, size_t inputs_num, size_t outputs_num,
                                          size_t &total_size) {
  size_t ir_inputs_size;
  size_t inputs_size;
  size_t outputs_size;
  if (ge::MulOverflow(sizeof(AnchorInstanceInfo), ir_inputs_num, ir_inputs_size)) {
    return ge::GRAPH_FAILED;
  }
  if (ge::MulOverflow(sizeof(CompileTimeTensorDesc), inputs_num, inputs_size)) {
    return ge::GRAPH_FAILED;
  }
  if (ge::MulOverflow(sizeof(CompileTimeTensorDesc), outputs_num, outputs_size)) {
    return ge::GRAPH_FAILED;
  }

  total_size = sizeof(ComputeNodeInfo);
  if (ge::AddOverflow(total_size, ir_inputs_size, total_size)) {
    return ge::GRAPH_FAILED;
  }
  if (ge::AddOverflow(total_size, inputs_size, total_size)) {
    return ge::GRAPH_FAILED;
  }
  if (ge::AddOverflow(total_size, outputs_size, total_size)) {
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}
// todo 不需要total_size了，下一步去掉
void ComputeNodeInfo::Init(size_t ir_inputs_num, size_t inputs_num, size_t outputs_num, size_t total_size,
                           const char *node_name, const char *node_type) {
  ir_inputs_num_ = ir_inputs_num;
  inputs_num_ = inputs_num;
  outputs_num_ = outputs_num;
  node_name_ = node_name;
  node_type_ = node_type;
}
size_t ComputeNodeInfo::GetSelfSize() const {
  size_t total_size = 0;
  if (CalcSize(ir_inputs_num_, inputs_num_, outputs_num_, total_size) != ge::GRAPH_SUCCESS) {
    return 0;
  }
  return total_size;
}
AnchorInstanceInfo *ComputeNodeInfo::MutableInputInstanceInfo(size_t ir_index) {
  return const_cast<AnchorInstanceInfo *>(GetInputInstanceInfo(ir_index));
}
CompileTimeTensorDesc *ComputeNodeInfo::MutableInputTdInfo(size_t index) {
  return const_cast<CompileTimeTensorDesc *>(GetInputTdInfo(index));
}
CompileTimeTensorDesc *ComputeNodeInfo::MutableOutputTdInfo(size_t index) {
  return const_cast<CompileTimeTensorDesc *>(GetOutputTdInfo(index));
}
RuntimeAttrs *ComputeNodeInfo::MutableAttrs() {
  return const_cast<RuntimeAttrs *>(GetAttrs());
}
void ComputeNodeInfo::SetIrInputsNum(size_t ir_inputs_num) {
  ir_inputs_num_ = ir_inputs_num;
}
void ComputeNodeInfo::SetInputsNum(size_t inputs_num) {
  inputs_num_ = inputs_num;
}
void ComputeNodeInfo::SetOutputsNum(size_t outputs_num) {
  outputs_num_ = outputs_num;
}
}  // namespace gert