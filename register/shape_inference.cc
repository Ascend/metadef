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
#include "register/shape_inference.h"
#include "exe_graph/runtime/kernel_run_context_builder.h"
#include "graph/debug/ge_util.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "register/op_impl_registry.h"

namespace gert {
namespace {
ge::graphStatus ConstructInferShapeContextInputs(const ge::Operator &op, const ge::OpDescPtr &op_desc,
                                                 std::vector<std::unique_ptr<uint8_t[]>> &inputs,
                                                 std::vector<std::unique_ptr<ge::Tensor>> &ge_tensors_holder) {
  size_t invalid_index_num = 0UL;
  for (size_t i = 0UL; i < op_desc->GetAllInputsSize(); i++) {
    const auto &compile_tensor = op_desc->GetInputDesc(i);
    if (compile_tensor.IsValid() != ge::GRAPH_SUCCESS) {
      GELOGD("input desc is not valid, skip add input[%zu] into context inputs.", i);
      invalid_index_num++;
      continue;
    }
    auto tensor_holder = ge::ComGraphMakeUnique<uint8_t[]>(sizeof(gert::Tensor));
    GE_ASSERT_NOTNULL(tensor_holder, "Create context holder inputs failed.");
    gert::StorageShape storage_shape;
    const auto &dims = compile_tensor.GetOriginShape().GetDims();
    for (const auto &dim : dims) {
      storage_shape.MutableOriginShape().AppendDim(dim);
      storage_shape.MutableStorageShape().AppendDim(dim);
    }
    // init tensor address, if can not get const tensor input, set it to nullptr
    TensorAddress address = nullptr;
    const auto &functions = OpImplRegistry::GetInstance().GetOpImpl(op_desc->GetType());
    size_t instance_index = i - invalid_index_num;
    size_t ir_index;
    GE_ASSERT_GRAPH_SUCCESS(ge::OpDescUtils::GetInputIrIndexByInstanceIndex(op_desc, instance_index, ir_index),
                            "[Get][InputIrIndexByInstanceIndex] failed, op[%s], instance index[%zu], i[%zu]",
                            op_desc->GetName().c_str(), instance_index, i);
    if (functions->IsInputDataDependency(ir_index)) {
      ge_tensors_holder[i] = ge::ComGraphMakeUnique<ge::Tensor>();
      GE_ASSERT_NOTNULL(ge_tensors_holder[i], "Create ge tensor holder inputs failed.");
      const auto index_name = op_desc->GetInputNameByIndex(i);
      if (op.GetInputConstData(index_name.c_str(), *(ge_tensors_holder[i].get())) == ge::GRAPH_SUCCESS) {
        address = ge_tensors_holder[i]->GetData();
      }
    }
    new (tensor_holder.get())
        gert::Tensor(storage_shape, {compile_tensor.GetOriginFormat(), compile_tensor.GetFormat(), {}}, gert::kOnHost,
                     compile_tensor.GetDataType(), address);
    inputs.emplace_back(std::move(tensor_holder));
  }
  // set infer shape_func to NULL
  inputs.emplace_back(nullptr);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ConstructInferShapeContextOutputs(const ge::OpDescPtr &op_desc,
                                                  std::vector<std::unique_ptr<uint8_t[]>> &outputs) {
  for (size_t i = 0UL; i < op_desc->GetAllOutputsDescSize(); i++) {
    auto tensor_holder = ge::ComGraphMakeUnique<uint8_t[]>(sizeof(gert::Tensor));
    GE_ASSERT_NOTNULL(tensor_holder, "Create context holder outputs failed, op[%s]", op_desc->GetName().c_str());
    outputs.emplace_back(std::move(tensor_holder));
  }
  return ge::GRAPH_SUCCESS;
}

void UpdateOpDescOutShape(const ge::OpDescPtr &op_desc, gert::InferShapeContext *infer_shape_ctx) {
  for (size_t index = 0UL; index < op_desc->GetOutputsSize(); index++) {
    auto &dst_out_shape = op_desc->MutableOutputDesc(index)->MutableShape();
    const auto *shape = infer_shape_ctx->GetOutputShape(index);
    dst_out_shape.SetDimNum(shape->GetDimNum());
    for (size_t dim = 0UL; dim < shape->GetDimNum(); dim++) {
      dst_out_shape.SetDim(dim, shape->GetDim(dim));
    }
  }
}

void ConstructDataTypeContextInputs(const ge::OpDescPtr &op_desc, std::vector<void *> &inputs) {
  for (size_t i = 0UL; i < op_desc->GetAllInputsSize(); i++) {
    const auto &compile_tensor = op_desc->GetInputDesc(i);
    inputs.emplace_back(reinterpret_cast<void *>(compile_tensor.GetDataType()));
  }
}

void ConstructDataTypeContextOutputs(const ge::OpDescPtr &op_desc, std::vector<void *> &outputs) {
  for (size_t i = 0UL; i < op_desc->GetAllOutputsDescSize(); i++) {
    const auto &compile_tensor = op_desc->GetOutputDesc(i);
    outputs.emplace_back(reinterpret_cast<void *>(compile_tensor.GetDataType()));
  }
}
}

ge::graphStatus InferShapeRangeOnCompile(const ge::OpDescPtr &op_desc) {
  // to realization
  (void) op_desc;
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus InferShapeOnCompile(const ge::Operator &op, const ge::OpDescPtr &op_desc) {
  const auto &functions = OpImplRegistry::GetInstance().GetOpImpl(op_desc->GetType());
  if ((functions == nullptr) || (functions->infer_shape == nullptr)) {
    GELOGW("Can not get infer shape func v2, op[%s], type[%s]",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return ge::GRAPH_PARAM_INVALID;
  }
  std::vector<std::unique_ptr<uint8_t[]>> inputs_holder;
  std::vector<std::unique_ptr<uint8_t[]>> outputs_holder;
  std::vector<std::unique_ptr<ge::Tensor>> ge_tensors_holder;
  ge_tensors_holder.resize(op_desc->GetAllInputsSize());
  GE_ASSERT_GRAPH_SUCCESS(ConstructInferShapeContextInputs(op, op_desc, inputs_holder, ge_tensors_holder),
                          "[Construct][InferShapeContextInputs] failed, op_desc[%s]", op_desc->GetName().c_str());
  GE_ASSERT_GRAPH_SUCCESS(ConstructInferShapeContextOutputs(op_desc, outputs_holder),
                          "[Construct][InferShapeContextOutputs] failed, op_desc[%s]", op_desc->GetName().c_str());
  // build kernel context
  std::vector<void *> inputs;
  std::vector<void *> outputs;
  inputs.reserve(inputs_holder.size() + 1UL);
  outputs.reserve(outputs_holder.size());
  for (const auto &input_holder : inputs_holder) {
    inputs.emplace_back(input_holder.get());
  }
  // inputs layout is input tensors + infer func + inference context ptr
  inputs.emplace_back(op.GetInferenceContext().get());
  for (const auto &output_holder : outputs_holder) {
    outputs.emplace_back(output_holder.get());
  }
  const auto kernel_context_holder = gert::KernelRunContextBuilder().Inputs(inputs).Outputs(outputs).Build(op_desc);
  auto infer_shape_ctx = reinterpret_cast<gert::InferShapeContext *>(kernel_context_holder.context_);
  const auto ret = functions->infer_shape(infer_shape_ctx);
  GE_CHK_STATUS_RET(ret, "[Call][InferShapeV2Func] failed, ret[%d]", ret);
  UpdateOpDescOutShape(op_desc, infer_shape_ctx);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus InferDataTypeOnCompile(const ge::OpDescPtr &op_desc) {
  const auto &functions = OpImplRegistry::GetInstance().GetOpImpl(op_desc->GetType());
  if ((functions == nullptr) ||
      (functions->infer_datatype == nullptr)) {
    return op_desc->DefaultInferDataType();
  }
  gert::KernelContextHolder kernel_context_holder;
  std::vector<void *> inputs;
  std::vector<void *> outputs;
  ConstructDataTypeContextInputs(op_desc, inputs);
  ConstructDataTypeContextOutputs(op_desc, outputs);
  kernel_context_holder = gert::KernelRunContextBuilder().Inputs(inputs).Outputs(outputs).Build(op_desc);
  const auto kernel_context = reinterpret_cast<gert::InferDataTypeContext *>(kernel_context_holder.context_);
  const auto ret = functions->infer_datatype(kernel_context);
  GE_CHK_STATUS_RET(ret, "[Check][InferDataType] result failed, ret[%d]", ret);
  for (size_t i = 0UL; i < op_desc->GetOutputsSize(); i++) {
    const auto &out_desc = op_desc->MutableOutputDesc(i);
    out_desc->SetDataType(kernel_context->GetOutputDataType(i));
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert