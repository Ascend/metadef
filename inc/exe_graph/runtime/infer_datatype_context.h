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

#ifndef METADEF_CXX_INC_EXE_GRAPH_RUNTIME_INFER_DATATYPE_CONTEXT_H_
#define METADEF_CXX_INC_EXE_GRAPH_RUNTIME_INFER_DATATYPE_CONTEXT_H_
#include <type_traits>
#include "tensor.h"
#include "runtime_attrs.h"
#include "extended_kernel_context.h"
#include "external/graph/types.h"
namespace gert {
/**
 * InferDataType kernel的context
 */
class InferDataTypeContext : public ExtendedKernelContext {
 public:
  /**
   * 根据输入index，获取输入DataType指针
   * @param index 输入index
   * @return 输入datatype指针，index非法时，返回空指针
   */
  const ge::DataType *GetInputDataType(size_t index) const {
    return GetInputPointer<ge::DataType>(index);
  }

  /**
   * 基于算子IR原型定义，获取`OPTIONAL_INPUT`类型的输入DataType指针
   * @param ir_index IR原型定义中的index
   * @return shape指针，index非法，或该INPUT没有实例化时，返回空指针
   */
  const ge::DataType *GetOptionalInputDataType(size_t index) const {
    return GetDynamicInputPointer<ge::DataType>(index, 0);
  }
  /**
   * 基于算子IR原型定义，获取`DYNAMIC_INPUT`类型的输入DataType指针
   * @param ir_index IR原型定义中的index
   * @param relative_index 该输入实例化后的相对index，例如某个DYNAMIC_INPUT实例化了3个输入，那么relative_index的有效范围是[0,2]
   * @return shape指针，index或relative_index非法时，返回空指针
   */
  const ge::DataType *GetDynamicInputDataType(size_t index, size_t relative_index) const {
    return GetDynamicInputPointer<ge::DataType>(index, relative_index);
  }

  /**
   * 根据输出index，获取输出DataType指针
   * @param index 输出index
   * @return 输出shape指针，index非法时，返回空指针
   */
  ge::DataType *GetOutputDataType(size_t index) {
    return GetOutputPointer<ge::DataType>(index);
  }
};
static_assert(std::is_standard_layout<InferDataTypeContext>::value, "The class InferDataTypeContext must be a POD");
}  // namespace gert
#endif  // METADEF_CXX_INC_EXE_GRAPH_RUNTIME_INFER_SHAPE_CONTEXT_H_
