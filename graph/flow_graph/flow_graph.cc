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

#include "flow_graph/flow_graph.h"
#include "common/checker.h"
#include "debug/ge_util.h"
#include "graph/flow_graph/data_flow_attr_define.h"
#include "graph/flow_graph/flow_attr_util.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "proto/dflow.pb.h"

namespace ge {
namespace dflow {
using ComputeGraphPtr = std::shared_ptr<ComputeGraph>;

FlowOperator::FlowOperator(const char *name, const char *type) : ge::Operator(name, type) {}
FlowOperator::~FlowOperator() = default;

FlowData::FlowData(const char *name, int64_t index) : FlowOperator(name, "Data") {
  ge::Operator::InputRegister("x", "TensorType::ALL()");
  ge::Operator::OutputRegister("y", "TensorType::ALL()");
  ge::Operator::AttrRegister("index", index);
}
FlowData::~FlowData() = default;

class FlowNodeImpl {
public:
  explicit FlowNodeImpl(FlowNode *flow_node) : flow_node_(flow_node) {}
  ~FlowNodeImpl() = default;
  void MapInput(uint32_t node_input_index, const ProcessPoint &pp, uint32_t pp_input_index,
                const std::vector<DataFlowInputAttr> &attr = {});
  void MapOutput(uint32_t node_output_index, const ProcessPoint &pp, uint32_t pp_output_index);
  void AddPp(const ProcessPoint &pp);
private:
  graphStatus AddInEdges(uint32_t node_input_index, const ProcessPoint &pp, uint32_t pp_input_index);
  graphStatus AddOutEdges(uint32_t node_output_index, const ProcessPoint &pp, uint32_t pp_output_index);
  FlowNode *flow_node_;
  // key1 : pp_name, key2: pp_input_index; value : node_input_index;
  std::map<std::string, std::map<uint32_t, uint32_t>> in_edges_;
  // key1 : pp_name, key2: pp_output_index; value : node_output_index;
  std::map<std::string, std::map<uint32_t, uint32_t>> out_edges_;
  std::map<std::string, bool> added_pps_;
};

graphStatus FlowNodeImpl::AddInEdges(uint32_t node_input_index, const ProcessPoint &pp, uint32_t pp_input_index) {
  std::vector<std::string> pps;
  ge::AscendString flow_node_name;
  (void)flow_node_->GetName(flow_node_name);
  auto op_desc = OpDescUtils::GetOpDescFromOperator(*flow_node_);
  GE_ASSERT_TRUE(ge::AttrUtils::GetListStr(op_desc, ATTR_NAME_DATA_FLOW_PROCESS_POINTS, pps));

  dataflow::ProcessPoint process_point;
  for (std::string &pp_str : pps) {
    GE_ASSERT_TRUE(process_point.ParseFromString(pp_str));
    if (process_point.name() != pp.GetProcessPointName()) {
      GELOGD("current pp(%s) is skipped for it's not equal to MapInput pp name(%s).",
             process_point.name().c_str(), pp.GetProcessPointName());
      continue;
    }
    // duplicate check
    if ((static_cast<int32_t>(pp_input_index) < process_point.in_edges_size()) &&
       (process_point.in_edges(pp_input_index).node_name() != "")) {
      GELOGE(ge::FAILED, "pp name(%s) has duplicate map input index(%d).", pp.GetProcessPointName(), pp_input_index);
      return ge::GRAPH_FAILED;
    }

    process_point.add_in_edges();
    if (static_cast<int32_t>(pp_input_index) < process_point.in_edges_size()) {
      auto in_edge = process_point.mutable_in_edges(pp_input_index);
      in_edge->set_node_name(flow_node_name.GetString());
      in_edge->set_index(node_input_index);
      GELOGI("add pp(%s) input index(%d) map node(%s) index(%d).", pp.GetProcessPointName(), pp_input_index,
             flow_node_name.GetString(), node_input_index);
    } else {
      in_edges_[pp.GetProcessPointName()][pp_input_index] = node_input_index;
    }

    for (auto it = in_edges_[pp.GetProcessPointName()].begin(); it != in_edges_[pp.GetProcessPointName()].end();) {
      if (static_cast<int32_t>(it->first) < process_point.in_edges_size()) {
        auto in_edge = process_point.mutable_in_edges(it->first);
        in_edge->set_node_name(flow_node_name.GetString());
        in_edge->set_index(it->second);
        GELOGI("add pp(%s) input index(%d) map node(%s) index(%d).", pp.GetProcessPointName(), it->first,
               flow_node_name.GetString(), it->second);
        in_edges_[pp.GetProcessPointName()].erase(it++);
      } else {
        it++;
      }
    }
    process_point.SerializeToString(&pp_str);
  }

  GE_ASSERT_TRUE(ge::AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_FLOW_PROCESS_POINTS, pps));
  return ge::GRAPH_SUCCESS;
}

graphStatus FlowNodeImpl::AddOutEdges(uint32_t node_output_index, const ProcessPoint &pp, uint32_t pp_output_index) {
  std::vector<std::string> pps;
  ge::AscendString name;
  (void)flow_node_->GetName(name);
  auto op_desc = OpDescUtils::GetOpDescFromOperator(*flow_node_);
  GE_ASSERT_TRUE(ge::AttrUtils::GetListStr(op_desc, ATTR_NAME_DATA_FLOW_PROCESS_POINTS, pps));

  dataflow::ProcessPoint process_point;
  for (std::string &pp_str : pps) {
    GE_ASSERT_TRUE(process_point.ParseFromString(pp_str));
    if (process_point.name() != pp.GetProcessPointName()) {
      GELOGD("current pp(%s) is skipped for it's not equal to MapInput pp name(%s)",
             process_point.name().c_str(), pp.GetProcessPointName());
      continue;
    }
    // duplicate check
    if ((static_cast<int32_t>(pp_output_index) < process_point.out_edges_size()) &&
        (process_point.out_edges(pp_output_index).node_name() != "")) {
      GELOGE(ge::FAILED, "pp name(%s) has duplicate map input index(%d).", pp.GetProcessPointName(), pp_output_index);
      return ge::GRAPH_FAILED;
    }

    process_point.add_out_edges();
    if (static_cast<int32_t>(pp_output_index) < process_point.out_edges_size()) {
      auto out_edge = process_point.mutable_out_edges(pp_output_index);
      out_edge->set_node_name(name.GetString());
      out_edge->set_index(node_output_index);
      GELOGI("add pp(%s) output index(%d) map node(%s) index(%d)", pp.GetProcessPointName(), pp_output_index,
             name.GetString(), node_output_index);
    } else {
      out_edges_[pp.GetProcessPointName()][pp_output_index] = node_output_index;
    }
    // proc back in_edges
    for (auto it = out_edges_[pp.GetProcessPointName()].begin(); it != out_edges_[pp.GetProcessPointName()].end();) {
      if (static_cast<int32_t>(it->first) < process_point.out_edges_size()) {
        auto out_edge = process_point.mutable_out_edges(it->first);
        out_edge->set_node_name(name.GetString());
        out_edge->set_index(it->second);
        GELOGI("add pp(%s) output index(%d) map node(%s) index(%d)", pp.GetProcessPointName(), it->first,
               name.GetString(), it->second);
        out_edges_[pp.GetProcessPointName()].erase(it++);
      } else {
        it++;
      }
    }
    process_point.SerializeToString(&pp_str);
  }

  GE_ASSERT_TRUE(ge::AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_FLOW_PROCESS_POINTS, pps));
  return ge::GRAPH_SUCCESS;
}

void FlowNodeImpl::MapInput(uint32_t node_input_index, const ProcessPoint &pp, uint32_t pp_input_index,
                            const std::vector<DataFlowInputAttr> &attrs) {
  ge::AscendString flow_node_name;
  (void)flow_node_->GetName(flow_node_name);
  if (node_input_index >= static_cast<uint32_t>(flow_node_->GetDynamicInputNum(ATTR_NAME_DATA_FLOW_INPUT))) {
    GELOGE(ge::FAILED, "invalid node(%s) input index[%u]. valid range is [0, %d)", flow_node_name.GetString(),
           node_input_index, flow_node_->GetDynamicInputNum(ATTR_NAME_DATA_FLOW_INPUT));
    return;
  }

  GE_RETURN_IF_TRUE(!added_pps_[pp.GetProcessPointName()], "Please exec addPp in node(%s) first.",
                    flow_node_name.GetString());

  auto op_desc = OpDescUtils::GetOpDescFromOperator(*flow_node_);
  auto input_tensor_desc = op_desc->MutableInputDesc(node_input_index);
  GE_RETURN_IF_NULL(input_tensor_desc, "[Check][Param] Node(%s)'s input(%u) tensor desc is nullptr.",
                    flow_node_name.GetString(), node_input_index);
  (void)FlowAttrUtil::SetAttrsToTensorDesc(attrs, input_tensor_desc);
  (void)AddInEdges(node_input_index, pp, pp_input_index);
  return;
}

void FlowNodeImpl::MapOutput(uint32_t node_output_index, const ProcessPoint &pp, uint32_t pp_output_index) {
  ge::AscendString flow_node_name;
  (void)flow_node_->GetName(flow_node_name);
  if (node_output_index >= static_cast<uint32_t>(flow_node_->GetDynamicOutputNum(ATTR_NAME_DATA_FLOW_OUTPUT))) {
    GELOGE(ge::FAILED, "invalid node(%s) output index[%u]. valid range is [0, %d)", flow_node_name.GetString(),
           node_output_index, flow_node_->GetDynamicOutputNum(ATTR_NAME_DATA_FLOW_OUTPUT));
    return;
  }

  GE_RETURN_IF_TRUE(!added_pps_[pp.GetProcessPointName()], "Please exec addPp in node(%s) first.",
                    flow_node_name.GetString());

  (void)AddOutEdges(node_output_index, pp, pp_output_index);
  return;
}

void FlowNodeImpl::AddPp(const ProcessPoint &pp) {
  if (pp.GetProcessPointType() == ProcessPointType::INVALID) {
    GELOGE(ge::FAILED, "process point type[%u] is invalid.", static_cast<uint32_t>(pp.GetProcessPointType()));
    return;
  }

  if (added_pps_[pp.GetProcessPointName()]) {
    GELOGI("Process point(%s) has been added to node.", pp.GetProcessPointName());
    return;
  }

  auto op_desc = OpDescUtils::GetOpDescFromOperator(*flow_node_);
  std::vector<std::string> pp_attrs;
  (void)ge::AttrUtils::GetListStr(op_desc, ATTR_NAME_DATA_FLOW_PROCESS_POINTS, pp_attrs);
  ge::AscendString target_str;
  pp.Serialize(target_str);
  pp_attrs.emplace_back(target_str.GetString(), target_str.GetLength());
  GE_RETURN_IF_TRUE(!ge::AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_FLOW_PROCESS_POINTS, pp_attrs),
                    "Set attr name %s failed.", ATTR_NAME_DATA_FLOW_PROCESS_POINTS);
  added_pps_[pp.GetProcessPointName()] = true;
  return;
}

FlowNode::FlowNode(const char *name, uint32_t input_num, uint32_t output_num) : FlowOperator(name, "FlowNode") {
  impl_ = std::make_shared<FlowNodeImpl>(this);
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "FlowNode make shared failed.");
  } else {
    ge::Operator::DynamicInputRegister(ATTR_NAME_DATA_FLOW_INPUT, input_num);
    ge::Operator::DynamicOutputRegister(ATTR_NAME_DATA_FLOW_OUTPUT, output_num);
  }
}

FlowNode::~FlowNode() = default;

FlowNode &FlowNode::SetInput(uint32_t dst_index, const FlowOperator &src_op, uint32_t src_index) {
  ge::Operator::SetInput(dst_index, src_op, src_index);
  return *this;
}

FlowNode &FlowNode::MapInput(uint32_t node_input_index, const ProcessPoint &pp, uint32_t pp_input_index,
                             const std::vector<DataFlowInputAttr> &attrs) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] MapInput:FlowNodeImpl is nullptr, check failed.");
    return *this;
  }

  impl_->MapInput(node_input_index, pp, pp_input_index, attrs);
  return *this;
}

FlowNode &FlowNode::MapOutput(uint32_t node_output_index, const ProcessPoint &pp, uint32_t pp_output_index) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] MapOutput:FlowNodeImpl is nullptr, check failed.");
    return *this;
  }

  impl_->MapOutput(node_output_index, pp, pp_output_index);
  return *this;
}

FlowNode &FlowNode::AddPp(const ProcessPoint &pp) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FlowNodeImpl is nullptr, check failed.");
    return *this;
  }

  if (pp.GetProcessPointType() == ProcessPointType::FUNCTION) {
    const FunctionPp *function_pp = dynamic_cast<const FunctionPp *>(&pp);
    if (function_pp == nullptr) {
      GELOGE(ge::FAILED, "ProcessPoint(%s) cast failed.", pp.GetProcessPointName());
      return *this;
    }

    auto invoked_closures = function_pp->GetInvokedClosures();
    if (invoked_closures.empty()) {
      impl_->AddPp(pp);
      return *this;
    }
    this->SubgraphRegister(pp.GetProcessPointName(), true);
    this->SubgraphCountRegister(pp.GetProcessPointName(), invoked_closures.size());
    uint32_t i = 0;
    for (auto iter = invoked_closures.cbegin(); iter != invoked_closures.cend(); ++iter) {
      GraphBuilder builder = iter->second.GetGraphBuilder();
      if (builder == nullptr) {
        GELOGE(ge::FAILED, "GraphPp(%s)'s graph builder is nullptr.", iter->second.GetProcessPointName());
        return *this;
      }
      this->SetSubgraphBuilder(pp.GetProcessPointName(), i++, builder);
    }
  } else if (pp.GetProcessPointType() == ProcessPointType::GRAPH) {
    const GraphPp *graph_pp = dynamic_cast<const GraphPp *>(&pp);
    if (graph_pp == nullptr) {
      GELOGE(ge::FAILED, "ProcessPoint(%s) cast failed.", pp.GetProcessPointName());
      return *this;
    }

    this->SubgraphRegister(pp.GetProcessPointName(), false);
    this->SubgraphCountRegister(pp.GetProcessPointName(), 1);
    GraphBuilder builder = graph_pp->GetGraphBuilder();
    if (builder == nullptr) {
      GELOGE(ge::FAILED, "GraphPp(%s)'s graph builder is nullptr.", graph_pp->GetProcessPointName());
      return *this;
    }
    this->SetSubgraphBuilder(pp.GetProcessPointName(), 0, builder);
  } else {
    GELOGE(ge::FAILED, "process point type[%u] is invalid.", static_cast<uint32_t>(pp.GetProcessPointType()));
    return *this;
  }

  impl_->AddPp(pp);
  return *this;
}

class FlowGraphImpl {
public:
  explicit FlowGraphImpl(const char *name) : name_(name), graph_(Graph(name)) {}
  ~FlowGraphImpl() = default;

  const ge::Graph &ToGeGraph() const {
    return graph_;
  }

  void SetInputs(const std::vector<FlowOperator> &inputs) {
    std::vector<ge::Operator> op_inputs;
    for (auto iter = inputs.cbegin(); iter != inputs.cend(); ++iter) {
      op_inputs.emplace_back(*iter);
    }

    (void)graph_.SetInputs(op_inputs);
    const auto compute_graph = ge::GraphUtils::GetComputeGraph(graph_);
    AttrUtils::SetBool(compute_graph, ATTR_NAME_IS_DATA_FLOW_GRAPH, true);
    return;
  }

  void SetOutputs(const std::vector<FlowOperator> &outputs) {
    std::vector<ge::Operator> op_outputs;
    for (auto iter = outputs.cbegin(); iter != outputs.cend(); ++iter) {
      op_outputs.emplace_back(*iter);
    }

    (void)graph_.SetOutputs(op_outputs);
    return;
  }

  const char *GetName() const {
    return name_.c_str();
  }
private:
  const std::string name_;
  ge::Graph graph_;
};

FlowGraph::FlowGraph(const char *name) {
  if (name != nullptr) {
    impl_ = ComGraphMakeShared<FlowGraphImpl>(name);
    if (impl_ == nullptr) {
      GELOGE(ge::FAILED, "FlowGraphImpl make shared failed.");
    }
  } else {
    impl_ = nullptr;
    GELOGE(ge::FAILED, "Input graph name is nullptr.");
  }
}
FlowGraph::~FlowGraph() = default;

const ge::Graph &FlowGraph::ToGeGraph() const {
  if (impl_ == nullptr) {
    static ge::Graph graph;
    GELOGE(GRAPH_FAILED, "ToGeGraph failed: graph can not be used, impl is nullptr.");
    return graph;
  }

  return impl_->ToGeGraph();
}

FlowGraph &FlowGraph::SetInputs(const std::vector<FlowOperator> &inputs) {
  if (impl_ == nullptr) {
    GELOGE(GRAPH_FAILED, "SetInputs failed: graph can not be used, impl is nullptr.");
    return *this;
  }

  if (inputs.empty()) {
    GELOGE(GRAPH_FAILED, "SetInputs failed: input operator size can not be 0.");
    return *this;
  }

  impl_->SetInputs(inputs);
  return *this;
}

FlowGraph &FlowGraph::SetOutputs(const std::vector<FlowOperator> &outputs) {
  if (impl_ == nullptr) {
    GELOGE(GRAPH_FAILED, "SetOutputs failed: graph can not be used, impl is nullptr.");
    return *this;
  }

  if (outputs.empty()) {
    GELOGE(GRAPH_FAILED, "SetOutputs failed: outputs operator size can not be 0.");
    return *this;
  }

  impl_->SetOutputs(outputs);
  return *this;
}

const char *FlowGraph::GetName() const {
  if (impl_ == nullptr) {
    GELOGE(GRAPH_FAILED, "GetName failed: graph can not be used, impl is nullptr.");
    return nullptr;
  }

  return impl_->GetName();
}
}  // namespace dflow
}  // namespace ge