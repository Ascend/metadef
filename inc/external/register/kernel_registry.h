/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#ifndef B369E37D560547C2B8DC137404F9713E_H
#define B369E37D560547C2B8DC137404F9713E_H
#include <functional>
#include "graph/ge_error_codes.h"
#include "exe_graph/runtime/base_type.h"
#include "exe_graph/runtime/kernel_context.h"
#include "graph/node.h"

namespace gert {
struct KernelRegistry {
  using CreateOutputsFunc = std::function<ge::graphStatus(const ge::Node *, KernelContext *)>;
  typedef UINT32 (*KernelFunc)(KernelContext *context);
  struct KernelFuncs {
    KernelFunc run_func;
    CreateOutputsFunc outputs_creator;
    CreateOutputsFunc outputs_initializer;
  };
  virtual ~KernelRegistry() {}
  virtual const KernelFuncs* FindKernelFuncs(const std::string &kernel_type) const = 0;
};
}

#endif
