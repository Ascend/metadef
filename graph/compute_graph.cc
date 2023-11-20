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

#include "graph/compute_graph.h"

#include <deque>
#include "graph/ge_context.h"
#include "graph/debug/ge_attr_define.h"
#include "debug/ge_log.h"
#include "debug/ge_op_types.h"
#include "debug/ge_util.h"
#include "common/ge_common/debug/ge_log.h"
#include "common/util/error_manager/error_manager.h"
#include "graph/compute_graph_impl.h"
#include "graph/utils/ge_ir_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/ge_common/string_util.h"
#include "common/ge_common/ge_types.h"
#include "graph/utils/tensor_utils.h"
#include "common/util/mem_utils.h"
#include "graph/utils/op_type_utils.h"

namespace ge {
namespace {
const size_t OUTPUT_PARAM_SIZE = 2UL;
constexpr int32_t kTopoSortingBfs = 0;
constexpr int32_t kTopoSortingDfs = 1;
constexpr int32_t kTopoSortingReverseDfs = 2;
const std::string kMemoryPriority = "MemoryPriority";
TopoSortingMode GetTopoSortingStrategy() {
  std::string topo_sorting_mode_str;
  if ((ge::GetContext().GetOption(ge::OPTION_TOPOSORTING_MODE, topo_sorting_mode_str) == GRAPH_SUCCESS) &&
      (!topo_sorting_mode_str.empty())) {
    const int32_t base = 10;
    const auto topo_sorting_mode = static_cast<int32_t>(std::strtol(topo_sorting_mode_str.c_str(), nullptr, base));
    if (topo_sorting_mode == kTopoSortingBfs) {
      return TopoSortingMode::kBFS;
    } else if (topo_sorting_mode == kTopoSortingDfs) {
      return TopoSortingMode::kDFS;
    } else if (topo_sorting_mode == kTopoSortingReverseDfs) {
      return TopoSortingMode::kRDFS;
    } else {
      GELOGW("OPTION_TOPOSORTING_MODE = %s is invalid", topo_sorting_mode_str.c_str());
    }
  }

  if (ge::GetContext().GetTrainGraphFlag()) {
    GELOGI("train flag is 1, use BFS.");
    return TopoSortingMode::kBFS;
  }

  GELOGI("train flag is 0, use DFS.");
  return TopoSortingMode::kDFS;
}

struct NodeStatus {
  size_t size = 0U;
  WalkStatus status;
};

void InitNodeStatus(const ConstComputeGraphPtr &compute_graph, std::vector<NodeStatus> &reverse_dfs_nodes_info) {
  reverse_dfs_nodes_info.clear();
  reverse_dfs_nodes_info.resize(compute_graph->GetDirectNodesSize());
  int64_t index = 0;
  for (const auto &node : compute_graph->GetDirectNode()) {
    reverse_dfs_nodes_info[index].size = 0;
    reverse_dfs_nodes_info[index].status = WalkStatus::kNotWalked;
    node->GetOpDesc()->SetId(index);
    index++;
  }
}

int64_t GetNodeOutputRealSize(const NodePtr &node, std::vector<NodeStatus> &reverse_dfs_nodes_info) {
  int64_t total_size = 0;
  if ((node == nullptr) || (node->GetOpDesc() == nullptr)) {
    return total_size;
  }
  NodeStatus &reverse_dfs_node_info = reverse_dfs_nodes_info[static_cast<size_t>(node->GetOpDesc()->GetId())];
  total_size = reverse_dfs_node_info.size;
  if (total_size != 0) {
    return total_size;
  }
  for (const auto &out_desc : node->GetOpDescBarePtr()->GetAllOutputsDescPtr()) {
    if (out_desc == nullptr) {
      continue;
    }
    int64_t output_size = 0;
    (void) ge::TensorUtils::CalcTensorMemSize(out_desc->GetShape(), out_desc->GetFormat(), out_desc->GetDataType(),
                                              output_size);
    total_size += output_size;
  }
  if (total_size != 0) {
    reverse_dfs_node_info.size = total_size;
  }
  return total_size;
}

struct NodeCmp {
  explicit NodeCmp(std::vector<NodeStatus> *reverse_dfs_nodes_info) : reverse_dfs_nodes_info_(reverse_dfs_nodes_info) {}
  bool operator()(const NodePtr &lhs, const NodePtr &rhs) const {
    const auto lhs_size = GetNodeOutputRealSize(lhs, *reverse_dfs_nodes_info_);
    const auto rhs_size = GetNodeOutputRealSize(rhs, *reverse_dfs_nodes_info_);
    if (lhs_size == rhs_size) {
      return strcmp(lhs->GetNamePtr(), rhs->GetNamePtr()) > 0;
    }
    return lhs_size > rhs_size;
  }
  std::vector<NodeStatus> *reverse_dfs_nodes_info_;
};

struct NodeOutInfo {
  NodeOutInfo(const NodePtr &node, std::vector<NodeStatus> *reverse_dfs_nodes_info)
      : num_out_data_nodes(node->GetOutDataNodesSize()),
        output_size(GetNodeOutputRealSize(node, *reverse_dfs_nodes_info)), node_name(node->GetName()) {}

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

bool IsMemoryPriority() {
  std::string memory_optimization_policy;
  (void) ge::GetContext().GetOption(MEMORY_OPTIMIZATION_POLICY, memory_optimization_policy);
  return (memory_optimization_policy == kMemoryPriority);
}

class TopoSortStack {
 public:
  explicit TopoSortStack(std::vector<NodeStatus> *reverse_dfs_nodes_info, const bool is_mem_priority = false,
                         const bool is_dfs = false, const bool is_reverse_dfs = false)
      : is_mem_priority_(is_mem_priority), is_dfs_(is_dfs), is_reverse_dfs_(is_reverse_dfs),
        reverse_dfs_nodes_info_(reverse_dfs_nodes_info) {}

  NodePtr Pop() {
    if (is_mem_priority_ && (!is_reverse_dfs_)) {
      const auto &it = mem_priority_stack_.cbegin();
      const NodePtr node = it->second;
      (void) mem_priority_stack_.erase(it);
      return node;
    }
    const NodePtr node = normal_stack_.back();
    normal_stack_.pop_back();
    return node;
  }

  void Push(const NodePtr &node) {
    if (is_mem_priority_ && (!is_reverse_dfs_)) {
      (void) mem_priority_stack_.emplace(NodeOutInfo(node, reverse_dfs_nodes_info_), node);
      return;
    }
    if (is_dfs_) {
      (void) normal_stack_.insert(normal_stack_.end(), node);
    } else {
      (void) normal_stack_.insert(normal_stack_.begin(), node);
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
  std::vector<NodePtr> normal_stack_;
  std::map<NodeOutInfo, NodePtr> mem_priority_stack_;
};
}  // namespace

ComputeGraphImpl::ComputeGraphImpl(const std::string &name)
    : name_(name),
      nodes_(),
      input_nodes_(),
      sub_graph_(),
      is_valid_flag_(false),
      need_iteration_(false) {
}

std::string ComputeGraphImpl::GetName() const { return name_; }

void ComputeGraphImpl::SetName(const std::string &name) { name_ = name; }

size_t ComputeGraphImpl::GetAllNodesSize(const ConstComputeGraphPtr &compute_graph) const {
  return GetAllNodes(compute_graph).size();
}

ComputeGraphImpl::Vistor<NodePtr> ComputeGraphImpl::GetAllNodes(const ConstComputeGraphPtr &compute_graph) const {
  std::vector<std::shared_ptr<ComputeGraph>> subgraphs;
  return AllGraphNodes(subgraphs, compute_graph);
}

void ComputeGraphImpl::GetAllNodesFromOpdesc(const OpDesc &op_desc, const GraphFilter &graph_filter,
                                             std::deque<NodePtr>& candidates, const NodePtr node) const {
  const auto &subgraph_names = op_desc.GetSubgraphInstanceNames();
  auto name_iter = subgraph_names.rbegin();
  while (name_iter != subgraph_names.rend()) {
    const auto subgraph = GetSubgraph(*name_iter);
    if (subgraph != nullptr) {
      if ((graph_filter == nullptr) || graph_filter(*node, name_iter->c_str(), subgraph)) {
        auto subgraph_nodes = subgraph->GetDirectNode();
        (void) (candidates.insert(candidates.begin(), subgraph_nodes.begin(), subgraph_nodes.end()));
      }
    }
    ++name_iter;
  }
}

ComputeGraphImpl::Vistor<NodePtr> ComputeGraphImpl::GetAllNodes(const NodeFilter &node_filter,
                                                                const GraphFilter &graph_filter,
                                                                const ConstComputeGraphPtr &compute_graph) const {
  std::vector<NodePtr> all_nodes;
  std::deque<NodePtr> candidates;

  (void)candidates.insert(candidates.begin(), nodes_.begin(), nodes_.end());
  while (!candidates.empty()) {
    NodePtr node = candidates.front();
    candidates.pop_front();

    if ((node_filter == nullptr) || node_filter(*node)) {
      all_nodes.emplace_back(node);
    }

    const auto op_desc = node->GetOpDescBarePtr();
    if (op_desc != nullptr) {
      GetAllNodesFromOpdesc(*op_desc, graph_filter, candidates, node);
    }
  }

  return Vistor<NodePtr>(compute_graph, all_nodes);
}

void inline ComputeGraphImpl::GetAllNodesFromOpdesc(std::vector<ComputeGraphPtr> &subgraphs, const OpDesc &op_desc,
                                                    std::deque<NodePtr>& candidates) const {
  const auto &subgraph_names = op_desc.GetSubgraphInstanceNames();
  auto name_iter = subgraph_names.rbegin();
  while (name_iter != subgraph_names.rend()) {
    auto subgraph = GetSubgraph(*name_iter);
    if (subgraph != nullptr) {
      subgraphs.emplace_back(subgraph);
      auto subgraph_nodes = subgraph->GetDirectNode();
      (void) candidates.insert(candidates.begin(), subgraph_nodes.begin(), subgraph_nodes.end());
    }
    ++name_iter;
  }
}

ComputeGraphImpl::Vistor<NodePtr> ComputeGraphImpl::AllGraphNodes(std::vector<ComputeGraphPtr> &subgraphs,
                                                                  const ConstComputeGraphPtr &compute_graph) const {
  std::vector<NodePtr> all_nodes;
  std::deque<NodePtr> candidates;

  (void)candidates.insert(candidates.begin(), nodes_.begin(), nodes_.end());
  while (!candidates.empty()) {
    NodePtr node = candidates.front();
    all_nodes.emplace_back(node);
    candidates.pop_front();

    const auto op_desc = node->GetOpDescBarePtr();
    if (op_desc != nullptr) {
      GetAllNodesFromOpdesc(subgraphs, *op_desc, candidates);
    }
  }

  return Vistor<NodePtr>(compute_graph, all_nodes);
}

ComputeGraphImpl::Vistor<NodePtr> ComputeGraphImpl::GetNodes(const bool is_unknown_shape,
                                                             const ConstComputeGraphPtr &compute_graph) const {
  if (is_unknown_shape) {
    return GetDirectNode(compute_graph);
  } else {
    return GetAllNodes(compute_graph);
  }
}

ComputeGraphImpl::Vistor<NodePtr> ComputeGraphImpl::GetNodes(const bool is_unknown_shape,
                                                             const NodeFilter &node_filter,
                                                             const GraphFilter &graph_filter,
                                                             const ConstComputeGraphPtr &compute_graph) const {
  return is_unknown_shape ? GetDirectNode(compute_graph) : GetAllNodes(node_filter, graph_filter, compute_graph);
}

size_t ComputeGraphImpl::GetDirectNodesSize() const { return direct_nodes_size_; }

ComputeGraphImpl::Vistor<NodePtr> ComputeGraphImpl::GetDirectNode(const ConstComputeGraphPtr &compute_graph) const {
  return Vistor<NodePtr>(compute_graph, nodes_);
}

ComputeGraphImpl::Vistor<NodePtr> ComputeGraphImpl::GetInputNodes(const ConstComputeGraphPtr &compute_graph) const {
  return Vistor<NodePtr>(compute_graph, input_nodes_);
}

ComputeGraphImpl::Vistor<NodePtr> ComputeGraphImpl::GetOutputNodes(const ConstComputeGraphPtr &compute_graph) const {
  std::vector<NodePtr> result;
  auto iter = output_nodes_info_.begin();
  while (iter != output_nodes_info_.end()) {
    result.push_back(iter->first);
    ++iter;
  }
  return Vistor<NodePtr>(compute_graph, result);
}

NodePtr ComputeGraphImpl::FindNode(const std::string &name) const {
  for (const auto &node : nodes_) {
    if (node == nullptr) {
      continue;
    }
    if (NodeUtils::IsNameEqual(node, name.c_str())) {
      return node;
    }
  }
  return nullptr;
}

NodePtr ComputeGraphImpl::FindFirstNodeMatchType(const std::string &type) const {
  for (const auto &node : nodes_) {
    if (node == nullptr) {
      continue;
    }
    if (NodeUtils::IsTypeEqual(node, type.c_str())) {
      return node;
    }
  }
  return nullptr;
}

bool ComputeGraphImpl::GraphAttrsAreEqual(const ComputeGraphImpl &r_graph) const {
  // 整改前实现中，只比较了属性名字，没有比较属性内容，暂时维持这个玩法
  return attrs_.GetAllAttrNames() == r_graph.attrs_.GetAllAttrNames();
}

/// Since there may be different input nodes
/// chosen by user in the same graph, special judgment is needed
bool ComputeGraphImpl::VectorInputNodePtrIsEqual(const std::vector<NodePtr> &left_nodes,
                                                 const std::vector<NodePtr> &right_nodes) const {
  const auto left_nodes_size = left_nodes.size();
  const auto right_nodes_size = right_nodes.size();
  if (left_nodes_size != right_nodes_size) {
    REPORT_INNER_ERROR("E18888", "Check failed with graph input_nodes_: "
                       "left inputNodes size %zu is different with right inputNodes size %zu .",
                       left_nodes_size, right_nodes_size);
    GELOGE(GRAPH_FAILED, "[Check][Param] failed with graph input_nodes_: "
           "left inputNodes size %zu is different with right inputNodes size %zu .",
           left_nodes_size, right_nodes_size);
    return false;
  }
  for (size_t j = 0UL; j < left_nodes_size; j++) {
    if ((left_nodes.at(j) == nullptr) || (right_nodes.at(j) == nullptr)) {
      REPORT_INNER_ERROR("E18888", "left_nodes.at(%zu) or right_nodes.at(%zu) is nullptr", j, j);
      GELOGE(GRAPH_FAILED, "[Check][Param] left_nodes.at(%zu) or right_nodes.at(%zu) is nullptr", j, j);
      return false;
    }
    const auto &left_input_name = left_nodes.at(j)->GetName();
    const auto &right_input_name = right_nodes.at(j)->GetName();
    if (left_input_name != right_input_name) {
      REPORT_INNER_ERROR("E18888", "Check failed with graph input_nodes_: "
                         "left inputNode name %s is different with right inputNode name %s at inputNodes index %zu.",
                         left_input_name.c_str(), right_input_name.c_str(), j);
      GELOGE(GRAPH_FAILED, "[Check][Param] failed with graph input_nodes_: "
             "left inputNode name %s is different with right inputNode name %s at inputNodes index %zu.",
             left_input_name.c_str(), right_input_name.c_str(), j);
      return false;
    }
  }
  return true;
}

bool ComputeGraphImpl::GraphMembersAreEqual(const ComputeGraphImpl &r_graph) const {
  return (IsEqual(this->sub_graph_.size(), r_graph.sub_graph_.size(), "graph.subgraphs_.size()") &&
          IsEqual(this->GetDirectNodesSize(), r_graph.GetDirectNodesSize(), "graph.nodes_.size()") &&
          VectorInputNodePtrIsEqual(this->input_nodes_, r_graph.input_nodes_) &&
          IsEqual(this->name_, r_graph.name_, "graph.name_") &&
          IsEqual(this->is_valid_flag_, r_graph.is_valid_flag_, "graph.is_valid_flag_") &&
          IsEqual(this->need_iteration_, r_graph.need_iteration_, "graph.need_iteration_") &&
          IsEqual(this->params_share_map_, r_graph.params_share_map_, "graph.params_share_map_") &&
          IsEqual(this->out_nodes_map_, r_graph.out_nodes_map_, "graph.out_nodes_map_") &&
          IsEqual(this->inputs_order_, r_graph.inputs_order_, "graph.inputs_order_") &&
          IsEqual(this->output_size_, r_graph.output_size_, "graph.output_size_") &&
          IsEqual(this->input_size_, r_graph.input_size_, "graph.input_size_") &&
          IsEqual(this->output_nodes_info_, r_graph.output_nodes_info_, "graph.output_nodes_info_"));
}

bool ComputeGraphImpl::operator==(const ComputeGraphImpl &r_graph) const {
  // Firstly: Graph's members equal
  if ((!GraphMembersAreEqual(r_graph)) || (!GraphAttrsAreEqual(r_graph))) {
    return false;
  }

  // Secondly: Node equal means the link relationship between node and node itself equal
  for (const auto &left_node : nodes_) {
    if (left_node == nullptr) {
      REPORT_INNER_ERROR("E18888", "left_node is nullptr, graph:%s", this->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Check][Param] left_node is nullptr");
      return false;
    }
    const auto &node_name = left_node->GetName();
    // After TopologicalSorting, node order can change, so find node by name
    const auto &right_node = r_graph.FindNode(node_name);
    GE_IF_BOOL_EXEC(right_node == nullptr,
                    REPORT_INNER_ERROR("E18888", "left_node:%s not find in r_graph:%s",
                                       node_name.c_str(), r_graph.GetName().c_str());
                    GELOGE(GRAPH_FAILED, "[Check][Param] right_node is NULL!!!"); return false);
    if (!((*right_node) == (*left_node))) {
      REPORT_INNER_ERROR("E18888", "Compare graph failed, node:%s not equal.", node_name.c_str());
      GELOGE(GRAPH_FAILED, "[Compare][Graph] failed, node:%s not equal.", node_name.c_str());
      return false;
    }
  }

  // Thirdly: Recursively determine whether the sub graphs are equal
  for (size_t i = 0UL; i < this->sub_graph_.size(); i++) {
    if (!((*((this->sub_graph_)[i])) == (*((r_graph.sub_graph_)[i])))) {
      return false;
    }
  }
  return true;
}

NodePtr ComputeGraphImpl::AddNodeFront(const NodePtr node) {
  if ((node == nullptr) || (node->GetOpDescBarePtr() == nullptr)) {
    REPORT_INNER_ERROR("E18888", "The node ptr or op desc should not be null.");
    GELOGE(GRAPH_FAILED, "[Check][Param] The node ptr or op desc should not be null.");
    return nullptr;
  }
  node->SetHostNode(is_valid_flag_);
  node->GetOpDescBarePtr()->SetId(static_cast<int64_t>(GetDirectNodesSize()));
  if ((GetDirectNodesSize() > 0UL) && ((*(nodes_.begin()))->GetType() == DATA)) {
    InsertToNodeList(next(nodes_.begin()), node);
  } else {
    InsertToNodeList(nodes_.begin(), node);
  }
  AddInputDataNode(node);
  return node;
}

NodePtr ComputeGraphImpl::AddNodeFront(const OpDescPtr &op,
                                       const ComputeGraphPtr &compute_graph) {
  if (op == nullptr) {
    REPORT_INNER_ERROR("E18888", "The OpDesc ptr should not be null.");
    GELOGE(GRAPH_FAILED, "[Check][Param] The OpDesc ptr should not be null.");
    return nullptr;
  }
  op->SetId(static_cast<int64_t>(GetDirectNodesSize()));
  const NodePtr node_ptr = std::shared_ptr<Node>(new (std::nothrow) Node(op, compute_graph));
  GE_IF_BOOL_EXEC(node_ptr == nullptr, GELOGE(GRAPH_FAILED, "[Create][Node] node_ptr is NULL!!!");
                  return nullptr);
  GE_IF_BOOL_EXEC(node_ptr->Init() != GRAPH_SUCCESS,
                  REPORT_CALL_ERROR("E18888", "node %s init failed.", op->GetName().c_str());
                  GELOGE(GRAPH_FAILED, "node init fail.");
                  return nullptr);
  return AddNodeFront(node_ptr);
}

NodePtr ComputeGraphImpl::AddNode(const NodePtr node) {
  if ((node == nullptr) || (node->GetOpDescBarePtr() == nullptr)) {
    REPORT_INNER_ERROR("E18888", "the node ptr or op desc ptr should not be null.");
    GELOGE(GRAPH_FAILED, "[Check][Param] The node ptr or op desc ptr should not be null.");
    return nullptr;
  }
  node->SetHostNode(is_valid_flag_);
  node->GetOpDescBarePtr()->SetId(static_cast<int64_t>(GetDirectNodesSize()));
  PushBackToNodeList(node);
  AddInputDataNode(node);
  return node;
}

NodePtr ComputeGraphImpl::AddNode(const OpDescPtr op, const ComputeGraphPtr &compute_graph) {
  if (op == nullptr) {
    REPORT_INNER_ERROR("E18888", "The OpDesc ptr should not be null.");
    GELOGE(GRAPH_FAILED, "[Check][Param] The OpDesc ptr should not be null.");
    return nullptr;
  }
  op->SetId(static_cast<int64_t>(GetDirectNodesSize()));
  const NodePtr node_ptr = std::shared_ptr<Node>(new (std::nothrow) Node(op, compute_graph));
  GE_IF_BOOL_EXEC(node_ptr == nullptr,
                  REPORT_CALL_ERROR("E18888", "create node failed.");
                  GELOGE(GRAPH_FAILED, "[Create][Node] node_ptr is NULL!!!"); return nullptr);
  GE_IF_BOOL_EXEC(node_ptr->Init() != GRAPH_SUCCESS,
                  REPORT_CALL_ERROR("E18888", "node:%s init failed.", op->GetName().c_str());
                  GELOGE(GRAPH_FAILED, "[Init][Node] %s fail.", op->GetName().c_str());
                  return nullptr);
  return AddNode(node_ptr);
}

NodePtr ComputeGraphImpl::AddNode(const OpDescPtr op, const int64_t id, const ComputeGraphPtr &compute_graph) {
  if (op == nullptr) {
    REPORT_INNER_ERROR("E18888", "The OpDesc ptr should not be null.");
    GELOGE(GRAPH_FAILED, "[Check][Param] The OpDesc ptr should not be null.");
    return nullptr;
  }
  op->SetId(id);
  const NodePtr node = std::shared_ptr<Node>(new (std::nothrow) Node(op, compute_graph));
  GE_IF_BOOL_EXEC(node == nullptr,
                  REPORT_CALL_ERROR("E18888", "create node failed.");
                  GELOGE(GRAPH_FAILED, "[Create][Node] node_ptr is NULL!!!"); return nullptr);
  GE_IF_BOOL_EXEC(node->Init() != GRAPH_SUCCESS,
                  REPORT_CALL_ERROR("E18888", "node init failed.");
                  GELOGE(GRAPH_FAILED, "[Init][Node] fail.");
                  return nullptr);
  node->SetHostNode(is_valid_flag_);
  PushBackToNodeList(node);
  AddInputDataNode(node);
  return node;
}

void ComputeGraphImpl::AddInputDataNode(const NodePtr &node) {
  if (OpTypeUtils::IsDataNode(node->GetType())) {
    if (std::find(input_nodes_.begin(), input_nodes_.end(), node) == input_nodes_.end()) {
      input_nodes_.push_back(node);
    }
  }
}

NodePtr ComputeGraphImpl::AddInputNode(const NodePtr node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node ptr should not be null.");
    GELOGE(GRAPH_FAILED, "[Check][Param] The node ptr should not be null.");
    return nullptr;
  }
  if (std::find(input_nodes_.begin(), input_nodes_.end(), node) == input_nodes_.end()) {
    input_nodes_.push_back(node);
  }
  if (std::find(nodes_.begin(), nodes_.end(), node) == nodes_.end()) {
    GE_CHK_BOOL_EXEC(AddNode(node) != nullptr, return nullptr, "[Add][Node] failed");
  }
  return node;
}

NodePtr ComputeGraphImpl::AddOutputNode(const NodePtr node) {
  return AddOutputNodeByIndex(node, 0);
}

NodePtr ComputeGraphImpl::AddOutputNodeByIndex(const NodePtr node, const int32_t index) {
  if ((node == nullptr) || (node->GetOpDescBarePtr() == nullptr)) {
    REPORT_INNER_ERROR("E18888", "The node ptr or opdesc should not be null.");
    GELOGE(GRAPH_FAILED, "[Check][Param] The node ptr or opdesc should not be null.");
    return nullptr;
  }

  bool already_have = false;
  NodePtr result = node;
  // [output_nodes_info_ : should not be null]
  for (const auto &item : output_nodes_info_) {
    if ((item.first->GetName() == node->GetName()) && (item.second == index)) {
      already_have = true;
      result = item.first;
      break;
    }
  }

  if (!already_have) {
    output_nodes_info_.emplace_back(std::make_pair(node, index));
    GELOGI("Push back node name:%s, index:%d, into output_nodes_info_.", node->GetName().c_str(), index);
  }

  if (std::find(nodes_.begin(), nodes_.end(), node) == nodes_.end()) {
    GE_CHK_BOOL_EXEC(AddNode(node) != nullptr, return nullptr, "[Add][Node] failed");
  }
  return result;
}

graphStatus ComputeGraphImpl::RemoveConstInput(const NodePtr &node) {
  GE_CHECK_NOTNULL(node);

  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    const auto out_anchor = in_anchor->GetPeerOutAnchor();
    if ((out_anchor == nullptr) || (out_anchor->GetOwnerNode() == nullptr)) {
      continue;
    }
    if ((out_anchor->GetOwnerNode()->GetType() == CONSTANT) || (out_anchor->GetOwnerNode()->GetType() == CONSTANTOP)) {
      GE_CHK_BOOL_RET_STATUS(GraphUtils::RemoveEdge(out_anchor, in_anchor) == GRAPH_SUCCESS, GRAPH_FAILED,
                             "[Remove][Edge] from const op %s failed.", out_anchor->GetOwnerNode()->GetName().c_str());
      if (out_anchor->GetOwnerNode()->GetOutNodes().size() == 0UL) {
        GELOGI("Remove const op %s.", out_anchor->GetOwnerNode()->GetName().c_str());
        const auto iter = find(nodes_.begin(), nodes_.end(), out_anchor->GetOwnerNode());
        if (iter != nodes_.end()) {
          EraseFromNodeList(iter);
        }
      }
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::RemoveNode(const NodePtr &node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node ptr should not be null, graph:%s.", name_.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] The node ptr should not be null.");
    return GRAPH_FAILED;
  }

  // delete const op for this node
  (void)RemoveConstInput(node);

  // if the node save as input node, delete it
  (void)RemoveInputNode(node);

  // if the node save as input node, delete it
  (void)RemoveOutputNode(node);

  if (IsolateNode(node) != GRAPH_SUCCESS) {
    GELOGE(GRAPH_FAILED, "[Isolate][Node] failed, node name: %s, graph:%s.", node->GetName().c_str(),
           name_.c_str());
    return GRAPH_FAILED;
  }

  const auto iter = find(nodes_.begin(), nodes_.end(), node);
  if (iter != nodes_.end()) {
    EraseFromNodeList(iter);
    return GRAPH_SUCCESS;
  }
  return GRAPH_FAILED;
}

// Used in sub_graph scenes
graphStatus ComputeGraphImpl::RemoveInputNode(const NodePtr &node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node ptr should not be null, graph:%s.", name_.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] The node ptr should not be null.");
    return GRAPH_FAILED;
  }

  const auto iter = find(input_nodes_.begin(), input_nodes_.end(), node);
  if (iter != input_nodes_.end()) {
    (void)input_nodes_.erase(iter);
    return GRAPH_SUCCESS;
  }
  return GRAPH_FAILED;
}

// Used in sub_graph scenes
graphStatus ComputeGraphImpl::RemoveOutputNode(const NodePtr &node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E18888", "The node ptr should not be null, graph:%s.", name_.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] The node ptr should not be null.");
    return GRAPH_FAILED;
  }

  auto iter = output_nodes_info_.begin();
  bool find_node = false;
  // [output_nodes_info_ : should not be null]
  while (iter != output_nodes_info_.end()) {
    if (node->GetName() == iter->first->GetName()) {
      iter = output_nodes_info_.erase(iter);
      find_node = true;
    } else {
      ++iter;
    }
  }
  GE_IF_BOOL_EXEC(!find_node, return GRAPH_FAILED);
  return GRAPH_SUCCESS;
}

std::shared_ptr<ComputeGraph> ComputeGraphImpl::AddSubGraph(const std::shared_ptr<ComputeGraph> &sub_graph) {
  if (sub_graph == nullptr) {
    REPORT_INNER_ERROR("E18888", "The graph ptr should not be null, graph:%s.", name_.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] The graph ptr should not be null.");
    return nullptr;
  }
  sub_graph_.push_back(sub_graph);
  names_to_subgraph_[sub_graph->GetName()] = sub_graph;
  return sub_graph;
}

graphStatus ComputeGraphImpl::RemoveSubGraph(const std::shared_ptr<ComputeGraph> &sub_graph) {
  if (sub_graph == nullptr) {
    REPORT_INNER_ERROR("E18888", "The graph ptr should not be null, graph:%s.", name_.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] The graph ptr should not be null.");
    return GRAPH_FAILED;
  }

  (void)names_to_subgraph_.erase(sub_graph->GetName());
  const auto iter = find(sub_graph_.begin(), sub_graph_.end(), sub_graph);
  if (iter != sub_graph_.end()) {
    (void)sub_graph_.erase(iter);
  } else {
    GELOGW("[Remove][Subgraph] find sub_graph failed");
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::AddSubgraph(const std::string &name,
                                          const std::shared_ptr<ComputeGraph> &subgraph) {
  if (subgraph == nullptr) {
    REPORT_INNER_ERROR("E18888", "Try to add a null subgraph, name %s", name.c_str());
    GE_LOGE("[Check][Param] Try to add a null subgraph, name %s", name.c_str());
    return GRAPH_PARAM_INVALID;
  }
  const auto parent_graph = subgraph->GetParentGraph();
  if (parent_graph == nullptr) {
    REPORT_CALL_ERROR("E18888", "Try to add subgraph without parent graph, name %s", name.c_str());
    GE_LOGE("[Get][Graph] Try to add subgraph without parent graph, name %s", name.c_str());
    return GRAPH_PARAM_INVALID;
  }
  const auto parent_node = subgraph->GetParentNode();
  if (parent_node == nullptr) {
    REPORT_CALL_ERROR("E18888", "Try to add a subgraph without parent node, name %s", name.c_str());
    GE_LOGE("[Get][Node] Try to add a subgraph without parent node, name %s", name.c_str());
    return GRAPH_PARAM_INVALID;
  }
  if (parent_node->GetOwnerComputeGraph() != parent_graph) {
    REPORT_INNER_ERROR("E18888", "Try to add a subgraph which parent node's graph is not equal to "
                       "the subgraph's parent graph, subgraph name %s, parent node name %s",
                       subgraph->GetName().c_str(), parent_graph->GetName().c_str());
    GE_LOGE("[Check][Param] Try to add a subgraph which parent node's graph is not equal to "
            "the subgraph's parent graph, subgraph name %s, parent node name %s",
            subgraph->GetName().c_str(), parent_graph->GetName().c_str());
    return GRAPH_PARAM_INVALID;
  }
  if (!this->parent_graph_.expired()) {
    GELOGW("[Add][Subgraph] The subgraphs should only be added to the root graph");
  }
  if (name != subgraph->GetName()) {
    GELOGW("[Add][Subgraph] The subgraph name %s is different with input %s", subgraph->GetName().c_str(),
           name.c_str());
  }
  if (names_to_subgraph_.find(name) != names_to_subgraph_.end()) {
    REPORT_INNER_ERROR("E18888", "The subgraph %s existed", name.c_str());
    GE_LOGE("[Check][Param] The subgraph %s existed", name.c_str());
    return GRAPH_PARAM_INVALID;
  }
  sub_graph_.push_back(subgraph);
  names_to_subgraph_[name] = subgraph;
  return GRAPH_SUCCESS;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraphImpl::RemoveSubgraph(const std::string &name) {
  const std::map<std::string, ge::ComputeGraphPtr>::const_iterator iter = names_to_subgraph_.find(name);
  if (iter == names_to_subgraph_.cend()) {
    return;
  }
  auto vec_iter = sub_graph_.begin();
  while (vec_iter != sub_graph_.end()) {
    if ((*vec_iter) == iter->second) {
      (void)sub_graph_.erase(vec_iter);
      break;
    }
    ++vec_iter;
  }
  (void)names_to_subgraph_.erase(iter);
}

std::shared_ptr<ComputeGraph> ComputeGraphImpl::GetSubgraph(const std::string &name) const {
  const std::shared_ptr<ComputeGraph> parent = parent_graph_.lock();
  if (parent == nullptr) {
    const auto iter = names_to_subgraph_.find(name);
    return (iter == names_to_subgraph_.end()) ? nullptr : iter->second;
  } else {
    return parent->GetSubgraph(name);
  }
}

std::vector<std::shared_ptr<ComputeGraph>> ComputeGraphImpl::GetAllSubgraphs() const {
  return sub_graph_;
}

void ComputeGraphImpl::SetAllSubgraphs(const std::vector<std::shared_ptr<ComputeGraph>> &subgraphs) {
  sub_graph_ = subgraphs;
}

shared_ptr<ComputeGraph> ComputeGraphImpl::GetParentGraph() const {
  return parent_graph_.lock();
}

const ComputeGraph *ComputeGraphImpl::GetParentGraphBarePtr() const {
  return parent_graph_bare_ptr_;
}

void ComputeGraphImpl::SetParentGraph(const std::shared_ptr<ComputeGraph> &parent) {
  parent_graph_ = parent;
  parent_graph_bare_ptr_ = parent_graph_.lock().get();
}

shared_ptr<Node> ComputeGraphImpl::GetParentNode() const {
  return parent_node_.lock();
}

const Node *ComputeGraphImpl::GetParentNodeBarePtr() const {
  return parent_node_bare_ptr_;
}

void ComputeGraphImpl::SetParentNode(const std::shared_ptr<Node> &parent) {
  parent_node_ = parent;
  parent_node_bare_ptr_ = parent_node_.lock().get();
}

shared_ptr<Node> ComputeGraphImpl::GetOrUpdateNetOutputNode() {
  auto graph_netoutput = graph_netoutput_.lock();
  if (graph_netoutput == nullptr || graph_netoutput->GetType() != NETOUTPUT) {
    graph_netoutput = FindFirstNodeMatchType(NETOUTPUT);
    SetNetOutputNode(graph_netoutput);
  }
  if (graph_netoutput == nullptr) {
    GELOGW("Graph %s has no netoutput node", GetName().c_str());
  }
  return graph_netoutput;
}

void ComputeGraphImpl::SetNetOutputNode(const std::shared_ptr<Node> &netoutput_node) {
  graph_netoutput_ = netoutput_node;
}

/// @brief Update input-mapping
/// @param [in] input_mapping : index_of_cur_graph_node_input -> index_of_new_graph_node_input
/// @return graphStatus
graphStatus ComputeGraphImpl::UpdateInputMapping(const std::map<uint32_t, uint32_t> &input_mapping) {
  for (auto &input : nodes_) {
    if (input->GetType() == DATA) {
      uint32_t cur_index = 0U;
      if (!ge::AttrUtils::GetInt(input->GetOpDescBarePtr(), ATTR_NAME_PARENT_NODE_INDEX, cur_index)) {
        continue;
      }
      const auto iter = input_mapping.find(cur_index);
      if (iter == input_mapping.end()) {
        continue;
      }
      if (!ge::AttrUtils::SetInt(input->GetOpDescBarePtr(), ATTR_NAME_PARENT_NODE_INDEX,
                                 static_cast<int64_t>(iter->second))) {
        REPORT_CALL_ERROR("E18888", "set attr ATTR_NAME_PARENT_NODE_INDEX failed, op:%s.",
                          input->GetOpDescBarePtr()->GetName().c_str());
        GE_LOGE("[Call][SetInt] UpdateInputMapping failed: set attr ATTR_NAME_PARENT_NODE_INDEX failed, op:%s.",
                input->GetOpDescBarePtr()->GetName().c_str());
        return GRAPH_FAILED;
      }
    }
  }

  return GRAPH_SUCCESS;
}

/// @brief Update output-mapping
/// @param [in] output_mapping : index_of_cur_graph_node_output -> index_of_new_graph_node_output
/// @return graphStatus
graphStatus ComputeGraphImpl::UpdateOutputMapping(const std::map<uint32_t, uint32_t> &output_mapping) const {
  const NodePtr net_output = FindFirstNodeMatchType(NETOUTPUT);
  if (net_output == nullptr) {
    REPORT_INNER_ERROR("E18888", "UpdateOutputMapping failed: node type %s not exist in graph.", NETOUTPUT);
    GE_LOGE("[Get][NodeType] UpdateOutputMapping failed: node type %s not exist in graph.", NETOUTPUT);
    return GRAPH_FAILED;
  }
  const auto op_desc = net_output->GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERROR("E18888", "net output's op desc pr should not be null.");
    GE_LOGE("[Get][OpDesc] UpdateOutputMapping failed: op_desc is NULL.");
    return GRAPH_FAILED;
  }

  const size_t num = op_desc->GetAllInputsSize();
  for (size_t i = 0UL; i < num; i++) {
    GeTensorDesc tensor = op_desc->GetInputDesc(static_cast<uint32_t>(i));
    uint32_t cur_index = 0U;
    if (!ge::AttrUtils::GetInt(tensor, ATTR_NAME_PARENT_NODE_INDEX, cur_index)) {
      continue;
    }
    const auto iter = output_mapping.find(cur_index);
    if (iter == output_mapping.end()) {
      continue;
    }
    if (!ge::AttrUtils::SetInt(tensor, ATTR_NAME_PARENT_NODE_INDEX, static_cast<int64_t>(iter->second))) {
      REPORT_CALL_ERROR("E18888", "op %s set %zu input tensor attr ATTR_NAME_PARENT_NODE_INDEX failed.",
                        op_desc->GetName().c_str(), i);
      GE_LOGE("[Set][Int] op %s set %zu input tensor attr ATTR_NAME_PARENT_NODE_INDEX failed.",
              op_desc->GetName().c_str(), i);
      return GRAPH_FAILED;
    }
    if (op_desc->UpdateInputDesc(static_cast<uint32_t>(i), tensor) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E18888", "op %s update %zu input_tensor failed.", op_desc->GetName().c_str(), i);
      GE_LOGE("[Update][InputDesc] UpdateOutputMapping failed: update %zu input_tensor failed.", i);
      return GRAPH_FAILED;
    }
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::ReorderEventNodes(const ConstComputeGraphPtr &compute_graph) {
  std::list<NodePtr> &node_list = nodes_;
  for (const auto &node : GetDirectNode(compute_graph)) {
    if (node->GetType() == RECV || (node->GetType() == RECV_NOTIFY)) {
      const auto iter = find(node_list.cbegin(), node_list.cend(), node);
      if (iter != node_list.cend()) {
        (void)node_list.erase(iter);
      }

      const auto dst_iter = find(node_list.cbegin(), node_list.cend(), node->GetOutControlNodes().at(0UL));
      (void)node_list.insert(dst_iter, node);
    }
    if (node->GetType() == SEND || (node->GetType() == SEND_NOTIFY)) {
      const auto iter = find(node_list.cbegin(), node_list.cend(), node);
      if (iter != node_list.cend()) {
        (void)node_list.erase(iter);
      }

      auto src_iter = find(node_list.cbegin(), node_list.cend(), node->GetInControlNodes().at(0UL));
      (void)node_list.insert(++src_iter, node);
    }
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::InsertGraphEvents(const ConstComputeGraphPtr &compute_graph) {
  auto status = ReorderEventNodes(compute_graph);
  if (status != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E18888", "Graph [%s] record event nodes failed, status:%d", name_.c_str(), status);
    GELOGE(status, "[Reorder][EventNodes] failed for Graph:%s, status:%d", name_.c_str(), status);
    return status;
  }

  // Partition subgraph
  for (const auto &graph : sub_graph_) {
    status = graph->ReorderEventNodes();
    if (status != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E18888", "ReorderEventNodes failed for SubGraph:%s, status:%d",
                        graph->GetName().c_str(), status);
      GELOGE(status, "[Reorder][EventNodes] failed for SubGraph:%s, status:%d", graph->GetName().c_str(), status);
      return status;
    }
  }

  std::vector<ComputeGraphPtr> subgraphs;
  const auto nodes = AllGraphNodes(subgraphs, compute_graph);
  for (size_t i = 0UL; i < nodes.size(); ++i) {
    const NodePtr node = nodes.at(i);   // [node: should not be null]
    node->GetOpDescBarePtr()->SetId(static_cast<int64_t>(i));  // [node->GetOpDescBarePtr(): should not be null]
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::DFSTopologicalSorting(std::vector<NodePtr> &node_vec, const bool reverse,
                                                    const ConstComputeGraphPtr &compute_graph) const {
  GELOGD("Runing_Dfs_Sort: %s", name_.c_str());
  std::vector<NodePtr> stack;
  std::map<NodePtr, uint32_t> map_in_edge_num;
  // Record the number of non data nodes but no input nodes
  GE_CHK_BOOL_EXEC(SortNodes(stack, map_in_edge_num, compute_graph) == GRAPH_SUCCESS,
                   return GRAPH_FAILED, "sort nodes failed");
  const bool is_mem_priority = IsMemoryPriority();
  std::vector<NodeStatus> reverse_dfs_nodes_info;
  if (is_mem_priority) {
    InitNodeStatus(compute_graph, reverse_dfs_nodes_info);
  }
  TopoSortStack topo_sort_stack(&reverse_dfs_nodes_info, is_mem_priority, true, reverse);
  for (const auto &node : stack) {
    topo_sort_stack.Push(node);
  }
  std::vector<NodePtr> out_nodes;
  const auto stack_push = [&reverse, &topo_sort_stack](std::vector<NodePtr>& tmp_out_nodes) {
      if (reverse) {
        std::reverse(tmp_out_nodes.begin(), tmp_out_nodes.end());
      }
      for (const auto &node: tmp_out_nodes) {
        topo_sort_stack.Push(node);
      }
      tmp_out_nodes.clear();
  };
  // Only data nodes here
  while (!topo_sort_stack.Empty()) {
    const NodePtr node = topo_sort_stack.Pop();
    node_vec.push_back(node);
    GE_CHECK_NOTNULL(node->GetOpDescBarePtr());
    for (const auto &anchor : node->GetAllOutDataAnchors()) {
      GE_CHECK_NOTNULL(anchor);
      for (const auto &peer_in_anchor : anchor->GetPeerInDataAnchors()) {
        GE_CHECK_NOTNULL(peer_in_anchor);
        GetOutNodesFromAnchor(peer_in_anchor, map_in_edge_num, out_nodes);
      }
      stack_push(out_nodes);
      for (const auto &peer_in_anchor : anchor->GetPeerInControlAnchors()) {
        GE_CHECK_NOTNULL(peer_in_anchor);
        GetOutNodesFromAnchor(peer_in_anchor, map_in_edge_num, out_nodes);
      }
      stack_push(out_nodes);
    }
    GE_IF_BOOL_EXEC(node->GetOutControlAnchor() != nullptr,
        for (const AnchorPtr peer_in_anchor : node->GetOutControlAnchor()->GetPeerAnchors()) {
          GE_CHECK_NOTNULL(peer_in_anchor);
          GetOutNodesFromAnchor(peer_in_anchor, map_in_edge_num, out_nodes);
        }
        stack_push(out_nodes);)
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::RDFSTopologicalSorting(std::vector<NodePtr> &node_vec, const bool reverse,
                                                     const ConstComputeGraphPtr &compute_graph) const {
  (void) reverse;
  GELOGD("Runing_Reverse_Dfs_Sort: %s", name_.c_str());
  std::vector<NodeStatus> reverse_dfs_nodes_info;
  InitNodeStatus(compute_graph, reverse_dfs_nodes_info);

  for (const auto &node : compute_graph->GetDirectNode()) {
    if (node->GetOutNodesSize() > 0U) {
      continue;
    }

    std::vector<NodePtr> stack = {node};
    while (!stack.empty()) {
      const auto current = stack.back();
      NodeStatus &reverse_dfs_node_info = reverse_dfs_nodes_info[current->GetOpDesc()->GetId()];
      if (reverse_dfs_node_info.status == WalkStatus::kNotWalked) {
        reverse_dfs_node_info.status = WalkStatus::kWalking;

        const auto in_all_nodes = current->GetInAllNodes();
        NodeCmp cmp(&reverse_dfs_nodes_info);
        std::set<NodePtr, NodeCmp> input_nodes{in_all_nodes.begin(), in_all_nodes.end(), cmp};
        stack.insert(stack.end(), input_nodes.cbegin(), input_nodes.cend());
        continue;
      }
      stack.pop_back();
      if (reverse_dfs_node_info.status == WalkStatus::kWalking) {
        reverse_dfs_node_info.status = WalkStatus::kWalked;
        node_vec.emplace_back(current);
      }
    }
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::BFSTopologicalSorting(std::vector<NodePtr> &node_vec, const bool reverse,
                                                    const ConstComputeGraphPtr &compute_graph) const {
  GELOGD("Runing_Bfs_Sort: %s", name_.c_str());
  (void) reverse;
  const bool is_mem_priority = IsMemoryPriority();
  std::vector<NodeStatus> reverse_dfs_nodes_info;
  if (is_mem_priority) {
    InitNodeStatus(compute_graph, reverse_dfs_nodes_info);
  }
  TopoSortStack topo_sort_stack(&reverse_dfs_nodes_info, is_mem_priority);
  std::vector<NodePtr> stack_input;
  std::map<std::string, NodePtr> breadth_node_map;
  std::map<NodePtr, uint32_t> map_in_edge_num;
  // Record the number of non data nodes but no input nodes
  GE_CHK_BOOL_EXEC(SortNodes(stack_input, map_in_edge_num, compute_graph) == GRAPH_SUCCESS,
                   return GRAPH_FAILED, "sort nodes failed");

  // Only data nodes here
  while ((!stack_input.empty()) || (!topo_sort_stack.Empty())) {
    NodePtr node = nullptr;
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
      (void) topo_sort_stack.Push(name_node.second);
    }
    breadth_node_map.clear();
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::CollectBreadthOutNode(const NodePtr &node, std::map<NodePtr, uint32_t> &map_in_edge_num,
    std::map<std::string, NodePtr> &breadth_node_map) const {
  for (const auto &anchor : node->GetAllOutDataAnchors()) {
    for (const auto &peer_in_anchor : anchor->GetPeerInDataAnchors()) {
      const auto iter = map_in_edge_num.find(peer_in_anchor->GetOwnerNode());
      if (iter != map_in_edge_num.end()) {
        --iter->second;
        if (iter->second == 0U) {
          (void)breadth_node_map.emplace(peer_in_anchor->GetOwnerNode()->GetName(), peer_in_anchor->GetOwnerNode());
        }
      }
    }

    for (const auto &peer_in_anchor : anchor->GetPeerInControlAnchors()) {
      const auto iter = map_in_edge_num.find(peer_in_anchor->GetOwnerNode());
      if (iter != map_in_edge_num.end()) {
        --iter->second;
        if (iter->second == 0U) {
          (void)breadth_node_map.emplace(peer_in_anchor->GetOwnerNode()->GetName(), peer_in_anchor->GetOwnerNode());
        }
      }
    }
  }
  if (node->GetOutControlAnchor() != nullptr) {
    for (const AnchorPtr &peer_in_anchor : node->GetOutControlAnchor()->GetPeerAnchors()) {
      const auto iter = map_in_edge_num.find(peer_in_anchor->GetOwnerNode());
      if (iter != map_in_edge_num.end()) {
        --iter->second;
        if (iter->second == 0U) {
          (void)breadth_node_map.emplace(peer_in_anchor->GetOwnerNode()->GetName(), peer_in_anchor->GetOwnerNode());
        }
      }
    }
  }
  return GRAPH_SUCCESS;
}

void ComputeGraphImpl::TopologicalSorting(const std::function<bool (const NodePtr &, const NodePtr &)> comp) {
  nodes_.sort(std::move(comp));
  int64_t num = 0;
  for (const NodePtr &node : nodes_) {
    node->GetOpDescBarePtr()->SetId(num++);  // node should not be null, node->GetOpDescBarePtr() should not be null]
  }
}

graphStatus ComputeGraphImpl::TopologicalSorting(const ComputeGraphPtr &const_graph_ptr,
                                                 const ConstComputeGraphPtr &const_compute_graph) {
  auto ret = TopologicalSortingGraph(const_compute_graph);
  if (ret != GRAPH_SUCCESS) {
    GE_DUMP(const_graph_ptr, "black_box" + name_);
    REPORT_CALL_ERROR("E18888", "Graph [%s] topological sort failed, saved to file black_box", name_.c_str());
    GELOGW("[Sort][Graph] Graph [%s] topological sort failed, saved to file black_box", name_.c_str());
    return ret;
  }

  if (sub_graph_.empty()) {
    return GRAPH_SUCCESS;
  }

  // partition sub graph
  for (const auto &sub_graph : sub_graph_) {
    ret = sub_graph->TopologicalSortingGraph();
    if (ret != GRAPH_SUCCESS) {
      GE_DUMP(sub_graph, "black_box" + sub_graph->GetName());
      REPORT_CALL_ERROR("E18888", "Sub graph[%s] topological sort failed, saved to file black_box",
                        sub_graph->GetName().c_str());
      GELOGW("[Sort][Graph] Sub graph[%s] topological sort failed, saved to file black_box",
             sub_graph->GetName().c_str());
      return ret;
    }
  }

  std::vector<std::shared_ptr<ComputeGraph>> subgraphs;
  auto nodes = AllGraphNodes(subgraphs, const_compute_graph);
  for (size_t i = 0UL; i < nodes.size(); i++) {
    const NodePtr node = nodes.at(i);   // [node: should not be null]
    node->GetOpDescBarePtr()->SetId(static_cast<int64_t>(i));  // [node->GetOpDescBarePtr(): should not be null]
  }
  if (sub_graph_.size() != subgraphs.size()) {  // Graph Partition use subgraph, Keep original
    GELOGW("[TopoSort][CheckNodeSize] Keep original subgraph for graph size %zu not equal %zu.", sub_graph_.size(),
           subgraphs.size());
    return GRAPH_SUCCESS;
  }
  sub_graph_.swap(subgraphs);
  return GRAPH_SUCCESS;
}

bool InputIsLongLifeTimeNode(const NodePtr& node, const ConstComputeGraphPtr &graph) {
  bool match = false;
  for (const auto &in_data_anchor : node->GetAllInDataAnchors()) {
    if (in_data_anchor == nullptr) {
      continue;
    }
    const auto &peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
    if (peer_out_anchor == nullptr) {
      continue;
    }
    const auto &peer_node = peer_out_anchor->GetOwnerNode();
    if (peer_node == nullptr) {
      continue;
    }
    const bool is_io_data =
        ((graph.get() == peer_node->GetOwnerComputeGraphBarePtr()) && OpTypeUtils::IsDataNode(peer_node->GetType()));
    std::string op_type;
    if ((!NodeUtils::GetConstOpType(peer_node, op_type)) && (!OpTypeUtils::IsVariableNode(peer_node->GetType()))
        && (!is_io_data)) {
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
graphStatus GetOutNodeIndex(std::vector<NodePtr> &nodes, size_t &index, size_t &out_count,
                            const ConstComputeGraphPtr &graph) {
  if (nodes.empty()) {
    return GRAPH_FAILED;
  }

  // first node's inputs muse be long life time
  if ((nodes.size() == 1U) && (!InputIsLongLifeTimeNode(nodes.front(), graph))) {
    return GRAPH_FAILED;
  }

  const auto &node = nodes.back();
  auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  // middle node must be single input
  if ((nodes.size() != 1U) && (node->GetAllInDataAnchorsSize() != 1U)) {
    return GRAPH_FAILED;
  }

  int64_t min_index = 0;
  Node *delay_node = nullptr;
  for (const auto &out_node : node->GetOutAllNodes()) {
    out_count++;
    GE_CHECK_NOTNULL(out_node);
    auto out_node_desc = out_node->GetOpDescBarePtr();
    GE_CHECK_NOTNULL(out_node_desc);
    GELOGD("Node:%s id:%ld peer node:%s id:%ld", node->GetName().c_str(), op_desc->GetId(),
           out_node_desc->GetName().c_str(), out_node_desc->GetId());
    if ((min_index == 0) || (out_node_desc->GetId() < min_index)) {
      min_index = out_node_desc->GetId();
      delay_node = out_node.get();
    }
  }

  if (delay_node != nullptr) {
    index = static_cast<size_t >(min_index);
    if (index > (static_cast<size_t>(op_desc->GetId()) + 1U)) {
      GELOGD("Node:%s id:%ld delay to:%s id:%zu", node->GetName().c_str(), op_desc->GetId(),
             delay_node->GetName().c_str(), index);
    }
    return GRAPH_SUCCESS;
  }
  return GRAPH_FAILED;
}

void DelayTopoSort(std::vector<NodePtr> &nodes, const ConstComputeGraphPtr &graph) {
  // pair.first:  this node can be delay or not
  // pair.second: delayed nodes to this node
  std::vector<std::pair<bool, std::vector<NodePtr>>> delay_nodes;
  delay_nodes.resize(nodes.size());

  // set init index
  for (size_t i = 0U; i < delay_nodes.size(); ++i) {
    nodes[i]->GetOpDescBarePtr()->SetId(static_cast<int64_t>(i));
    delay_nodes[i].first = true;
    delay_nodes[i].second.emplace_back(nodes[i]);
  }

  // move delayed node to fit node
  size_t delay_node_count = 0U;
  for (size_t i = 0U; i < delay_nodes.size(); ++i) {
    size_t delay_to_index = 0U;
    size_t out_count = 0U;
    if (delay_nodes[i].first
        && (GetOutNodeIndex(delay_nodes[i].second, delay_to_index, out_count, graph) == GRAPH_SUCCESS)
        && (delay_to_index < delay_nodes.size()) && (delay_to_index > (i + 1U))) {
      delay_nodes[delay_to_index].second.insert(delay_nodes[delay_to_index].second.begin(),
                                                delay_nodes[i].second.begin(),
                                                delay_nodes[i].second.end());
      if (out_count > 1U) {
        // last node can not be delay
        delay_nodes[delay_to_index].first = false;
      }
      delay_nodes[i].second.clear();
      delay_node_count++;
    }
  }
  if (delay_node_count > 0U) {
    nodes.clear();
    for (size_t i = 0U; i < delay_nodes.size(); ++i) {
      if (!delay_nodes[i].second.empty()) {
        nodes.insert(nodes.end(), delay_nodes[i].second.begin(), delay_nodes[i].second.end());
      }
    }
    GELOGI("Delay %zu nodes.", delay_node_count);
  }
}

graphStatus ComputeGraphImpl::TopologicalSortingGraph(const ConstComputeGraphPtr &compute_graph,
                                                      const bool dfs_reverse) {
  using TopoSortingStrategy =
      std::function<graphStatus(ComputeGraphImpl *, std::vector<NodePtr> &, const bool, const ConstComputeGraphPtr &)>;
  static const std::map<TopoSortingMode, TopoSortingStrategy> topo_sorting_strategy{
      {TopoSortingMode::kBFS, &ComputeGraphImpl::BFSTopologicalSorting},
      {TopoSortingMode::kDFS, &ComputeGraphImpl::DFSTopologicalSorting},
      {TopoSortingMode::kRDFS, &ComputeGraphImpl::RDFSTopologicalSorting}};

  std::vector<NodePtr> node_vec;
  const auto use_topo_strategy = GetTopoSortingStrategy();
  const auto it = topo_sorting_strategy.find(use_topo_strategy);
  if (it == topo_sorting_strategy.end()) {
    GELOGE(GRAPH_FAILED, "Can not find topo sorting strategy of %d.", static_cast<int32_t>(use_topo_strategy));
    return GRAPH_FAILED;
  }
  if (it->second(this, node_vec, dfs_reverse, compute_graph) != GRAPH_SUCCESS) {
    return GRAPH_FAILED;
  }

  // If they are not equal, there is a closed loop
  if (node_vec.size() != GetDirectNodesSize()) {
    std::set<Node *> itered_nodes_set;
    for (auto &node : node_vec) {
      (void)itered_nodes_set.insert(node.get());
    }
    REPORT_INNER_ERROR("E18888", "Failed to do topo sorting total %zu, itered %zu, exist closed loop in graph:%s",
                       GetDirectNodesSize(), node_vec.size(), name_.c_str());
    GELOGW("[Check][Param] Failed to do topo sorting total %zu, itered %zu, exist closed loop in graph.",
           GetDirectNodesSize(), node_vec.size());
    for (auto &node : nodes_) {
      if (itered_nodes_set.count(node.get()) == 0UL) {
        GELOGW("[Check][Param] The node %s does not itered when topological sorting", node->GetName().c_str());
      }
    }
    return GRAPH_FAILED;
  }

  ClearNodeList();
  if (IsMemoryPriority() || (use_topo_strategy == TopoSortingMode::kRDFS)) {
    DelayTopoSort(node_vec, compute_graph);
  }
  for (size_t i = 0UL; i < node_vec.size(); i++) {
    const NodePtr node = node_vec[i];   // [node: should not be null]
    node->GetOpDescBarePtr()->SetId(static_cast<int64_t>(i));  // [node->GetOpDescBarePtr(): should not be null]
    PushBackToNodeList(node);
  }

  is_valid_flag_ = true;
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphImpl::SortNodes(std::vector<NodePtr> &stack,
                                        std::map<NodePtr, uint32_t> &map_in_edge_num,
                                        const ConstComputeGraphPtr &compute_graph) const {
  // Record the number of non data nodes but no input nodes
  uint32_t spec_node_size = 0U;
  for (const auto &node : GetDirectNode(compute_graph)) {
    GE_IF_BOOL_EXEC(node->GetOpDescBarePtr() == nullptr, continue);
    map_in_edge_num[node] = static_cast<uint32_t>(GetInEdgeSize(node));
    if (map_in_edge_num[node] == 0U) {
      if ((!OpTypeUtils::IsDataNode(node->GetOpDescBarePtr()->GetType())) &&
          (node->GetOpDescBarePtr()->GetType() != INPUT_TYPE)) {
        (void)stack.insert(stack.begin(), node);
        spec_node_size++;
        continue;
      }
      // Need to insert the data nodes in reverse order
      (void)stack.insert(stack.begin() + static_cast<int64_t>(spec_node_size), node);
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
      const auto it_j = std::find(inputs_order_.begin(), inputs_order_.end(), stack[j]->GetName());
      GE_IF_BOOL_EXEC(it_j == inputs_order_.end(), continue);

      // Compare index, swap them if it should be
      const auto inx_j = it_j - inputs_order_.begin();
      GE_IF_BOOL_EXEC(inx_i < inx_j, std::swap(stack[i], stack[j]));
    }
  }

  return GRAPH_SUCCESS;
}

size_t ComputeGraphImpl::GetInEdgeSize(const NodePtr &node) const {
  size_t in_edge_size = 0UL;
  if (node == nullptr) {
    return in_edge_size;
  }
  for (const auto &anchor : node->GetAllInDataAnchors()) {
    in_edge_size = in_edge_size + anchor->GetPeerAnchorsSize();
    // Break flow control data loop.
    const OutDataAnchorPtr out_anchor = anchor->GetPeerOutAnchor();
    if ((out_anchor != nullptr) && (out_anchor->GetOwnerNode() != nullptr)) {
      const NodePtr out_node = out_anchor->GetOwnerNode();
      if ((out_node->GetType() == NEXTITERATION) || (out_node->GetType() == REFNEXTITERATION)) {
        GE_IF_BOOL_EXEC(in_edge_size == 0UL,
                        GELOGE(GRAPH_FAILED, "[Check][Param] If [in_edge_size = 0], the result will be reversed");
                        return in_edge_size);
        in_edge_size -= 1UL;
      }
    }
  }
  if (node->GetInControlAnchor() != nullptr) {
    in_edge_size = in_edge_size + node->GetInControlAnchor()->GetPeerAnchorsSize();
  }
  return in_edge_size;
}

size_t ComputeGraphImpl::GetOutEdgeSize(const NodePtr &node) const {
  size_t out_edge_size = 0UL;
  if (node == nullptr) {
    return out_edge_size;
  }

  // Break flow control data loop.
  if ((node->GetType() != NEXTITERATION) && (node->GetType() != REFNEXTITERATION)) {
    for (const auto &anchor : node->GetAllOutDataAnchors()) {
      if (anchor != nullptr) {
        out_edge_size = out_edge_size + anchor->GetPeerAnchors().size();
      }
    }
  }
  if (node->GetOutControlAnchor() != nullptr) {
    if (out_edge_size > (UINT64_MAX - node->GetOutControlAnchor()->GetPeerAnchors().size())) {
      return 0UL;
    }
    out_edge_size = out_edge_size + node->GetOutControlAnchor()->GetPeerAnchors().size();
  }
  return out_edge_size;
}

bool ComputeGraphImpl::IsValid() const { return is_valid_flag_; }

void ComputeGraphImpl::InValid() { is_valid_flag_ = false; }

void ComputeGraphImpl::Dump(const ConstComputeGraphPtr &graph) const {
  if (!IsLogEnable(GE_MODULE_NAME, DLOG_INFO)) {
    return;
  }

  GELOGI("graph name = %s.", GetName().c_str());
  for (const auto &node : GetAllNodes(graph)) {
    GELOGD("node name = %s.", node->GetName().c_str());
    for (const auto &anchor : node->GetAllOutDataAnchors()) {
      for (const auto &peer_in_anchor : anchor->GetPeerInDataAnchors()) {
        GE_IF_BOOL_EXEC((peer_in_anchor != nullptr) && (peer_in_anchor->GetOwnerNode() != nullptr),
                        GELOGI("node name = %s, out data node name = %s.", node->GetName().c_str(),
                               peer_in_anchor->GetOwnerNode()->GetName().c_str()));
      }
      for (const auto &peer_in_anchor : anchor->GetPeerInControlAnchors()) {
        GE_IF_BOOL_EXEC((peer_in_anchor != nullptr) && (peer_in_anchor->GetOwnerNode() != nullptr),
                        GELOGI("node name = %s, out control node name = %s.", node->GetName().c_str(),
                               peer_in_anchor->GetOwnerNode()->GetName().c_str()));
      }
    }
    const auto out_control_anchor = node->GetOutControlAnchor();
    if (out_control_anchor != nullptr) {
      for (const auto &peer_in_anchor : out_control_anchor->GetPeerInControlAnchors()) {
        GE_IF_BOOL_EXEC((peer_in_anchor != nullptr) && (peer_in_anchor->GetOwnerNode() != nullptr),
                        GELOGI("node name = %s, out control node name = %s.", node->GetName().c_str(),
                               peer_in_anchor->GetOwnerNode()->GetName().c_str()));
      }
      for (const auto &peer_in_anchor : out_control_anchor->GetPeerInDataAnchors()) {
        GE_IF_BOOL_EXEC((peer_in_anchor != nullptr) && (peer_in_anchor->GetOwnerNode() != nullptr),
                        GELOGI("node name = %s, out control node name = %s.", node->GetName().c_str(),
                               peer_in_anchor->GetOwnerNode()->GetName().c_str()));
      }
    }
  }
}

void ComputeGraphImpl::Swap(ComputeGraphImpl &graph) {
  origGraph_.swap(graph.origGraph_);

  name_.swap(graph.name_);
  std::swap(graph_id_, graph.graph_id_);
  attrs_.Swap(graph.attrs_);
  nodes_.swap(graph.nodes_);
  const auto tmp_size = direct_nodes_size_;
  direct_nodes_size_ = graph.direct_nodes_size_;
  graph.direct_nodes_size_ = tmp_size;
  all_nodes_infos_.swap(graph.all_nodes_infos_);
  target_nodes_info_.swap(graph.target_nodes_info_);

  input_nodes_.swap(graph.input_nodes_);
  inputs_order_.swap(graph.inputs_order_);
  std::swap(input_size_, graph.input_size_);
  out_nodes_map_.swap(graph.out_nodes_map_);
  std::swap(output_size_, graph.output_size_);
  output_nodes_info_.swap(graph.output_nodes_info_);

  sub_graph_.swap(graph.sub_graph_);
  names_to_subgraph_.swap(graph.names_to_subgraph_);
  parent_graph_.swap(graph.parent_graph_);
  parent_graph_bare_ptr_ = parent_graph_.lock().get();
  parent_node_.swap(graph.parent_node_);
  parent_node_bare_ptr_ = parent_node_.lock().get();
  graph_netoutput_.swap(graph.graph_netoutput_);

  // the members followed should not in the ComputeGraphImpl class
  std::swap(is_valid_flag_, graph.is_valid_flag_);
  std::swap(is_summary_graph_, graph.is_summary_graph_);
  std::swap(need_iteration_, graph.need_iteration_);
  params_share_map_.swap(graph.params_share_map_);
  op_name_map_.swap(graph.op_name_map_);
  std::swap(session_id_, graph.session_id_);
  std::swap(data_format_, graph.data_format_);
}

void ComputeGraphImpl::SetNodesOwner(const ComputeGraphPtr &compute_graph) {
  for (const auto &node : nodes_) {
    if (node == nullptr) {
      continue;
    }
    (void)node->SetOwnerComputeGraph(compute_graph);
  }
}

void ComputeGraphImpl::SetTopParentGraph(const ComputeGraphPtr &compute_graph) {
  for (const auto &sub_graph : sub_graph_) {
    if ((sub_graph == nullptr) || (sub_graph->GetParentGraph() == nullptr) ||
        (sub_graph->GetParentGraph()->GetParentGraph() != nullptr)) {
      continue;
    }
    (void)sub_graph->SetParentGraph(compute_graph);
  }
}

graphStatus ComputeGraphImpl::IsolateNode(const NodePtr &node) const {
  GE_CHECK_NOTNULL(node);
  const auto next_nodes = node->GetOutAllNodes();
  // If there is input data side
  for (size_t i = 0UL; i < node->GetAllInDataAnchors().size(); i++) {
    const auto in_data_anchor = node->GetInDataAnchor(static_cast<int32_t>(i));
    const auto pre_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
    if (pre_out_data_anchor != nullptr) {
      GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(pre_out_data_anchor, in_data_anchor) == GRAPH_SUCCESS,
                       REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                         pre_out_data_anchor->GetOwnerNode()->GetName().c_str(),
                                         in_data_anchor->GetOwnerNode()->GetName().c_str());
                       return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                       pre_out_data_anchor->GetOwnerNode()->GetName().c_str(),
                       in_data_anchor->GetOwnerNode()->GetName().c_str());
      GE_IF_BOOL_EXEC((pre_out_data_anchor->GetOwnerNode()->GetType() == CONSTANT) ||
                      (pre_out_data_anchor->GetOwnerNode()->GetType() == CONSTANTOP),
                      continue);
      for (const auto &out_data_anchor : node->GetAllOutDataAnchors()) {
        for (const auto &next_in_data_anchor : out_data_anchor->GetPeerInDataAnchors()) {
          GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_data_anchor, next_in_data_anchor) == GRAPH_SUCCESS,
                           REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                             out_data_anchor->GetOwnerNode()->GetName().c_str(),
                                             next_in_data_anchor->GetOwnerNode()->GetName().c_str());
                           return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                           out_data_anchor->GetOwnerNode()->GetName().c_str(),
                           next_in_data_anchor->GetOwnerNode()->GetName().c_str());
          GE_CHK_BOOL_EXEC(GraphUtils::AddEdge(pre_out_data_anchor, next_in_data_anchor) == GRAPH_SUCCESS,
                           REPORT_CALL_ERROR("E18888", "add edge from %s to %s failed",
                                             pre_out_data_anchor->GetOwnerNode()->GetName().c_str(),
                                             next_in_data_anchor->GetOwnerNode()->GetName().c_str());
                           return GRAPH_FAILED, "[Add][Edge] from %s to %s failed",
                           pre_out_data_anchor->GetOwnerNode()->GetName().c_str(),
                           next_in_data_anchor->GetOwnerNode()->GetName().c_str());
        }
        for (const auto &next_in_ctrl_anchor : out_data_anchor->GetPeerInControlAnchors()) {
          GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_data_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                           REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                             out_data_anchor->GetOwnerNode()->GetName().c_str(),
                                             next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                           return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                           out_data_anchor->GetOwnerNode()->GetName().c_str(),
                           next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
          GE_CHK_BOOL_EXEC(GraphUtils::AddEdge(pre_out_data_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                           REPORT_CALL_ERROR("E18888", "add edge from %s to %s failed",
                                             pre_out_data_anchor->GetOwnerNode()->GetName().c_str(),
                                             next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                           return GRAPH_FAILED, "[Add][Edge] from %s to %s failed",
                           pre_out_data_anchor->GetOwnerNode()->GetName().c_str(),
                           next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
        }
      }
      const auto out_ctrl_anchor = node->GetOutControlAnchor();
      GE_CHECK_NOTNULL(out_ctrl_anchor);
      const auto pre_out_ctrl_anchor = pre_out_data_anchor->GetOwnerNode()->GetOutControlAnchor();
      GE_CHECK_NOTNULL(pre_out_ctrl_anchor);
      for (const auto &next_in_ctrl_anchor : out_ctrl_anchor->GetPeerInControlAnchors()) {
        GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_ctrl_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                         REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                           out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                                           next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                         return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                         out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                         next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
        GE_CHK_BOOL_EXEC(GraphUtils::AddEdge(pre_out_ctrl_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                         REPORT_CALL_ERROR("E18888", "add edge from %s to %s failed",
                                           pre_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                                           next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                         return GRAPH_FAILED, "[Add][Edge] from %s to %s failed",
                         pre_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                         next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
      }
    }
  }

  // If there is an input control side
  const auto in_ctrl_anchor = node->GetInControlAnchor();
  GE_CHECK_NOTNULL(in_ctrl_anchor);
  for (const auto &pre_out_ctrl_anchor : in_ctrl_anchor->GetPeerOutControlAnchors()) {
    GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(pre_out_ctrl_anchor, in_ctrl_anchor) == GRAPH_SUCCESS,
                     REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                       pre_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                                       in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                     return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                     pre_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                     in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
    for (const auto &out_data_anchor : node->GetAllOutDataAnchors()) {
      for (const auto &next_in_ctrl_anchor : out_data_anchor->GetPeerInControlAnchors()) {
        GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_data_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                         REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                           out_data_anchor->GetOwnerNode()->GetName().c_str(),
                                           next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                         return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                         out_data_anchor->GetOwnerNode()->GetName().c_str(),
                         next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
        GE_CHK_BOOL_EXEC(GraphUtils::AddEdge(pre_out_ctrl_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                         REPORT_CALL_ERROR("E18888", "add edge from %s to %s failed",
                                           pre_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                                           next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                         return GRAPH_FAILED, "[Add][Edge] from %s to %s failed",
                         pre_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                         next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
      }
    }
    const auto out_ctrl_anchor = node->GetOutControlAnchor();
    if (out_ctrl_anchor != nullptr) {
      for (const auto &next_in_ctrl_anchor : out_ctrl_anchor->GetPeerInControlAnchors()) {
        GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_ctrl_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                         REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                           out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                                           next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                         return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                         out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                         next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
        GE_CHK_BOOL_EXEC(GraphUtils::AddEdge(pre_out_ctrl_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                         REPORT_CALL_ERROR("E18888", "add edge from %s to %s failed",
                                           pre_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                                           next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                         return GRAPH_FAILED, "[Add][Edge] from %s to %s failed",
                         pre_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                         next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
      }
    }
  }

  for (const auto &out_peer_data_anchor : in_ctrl_anchor->GetPeerOutDataAnchors()) {
    GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_peer_data_anchor, in_ctrl_anchor) == GRAPH_SUCCESS,
                     REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                       out_peer_data_anchor->GetOwnerNode()->GetName().c_str(),
                                       in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                     return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                     out_peer_data_anchor->GetOwnerNode()->GetName().c_str(),
                     in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
    for (const auto &next_node : next_nodes) {
      const auto next_in_control_anchor = next_node->GetInControlAnchor();
      GE_CHK_BOOL_EXEC(GraphUtils::AddEdge(out_peer_data_anchor, next_in_control_anchor) == GRAPH_SUCCESS,
                       REPORT_CALL_ERROR("E18888", "add edge from %s to %s failed",
                                         out_peer_data_anchor->GetOwnerNode()->GetName().c_str(),
                                         next_in_control_anchor->GetOwnerNode()->GetName().c_str());
                       return GRAPH_FAILED, "[Add][Edge] from %s to %s failed",
                       out_peer_data_anchor->GetOwnerNode()->GetName().c_str(),
                       next_in_control_anchor->GetOwnerNode()->GetName().c_str());
    }
  }

  return RemoveExtraOutEdge(node);
}

graphStatus ComputeGraphImpl::RemoveExtraOutEdge(const NodePtr &node) const {
  GE_CHECK_NOTNULL(node);
  // Remove redundant output edges
  for (const auto &out_data_anchor : node->GetAllOutDataAnchors()) {
    for (const auto &next_in_data_anchor : out_data_anchor->GetPeerInDataAnchors()) {
      GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_data_anchor, next_in_data_anchor) == GRAPH_SUCCESS,
                       REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                         out_data_anchor->GetOwnerNode()->GetName().c_str(),
                                         next_in_data_anchor->GetOwnerNode()->GetName().c_str());
                       return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                       out_data_anchor->GetOwnerNode()->GetName().c_str(),
                       next_in_data_anchor->GetOwnerNode()->GetName().c_str());
    }

    for (const auto &next_in_ctrl_anchor : out_data_anchor->GetPeerInControlAnchors()) {
      GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_data_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                       REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                         out_data_anchor->GetOwnerNode()->GetName().c_str(),
                                         next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                       return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                       out_data_anchor->GetOwnerNode()->GetName().c_str(),
                       next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
    }
  }
  const auto out_ctrl_anchor = node->GetOutControlAnchor();
  if (out_ctrl_anchor != nullptr) {
    for (const auto &next_in_ctrl_anchor : out_ctrl_anchor->GetPeerInControlAnchors()) {
      GE_CHK_BOOL_EXEC(GraphUtils::RemoveEdge(out_ctrl_anchor, next_in_ctrl_anchor) == GRAPH_SUCCESS,
                       REPORT_CALL_ERROR("E18888", "remove edge from %s to %s failed",
                                         out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                                         next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
                       return GRAPH_FAILED, "[Remove][Edge] from %s to %s failed",
                       out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
                       next_in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
    }
  }
  return GRAPH_SUCCESS;
}

ProtoAttrMap &ComputeGraphImpl::MutableAttrMap() {
  return attrs_;
}

ConstProtoAttrMap &ComputeGraphImpl::GetAttrMap() const {
  return attrs_;
}

const std::map<OperatorImplPtr, NodePtr> &ComputeGraphImpl::GetAllNodesInfo() const { return all_nodes_infos_; }

void ComputeGraphImpl::SetUserDefOutput(const std::string &output_name) {
  if (output_name.empty()) {
    return;
  }

  const std::vector<std::string> nodes = StringUtils::Split(output_name, ';');
  for (const std::string &node : nodes) {
    std::vector<std::string> item = StringUtils::Split(node, ':');
    if (item.size() != OUTPUT_PARAM_SIZE) {
      REPORT_INNER_ERROR("W18888", "Check output param size failed, output_name:%s", output_name.c_str());
      GELOGW("[Check][Output] Check output param size failed, output_name:%s", output_name.c_str());
      continue;
    }

    int32_t index;
    try {
      index = stoi(StringUtils::Trim(item[1UL]));
    } catch (const std::out_of_range &) {
      REPORT_INNER_ERROR("W18888", "Catch out_of_range exception, output_name:%s", output_name.c_str());
      GELOGW("[Catch][Exception] Catch out_of_range exception, output_name:%s", output_name.c_str());
      continue;
    } catch (const std::invalid_argument &) {
      REPORT_INNER_ERROR("W18888", "Catch invalid_argument exception, output_name:%s", output_name.c_str());
      GELOGW("[Catch][Exception] Catch invalid_argument exception, output_name:%s", output_name.c_str());
      continue;
    } catch (...) {
      REPORT_INNER_ERROR("W18888", "Catch exception, output_name:%s", output_name.c_str());
      GELOGW("[Catch][Exception] Catch exception, output_name:%s", output_name.c_str());
      continue;
    }
    const auto iter = out_nodes_map_.find(item[0UL]);
    if (iter == out_nodes_map_.end()) {
      out_nodes_map_[item[0UL]] = std::vector<int32_t>(1UL, index);
    } else {
      const auto idx_iter = std::find(iter->second.begin(), iter->second.end(), index);
      if (idx_iter == iter->second.end()) {
        iter->second.push_back(index);
      }
    }
  }
}

const std::string ComputeGraphImpl::GetOutput() {
  static const int32_t resultDefaultSize = 2048;
  std::string result;
  result.reserve(static_cast<uint64_t>(resultDefaultSize));
  auto iter = out_nodes_map_.begin();
  while (iter != out_nodes_map_.end()) {
    const auto idxes = iter->second;
    for (const auto idx : idxes) {
      (void)result.append(iter->first).append(":").append(std::to_string(idx)).append(";");
    }
    ++iter;
  }

  return result.substr(0UL, result.length() - 1UL);
}


void ComputeGraphImpl::EraseFromNodeList(const std::list<NodePtr>::iterator &position) {
  (void) nodes_.erase(position);
  --direct_nodes_size_;
}

void ComputeGraphImpl::InsertToNodeList(const std::list<NodePtr>::iterator &position, const NodePtr &node) {
  (void) nodes_.insert(position, node);
  ++direct_nodes_size_;
}

void ComputeGraphImpl::PushBackToNodeList(const NodePtr &node) {
  (void) nodes_.push_back(node);
  ++direct_nodes_size_;
}

void ComputeGraphImpl::EmplaceBackToNodeList(const NodePtr &node) {
  (void) nodes_.emplace_back(node);
  ++direct_nodes_size_;
}

void ComputeGraphImpl::ClearNodeList() {
  (void) nodes_.clear();
  direct_nodes_size_ = 0UL;
}

void ComputeGraphImpl::ReorderByNodeId() {
  std::vector<NodePtr> node_vec(nodes_.begin(), nodes_.end());
  std::sort(node_vec.begin(), node_vec.end(), [](const NodePtr &lhs, const NodePtr &rhs) {
    return lhs->GetOpDesc()->GetId() < rhs->GetOpDesc()->GetId();
  });
  ClearNodeList();
  for (const auto &node : node_vec) {
    PushBackToNodeList(node);
  }
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraph::ComputeGraph(const std::string &name)
    : enable_shared_from_this(),
      AttrHolder(),
      impl_(ComGraphMakeShared<ComputeGraphImpl>(name)) {}

ComputeGraph::~ComputeGraph() {}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraph::ComputeGraph(const ge::ComputeGraph& compute_graph)
    : enable_shared_from_this(),
      AttrHolder(compute_graph),
      impl_(ComGraphMakeShared<ComputeGraphImpl>(*(compute_graph.impl_))) {}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraph::ComputeGraph(ge::ComputeGraph&& compute_graph)
    : enable_shared_from_this(),
      AttrHolder(std::move(compute_graph)),
      impl_(ComGraphMakeShared<ComputeGraphImpl>(std::move(*(compute_graph.impl_)))) {}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::string ComputeGraph::GetName() const { return impl_->GetName(); }

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetName(const std::string &name) {
  impl_->SetName(name);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY size_t ComputeGraph::GetAllNodesSize() const {
  return GetAllNodes().size();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraph::Vistor<NodePtr> ComputeGraph::GetAllNodes() const {
  std::vector<std::shared_ptr<ComputeGraph>> subgraphs;
  return AllGraphNodes(subgraphs);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraph::Vistor<NodePtr>
ComputeGraph::GetAllNodes(const NodeFilter &node_filter, const GraphFilter &graph_filter) const {
  return impl_->GetAllNodes(node_filter, graph_filter, shared_from_this());
}

ComputeGraph::Vistor<NodePtr> ComputeGraph::AllGraphNodes(std::vector<ComputeGraphPtr> &subgraphs) const {
  return impl_->AllGraphNodes(subgraphs, shared_from_this());
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
ComputeGraph::Vistor<NodePtr> ComputeGraph::GetNodes(const bool is_unknown_shape) const {
  return impl_->GetNodes(is_unknown_shape, shared_from_this());
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraph::Vistor<NodePtr>
ComputeGraph::GetNodes(const bool is_unknown_shape, const NodeFilter &node_filter,
                       const GraphFilter &graph_filter) const {
  return impl_->GetNodes(is_unknown_shape, node_filter, graph_filter, shared_from_this());
}

size_t ComputeGraph::GetDirectNodesSize() const {
  return impl_->GetDirectNodesSize();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraph::Vistor<NodePtr> ComputeGraph::GetDirectNode() const {
  return impl_->GetDirectNode(shared_from_this());
}

ComputeGraph::Vistor<NodePtr> ComputeGraph::GetInputNodes() const {
  return impl_->GetInputNodes(shared_from_this());
}

ComputeGraph::Vistor<NodePtr> ComputeGraph::GetOutputNodes() const {
  return impl_->GetOutputNodes(shared_from_this());
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY NodePtr ComputeGraph::FindNode(const std::string &name) const {
  return impl_->FindNode(name);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
NodePtr ComputeGraph::FindFirstNodeMatchType(const std::string &name) const {
  return impl_->FindFirstNodeMatchType(name);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ComputeGraph::GraphAttrsAreEqual(
    const ComputeGraph &r_graph) const {
  return impl_->GraphAttrsAreEqual(*(r_graph.impl_));
}

/// Since there may be different input nodes
/// chosen by user in the same graph, special judgment is needed
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ComputeGraph::VectorInputNodePtrIsEqual(
    const std::vector<NodePtr> &left_nodes, const std::vector<NodePtr> &right_nodes) const {
  return impl_->VectorInputNodePtrIsEqual(left_nodes, right_nodes);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ComputeGraph::GraphMembersAreEqual(
    const ComputeGraph &r_graph) const {
  return impl_->GraphMembersAreEqual(*(r_graph.impl_));
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ComputeGraph::operator==(
    const ComputeGraph &r_compute_graph) const {
  return (*impl_) == (*(r_compute_graph.impl_));
}

ComputeGraph& ComputeGraph::operator=(ge::ComputeGraph &compute_graph) {
  if (&compute_graph == this) {
    return *this;
  }
  AttrHolder::SwapBase(compute_graph);
  *impl_ = *(compute_graph.impl_);
  return *this;
}

NodePtr ComputeGraph::AddNodeFront(const NodePtr node) {
  return impl_->AddNodeFront(node);
}

NodePtr ComputeGraph::AddNodeFront(const OpDescPtr &op) {
  return impl_->AddNodeFront(op, shared_from_this());
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY NodePtr ComputeGraph::AddNode(const NodePtr node) {
  return impl_->AddNode(node);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY NodePtr ComputeGraph::AddNode(const OpDescPtr op) {
  return impl_->AddNode(op, shared_from_this());
}

NodePtr ComputeGraph::AddNode(const OpDescPtr op, const int64_t id) {  // for unserialize.
  return impl_->AddNode(op, id, shared_from_this());
}

NodePtr ComputeGraph::AddInputNode(const NodePtr node) {
  return impl_->AddInputNode(node);
}

NodePtr ComputeGraph::AddOutputNode(const NodePtr node) {
  return AddOutputNodeByIndex(node, 0);
}

NodePtr ComputeGraph::AddOutputNodeByIndex(const NodePtr node, const int32_t index) {
  return impl_->AddOutputNodeByIndex(node, index);
}

graphStatus ComputeGraph::RemoveConstInput(const NodePtr &node) {
  return impl_->RemoveConstInput(node);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus ComputeGraph::RemoveNode(const NodePtr &node) {
  return impl_->RemoveNode(node);
}

// Used in sub_graph scenes
graphStatus ComputeGraph::RemoveInputNode(const NodePtr &node) {
  return impl_->RemoveInputNode(node);
}

graphStatus ComputeGraph::RemoveOutputNode(const NodePtr &node) {
  return impl_->RemoveOutputNode(node);
}

std::shared_ptr<ComputeGraph> ComputeGraph::AddSubGraph(const std::shared_ptr<ComputeGraph> sub_graph) {
  return impl_->AddSubGraph(sub_graph);
}

graphStatus ComputeGraph::RemoveSubGraph(const std::shared_ptr<ComputeGraph> &sub_graph) {
  return impl_->RemoveSubGraph(sub_graph);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
ComputeGraph::AddSubgraph(const std::string &name, const std::shared_ptr<ComputeGraph> &subgraph) {
  return impl_->AddSubgraph(name, subgraph);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
ComputeGraph::AddSubgraph(const std::shared_ptr<ComputeGraph> &subgraph) {
  if (subgraph == nullptr) {
    return GRAPH_PARAM_INVALID;
  }
  return AddSubgraph(subgraph->GetName(), subgraph);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::RemoveSubgraph(const std::string &name) {
  return impl_->RemoveSubgraph(name);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::RemoveSubgraph(
    const std::shared_ptr<ComputeGraph> &subgraph) {
  if (subgraph != nullptr) {
    RemoveSubgraph(subgraph->GetName());
  }
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::shared_ptr<ComputeGraph> ComputeGraph::GetSubgraph(
    const std::string &name) const {
  return impl_->GetSubgraph(name);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::vector<std::shared_ptr<ComputeGraph>>
ComputeGraph::GetAllSubgraphs() const {
  return impl_->GetAllSubgraphs();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetAllSubgraphs(
    const std::vector<std::shared_ptr<ComputeGraph>> &subgraphs) {
  return impl_->SetAllSubgraphs(subgraphs);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
const std::map<std::vector<std::string>, std::vector<std::string>> &ComputeGraph::GetShareParamLayer() const {
  return impl_->GetShareParamLayer();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetShareParamLayer(
    const std::map<std::vector<std::string>, std::vector<std::string>> params_share_map) {
  impl_->SetShareParamLayer(params_share_map);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetInputsOrder(
    const std::vector<std::string> &inputs_order) {
  impl_->SetInputsOrder(inputs_order);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetGraphOutNodes(
    const std::map<std::string, std::vector<int32_t>> out_nodes_map) {
  impl_->SetGraphOutNodes(out_nodes_map);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::AppendGraphOutNodes(
    const std::map<std::string, std::vector<int32_t>> out_nodes_map) {
  impl_->AppendGraphOutNodes(out_nodes_map);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::shared_ptr<ComputeGraph> ComputeGraph::GetParentGraph() {
  return impl_->GetParentGraph();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY const ComputeGraph *ComputeGraph::GetParentGraphBarePtr() const {
  return impl_->GetParentGraphBarePtr();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetParentGraph(
    const std::shared_ptr<ComputeGraph> &parent) {
  impl_->SetParentGraph(parent);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::shared_ptr<Node> ComputeGraph::GetParentNode() {
  return impl_->GetParentNode();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY const Node *ComputeGraph::GetParentNodeBarePtr() const {
  return impl_->GetParentNodeBarePtr();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetNetOutputNode(
    const std::shared_ptr<Node> &netoutput_node) {
  return impl_->SetNetOutputNode(netoutput_node);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::shared_ptr<Node> ComputeGraph::GetOrUpdateNetOutputNode() {
  return impl_->GetOrUpdateNetOutputNode();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetParentNode(const std::shared_ptr<Node> &parent) {
  return impl_->SetParentNode(parent);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
const std::map<std::string, std::vector<int32_t>> &ComputeGraph::GetGraphOutNodes() const {
  return impl_->GetGraphOutNodes();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetOrigGraph(const ComputeGraphPtr orig_graph) {
  impl_->SetOrigGraph(orig_graph);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraphPtr ComputeGraph::GetOrigGraph(void) {
  return impl_->GetOrigGraph();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetOutputSize(const uint32_t size) {
  impl_->SetOutputSize(size);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY uint32_t ComputeGraph::GetOutputSize() const {
  return impl_->GetOutputSize();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetInputSize(const uint32_t size) {
  impl_->SetInputSize(size);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY uint32_t ComputeGraph::GetInputSize() const {
  return impl_->GetInputSize();
}

// false: known shape  true: unknow shape
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ComputeGraph::GetGraphUnknownFlag() const {
  bool is_unknown = false;
  (void)AttrUtils::GetBool(this, ATTR_NAME_GRAPH_UNKNOWN_FLAG, is_unknown);
  return is_unknown;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetGraphUnknownFlag(const bool flag) {
  (void)AttrUtils::SetBool(this, ATTR_NAME_GRAPH_UNKNOWN_FLAG, flag);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetNeedIteration(const bool need_iteration) {
  impl_->SetNeedIteration(need_iteration);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ComputeGraph::GetNeedIteration() const {
  return impl_->GetNeedIteration();
}

///
/// @brief Update input-mapping
/// @param [in] input_mapping : index_of_cur_graph_node_input -> index_of_new_graph_node_input
/// @return graphStatus
///
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
ComputeGraph::UpdateInputMapping(const std::map<uint32_t, uint32_t> &input_mapping) {
  return impl_->UpdateInputMapping(input_mapping);
}

///
/// @brief Update output-mapping
/// @param [in] output_mapping : index_of_cur_graph_node_output -> index_of_new_graph_node_output
/// @return graphStatus
///
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
ComputeGraph::UpdateOutputMapping(const std::map<uint32_t, uint32_t> &output_mapping) {
  return impl_->UpdateOutputMapping(output_mapping);
}

graphStatus ComputeGraph::ReorderEventNodes() {
  return impl_->ReorderEventNodes(shared_from_this());
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus ComputeGraph::InsertGraphEvents() {
  return impl_->InsertGraphEvents(shared_from_this());
}

graphStatus ComputeGraph::DFSTopologicalSorting(std::vector<NodePtr> &node_vec,
                                                std::map<NodePtr, uint32_t> &map_in_edge_num,
                                                std::vector<NodePtr> &stack, const bool reverse) {
  (void) map_in_edge_num;
  (void) stack;
  return impl_->DFSTopologicalSorting(node_vec, reverse, shared_from_this());
}

graphStatus ComputeGraph::BFSTopologicalSorting(std::vector<NodePtr> &node_vec,
                                                std::map<NodePtr, uint32_t> &map_in_edge_num,
                                                std::deque<NodePtr> &stack) {
  (void) map_in_edge_num;
  (void) stack;
  return impl_->BFSTopologicalSorting(node_vec, false, shared_from_this());
}

graphStatus ComputeGraph::CollectBreadthOutNode(const NodePtr &node, std::map<NodePtr, uint32_t> &map_in_edge_num,
                                                std::map<std::string, NodePtr> &breadth_node_map) {
  return impl_->CollectBreadthOutNode(node, map_in_edge_num, breadth_node_map);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::TopologicalSorting(
    const std::function<bool (const NodePtr &, const NodePtr &)> comp) {
  return impl_->TopologicalSorting(comp);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus ComputeGraph::TopologicalSorting() {
  return impl_->TopologicalSorting(shared_from_this(), shared_from_this());
}

graphStatus ComputeGraph::TopologicalSortingGraph(const bool dfs_reverse) {
  return impl_->TopologicalSortingGraph(shared_from_this(), dfs_reverse);
}

graphStatus ComputeGraph::SortNodes(std::vector<NodePtr> &stack, std::map<NodePtr, uint32_t> &map_in_edge_num) {
  return impl_->SortNodes(stack, map_in_edge_num, shared_from_this());
}

size_t ComputeGraph::GetInEdgeSize(const NodePtr &node) const {
  return impl_->GetInEdgeSize(node);
}

size_t ComputeGraph::GetOutEdgeSize(const NodePtr &node) const {
  return impl_->GetOutEdgeSize(node);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ComputeGraph::IsValid() const {
  return impl_->IsValid();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY  void ComputeGraph::InValid() {
  impl_->InValid();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::Dump() const {
  return impl_->Dump(shared_from_this());
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::Swap(ComputeGraph &graph) {
  this->AttrHolder::SwapBase(graph);
  impl_->Swap(*(graph.impl_));

  // Update Node owner.
  SetNodesOwner();
  graph.SetNodesOwner();

  // Update parent graph of 'TOP subgraph'. 'TOP subgraph' refers to the direct subgraph of the root graph.
  SetTopParentGraph();
  graph.SetTopParentGraph();
}

void ComputeGraph::SetNodesOwner() {
  return impl_->SetNodesOwner(shared_from_this());
}

void ComputeGraph::SetTopParentGraph() {
  return impl_->SetTopParentGraph(shared_from_this());
}

void ComputeGraph::EraseFromNodeList(const std::list<NodePtr>::iterator position) {
  impl_->EraseFromNodeList(position);
}

void ComputeGraph::ClearNodeList() {
  impl_->ClearNodeList();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus ComputeGraph::IsolateNode(const NodePtr &node) {
  return impl_->IsolateNode(node);
}

graphStatus ComputeGraph::RemoveExtraOutEdge(const NodePtr &node) const {
  return impl_->RemoveExtraOutEdge(node);
}

ProtoAttrMap &ComputeGraph::MutableAttrMap() {
  return impl_->MutableAttrMap();
}

ConstProtoAttrMap &ComputeGraph::GetAttrMap() const {
  return impl_->GetAttrMap();
}

const std::map<OperatorImplPtr, NodePtr> &ComputeGraph::GetAllNodesInfo() const {
  return impl_->GetAllNodesInfo();
}

void ComputeGraph::SetUserDefOutput(const std::string &output_name) {
  impl_->SetUserDefOutput(output_name);
}

const std::string ComputeGraph::GetOutput() {
  return impl_->GetOutput();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetGraphOpName(
    const std::map<uint32_t, std::string> &op_name_map) {
  impl_->SetGraphOpName(op_name_map);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
const std::map<uint32_t, std::string> &ComputeGraph::GetGraphOpName() const {
  return impl_->GetGraphOpName();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetAllNodesInfo(
    const std::map<OperatorImplPtr, NodePtr> &nodes) {
  impl_->SetAllNodesInfo(nodes);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetGraphOutNodesInfo(
    std::vector<std::pair<NodePtr, int32_t>> &out_nodes_info) {
  impl_->SetGraphOutNodesInfo(out_nodes_info);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::AppendGraphOutNodesInfo(
    std::vector<std::pair<NodePtr, int32_t>> &out_nodes_info) {
  impl_->AppendGraphOutNodesInfo(out_nodes_info);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
const std::vector<std::pair<NodePtr, int32_t>> &ComputeGraph::GetGraphOutNodesInfo() const {
  return impl_->GetGraphOutNodesInfo();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetGraphTargetNodesInfo(
    const std::vector<NodePtr> &target_nodes_info) {
  impl_->SetGraphTargetNodesInfo(target_nodes_info);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
const std::vector<NodePtr> &ComputeGraph::GetGraphTargetNodesInfo() const {
  return impl_->GetGraphTargetNodesInfo();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetSessionID(const uint64_t session_id) {
  impl_->SetSessionID(session_id);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY uint64_t ComputeGraph::GetSessionID() const {
  return impl_->GetSessionID();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetGraphID(const uint32_t graph_id) {
  impl_->SetGraphID(graph_id);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY uint32_t ComputeGraph::GetGraphID() const {
  return impl_->GetGraphID();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SaveDataFormat(const ge::Format data_format) {
  impl_->SaveDataFormat(data_format);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ge::Format ComputeGraph::GetDataFormat() const {
  return impl_->GetDataFormat();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ComputeGraph::IsSummaryGraph() const {
  return impl_->IsSummaryGraph();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::SetSummaryFlag(const bool is_summary_graph) {
  impl_->SetSummaryFlag(is_summary_graph);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY void ComputeGraph::ReorderByNodeId() {
  impl_->ReorderByNodeId();
}
}  // namespace ge
