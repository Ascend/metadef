/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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
#ifndef INC_EXTERNAL_REGISTER_OP_IMPL_KERNEL_REGISTRY_TYPES_H_
#define INC_EXTERNAL_REGISTER_OP_IMPL_KERNEL_REGISTRY_TYPES_H_
#include <cstdint>
#include <string>
#include <unordered_set>
#include "exe_graph/runtime/base_type.h"

namespace ge {
class AscendString;
class AnyValue;
};
namespace gert {
class InferShapeContext;
class InferShapeRangeContext;
class TilingContext;
class InferDataTypeContext;
class OpExecuteContext;
class KernelContext;
class TilingParseContext;

using InferShapeKernelFunc = UINT32 (*)(InferShapeContext *);
using InferShapeRangeKernelFunc = UINT32 (*)(InferShapeRangeContext *);
using TilingKernelFunc = UINT32 (*)(TilingContext *);
using InferDataTypeKernelFunc = UINT32 (*)(InferDataTypeContext *);
// aclnn接口的原型，入参含义：
// OpExecuteContext：保存算子的Input，Output，Attr信息
using OpExecuteFunc = UINT32 (*)(OpExecuteContext *);
using OpType = ge::AscendString;
using PrivateAttrList = std::vector<std::pair<ge::AscendString, ge::AnyValue>>;
using PrivateAttrSet = std::unordered_set<ge::AscendString>;
using CompileInfoCreatorFunc = void *(*) ();
using CompileInfoDeleterFunc = void (*)(void *);
using KernelFunc = UINT32 (*)(KernelContext *context);
using TilingParseFunc = UINT32 (*)(TilingParseContext *context);
}  // namespace gert

#endif
