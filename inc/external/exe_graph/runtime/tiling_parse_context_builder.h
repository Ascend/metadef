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
#ifndef GE_RUNTIME_TILING_PARSE_CONTEXT_BUILDER_H_
#define GE_RUNTIME_TILING_PARSE_CONTEXT_BUILDER_H_

#include "exe_graph/runtime/kernel_context.h"
#include "graph/operator.h"
#include "exe_graph/runtime/kernel_run_context_builder.h"

namespace gert {
class TilingParseContextBuilder {
 public:
  TilingParseContextBuilder &CompileInfo(void *compile_info);
  TilingParseContextBuilder &PlatformInfo(void *platform_info);
  KernelContextHolder Build(const ge::Operator &op);

 private:
  void *compile_info_{ nullptr };
  void *platform_info_{ nullptr };
};
}  // namespace gert
#endif // GE_RUNTIME_TILING_PARSE_CONTEXT_BUILDER_H_
