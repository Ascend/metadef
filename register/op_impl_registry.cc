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

#include "register/op_impl_registry.h"
#include "framework/common/debug/ge_log.h"

namespace gert {
OpImplRegister::OpImplRegister(const char *op_type)
    : op_type_(op_type), functions_(OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type)) {}
OpImplRegister &OpImplRegister::InferShape(OpImplKernelRegistry::InferShapeKernelFunc infer_shape_func) {
  functions_.infer_shape = infer_shape_func;
  return *this;
}
OpImplRegister &OpImplRegister::Tiling(OpImplKernelRegistry::TilingKernelFunc tiling_func,
                                       size_t max_tiling_data_size) {
  functions_.tiling = tiling_func;
  functions_.max_tiling_data_size = max_tiling_data_size;
  return *this;
}
OpImplRegister &OpImplRegister::InputsDataDependency(std::initializer_list<int32_t> inputs) {
  functions_.inputs_dependency = 0;
  for (const auto index : inputs) {
    if (functions_.SetInputDataDependency(index) != ge::GRAPH_SUCCESS) {
      GELOGE(ge::FAILED, "Failed to set data dependency for node %s, the input index %d", op_type_, index);
      return *this;
    }
  }
  return *this;
}
OpImplRegistry &OpImplRegistry::GetInstance() {
  static OpImplRegistry instance;
  return instance;
}
OpImplRegistry::OpImplFunctions &OpImplRegistry::CreateOrGetOpImpl(const OpImplRegistry::OpType &op_type) {
  return types_to_impl_[op_type];
}
const OpImplRegistry::OpImplFunctions *OpImplRegistry::GetOpImpl(const OpImplRegistry::OpType &op_type) const {
  auto iter = types_to_impl_.find(op_type);
  if (iter == types_to_impl_.end()) {
    return nullptr;
  }
  return &iter->second;
}
}  // namespace gert