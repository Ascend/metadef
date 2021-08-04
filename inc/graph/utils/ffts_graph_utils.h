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

#ifndef INC_GRAPH_UTILS_FFTS_GRAPH_UTILS_H_
#define INC_GRAPH_UTILS_FFTS_GRAPH_UTILS_H_

#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/graph.h"
#include "graph/node.h"

namespace ge {
class FftsGraphUtils {
 public:
  static graphStatus GraphPartition(ComputeGraph &graph, const std::set<NodePtr> &unsupported_nodes);

  static graphStatus GraphPartition(ComputeGraph &graph,
                                    const std::function<std::vector<uint32_t>(const NodePtr &)> &calc_func,
                                    const std::vector<uint32_t> &upper_limit);
 private:
  static graphStatus CollectClipNodesAndGraphs(const ComputeGraphPtr &graph,
                                               const std::set<NodePtr> &unsupported_nodes,
                                               std::unordered_set<NodePtr> &nodes_need_clip,
                                               std::unordered_set<ComputeGraphPtr> &graphs_need_split);

  static bool IsGraphNeedSplit(const ComputeGraphPtr &graph, const std::unordered_set<NodePtr> &nodes_need_clip);

  static graphStatus SplitNodesWithCheck(const ComputeGraphPtr &graph,
                                         const std::unordered_set<NodePtr> &nodes_need_clip,
                                         std::vector<std::pair<bool, std::set<NodePtr>>> &split_nodes);

  static graphStatus BuildFftsPlusSubgraphWithAllNodes(const ComputeGraphPtr &subgraph);

  static void CollectCalcNodeInSubgraph(const ComputeGraphPtr &subgraph, std::set<NodePtr> &calc_nodes);

  static ComputeGraphPtr GetFftsPlusGraph(ComputeGraph &graph);

  static graphStatus SetAttrForFftsPlusSubgraph(const ComputeGraphPtr &subgraph);

  static graphStatus Calculate(const ComputeGraphPtr &graph,
                               const std::function<std::vector<uint32_t>(const NodePtr &)> &calc_func,
                               std::map<NodePtr, std::vector<uint32_t>> &node_value,
                               std::map<ComputeGraphPtr, std::vector<uint32_t>> &graph_value,
                               uint32_t recursive_depth = 1);

  static bool IsValueValid(const ComputeGraphPtr &graph, const std::vector<uint32_t> &upper_limit,
                           const std::map<NodePtr, std::vector<uint32_t>> &node_value,
                           const std::map<ComputeGraphPtr, std::vector<uint32_t>> &graph_value);

  static graphStatus PartitionGraphWithLimit(const ComputeGraphPtr &graph, const std::vector<uint32_t> &upper_limit,
                                             std::map<NodePtr, std::vector<uint32_t>> &node_value,
                                             std::map<ComputeGraphPtr, std::vector<uint32_t>> &graph_value,
                                             uint32_t recursive_depth = 1);

  static graphStatus SplitFuncNode(const std::vector<NodePtr> exceed_single_node,
                                   const std::vector<uint32_t> &upper_limit,
                                   std::map<NodePtr, std::vector<uint32_t>> &node_value,
                                   std::map<ComputeGraphPtr, std::vector<uint32_t>> &graph_value,
                                   uint32_t recursive_depth = 1);
};
}  // namespace ge
#endif  // INC_GRAPH_UTILS_GRAPH_UTILS_H_
