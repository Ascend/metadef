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
#include "exe_graph/runtime/context_extend.h"
#include "graph/utils/math_util.h"

namespace gert {
std::unique_ptr<uint8_t[]> KernelExtendInfo::Create(size_t ir_inputs_num, size_t inputs_num, size_t outputs_num,
                                                    const char *node_name, const char *node_type) {
  size_t total_size = 0;
  if (ComputeNodeInfo::CalcSize(ir_inputs_num, inputs_num, outputs_num, total_size) != ge::GRAPH_SUCCESS) {
    return nullptr;
  }

  auto ret = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_size]);
  if (ret == nullptr) {
    return nullptr;
  }

  auto extend_info = reinterpret_cast<KernelExtendInfo *>(ret.get());
  extend_info->compute_node_info.Init(ir_inputs_num, inputs_num, outputs_num, total_size, node_name, node_type);

  return ret;
}
}  // namespace gert