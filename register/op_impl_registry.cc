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

#include "register/op_impl_registry.h"
#include "register/op_impl_registry_api.h"
#include "framework/common/debug/ge_log.h"
#include "graph/operator_factory_impl.h"
#include "register/shape_inference.h"

namespace gert {
OpImplRegister::OpImplRegister(const char *op_type) : op_type_(op_type) {
  functions_.infer_shape = nullptr;
  functions_.infer_shape_range = nullptr;
  functions_.infer_datatype = nullptr;
  functions_.tiling = nullptr;
  functions_.tiling_parse = nullptr;
  functions_.compile_info_creator = nullptr;
  functions_.compile_info_deleter = nullptr;
  functions_.max_tiling_data_size = 0;
  functions_.inputs_dependency = 0;
  functions_.is_register = false;
#ifndef ONLY_COMPILE_OPEN_SRC
  memset(reserved_, 0, sizeof(reserved_));
#endif
}
OpImplRegister::OpImplRegister(const OpImplRegister &other) {
  OpImplRegistry::GetInstance().RegisterOpImpl(other.op_type_, other.functions_);
}

OpImplRegister &OpImplRegister::InferShape(OpImplKernelRegistry::InferShapeKernelFunc infer_shape_func) {
  functions_.is_register = true;
  functions_.infer_shape = infer_shape_func;
  // only infer shape is necessary, as register all infer func in infer shape
  (void) ge::OperatorFactoryImpl::RegisterInferShapeV2Func(gert::InferShapeOnCompile);
  (void) ge::OperatorFactoryImpl::RegisterInferShapeRangeFunc(gert::InferShapeRangeOnCompile);
  (void) ge::OperatorFactoryImpl::RegisterInferDataTypeFunc(gert::InferDataTypeOnCompile);
  return *this;
}

OpImplRegister &OpImplRegister::InferShapeRange(
    OpImplKernelRegistry::InferShapeRangeKernelFunc infer_shape_range_func) {
  functions_.is_register = true;
  functions_.infer_shape_range = infer_shape_range_func;
  return *this;
}

OpImplRegister &OpImplRegister::InferDataType(OpImplKernelRegistry::InferDataTypeKernelFunc infer_datatype_func) {
  functions_.is_register = true;
  functions_.infer_datatype = infer_datatype_func;
  return *this;
}
OpImplRegister &OpImplRegister::Tiling(OpImplKernelRegistry::TilingKernelFunc tiling_func,
                                       size_t max_tiling_data_size) {
  functions_.is_register = true;
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
  functions_.is_register = true;
  return *this;
}

OpImplRegister &OpImplRegister::PrivateAttrImpl(const char *private_attr, ge::AnyValue private_attr_av) {
  if (private_attr == nullptr) {
    GELOGE(ge::FAILED, "Failed to set private attr name using nullptr!");
  } else if (strncmp(private_attr, "", 1U) == 0) {
    GELOGE(ge::FAILED, "Failed to set private attr name using empty string("")!");
  } else {
    if (functions_.unique_private_attrs.insert(private_attr).second) {
      functions_.is_register = true;
      functions_.private_attrs.emplace_back(std::make_pair(private_attr, std::move(private_attr_av)));
    } else {
      GELOGE(ge::FAILED, "The private attr name: %s has already existed.", private_attr);
    }
  }
  return *this;
}
OpImplRegister &OpImplRegister::PrivateAttr(const char *private_attr) {
  static ge::AnyValue emptyPrivateAttrAV;
  return PrivateAttrImpl(private_attr, emptyPrivateAttrAV);
}
OpImplRegister &OpImplRegister::PrivateAttr(const char *private_attr, int64_t private_attr_val) {
  return PrivateAttrImpl(private_attr, ge::AnyValue::CreateFrom<int64_t>(private_attr_val));
}
OpImplRegister &OpImplRegister::PrivateAttr(const char *private_attr, const std::vector<int64_t> &private_attr_val) {
  return PrivateAttrImpl(private_attr, ge::AnyValue::CreateFrom<std::vector<int64_t>>(private_attr_val));
}
OpImplRegister &OpImplRegister::PrivateAttr(const char *private_attr, const char *private_attr_val) {
  return PrivateAttrImpl(private_attr, ge::AnyValue::CreateFrom<std::string>(private_attr_val));
}
OpImplRegister &OpImplRegister::PrivateAttr(const char *private_attr, float private_attr_val) {
  return PrivateAttrImpl(private_attr, ge::AnyValue::CreateFrom<float>(private_attr_val));
}
OpImplRegister &OpImplRegister::PrivateAttr(const char *private_attr, bool private_attr_val) {
  return PrivateAttrImpl(private_attr, ge::AnyValue::CreateFrom<bool>(private_attr_val));
}
OpImplRegister &OpImplRegister::PrivateAttr(const char *private_attr, const vector<float> &private_attr_val) {
  return PrivateAttrImpl(private_attr, ge::AnyValue::CreateFrom<std::vector<float>>(private_attr_val));
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
const OpImplRegistry::PrivateAttrList &OpImplRegistry::GetPrivateAttrs(const OpImplRegistry::OpType &op_type) const {
  auto op_impl_ptr = GetOpImpl(op_type);
  if (op_impl_ptr == nullptr) {
    static OpImplRegistry::PrivateAttrList emptyPrivateAttr;
    return emptyPrivateAttr;
  }
  return op_impl_ptr->private_attrs;
}

void OpImplRegistry::RegisterOpImpl(const OpType &op_type, OpImplRegistry::OpImplFunctions func) {
#ifndef ONLY_COMPILE_OPEN_SRC
  (void) reserved_;
#endif
  if (!func.is_register) {
      types_to_impl_[op_type] = func;
      return;
  }
  types_to_impl_[op_type].is_register = func.is_register;
  if (func.infer_shape != nullptr) {
    types_to_impl_[op_type].infer_shape= std::move(func.infer_shape);
  }
  if (func.infer_shape_range != nullptr) {
    types_to_impl_[op_type].infer_shape_range= std::move(func.infer_shape_range);
  }
  if (func.infer_datatype != nullptr) {
    types_to_impl_[op_type].infer_datatype= std::move(func.infer_datatype);
  }
  if (func.tiling != nullptr) {
    types_to_impl_[op_type].tiling= std::move(func.tiling);
  }
  if (func.tiling_parse != nullptr) {
    types_to_impl_[op_type].tiling_parse= std::move(func.tiling_parse);
  }
  if (func.compile_info_creator != nullptr) {
    types_to_impl_[op_type].compile_info_creator= std::move(func.compile_info_creator);
  }
  if (func.compile_info_deleter != nullptr) {
    types_to_impl_[op_type].compile_info_deleter= std::move(func.compile_info_deleter);
  }
  if (func.max_tiling_data_size != 0) {
    types_to_impl_[op_type].max_tiling_data_size= std::move(func.max_tiling_data_size);
  }
  if (func.inputs_dependency != 0) {
    types_to_impl_[op_type].inputs_dependency= std::move(func.inputs_dependency);
  }
  if (!func.private_attrs.empty()) {
    types_to_impl_[op_type].private_attrs= std::move(func.private_attrs);
  }
  if (!func.unique_private_attrs.empty()) {
    types_to_impl_[op_type].unique_private_attrs= std::move(func.unique_private_attrs);
  }
}
}  // namespace gert

#ifdef __cplusplus
extern "C" {
#endif

size_t GetRegisteredOpNum(void) {
  return gert::OpImplRegistry::GetInstance().GetTyepsToImpl().size();
}
int32_t GetOpImplFunctions(TypesToImpl *impl, size_t impl_num) {
  auto types_to_impl = gert::OpImplRegistry::GetInstance().GetTyepsToImpl();
  if (impl_num != types_to_impl.size()) {
    GELOGE(ge::FAILED, "Get types_to_impl_ failed, impl_num[%u] and map size[%u] not match",
           impl_num, types_to_impl.size());
    return ge::GRAPH_FAILED;
  }
  size_t cnt = 0U;
  for (auto &it : types_to_impl) {
    impl[cnt].op_type = it.first.c_str();
    impl[cnt].funcs = it.second;
    cnt++;
  }
  return ge::GRAPH_SUCCESS;
}

#ifdef __cplusplus
}
#endif

