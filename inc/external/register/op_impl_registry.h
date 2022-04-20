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

#ifndef AIR_CXX_RUNTIME_V2_IR_IMPL_KERNEL_IMPL_REGISTRY_H_
#define AIR_CXX_RUNTIME_V2_IR_IMPL_KERNEL_IMPL_REGISTRY_H_
#include <initializer_list>
#include <string>
#include <map>
#include "graph/ge_error_codes.h"
#include "op_impl_kernel_registry.h"
namespace gert {
class OpImplRegistry :public OpImplKernelRegistry {
 public:
  using OpType = std::string;
  static OpImplRegistry &GetInstance();
  OpImplFunctions &CreateOrGetOpImpl(const OpType &op_type);
  const OpImplFunctions *GetOpImpl(const OpType &op_type) const override;

 private:
  std::map<OpType, OpImplFunctions> types_to_impl_;
};

class OpImplRegister {
 public:
  explicit OpImplRegister(const char *op_type);
  OpImplRegister &InferShape(OpImplKernelRegistry::InferShapeKernelFunc infer_shape_func);
  OpImplRegister &Tiling(OpImplKernelRegistry::TilingKernelFunc tiling_func, size_t max_tiling_data_size =2048);
  template<typename T>
  OpImplRegister &TilingParse(KernelRegistry::KernelFunc tiling_parse_func){
    functions_.tiling_parse = tiling_parse_func;
    functions_.compile_info_creator = CreateCompileInfo<T>;
    functions_.compile_info_deleter = DeleteCompileInfo<T>;
    return *this;
  }
  OpImplRegister &InputsDataDependency(std::initializer_list<int32_t> inputs);

 private:
  template<typename T, typename std::enable_if<(!std::is_array<T>::value), int>::type = 0>
  static void *CreateCompileInfo() {
    return new T();
  }
  template<typename T>
  static void DeleteCompileInfo(void *obj) {
    delete reinterpret_cast<T *>(obj);
  }
  template<size_t MaxLen>
  static void *CreateDynamicLenTilingData() {
    return TilingData::CreateCap(MaxLen).release();
  }

 private:
  const char *op_type_;
  OpImplRegistry::OpImplFunctions &functions_;
};
}

#define IMPL_OP(op_type) static gert::OpImplRegister op_impl_register_##op_type = gert::OpImplRegister(#op_type)
#define IMPL_OP_DEFAULT() IMPL_OP(DefaultImpl)

#endif  //AIR_CXX_RUNTIME_V2_IR_IMPL_KERNEL_IMPL_REGISTRY_H_
