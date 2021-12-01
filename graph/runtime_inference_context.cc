/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#include <cstdint>
#include <utility>

#include "graph/runtime_inference_context.h"
#include "graph/utils/tensor_adapter.h"
#include "framework/common/debug/ge_log.h"

namespace ge {
void RuntimeInferenceContext::Release() {
  std::lock_guard<std::mutex> lk(mu_);
  ge_tensors_.clear();
}

graphStatus RuntimeInferenceContext::SetTensor(int64_t node_id, int32_t output_id, GeTensorPtr tensor) {
  std::lock_guard<std::mutex> lk(mu_);
  auto &output_ge_tensors = ge_tensors_[node_id];
  if (static_cast<uint32_t>(output_id) >= output_ge_tensors.size()) {
    output_ge_tensors.resize(output_id + 1);
  }

  GELOGD("Set tensor for node_id = %ld, output_id = %d", node_id, output_id);
  output_ge_tensors[output_id] = std::move(tensor);

  return GRAPH_SUCCESS;
}

graphStatus RuntimeInferenceContext::GetTensor(int64_t node_id, int32_t output_id, GeTensorPtr &tensor) {
  if (output_id < 0) {
    REPORT_INNER_ERROR("E19999", "Invalid output index: %d", output_id);
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] Invalid output index: %d", output_id);
    return GRAPH_PARAM_INVALID;
  }

  std::lock_guard<std::mutex> lk(mu_);
  auto iter = ge_tensors_.find(node_id);
  if (iter == ge_tensors_.end()) {
    GELOGW("Node not register. Id = %ld", node_id);
    return INTERNAL_ERROR;
  }

  auto &output_tensors = iter->second;
  if (static_cast<uint32_t>(output_id) >= output_tensors.size()) {
    REPORT_INNER_ERROR("E19999", "Node output is not registered. node_id = %ld, output index = %d",
                       node_id, output_id);
    GELOGE(GRAPH_FAILED, "[Check][Param] Node output is not registered. node_id = %ld, output index = %d",
           node_id, output_id);
    return GRAPH_FAILED;
  }

  GELOGD("Get ge tensor for node_id = %ld, output_id = %d", node_id, output_id);
  tensor = output_tensors[output_id];
  if (tensor == nullptr) {
    GELOGW("Node output is not registered. node_id = %ld, output index = %d", node_id, output_id);
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}
} // namespace ge