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

#ifndef AIR_CXX_RUNTIME_V2_KERNEL_INFERSHAPECONTEXT_H_
#define AIR_CXX_RUNTIME_V2_KERNEL_INFERSHAPECONTEXT_H_
#include <cstdint>
#include <type_traits>
#include "shape.h"
#include "tensor.h"
#include "runtime_attrs.h"
#include "extended_kernel_context.h"
namespace gert {
class InferShapeContext : public ExtendedKernelContext {
 public:
  const Shape *GetInputShape(size_t index) {
    return GetInputPointer<Shape>(index);
  }
  const Tensor *GetInputTensor(size_t index) {
    return GetInputPointer<Tensor>(index);
  }

  const Shape *GetOptionalInputShape(size_t ir_index) {
    return GetDynamicInputPointer<Shape>(ir_index, 0);
  }
  const Shape *GetDynamicInputShape(size_t ir_index, size_t relative_index) {
    return GetDynamicInputPointer<Shape>(ir_index, relative_index);
  }

  /*
   * outputs, 输出如下排列：
   * outputs[0~n]: 所有输出的shape
   */
  Shape *GetOutputShape(size_t index) {
    return GetOutputPointer<Shape>(index);
  }
};
static_assert(std::is_standard_layout<InferShapeContext>::value, "The class InferShapeContext must be a POD");
}  // namespace gert
#endif  //AIR_CXX_RUNTIME_V2_KERNEL_INFERSHAPECONTEXT_H_
