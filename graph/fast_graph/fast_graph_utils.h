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

#ifndef FAST_GRAPH_FAST_GRAPH_UTILS_H
#define FAST_GRAPH_FAST_GRAPH_UTILS_H

#include <string.h>
#include "graph/anchor.h"
#include "quick_list.h"
#include "graph/execute_graph.h"
#include "graph/utils/tensor_utils.h"

namespace ge {
enum class FastWalkStatus { kNotWalked, kWalking, kWalked };
struct NodeStatus {
  size_t size = 0U;
  FastWalkStatus status;
};

struct GraphExtendInfo {
  bool is_valid_flag_ = false;
};

using QuickNode = ListElement<FastNode>;
using QuickEdge = ListElement<Edge<FastNode>>;
using QuickGraph = ListElement<ExecuteGraph *>;

class FastGraphUtils {
 public:
  static inline Edge<FastNode> &GetEdge(QuickEdge *quick_edge) {
    return quick_edge->data;
  }

  static inline FastNode *&GetEdgeSrc(QuickEdge *quick_edge) {
    return quick_edge->data.src;
  }

  static inline FastNode *const &GetConstEdgeSrc(const QuickEdge *quick_edge) {
    return quick_edge->data.src;
  }

  static inline FastNode *&GetEdgeDst(QuickEdge *quick_edge) {
    return quick_edge->data.dst;
  }

  static inline FastNode *const &GetConstEdgeDst(const QuickEdge *quick_edge) {
    return quick_edge->data.dst;
  }

  static inline int32_t &GetEdgeSrcOutput(QuickEdge *quick_edge) {
    return quick_edge->data.src_output;
  }

  static inline int32_t GetConstEdgeSrcOutput(const QuickEdge *quick_edge) {
    return quick_edge->data.src_output;
  }

  static inline int32_t &GetEdgeDstInput(QuickEdge *quick_edge) {
    return quick_edge->data.dst_input;
  }

  static inline int32_t GetConstEdgeDstInput(const QuickEdge *quick_edge) {
    return quick_edge->data.dst_input;
  }

  static inline int32_t &GetEdgeInEdgeIndex(QuickEdge *quick_edge) {
    return quick_edge->data.in_edge_index;
  }

  static inline int32_t &GetEdgeOutEdgeIndex(QuickEdge *quick_edge) {
    return quick_edge->data.out_edge_index;
  }

  static inline ExecuteGraph *GetGraph(const ListElement<ExecuteGraph *> *quick_graph) {
    return quick_graph->data;
  }

  static inline FastNode &GetNode(QuickNode *quick_node) {
    return quick_node->data;
  }

  static inline const FastNode &GetConstNode(const QuickNode *quick_node) {
    return quick_node->data;
  }

  template <class T>
  static inline ListMode &GetMode(ListElement<T> *list_element) {
    return list_element->mode;
  }

  template <class T>
  static inline QuickList<T> *GetOwner(ListElement<T> *list_element) {
    return list_element->owner;
  }

  static inline QuickNode *GetListElementAddr(FastNode *fast_node) {
    return reinterpret_cast<QuickNode *>(reinterpret_cast<size_t>(fast_node) - offsetof(QuickNode, data));
  }

  static inline QuickEdge *GetListElementAddr(Edge<FastNode> *edge) {
    return reinterpret_cast<QuickEdge *>(reinterpret_cast<size_t>(edge) - offsetof(QuickEdge, data));
  }
};

template <class NODE_T>
int64_t GetNodeOutputSize(NODE_T *node, std::vector<NodeStatus> &reverse_dfs_nodes_info) {
  int64_t total_size = 0LL;
  if ((node == nullptr) || (node->GetOpDescBarePtr() == nullptr)) {
    return total_size;
  }

  NodeStatus &reverse_dfs_node_info = reverse_dfs_nodes_info[static_cast<size_t>(node->GetOpDescBarePtr()->GetId())];
  total_size = reverse_dfs_node_info.size;
  if (total_size != 0) {
    return total_size;
  }
  for (const auto &out_desc : node->GetOpDescBarePtr()->GetAllOutputsDescPtr()) {
    if (out_desc == nullptr) {
      continue;
    }
    int64_t output_size = 0LL;
    (void)ge::TensorUtils::CalcTensorMemSize(out_desc->GetShape(), out_desc->GetFormat(), out_desc->GetDataType(),
                                             output_size);
    total_size += output_size;
  }
  if (total_size != 0) {
    reverse_dfs_node_info.size = total_size;
  }
  return total_size;
}

template <class NODE_T>
struct NodeCmp {
  explicit NodeCmp(std::vector<NodeStatus> *reverse_dfs_nodes_info) : reverse_dfs_nodes_info_(reverse_dfs_nodes_info) {}
  bool operator()(NODE_T *lhs, NODE_T *rhs) const {
    const auto lhs_size = GetNodeOutputSize(lhs, *reverse_dfs_nodes_info_);
    const auto rhs_size = GetNodeOutputSize(rhs, *reverse_dfs_nodes_info_);
    if (lhs_size == rhs_size) {
      return strcmp(lhs->GetNamePtr(), rhs->GetNamePtr()) > 0;
    }
    return lhs_size > rhs_size;
  }
  std::vector<NodeStatus> *reverse_dfs_nodes_info_;
};

template <class NODE_T>
struct NodeOutInfo {
  NodeOutInfo(NODE_T *node, std::vector<NodeStatus> *reverse_dfs_nodes_info)
      : num_out_data_nodes(node->GetAllOutEdgesSize()),
        output_size(GetNodeOutputSize(node, *reverse_dfs_nodes_info)),
        node_name(node->GetName()) {}

  bool operator<(const NodeOutInfo &rhs) const {
    if (num_out_data_nodes < rhs.num_out_data_nodes) {
      return true;
    }
    if (num_out_data_nodes > rhs.num_out_data_nodes) {
      return false;
    }
    if (output_size < rhs.output_size) {
      return true;
    }
    if (output_size > rhs.output_size) {
      return false;
    }
    return node_name < rhs.node_name;
  }

  int64_t num_out_data_nodes;
  int64_t output_size;
  std::string node_name;
};

template <class NODE_T>
class TopoSortStack {
 public:
  explicit TopoSortStack(std::vector<NodeStatus> *reverse_dfs_nodes_info, const bool is_mem_priority = false,
                         const bool is_dfs = false, const bool is_reverse_dfs = false)
      : is_mem_priority_(is_mem_priority),
        is_dfs_(is_dfs),
        is_reverse_dfs_(is_reverse_dfs),
        reverse_dfs_nodes_info_(reverse_dfs_nodes_info) {}

  NODE_T *Pop() {
    if (is_mem_priority_ && (!is_reverse_dfs_)) {
      const auto &it = mem_priority_stack_.cbegin();
      NODE_T *node = it->second;
      (void)mem_priority_stack_.erase(it);
      return node;
    }
    NODE_T *node = normal_stack_.back();
    normal_stack_.pop_back();
    return node;
  }

  void Push(NODE_T *node) {
    if (is_mem_priority_ && (!is_reverse_dfs_)) {
      (void)mem_priority_stack_.emplace(NodeOutInfo<NODE_T>(node, reverse_dfs_nodes_info_), node);
      return;
    }

    if (is_dfs_) {
      (void)normal_stack_.insert(normal_stack_.end(), node);
    } else {
      (void)normal_stack_.insert(normal_stack_.begin(), node);
    }
  }

  bool Empty() {
    if (is_mem_priority_ && (!is_reverse_dfs_)) {
      return mem_priority_stack_.empty();
    }
    return normal_stack_.empty();
  }

 private:
  bool is_mem_priority_;
  bool is_dfs_;
  bool is_reverse_dfs_;
  std::vector<NodeStatus> *reverse_dfs_nodes_info_;
  std::list<NODE_T *> normal_stack_;
  std::map<NodeOutInfo<NODE_T>, NODE_T *> mem_priority_stack_;
};

}  // namespace ge
#endif  // FAST_GRAPH_FAST_GRAPH_UTILS_H
