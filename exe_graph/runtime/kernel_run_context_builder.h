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
#ifndef METADEF_CXX_RUNTIME_KERNEL_CONTEXT_BUILDER_H_
#define METADEF_CXX_RUNTIME_KERNEL_CONTEXT_BUILDER_H_

#include "graph/node.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "exe_graph/runtime/kernel_context.h"
#include "exe_graph/lowering/buffer_pool.h"

namespace gert {
struct KernelContextHolder {
  std::unique_ptr<uint8_t[]> context_holder;
  std::vector<AsyncAnyValue> value_holder;
  std::unique_ptr<uint8_t[]> compute_node_extend_holder;
  bg::BufferPool buffer_pool;
  KernelContext *context;
};
class KernelRunContextBuilder {
public:
  KernelRunContextBuilder() = default;
  KernelRunContextBuilder &Inputs(std::vector<void *> inputs) {
    inputs_ = std::move(inputs);
    return *this;
  }
  KernelRunContextBuilder &Outputs(std::vector<void *> outputs) {
    outputs_ = std::move(outputs);
    return *this;
  }
  KernelContextHolder Build(ge::OpDescPtr &op_desc);

private:
  ge::NodePtr MakeNode(ge::OpDescPtr &op_desc) const;

private:
  std::vector<void *> inputs_;
  std::vector<void *> outputs_;
};
}  // namespace gert
#endif