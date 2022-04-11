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
#include "kernel_run_context_facker.h"
#include "graph/compute_graph.h"
#include "exe_graph/lowering/bg_kernel_context_extend.h"
#include "exe_graph/runtime/tiling_context.h"

namespace gert {
KernelRunContextHolder BuildKernelRunContext(size_t input_num, size_t output_num) {
  KernelRunContextHolder holder;
  size_t size = sizeof(KernelRunContext) + sizeof(AsyncAnyValue *) * (input_num + output_num);
  holder.context_holder = std::unique_ptr<uint8_t[]>(new uint8_t[size]);
  memset(holder.context_holder.get(), 0xff, size);
  holder.value_holder.resize(input_num + output_num);
  holder.extend_info_holder = KernelExtendInfo::Create(input_num, input_num, output_num, "Unknown", "Unknown");

  holder.context = reinterpret_cast<KernelRunContext *>(holder.context_holder.get());
  holder.context->input_size = input_num;
  holder.context->output_size = output_num;
  holder.context->extend_info = holder.extend_info_holder.get();
  holder.context->output_start = &(holder.context->values[holder.context->input_size]);

  for (size_t i = 0; i < input_num + output_num; ++i) {
    holder.context->values[i] = &holder.value_holder[i];
  }
  return holder;
}
KernelRunContextFaker &KernelRunContextFaker::KernelIONum(size_t input_num, size_t output_num) {
  kernel_input_num_ = input_num;
  kernel_output_num_ = output_num;
  return *this;
}
KernelRunContextFaker &KernelRunContextFaker::NodeIoNum(size_t input_num, size_t output_num) {
  node_input_num_ = input_num;
  node_output_num_ = output_num;
  node_input_tds_.resize(input_num);
  node_output_tds_.resize(output_num);
  return *this;
}
KernelRunContextFaker &KernelRunContextFaker::IrInputNum(size_t input_num) {
  ir_instance_num_ = std::vector<uint32_t>(input_num, 1);
  return *this;
}
KernelRunContextFaker &KernelRunContextFaker::IrInstanceNum(std::vector<uint32_t> instance_num) {
  ir_instance_num_ = std::move(instance_num);
  return *this;
}
ge::NodePtr KernelRunContextFaker::FakeNode() const {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  size_t input_index = 0;
  for (size_t ir_index = 0; ir_index < ir_instance_num_.size(); ++ir_index) {
    auto ir_ins_num = ir_instance_num_[ir_index];
    auto prefix = "x_" + std::to_string(ir_index) + "_";
    op_desc->AppendIrInput(prefix, ge::kIrInputDynamic);
    for (size_t i = 0; i < ir_ins_num; ++i, ++input_index) {
      auto td = ge::GeTensorDesc();
      if (node_input_tds_.size() > input_index) {
        td.SetOriginFormat(node_input_tds_[input_index].GetStorageFormat().GetOriginFormat());
        td.SetFormat(node_input_tds_[input_index].GetStorageFormat().GetStorageFormat());
        td.SetDataType(node_input_tds_[input_index].GetDataType());
        td.SetOriginDataType(node_input_tds_[input_index].GetDataType());
      }
      op_desc->AddInputDesc(prefix + std::to_string(i), td);
    }
  }
  for (size_t i = 0; i < node_output_num_; ++i) {
    auto td = ge::GeTensorDesc();
    if (node_output_tds_.size() > i) {
      td.SetOriginFormat(node_output_tds_[i].GetStorageFormat().GetOriginFormat());
      td.SetFormat(node_output_tds_[i].GetStorageFormat().GetStorageFormat());
      td.SetDataType(node_output_tds_[i].GetDataType());
      td.SetOriginDataType(node_output_tds_[i].GetDataType());
    }
    op_desc->AddOutputDesc("y" + std::to_string(i), td);
  }
  for (const auto &attr : attrs_) {
    op_desc->AppendIrAttrName(attr.first);
    op_desc->SetAttr(attr.first, attr.second);
  }
  auto graph = std::make_shared<ge::ComputeGraph>("tmp");
  return graph->AddNode(op_desc);
}
KernelRunContextHolder KernelRunContextFaker::Build() const {
  KernelRunContextHolder holder;
  size_t size = sizeof(KernelRunContext) + sizeof(AsyncAnyValue *) * (kernel_input_num_ + kernel_output_num_);
  holder.context_holder = std::unique_ptr<uint8_t[]>(new uint8_t[size]);
  memset(holder.context_holder.get(), 0xff, size);
  holder.value_holder.resize(kernel_input_num_ + kernel_output_num_);
  size_t extend_info_size;
  holder.extend_info_holder = bg::CreateKernelContextExtend(FakeNode(), holder.buffer_pool, extend_info_size);
  auto extend_info = reinterpret_cast<KernelExtendInfo *>(holder.extend_info_holder.get());
  extend_info->MutableComputeNodeInfo().SetNodeName(
      holder.buffer_pool.GetBufById(reinterpret_cast<size_t>(extend_info->GetComputeNodeInfo().GetNodeName())));
  extend_info->MutableComputeNodeInfo().SetNodeType(
      holder.buffer_pool.GetBufById(reinterpret_cast<size_t>(extend_info->GetComputeNodeInfo().GetNodeType())));

  holder.context = reinterpret_cast<KernelRunContext *>(holder.context_holder.get());
  holder.context->input_size = kernel_input_num_;
  holder.context->output_size = kernel_output_num_;
  holder.context->extend_info = holder.extend_info_holder.get();
  holder.context->output_start = &(holder.context->values[holder.context->input_size]);

  for (size_t i = 0; i < kernel_input_num_ + kernel_output_num_; ++i) {
    holder.context->values[i] = &holder.value_holder[i];
  }

  if (inputs_.size() == kernel_input_num_) {
    for (size_t i = 0; i < inputs_.size(); ++i) {
      holder.value_holder[i].data = inputs_[i];
    }
  }
  if (outputs_.size() == kernel_output_num_) {
    for (size_t i = 0; i < outputs_.size(); ++i) {
      holder.value_holder[i + kernel_input_num_].data = outputs_[i];
    }
  }
  return holder;
}
KernelRunContextFaker &KernelRunContextFaker::NodeInputTd(int32_t index, ge::DataType dt, ge::Format origin_format,
                                                          ge::Format storage_format) {
  node_input_tds_[index].SetDataType(dt);
  node_input_tds_[index].MutableStorageFormat().SetOriginFormat(origin_format);
  node_input_tds_[index].MutableStorageFormat().SetStorageFormat(storage_format);
  return *this;
}
KernelRunContextFaker &KernelRunContextFaker::NodeOutputTd(int32_t index, ge::DataType dt, ge::Format origin_format,
                                                           ge::Format storage_format) {
  node_output_tds_[index].SetDataType(dt);
  node_output_tds_[index].MutableStorageFormat().SetOriginFormat(origin_format);
  node_output_tds_[index].MutableStorageFormat().SetStorageFormat(storage_format);
  return *this;
}
KernelRunContextFaker &KernelRunContextFaker::Inputs(std::vector<void *> inputs) {
  inputs_ = std::move(inputs);
  return *this;
}
KernelRunContextFaker &KernelRunContextFaker::Outputs(std::vector<void *> outputs) {
  outputs_ = std::move(outputs);
  return *this;
}
KernelRunContextFaker &
KernelRunContextFaker::NodeAttrs(std::vector<std::pair<std::string, ge::AnyValue>> keys_to_value) {
  attrs_ = std::move(keys_to_value);
  return *this;
}
InferShapeContextFaker &InferShapeContextFaker::NodeIoNum(size_t input_num, size_t output_num) {
  base_faker_.KernelIONum(input_num + kInputsAppendEnd, output_num);
  base_faker_.NodeIoNum(input_num, output_num);
  return *this;
}
InferShapeContextFaker &InferShapeContextFaker::InputShapes(std::vector<void *> input_shapes) {
  std::vector<void *> inputs(std::move(input_shapes));
  inputs.push_back(nullptr);  // infershape func
  base_faker_.Inputs(std::move(inputs));
  return *this;
}
InferShapeContextFaker &InferShapeContextFaker::OutputShapes(std::vector<void *> output_shapes) {
  base_faker_.Outputs(std::move(output_shapes));
  return *this;
}
KernelRunContextHolder InferShapeContextFaker::Build() const {
  return base_faker_.Build();
}
TilingContextFaker &TilingContextFaker::NodeIoNum(size_t input_num, size_t output_num) {
  base_faker_.KernelIONum(input_num + output_num + kInputsAppendEnd, gert::TilingContext::kOutputNum);
  base_faker_.NodeIoNum(input_num, output_num);
  return *this;
}
TilingContextFaker &TilingContextFaker::InputShapes(std::vector<gert::StorageShape *> input_shapes) {
  input_shapes_ = std::move(input_shapes);
  UpdateInputs();
  return *this;
}
TilingContextFaker &TilingContextFaker::OutputShapes(std::vector<gert::StorageShape *> output_shapes) {
  output_shapes_ = std::move(output_shapes);
  UpdateInputs();
  return *this;
}
TilingContextFaker &TilingContextFaker::CompileInfo(void *compile_info) {
  compile_info_ = compile_info;
  UpdateInputs();
  return *this;
}
TilingContextFaker &TilingContextFaker::TilingData(void *tiling_data) {
  outputs_[TilingContext::kOutputTilingData] = tiling_data;
  base_faker_.Outputs(outputs_);
  return *this;
}
TilingContextFaker &TilingContextFaker::Workspace(ContinuousVector *workspace) {
  outputs_[TilingContext::kOutputWorkspace] = workspace;
  base_faker_.Outputs(outputs_);
  return *this;
}
KernelRunContextHolder TilingContextFaker::Build() const {
  return base_faker_.Build();
}
void TilingContextFaker::UpdateInputs() {
  std::vector<void *> inputs;
  for (const auto input_shape : input_shapes_) {
    inputs.push_back(input_shape);
  }
  for (const auto output_shape : output_shapes_) {
    inputs.push_back(output_shape);
  }
  inputs.push_back(compile_info_);  // kInputsCompileInfo
  inputs.push_back(nullptr);        // kInputsTilingFunc
  base_faker_.Inputs(std::move(inputs));
}
}  // namespace gert