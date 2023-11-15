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

#include "graph/execute_graph.h"
#include "graph/debug/ge_log.h"
#include "common/ge_common/ge_types.h"
#include "fast_graph/fast_graph_impl.h"
#include "fast_graph/fast_node_utils.h"

namespace ge {
namespace {
enum class FastTopoSortingMode { kBFS = 0, kDFS, kRDFS };
const std::string kMemoryPriority = "MemoryPriority";
constexpr int32_t kTopoSortingBfs = 0;
constexpr int32_t kTopoSortingDfs = 1;
constexpr int32_t kTopoSortingReverseDfs = 2;

FastTopoSortingMode GetTopoSortingStrategy() {
  std::string topo_sorting_mode_str;
  if ((ge::GetContext().GetOption(ge::OPTION_TOPOSORTING_MODE, topo_sorting_mode_str) == GRAPH_SUCCESS) &&
      (!topo_sorting_mode_str.empty())) {
    const int32_t base = 10;
    const auto topo_sorting_mode = static_cast<int32_t>(std::strtol(topo_sorting_mode_str.c_str(), nullptr, base));
    if (topo_sorting_mode == kTopoSortingBfs) {
      return FastTopoSortingMode::kBFS;
    } else if (topo_sorting_mode == kTopoSortingDfs) {
      return FastTopoSortingMode::kDFS;
    } else if (topo_sorting_mode == kTopoSortingReverseDfs) {
      return FastTopoSortingMode::kRDFS;
    } else {
      GELOGW("OPTION_TOPOSORTING_MODE = %s is invalid", topo_sorting_mode_str.c_str());
    }
  }
  if (ge::GetContext().GetTrainGraphFlag()) {
    GELOGI("train flag is 1, use BFS.");
    return FastTopoSortingMode::kBFS;
  }

  GELOGI("train flag is 0, use DFS.");
  return FastTopoSortingMode::kDFS;
}

bool IsMemoryPriority() {
  std::string memory_optimization_policy;
  (void)ge::GetContext().GetOption(MEMORY_OPTIMIZATION_POLICY, memory_optimization_policy);
  return (memory_optimization_policy == ge::kMemoryPriority);
}

void GetOutNodesFromEdge(std::map<FastNode *, uint32_t> &map_in_edge_num, FastNode *node,
                         std::vector<FastNode *> &out_nodes) {
  const auto iter = map_in_edge_num.find(node);
  if (iter != map_in_edge_num.end()) {
    --iter->second;
    if (iter->second == 0U) {
      out_nodes.push_back(node);
    }
  }
}

bool InputIsLongLifeTimeNode(const FastNode *node, const ExecuteGraph *execute_graph) {
  bool match = false;
  auto num = node->GetDataInNum();
  for (size_t i = 0LL; i < num; ++i) {
    // the input parameter must be the id of data io
    const auto &edge = node->GetPeerOutDataEdge(i);
    if (edge == nullptr) {
      continue;
    }

    auto &peer_node = edge->src;
    if ((peer_node == nullptr) || (peer_node->GetExtendInfo() == nullptr)) {
      continue;
    }

    const auto type = peer_node->GetType();
    static std::unordered_set<std::string> kDataSet = {DATA, REFDATA, AIPPDATA, ANN_DATA};
    auto graph = peer_node->GetExtendInfo()->GetOwnerGraphBarePtr();
    const bool is_io_data = (execute_graph == graph) && (kDataSet.count(type) > 0);
    if ((!FastNodeUtils::GetConstOpType(peer_node)) && (type != VARIABLE) && (type != VARIABLEV2) && (!is_io_data)) {
      return false;
    } else {
      match = true;
    }
    GELOGD("Node:%s peer:%s type :%s", node->GetName().c_str(), peer_node->GetName().c_str(),
           peer_node->GetType().c_str());
  }

  return match;
}

///  variable  const
///      \    /
///   first node
///       |
///   middle node
///       |
///   last node
///     /  |
/// node1  node2
graphStatus GetOutNodeIndex(std::vector<FastNode *> &nodes, size_t &index, size_t &out_count,
                            const ExecuteGraph *execute_graph) {
  if (nodes.empty()) {
    return GRAPH_FAILED;
  }

  // first node's inputs muse be long life time
  if ((nodes.size() == 1UL) && (!InputIsLongLifeTimeNode(nodes.front(), execute_graph))) {
    return GRAPH_FAILED;
  }

  const auto &node = nodes.back();
  auto op_desc = node->GetOpDescBarePtr();
  GE_CHECK_NOTNULL(op_desc);
  // middle node must be single input
  if ((nodes.size() != 1UL) && (node->GetDataInNum() != 1UL)) {
    return GRAPH_FAILED;
  }

  int64_t min_index = 0LL;
  FastNode *delay_node = nullptr;
  for (const auto &out_node : node->GetAllOutNodes()) {
    out_count++;
    GE_CHECK_NOTNULL(out_node);
    auto out_node_desc = out_node->GetOpDescBarePtr();
    GE_CHECK_NOTNULL(out_node_desc);
    GELOGD("Node:%s id:%ld peer node:%s id:%ld", node->GetName().c_str(), op_desc->GetId(),
           out_node_desc->GetName().c_str(), out_node_desc->GetId());
    if ((min_index == 0LL) || (out_node_desc->GetId() < min_index)) {
      min_index = out_node_desc->GetId();
      delay_node = out_node;
    }
  }

  if (delay_node != nullptr) {
    index = static_cast<size_t>(min_index);
    if (index > (static_cast<size_t>(op_desc->GetId()) + 1UL)) {
      GELOGD("Node:%s id:%ld delay to:%s id:%zu", node->GetName().c_str(), op_desc->GetId(),
             delay_node->GetName().c_str(), index);
    }
    return GRAPH_SUCCESS;
  }
  return GRAPH_FAILED;
}

void DelayTopoSort(std::vector<FastNode *> &nodes, const ExecuteGraph *execute_graph) {
  // pair.first:  this node can be delay or not
  // pair.second: delayed nodes to this node
  std::vector<std::pair<bool, std::vector<FastNode *>>> delay_nodes;
  delay_nodes.resize(nodes.size());

  // set init index
  for (size_t i = 0UL; i < delay_nodes.size(); ++i) {
    nodes[i]->GetOpDescBarePtr()->SetId(static_cast<int64_t>(i));
    delay_nodes[i].first = true;
    delay_nodes[i].second.emplace_back(nodes[i]);
  }

  // move delayed node to fit node
  size_t delay_node_count = 0UL;
  for (size_t i = 0UL; i < delay_nodes.size(); ++i) {
    size_t delay_to_index = 0UL;
    size_t out_count = 0UL;
    if (delay_nodes[i].first &&
        (GetOutNodeIndex(delay_nodes[i].second, delay_to_index, out_count, execute_graph) == GRAPH_SUCCESS) &&
        (delay_to_index < delay_nodes.size()) && (delay_to_index > (i + 1UL))) {
      delay_nodes[delay_to_index].second.insert(delay_nodes[delay_to_index].second.begin(),
                                                delay_nodes[i].second.begin(), delay_nodes[i].second.end());
      if (out_count > 1UL) {
        // last node can not be delay
        delay_nodes[delay_to_index].first = false;
      }
      delay_nodes[i].second.clear();
      delay_node_count++;
    }
  }
  if (delay_node_count > 0UL) {
    nodes.clear();
    for (size_t i = 0UL; i < delay_nodes.size(); ++i) {
      if (!delay_nodes[i].second.empty()) {
        nodes.insert(nodes.end(), delay_nodes[i].second.begin(), delay_nodes[i].second.end());
      }
    }
    GELOGI("Delay %zu nodes.", delay_node_count);
  }
}

void InitNodeStatus(const ExecuteGraph *compute_graph, std::vector<NodeStatus> &reverse_dfs_nodes_info) {
  reverse_dfs_nodes_info.clear();
  reverse_dfs_nodes_info.resize(compute_graph->GetDirectNodesSize());
  int64_t index = 0;
  for (const auto &node : compute_graph->GetDirectNode()) {
    reverse_dfs_nodes_info[index].size = 0U;
    reverse_dfs_nodes_info[index].status = FastWalkStatus::kNotWalked;
    node->GetOpDescBarePtr()->SetId(index);
    index++;
  }
}
}  // namespace

ExecuteGraph::ExecuteGraph(const std::string &name) {
  graph_shared_ = std::make_shared<FastGraphImpl<FastNode, ExecuteGraph>>(name);
  graph_shared_->SetOwnerGraph(this);
}

ExecuteGraph &ExecuteGraph::operator=(ge::ExecuteGraph &exec_graph) {
  if (&exec_graph == this) {
    return *this;
  }

  graph_shared_ = exec_graph.graph_shared_;
  names_to_subgraph_ = exec_graph.names_to_subgraph_;
  inputs_order_ = exec_graph.inputs_order_;
  AttrHolder::SwapBase(exec_graph);
  return *this;
}

ExecuteGraph &ExecuteGraph::CompleteCopy(ge::ExecuteGraph &exec_graph) {
  if (&exec_graph == this) {
    return *this;
  }

  graph_shared_->DeepCopy(*(exec_graph.graph_shared_));

  const std::map<string, GeAttrValue> &original_attrs = AttrUtils::GetAllAttrs(exec_graph);
  for (auto const &attr_iter : original_attrs) {
    if (this->TrySetAttr(attr_iter.first, attr_iter.second) != GRAPH_SUCCESS) {
      GELOGW("Set inherit original attr[%s] failed, Please Check.", attr_iter.first.c_str());
    }
  }

  inputs_order_.clear();
  for (auto &item : exec_graph.inputs_order_) {
    inputs_order_.push_back(item);
  }
  return *this;
}

FastNode *ExecuteGraph::AddNode(const OpDescPtr &op) {
  return graph_shared_->AddNode(op, this);
}

FastNode *ExecuteGraph::AddNode(const OpDescPtr &op, int64_t id) {
  return graph_shared_->AddNode(op, this, id);
}

void ExecuteGraph::RemoveNodeFromNodesFree(FastNode *fast_node) const {
  auto quick_node = FastGraphUtils::GetListElementAddr(fast_node);
  auto owner = quick_node->owner;
  auto mode = quick_node->mode;
  if ((owner != nullptr) && (mode == ListMode::kFreeMode)) {
    owner->erase(quick_node);
  }
}

FastNode *ExecuteGraph::AddNode(FastNode *fast_node) {
  if (fast_node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return nullptr;
  }

  RemoveNodeFromNodesFree(fast_node);
  return graph_shared_->AddNode(fast_node);
}

FastNode *ExecuteGraph::AddNodeFront(const OpDescPtr &op) {
  return graph_shared_->AddNodeFront(op, this);
}

FastNode *ExecuteGraph::AddNodeFront(FastNode *fast_node) {
  if (fast_node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return nullptr;
  }

  RemoveNodeFromNodesFree(fast_node);
  return graph_shared_->AddNodeFront(fast_node);
}

graphStatus ExecuteGraph::RemoveJustNode(FastNode *fast_node) {
  if (fast_node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return GRAPH_FAILED;
  }
  return graph_shared_->RemoveJustNode(FastGraphUtils::GetListElementAddr(fast_node));
}

FastEdge *ExecuteGraph::AddEdge(FastNode *src, int32_t src_index, FastNode *dst, int32_t dst_index) {
  if ((src == nullptr) || (dst == nullptr)) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return nullptr;
  }

  return graph_shared_->AddEdge(src, src_index, dst, dst_index);
}

graphStatus ExecuteGraph::RemoveEdge(FastEdge *edge) {
  if (edge == nullptr) {
    REPORT_INNER_ERROR("E18888", "The edge is nullptr.");
    GE_LOGE("[Check][Param] The edge is nullptr.");
    return GRAPH_FAILED;
  }
  return graph_shared_->RemoveEdge(FastGraphUtils::GetListElementAddr(edge));
}

const FastNode *ExecuteGraph::GetParentNodeBarePtr() const {
  return graph_shared_->GetParentNode();
}

void ExecuteGraph::SetParentNode(FastNode *node) {
  graph_shared_->SetParentNode(node);
}

ExecuteGraph *ExecuteGraph::AddSubGraph(std::shared_ptr<ExecuteGraph> &sub_graph) {
  if (sub_graph == nullptr) {
    REPORT_INNER_ERROR("E18888", "Try to add a null subgraph");
    GE_LOGE("[Check][Param] Try to add a null subgraph");
    return nullptr;
  }

  auto ret = graph_shared_->AddSubGraph(sub_graph.get());
  if (ret == nullptr) {
    return nullptr;
  }

  names_to_subgraph_[sub_graph->GetName()] = {sub_graph, ret};
  return ret->data;
}

graphStatus ExecuteGraph::RemoveSubGraph(const ExecuteGraph *sub_graph) {
  if (sub_graph == nullptr) {
    REPORT_INNER_ERROR("E18888", "Try to add a null subgraph");
    GE_LOGE("[Check][Param] Try to add a null subgraph");
    return GRAPH_PARAM_INVALID;
  }

  return RemoveSubGraph(sub_graph->GetName());
}

ExecuteGraph *ExecuteGraph::AddSubGraph(std::shared_ptr<ExecuteGraph> &sub_graph_ptr, std::string &name) {
  if (sub_graph_ptr == nullptr) {
    REPORT_INNER_ERROR("E18888", "Try to add a null subgraph, name %s", name.c_str());
    GE_LOGE("[Check][Param] Try to add a null subgraph, name %s", name.c_str());
    return nullptr;
  }

  auto sub_graph = sub_graph_ptr.get();
  const auto parent_graph = sub_graph->GetParentGraphBarePtr();
  if (parent_graph == nullptr) {
    REPORT_CALL_ERROR("E18888", "Try to add subgraph without parent graph, name %s", name.c_str());
    GE_LOGE("[Get][Graph] Try to add subgraph without parent graph, name %s", name.c_str());
    return nullptr;
  }

  const auto parent_node = sub_graph->GetParentNodeBarePtr();
  if ((parent_node == nullptr) || (parent_node->GetExtendInfo() == nullptr)) {
    REPORT_CALL_ERROR("E18888", "Try to add a subgraph without parent node, name %s", name.c_str());
    GE_LOGE("[Get][Node] Try to add a subgraph without parent node, name %s", name.c_str());
    return nullptr;
  }

  if (parent_node->GetExtendInfo()->GetOwnerGraphBarePtr() != parent_graph) {
    REPORT_INNER_ERROR("E18888",
                       "Try to add a subgraph which parent node's graph is not equal to "
                       "the subgraph's parent graph, subgraph name %s, parent node name %s",
                       sub_graph->GetName().c_str(), parent_graph->GetName().c_str());
    GE_LOGE(
        "[Check][Param] Try to add a subgraph which parent node's graph is not equal to "
        "the subgraph's parent graph, subgraph name %s, parent node name %s",
        sub_graph->GetName().c_str(), parent_graph->GetName().c_str());
    return nullptr;
  }

  if (name != sub_graph->GetName()) {
    GELOGW("[Add][Subgraph] The subgraph name %s is different with input %s", sub_graph->GetName().c_str(),
           name.c_str());
  }

  if (names_to_subgraph_.find(sub_graph->GetName()) != names_to_subgraph_.end()) {
    REPORT_INNER_ERROR("E18888", "The subgraph %s existed", GetName().c_str());
    GE_LOGE("[Check][Param] The subgraph %s existed", GetName().c_str());
    return nullptr;
  }

  auto ret = graph_shared_->AddSubGraph(sub_graph);
  if (ret == nullptr) {
    return nullptr;
  }
  names_to_subgraph_[sub_graph->GetName()] = {sub_graph_ptr, ret};
  return ret->data;
}

graphStatus ExecuteGraph::RemoveSubGraph(const std::string &name) {
  auto iter = names_to_subgraph_.find(name);
  if (iter != names_to_subgraph_.end()) {
    auto quick_graph = reinterpret_cast<QuickGraph *>(iter->second.quick_graph);
    graph_shared_->RemoveSubGraph(quick_graph);
    names_to_subgraph_.erase(iter);
  }

  return GRAPH_SUCCESS;
}

ExecuteGraph *ExecuteGraph::GetSubGraph(const std::string &name) const {
  const ExecuteGraph *exec_graph = graph_shared_->GetParentGraph();
  if (exec_graph == nullptr) {
    const auto iter = names_to_subgraph_.find(name);
    if (iter == names_to_subgraph_.end()) {
      return nullptr;
    }
    // iter->second.quick_graph is not nullptr
    auto quick_graph = reinterpret_cast<QuickGraph *>(iter->second.quick_graph);
    return quick_graph->data;
  } else {
    return exec_graph->GetSubGraph(name);
  }
}

void ExecuteGraph::ClearAllSubGraph() {
  names_to_subgraph_.clear();
  return graph_shared_->ClearAllSubGraph();
}

std::vector<FastNode *> ExecuteGraph::GetDirectNode() const {
  return graph_shared_->GetDirectNode();
}

size_t ExecuteGraph::GetDirectNodesSize() const {
  return graph_shared_->GetDirectNodesSize();
}

std::vector<FastEdge *> ExecuteGraph::GetAllEdges() const {
  return graph_shared_->GetAllEdges();
}

std::vector<ExecuteGraph *> ExecuteGraph::GetAllSubgraphs() const {
  return graph_shared_->GetAllSubgraphs();
}

FastNode *ExecuteGraph::AddInputNode(FastNode *fast_node) {
  if (fast_node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return nullptr;
  }

  RemoveNodeFromNodesFree(fast_node);
  return graph_shared_->AddInputNode(fast_node);
}

graphStatus ExecuteGraph::RemoveInputNode(FastNode *fast_node) {
  if (fast_node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return GRAPH_FAILED;
  }

  return graph_shared_->RemoveInputNode(fast_node);
}

FastNode *ExecuteGraph::AddOutputNodeByIndex(FastNode *fast_node, int32_t index) {
  if (fast_node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return nullptr;
  }

  RemoveNodeFromNodesFree(fast_node);
  return graph_shared_->AddOutputNodeByIndex(fast_node, index);
}

graphStatus ExecuteGraph::RemoveOutputNode(FastNode *fast_node) {
  if (fast_node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return GRAPH_FAILED;
  }

  return graph_shared_->RemoveOutputNode(fast_node);
}

const FastNode *ExecuteGraph::FindNode(size_t token) const {
  auto quick_node = graph_shared_->FindNode(token);
  return ((quick_node == nullptr) ? nullptr : &(quick_node->data));
}

graphStatus ExecuteGraph::SortNodes(std::vector<FastNode *> &stack,
                                    std::map<FastNode *, uint32_t> &map_in_edge_num) const {
  // Record the number of non data nodes but no input nodes
  uint32_t spec_node_size = 0U;
  for (const auto &node : graph_shared_->GetDirectNodeToModify()) {
    // The node is not nullptr.
    auto fast_node = &FastGraphUtils::GetNode(node);
    GE_IF_BOOL_EXEC(fast_node->GetOpDescBarePtr() == nullptr, continue);
    map_in_edge_num[fast_node] = static_cast<uint32_t>(fast_node->GetInEdgeSize());
    if (map_in_edge_num[fast_node] == 0U) {
      std::string type = fast_node->GetOpDescBarePtr()->GetType();
      if ((!OpTypeUtils::IsDataNode(type)) && (strcmp(type.c_str(), INPUT_TYPE) != 0)) {
        (void)stack.insert(stack.begin(), fast_node);
        spec_node_size++;
        continue;
      }

      // Need to insert the data nodes in reverse order
      (void)stack.insert(stack.begin() + static_cast<int64_t>(spec_node_size), fast_node);
    }
  }

  /// Make sure the inputs order matches with user-designated
  /// 1. Get the index of two input nodes in the user-inputs-order(inputs_order_)
  /// 2. Compare two indices, if not match, swap the positions of two inputs
  /// *: Remind: stack is reverse-order
  for (size_t i = 0UL; i < stack.size(); ++i) {
    // If not found in 'inputs_order_', skip it
    const auto it_i = std::find(inputs_order_.begin(), inputs_order_.end(), stack[i]->GetName());
    GE_IF_BOOL_EXEC(it_i == inputs_order_.end(), continue);
    const auto inx_i = it_i - inputs_order_.begin();
    for (size_t j = i + 1UL; j < stack.size(); ++j) {
      // If not found in 'inputs_order_', skip it
      const auto it_j = std::find(inputs_order_.begin(), inputs_order_.end(), stack[i]->GetName());
      GE_IF_BOOL_EXEC(it_j == inputs_order_.end(), continue);

      // Compare index, swap them if it should be
      const auto inx_j = it_j - inputs_order_.begin();
      GE_IF_BOOL_EXEC(inx_i < inx_j, std::swap(stack[i], stack[j]));
    }
  }

  return GRAPH_SUCCESS;
}

void ExecuteGraph::GetOutNodesFromEdgesToMap(std::map<FastNode *, uint32_t> &map_in_edge_num, FastNode *node,
                                             std::map<std::string, FastNode *> &breadth_node_map) const {
  auto iter = map_in_edge_num.find(node);
  if (iter != map_in_edge_num.end()) {
    --iter->second;
    if (iter->second == 0U) {
      (void)breadth_node_map.emplace(node->GetName(), node);
    }
  }
}

graphStatus ExecuteGraph::CollectBreadthOutNode(FastNode *&node, std::map<FastNode *, uint32_t> &map_in_edge_num,
                                                std::map<std::string, FastNode *> &breadth_node_map) const {
  auto &edges = node->GetAllOutDataEdgesRef();
  if (edges.empty()) {
    return GRAPH_SUCCESS;
  }

  for (size_t i = 0UL; i < edges.size(); ++i) {
    std::for_each(edges[i].begin(), edges[i].end(), [&map_in_edge_num, &breadth_node_map, this](FastEdge *edge) {
      if ((edge != nullptr) && (edge->dst_input != kControlEdgeIndex)) {
        GetOutNodesFromEdgesToMap(map_in_edge_num, edge->dst, breadth_node_map);
      }
    });
  }

  auto &control_edges = node->GetAllOutControlEdgesRef();
  if (control_edges.empty()) {
    return GRAPH_SUCCESS;
  }
  std::for_each(control_edges.begin(), control_edges.end(),
                [&map_in_edge_num, &breadth_node_map, this](FastEdge *edge) {
                  if (edge != nullptr) {
                    GetOutNodesFromEdgesToMap(map_in_edge_num, edge->dst, breadth_node_map);
                  }
                });

  return GRAPH_SUCCESS;
}

graphStatus ExecuteGraph::BFSTopologicalSorting(std::vector<FastNode *> &node_vec, const bool reverse,
                                                const ExecuteGraph *compute_graph) const {
  GELOGD("Runing_Bfs_Sort: %s", GetName().c_str());
  (void)reverse;
  const bool is_mem_priority = IsMemoryPriority();
  std::vector<NodeStatus> reverse_dfs_nodes_info;
  if (is_mem_priority) {
    InitNodeStatus(compute_graph, reverse_dfs_nodes_info);
  }
  TopoSortStack<FastNode> topo_sort_stack(&reverse_dfs_nodes_info, is_mem_priority);
  std::vector<FastNode *> stack_input;
  std::map<std::string, FastNode *> breadth_node_map;
  std::map<FastNode *, uint32_t> map_in_edge_num;
  // Record the number of non data nodes but no input nodes
  GE_CHK_BOOL_EXEC(SortNodes(stack_input, map_in_edge_num) == GRAPH_SUCCESS, return GRAPH_FAILED, "sort nodes failed");
  // Only data nodes here
  while ((!stack_input.empty()) || (!topo_sort_stack.Empty())) {
    FastNode *node = nullptr;
    if (!topo_sort_stack.Empty()) {
      node = topo_sort_stack.Pop();
    } else {
      node = stack_input.back();
      stack_input.pop_back();
    }

    node_vec.push_back(node);
    GE_CHECK_NOTNULL(node->GetOpDescBarePtr());
    GELOGD("node_vec.push_back %s", node->GetOpDescBarePtr()->GetName().c_str());
    (void)CollectBreadthOutNode(node, map_in_edge_num, breadth_node_map);

    for (const auto &name_node : breadth_node_map) {
      (void)topo_sort_stack.Push(name_node.second);
    }
    breadth_node_map.clear();
  }
  return GRAPH_SUCCESS;
}

graphStatus ExecuteGraph::DFSTopologicalSorting(std::vector<FastNode *> &node_vec, const bool reverse,
                                                const ExecuteGraph *compute_graph) const {
  GELOGD("Runing_Dfs_Sort: %s", GetName().c_str());
  std::vector<FastNode *> stack;
  std::map<FastNode *, uint32_t> map_in_edge_num;
  // Record the number of non data nodes but no input nodes
  GE_CHK_BOOL_EXEC(SortNodes(stack, map_in_edge_num) == GRAPH_SUCCESS, return GRAPH_FAILED, "sort nodes failed");
  const bool is_mem_priority = IsMemoryPriority();
  std::vector<NodeStatus> reverse_dfs_nodes_info;
  if (is_mem_priority) {
    InitNodeStatus(compute_graph, reverse_dfs_nodes_info);
  }
  TopoSortStack<FastNode> topo_sort_stack(&reverse_dfs_nodes_info, is_mem_priority, true, reverse);
  for (const auto &node : stack) {
    topo_sort_stack.Push(node);
  }

  std::vector<FastNode *> out_nodes;
  const auto stack_push = [&reverse, &topo_sort_stack](std::vector<FastNode *> &tmp_out_nodes) {
    if (reverse) {
      std::reverse(tmp_out_nodes.begin(), tmp_out_nodes.end());
    }
    for (const auto &node : tmp_out_nodes) {
      topo_sort_stack.Push(node);
    }
    tmp_out_nodes.clear();
  };
  // Only data nodes here
  while (!topo_sort_stack.Empty()) {
    FastNode *node = topo_sort_stack.Pop();
    node_vec.push_back(node);
    GE_CHECK_NOTNULL(node->GetOpDescBarePtr());
    auto &edges = node->GetAllOutDataEdgesRef();
    if (edges.empty()) {
      continue;
    }

    for (size_t i = 0UL; i < edges.size(); ++i) {
      std::for_each(edges[i].begin(), edges[i].end(), [&map_in_edge_num, &out_nodes](FastEdge *edge) {
        if (edge != nullptr) {
          GetOutNodesFromEdge(map_in_edge_num, edge->dst, out_nodes);
        }
      });

      stack_push(out_nodes);
    }

    auto control_edges = node->GetAllOutControlEdgesRef();
    std::for_each(control_edges.begin(), control_edges.end(), [&map_in_edge_num, &out_nodes](FastEdge *edge) {
      if (edge != nullptr) {
        GetOutNodesFromEdge(map_in_edge_num, edge->dst, out_nodes);
      }
    });
    stack_push(out_nodes);
  }

  return GRAPH_SUCCESS;
}

void ExecuteGraph::GetInNodes(const FastNode *current, std::vector<FastNode *> &input_nodes) const {
  auto &in_data_edges = current->GetAllInDataEdgesRef();
  auto &ref = input_nodes;
  for (size_t i = 0UL; i < in_data_edges.size(); i++) {
    auto edge = in_data_edges[i];
    if (edge != nullptr) {
      ref.push_back(edge->src);
    }
  }

  auto &in_control_edges = current->GetAllInControlEdgesRef();
  std::for_each(in_control_edges.begin(), in_control_edges.end(), [&ref](FastEdge *edge) {
    if (edge != nullptr) {
      ref.push_back(edge->src);
    }
  });
}

graphStatus ExecuteGraph::RDFSTopologicalSorting(std::vector<FastNode *> &node_vec, const bool reverse,
                                                 const ExecuteGraph *compute_graph) const {
  (void)reverse;
  GELOGD("Runing_Reverse_Dfs_Sort: %s", GetName().c_str());
  std::vector<NodeStatus> reverse_dfs_nodes_info;
  InitNodeStatus(compute_graph, reverse_dfs_nodes_info);

  for (const auto quick_node : graph_shared_->GetDirectNodeToModify()) {
    auto node = &FastGraphUtils::GetNode(quick_node);
    if (!node->OutNodesIsEmpty()) {
      continue;
    }
    std::vector<FastNode *> stack = {node};
    while (!stack.empty()) {
      const auto current = stack.back();
      NodeStatus &reverse_dfs_node_info = reverse_dfs_nodes_info[current->GetOpDescBarePtr()->GetId()];
      if (reverse_dfs_node_info.status == FastWalkStatus::kNotWalked) {
        reverse_dfs_node_info.status = FastWalkStatus::kWalking;

        std::vector<FastNode *> in_all_nodes;
        GetInNodes(current, in_all_nodes);

        NodeCmp<FastNode> cmp(&reverse_dfs_nodes_info);
        std::set<FastNode *, NodeCmp<FastNode>> input_nodes{in_all_nodes.begin(), in_all_nodes.end(), cmp};
        stack.insert(stack.end(), input_nodes.cbegin(), input_nodes.cend());
        continue;
      }
      stack.pop_back();
      if (reverse_dfs_node_info.status == FastWalkStatus::kWalking) {
        reverse_dfs_node_info.status = FastWalkStatus::kWalked;
        node_vec.emplace_back(current);
      }
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus ExecuteGraph::TopologicalSortingGraph(const ExecuteGraph *execute_graph, const bool dfs_reverse) {
  using TopoSortingStrategy = std::function<graphStatus(ExecuteGraph *, std::vector<FastNode *> &, const bool,
                                                        const ExecuteGraph *compute_graph)>;
  static const std::map<FastTopoSortingMode, TopoSortingStrategy> topo_sorting_strategy{
      {FastTopoSortingMode::kBFS, &ExecuteGraph::BFSTopologicalSorting},
      {FastTopoSortingMode::kDFS, &ExecuteGraph::DFSTopologicalSorting},
      {FastTopoSortingMode::kRDFS, &ExecuteGraph::RDFSTopologicalSorting}};

  std::vector<FastNode *> node_vec;
  const auto use_topo_strategy = GetTopoSortingStrategy();
  const auto it = topo_sorting_strategy.find(use_topo_strategy);
  if (it == topo_sorting_strategy.end()) {
    GELOGE(GRAPH_FAILED, "Can not find topo sorting strategy of %d.", static_cast<int32_t>(use_topo_strategy));
    return GRAPH_FAILED;
  }

  if (it->second(this, node_vec, dfs_reverse, execute_graph) != GRAPH_SUCCESS) {
    return GRAPH_FAILED;
  }

  // If they are not equal, there is a closed loop
  if (node_vec.size() != GetDirectNodesSize()) {
    std::set<FastNode *> itered_nodes_set;
    for (auto &node : node_vec) {
      (void)itered_nodes_set.insert(node);
    }
    REPORT_INNER_ERROR("E18888", "Failed to do topo sorting total %zu, itered %zu, exist closed loop in graph:%s",
                       GetDirectNodesSize(), node_vec.size(), GetName().c_str());
    GELOGW("[Check][Param] Failed to do topo sorting total %zu, itered %zu, exist closed loop in graph.",
           GetDirectNodesSize(), node_vec.size());
    for (auto node : graph_shared_->GetDirectNodeToModify()) {
      if (itered_nodes_set.count(&FastGraphUtils::GetNode(node)) == 0UL) {
        GELOGW("[Check][Param] The node %s does not itered when topological sorting",
               FastGraphUtils::GetNode(node).GetName().c_str());
      }
    }
    return GRAPH_FAILED;
  }

  if (IsMemoryPriority() || (use_topo_strategy == FastTopoSortingMode::kRDFS)) {
    DelayTopoSort(node_vec, execute_graph);
  }

  auto ret = graph_shared_->SetNodesAfterSorting(node_vec);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  graph_shared_->SetValidFlag(true);
  return GRAPH_SUCCESS;
}

void ExecuteGraph::GetAllNodesFromOpdesc(std::vector<ExecuteGraph *> &subgraphs, const OpDesc &op_desc,
                                         std::deque<FastNode *> &candidates) const {
  const auto &subgraph_names = op_desc.GetSubgraphInstanceNames();
  auto name_iter = subgraph_names.rbegin();
  while (name_iter != subgraph_names.rend()) {
    auto subgraph = GetSubGraph(*name_iter);
    if (subgraph != nullptr) {
      subgraphs.emplace_back(subgraph);
      auto subgraph_nodes = subgraph->GetDirectNode();
      (void)candidates.insert(candidates.begin(), subgraph_nodes.begin(), subgraph_nodes.end());
    }
    ++name_iter;
  }
}

std::vector<FastNode *> ExecuteGraph::AllGraphNodes(std::vector<ExecuteGraph *> &subgraphs) const {
  std::vector<FastNode *> all_nodes;
  std::deque<FastNode *> candidates;

  auto &ref = graph_shared_->GetDirectNodeToModify();
  for (auto iter = ref.begin(); iter != ref.end(); ++iter) {
    QuickNode *node = *iter;
    candidates.push_back(&(node->data));
  }

  while (!candidates.empty()) {
    FastNode *node = candidates.front();
    all_nodes.emplace_back(node);
    candidates.pop_front();

    const auto op_desc = node->GetOpDescBarePtr();
    if (op_desc != nullptr) {
      GetAllNodesFromOpdesc(subgraphs, *op_desc, candidates);
    }
  }

  return all_nodes;
}

graphStatus ExecuteGraph::TopologicalSorting() {
  auto ret = TopologicalSortingGraph(this, false);
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E18888", "Graph [%s] topological sort failed, saved to file black_box", GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Sort][Graph] Graph [%s] topological sort failed, saved to file black_box",
           GetName().c_str());
    return ret;
  }

  const auto &src_sub_graphs = graph_shared_->sub_graphs_;
  if (src_sub_graphs.empty()) {
    return GRAPH_SUCCESS;
  }

  // partition sub graph
  for (auto sub_graph : src_sub_graphs) {
    GE_CHECK_NOTNULL(sub_graph);
    GE_CHECK_NOTNULL(FastGraphUtils::GetGraph(sub_graph));
    ret = FastGraphUtils::GetGraph(sub_graph)->TopologicalSortingGraph(FastGraphUtils::GetGraph(sub_graph), false);
    if (ret != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E18888", "Sub graph[%s] topological sort failed, saved to file black_box",
                        FastGraphUtils::GetGraph(sub_graph)->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Sort][Graph] Sub graph[%s] topological sort failed, saved to file black_box",
             FastGraphUtils::GetGraph(sub_graph)->GetName().c_str());
      return ret;
    }
  }

  std::vector<ExecuteGraph *> subgraphs;
  auto nodes = AllGraphNodes(subgraphs);
  int64_t i = 0LL;
  for (auto iter = nodes.begin(); iter != nodes.end(); ++iter) {
    FastNode *node = *iter;              // [node: should not be null]
    node->GetOpDescBarePtr()->SetId(i);  // [node->GetOpDescBarePtr(): should not be null]
    ++i;
  }

  if (src_sub_graphs.size() != subgraphs.size()) {  // Graph Partition use subgraph, Keep original
    GELOGW("[TopoSort][CheckNodeSize] Keep original subgraph for graph size %zu not equal %zu.", src_sub_graphs.size(),
           subgraphs.size());
    return GRAPH_SUCCESS;
  }
  graph_shared_->SetSubGraph(subgraphs);
  return GRAPH_SUCCESS;
}

void ExecuteGraph::SetName(const std::string &name) {
  graph_shared_->SetName(name);
}

std::string ExecuteGraph::GetName() const {
  return graph_shared_->GetName();
}

void ExecuteGraph::SetParentGraph(ExecuteGraph *parent_graph) {
  graph_shared_->SetParentGraph(parent_graph);
}

const ExecuteGraph *ExecuteGraph::GetParentGraphBarePtr(void) const {
  return graph_shared_->GetParentGraph();
}

graphStatus ExecuteGraph::RecycleQuickEdge(FastEdge *fast_edge) {
  if (fast_edge == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return GRAPH_FAILED;
  }
  return graph_shared_->RecycleQuickEdge(FastGraphUtils::GetListElementAddr(fast_edge));
}

graphStatus ExecuteGraph::RecycleQuickNode(FastNode *fast_node) {
  if (fast_node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node is nullptr.");
    GE_LOGE("[Check][Param] The node is nullptr.");
    return GRAPH_FAILED;
  }
  return graph_shared_->RecycleQuickNode(FastGraphUtils::GetListElementAddr(fast_node));
}

std::vector<FastNode *> ExecuteGraph::GetAllNodes() const {
  std::vector<ExecuteGraph *> subgraphs;
  return AllGraphNodes(subgraphs);
}

void ExecuteGraph::SetInputsOrder(const std::vector<std::string> &inputs_order) {
  inputs_order_ = inputs_order;
}

void ExecuteGraph::ReorderByNodeId() {
  graph_shared_->ReorderByNodeId();
}

void ExecuteGraph::SetGraphId(size_t graph_id) {
  graph_shared_->SetGraphId(graph_id);
}

size_t ExecuteGraph::GetGraphId() const {
  return graph_shared_->GetGraphId();
}

ProtoAttrMap &ExecuteGraph::MutableAttrMap() {
  return attrs_;
}

ConstProtoAttrMap &ExecuteGraph::GetAttrMap() const {
  return attrs_;
}

}  // namespace ge