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

#ifndef AIR_CXX_RUNTIME_V2_METADEF_EXE_GRAPH_GRAPH_FRAME_H_
#define AIR_CXX_RUNTIME_V2_METADEF_EXE_GRAPH_GRAPH_FRAME_H_
#include <stack>
#include <memory>
#include <utility>
#include <string>
#include "graph/node.h"
#include "buffer_pool.h"
#include "bg_kernel_context_extend.h"

namespace gert {
namespace bg {
struct ExtendInfo {
  std::unique_ptr<uint8_t[]> compute_node_info;
};
class GraphFrame {
 public:
  GraphFrame(const GraphFrame &) = delete;
  GraphFrame(GraphFrame &&) = delete;
  GraphFrame operator=(const GraphFrame &) = delete;
  GraphFrame operator=(GraphFrame &&) = delete;

  GraphFrame(ge::ComputeGraphPtr exe_graph, GraphFrame &root_frame)
      : exe_graph_(std::move(exe_graph)), current_compute_node_(nullptr), root_frame_(root_frame) {}

  explicit GraphFrame(ge::ComputeGraphPtr exe_graph)
      : exe_graph_(std::move(exe_graph)), current_compute_node_(nullptr), root_frame_(*this) {}

  const ge::NodePtr &GetCurrentComputeNode() const {
    return current_compute_node_;
  }
  void SetCurrentComputeNode(const ge::NodePtr &current_node) {
    if (current_node != nullptr) {
      AddNodeExtendInfo(current_node);
    }
    current_compute_node_ = current_node;
  }
  bool GetCurrentNodeIndex(size_t &index) {
    auto current_node = GetCurrentComputeNode();
    if (current_node == nullptr) {
      return false;
    }
    auto iter = node_names_to_index_.find(current_node->GetName());
    if (iter == node_names_to_index_.end()) {
      return false;
    }
    index = iter->second;
    return true;
  }

  void AddNodeExtendInfo(const ge::NodePtr &node) {
    auto ret = node_names_to_index_.emplace(node->GetName(), GetAllComputeNodeInfos().GetSize());
    if (ret.second) {
      size_t total_size = 0;
      auto compute_node_info = CreateComputeNodeInfo(node, GetBufferPool(), total_size);
      auto index = GetAllComputeNodeInfos().AddBuf(compute_node_info.get(), total_size);
      ret.first->second = index;
    }
  }
  const uint8_t *GetComputeNodeInfo(size_t index) const {
    if (GetAllComputeNodeInfos().GetSize() <= index) {
      return nullptr;
    }
    return reinterpret_cast<const uint8_t *>(GetAllComputeNodeInfos().GetBufById(index));
  }
  const BufferPool &GetAllComputeNodeInfos() const {
    return root_frame_.compute_node_info_buffer_pool_;
  }
  BufferPool &GetAllComputeNodeInfos() {
    return root_frame_.compute_node_info_buffer_pool_;
  }

  BufferPool &GetKernelExtendInfos() {
    return kernel_extend_buffer_pool_;
  }

  const BufferPool &GetKernelExtendInfos() const {
    return kernel_extend_buffer_pool_;
  }

  BufferPool &GetKernelModelDesc() {
    return model_desc_buffer_pool_;
  }

  const BufferPool &GetKernelModelDesc() const {
    return model_desc_buffer_pool_;
  }

  const BufferPool &GetBufferPool() const {
    return root_frame_.buffer_pool_;
  }
  BufferPool &GetBufferPool() {
    return root_frame_.buffer_pool_;
  }
  bool IsRootFrame() const {
    return &root_frame_ == this;
  }
  const ge::ComputeGraphPtr &GetExeGraph() const {
    return exe_graph_;
  }

 private:
  ge::ComputeGraphPtr exe_graph_;
  ge::NodePtr current_compute_node_;
  GraphFrame &root_frame_;
  std::unordered_map<std::string, size_t> node_names_to_index_;
  BufferPool compute_node_info_buffer_pool_;
  BufferPool kernel_extend_buffer_pool_;
  BufferPool model_desc_buffer_pool_;
  BufferPool buffer_pool_;
};
}  // namespace bg
}  // namespace gert

#endif  //AIR_CXX_RUNTIME_V2_METADEF_EXE_GRAPH_GRAPH_FRAME_H_
