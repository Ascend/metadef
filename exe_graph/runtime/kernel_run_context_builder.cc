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
#include "exe_graph/runtime/kernel_run_context_builder.h"
#include "exe_graph/lowering/bg_kernel_context_extend.h"
#include "graph/compute_graph.h"

namespace gert {
KernelContextHolder KernelRunContextBuilder::Build(ge::OpDescPtr &op_desc) {
  KernelContextHolder holder;
  size_t size = sizeof(KernelRunContext) + sizeof(AsyncAnyValue *) * (inputs_.size() + outputs_.size());
  holder.context_holder = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[size]);
  if (holder.context_holder == nullptr) {
    GELOGE(ge::GRAPH_FAILED, "Create context holder failed.");
    return holder;
  }
  size_t extend_info_size;
  holder.compute_node_extend_holder =
      bg::CreateComputeNodeInfo(MakeNode(op_desc), holder.buffer_pool, extend_info_size);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(holder.compute_node_extend_holder.get());
  compute_node_info->SetNodeName(
      holder.buffer_pool.GetBufById(reinterpret_cast<size_t>(compute_node_info->GetNodeName())));
  compute_node_info->SetNodeType(
      holder.buffer_pool.GetBufById(reinterpret_cast<size_t>(compute_node_info->GetNodeType())));
  holder.context = reinterpret_cast<KernelContext *>(holder.context_holder.get());
  auto kernel_run_context = holder.context->GetContext();
  kernel_run_context->input_size = inputs_.size();
  kernel_run_context->output_size = outputs_.size();
  kernel_run_context->compute_node_info = compute_node_info;
  kernel_run_context->output_start = &(kernel_run_context->values[kernel_run_context->input_size]);
  holder.value_holder.resize(inputs_.size() + outputs_.size());
  for (size_t i = 0UL; i < holder.value_holder.size(); ++i) {
    kernel_run_context->values[i] = &holder.value_holder[i];
  }
  for (size_t i = 0UL; i < inputs_.size(); ++i) {
    holder.value_holder[i].data.pointer = inputs_[i];
  }
  for (size_t i = 0UL; i < outputs_.size(); ++i) {
    holder.value_holder[inputs_.size() + i].data.pointer = outputs_[i];
  }
  return holder;
}

ge::NodePtr KernelRunContextBuilder::MakeNode(ge::OpDescPtr &op_desc) const {
  auto graph = std::make_shared<ge::ComputeGraph>("tmp");
  return graph->AddNode(op_desc);
}
}  // namespace gert