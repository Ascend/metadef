/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "graph/utils/op_desc_utils.h"

#include <queue>

#include "common/util/mem_utils.h"
#include "common/checker.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"
#include "graph/debug/ge_util.h"
#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/ge_context.h"
#include "graph/op_desc_impl.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/constant_utils.h"
#include "graph/operator_impl.h"
#include "graph/detail/model_serialize_imp.h"
#include "mmpa/mmpa_api.h"

/*lint -e512 -e737 -e752*/
namespace ge {
const char_t OP_DESC_QUANT_PARAMS[] = "quantize_factor";

namespace {
const uint32_t CONST_OP_NORMAL_WEIGHT_SIZE = 1U;
const char* const kMultiThreadCompile = "MULTI_THREAD_COMPILE";
const char* const kDisEnableFlag = "0";
void GetConstantOpName(std::string &op_name) {
  thread_local int64_t const_count = 0;
  std::string compile_thread;
  if ((ge::GetContext().GetOption(kMultiThreadCompile, compile_thread) == GRAPH_SUCCESS)
      && (compile_thread.compare(kDisEnableFlag) == 0)) {
    op_name = "dynamic_const_" + std::to_string(const_count);
  } else {
    op_name = "dynamic_const_" + std::to_string(GeLog::GetTid()) + "_" + std::to_string(const_count);
  }
  ++const_count;
}

bool FindSubsequentMatches(const std::map<uint32_t, std::string> &valid_index_2_names, size_t start_index,
                           const std::string &ir_name) {
  for (size_t i = start_index; i < valid_index_2_names.size(); ++i) {
    const auto name = valid_index_2_names.at(i);
    if (name == ir_name) {
      GELOGI("ir_name:%s, node input index:%zu", ir_name.c_str(), i);
      return true;
    }
  }
  return false;
}

std::string InputsNamesStr(const OpDescPtr &op_desc) {
  std::stringstream ss;
  ss << "node: " << op_desc->GetName() << "(" << op_desc->GetType() << ") ir inputs names: [";
  for (const auto &ir_input : op_desc->GetIrInputs()) {
    ss << ir_input.first << ", ";
  }
  ss << "], actual inputs names: [";
  for (size_t i = 0U; i < op_desc->GetAllInputsSize(); i++) {
    if (op_desc->MutableInputDesc(static_cast<uint32_t>(i)) != nullptr) {
      const auto valid_name = op_desc->GetInputNameByIndex(static_cast<uint32_t>(i));
      ss << valid_name << ", ";
    }
  }
  ss << "]";
  return ss.str();
}
}

bool OpDescUtils::ClearInputDesc(const NodePtr &node) {
  GE_CHK_BOOL_EXEC(node != nullptr, REPORT_INNER_ERROR("E18888", "param node is nullptr, check invalid.");
                   return false, "[Check][Param] node is nullptr");
  GE_CHK_BOOL_EXEC(node->GetOpDesc() != nullptr, REPORT_INNER_ERROR("E18888", "opdesc is nullptr.");
                   return false, "[Check][Param] opdesc is nullptr");
  std::vector<int32_t> index_list;
  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    if (in_anchor->GetPeerOutAnchor() == nullptr) {
      index_list.push_back(in_anchor->GetIdx());
    }
  }
  std::sort(index_list.begin(), index_list.end());
  // Node's in anchor index need shrink
  if (node->GetOpDesc()->impl_ == nullptr) {
    GELOGE(FAILED, "[Clear][InputDesc] Op desc impl is nullptr. ");
    return false;
  }
  for (size_t i = 0UL; i < index_list.size(); ++i) {
    const auto iter = node->GetOpDesc()->impl_->inputs_desc_.begin() + static_cast<int64_t>(index_list[i]);
    if (iter < node->GetOpDesc()->impl_->inputs_desc_.end()) {
      (void)node->GetOpDesc()->impl_->inputs_desc_.erase(iter);
    } else {
      GELOGW("[Clear][InputDesc] inputs_desc_ iterator out of range.");
    }
  }

  return true;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool OpDescUtils::ClearInputDesc(const OpDescPtr op_desc,
                                                                                const uint32_t index) {
  return NodeUtils::ClearInputDesc(op_desc, index);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool OpDescUtils::HasQuantizeFactorParams(const OpDescPtr &op_desc) {
  if (op_desc == nullptr) {
    GELOGI("op_desc is nullptr");
    return false;
  }
  return op_desc->HasAttr(OP_DESC_QUANT_PARAMS);
}

bool OpDescUtils::ClearOutputDesc(const NodePtr &node) {
  GE_CHK_BOOL_EXEC(node != nullptr, REPORT_INNER_ERROR("E18888", "node is nullptr, check invalid.");
                   return false, "[Check][Param] node is nullptr");
  GE_CHK_BOOL_EXEC(node->GetOpDesc() != nullptr, REPORT_INNER_ERROR("E18888", "opdesc is nullptr.");
                   return false, "[Check][Param] opdesc is nullptr");
  std::vector<int32_t> index_list;
  for (const auto &out_anchor : node->GetAllOutDataAnchors()) {
    if (out_anchor->GetPeerInDataAnchors().empty()) {
      index_list.push_back(out_anchor->GetIdx());
    }
  }
  std::sort(index_list.begin(), index_list.end());
  // Node's out anchor index need shrink
  if (node->GetOpDesc()->impl_ == nullptr) {
    GELOGE(FAILED, "[Clear][OutputDesc] Op desc impl is nullptr. ");
    return false;
  }
  for (size_t i = 0UL; i < index_list.size(); ++i) {
    const auto iter = node->GetOpDesc()->impl_->outputs_desc_.begin() + static_cast<int64_t>(index_list[i]);
    if (iter < node->GetOpDesc()->impl_->outputs_desc_.end()) {
      (void)node->GetOpDesc()->impl_->outputs_desc_.erase(iter);
    } else {
      GELOGW("[Clear][OutputDesc] outputs_desc_ iterator out of range.");
    }
  }

  return true;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool OpDescUtils::ClearOutputDesc(const OpDescPtr &op_desc,
                                                                                 const uint32_t index) {
  return NodeUtils::ClearOutputDesc(op_desc, index);
}

bool OpDescUtils::HasQuantizeFactorParams(const OpDesc &op_desc) { return op_desc.HasAttr(OP_DESC_QUANT_PARAMS); }

GeTensorPtr OpDescUtils::MutableWeights(OpDesc &op_desc) {
  GeTensorPtr weight = nullptr;
  (void)AttrUtils::MutableTensor(&op_desc, ATTR_NAME_WEIGHTS, weight);
  return weight;
}

GE_FUNC_HOST_VISIBILITY GeTensorPtr OpDescUtils::MutableWeights(const OpDescPtr op_desc) {
  if (op_desc == nullptr) {
    REPORT_INNER_ERROR("E18888", "op_desc is null, check invalid");
    GELOGE(GRAPH_FAILED, "[Check][Param] op_desc is null");
    return nullptr;
  }
  return MutableWeights(*op_desc);
}

graphStatus OpDescUtils::SetWeights(OpDesc &op_desc, const GeTensorPtr weight) {
  if (weight == nullptr) {
    REPORT_INNER_ERROR("E18888", "weight is null, check invalid");
    GELOGE(GRAPH_FAILED, "[Check][Param] weight is null");
    return GRAPH_FAILED;
  }
  return AttrUtils::SetTensor(&op_desc, ATTR_NAME_WEIGHTS, weight) ? GRAPH_SUCCESS : GRAPH_FAILED;
}

graphStatus OpDescUtils::SetWeights(OpDescPtr op_desc, const GeTensorPtr weight) {
  GE_CHECK_NOTNULL(op_desc);
  GE_CHECK_NOTNULL(weight);
  return SetWeights(*op_desc, weight);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
std::vector<ConstGeTensorPtr> OpDescUtils::GetWeights(const ge::Node &node) {
  auto weights = MutableWeights(node);
  std::vector<ConstGeTensorPtr> ret(weights.size());
  (void)std::copy(weights.begin(), weights.end(), ret.begin());
  return ret;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::vector<ConstGeTensorPtr> OpDescUtils::GetWeights(
    const ge::ConstNodePtr &node) {
  if (node == nullptr) {
    return std::vector<ge::ConstGeTensorPtr>();
  }
  return GetWeights(*node);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::vector<ge::NodePtr> OpDescUtils::GetConstInputNode(
    const ge::Node &node) {
  std::vector<ge::NodePtr> ret;
  const auto in_anchors = node.GetAllInDataAnchors();
  for (const auto &in_anchor : in_anchors) {
    const auto out_anchor = in_anchor->GetPeerOutAnchor();
    if (out_anchor == nullptr) {
      // normally out_anchor could be null, this is ok
      GELOGD("node %s' peer_out_anchor is null", node.GetName().c_str());
      continue;
    }
    auto in_node = out_anchor->GetOwnerNode();
    while (true) {
      if (in_node == nullptr) {
        break;
      }
      if (ConstantUtils::IsConstant(in_node)) {
        ret.push_back(in_node);
        break;
      } else if (in_node->GetType() == DATA) {
        if (NodeUtils::IsWhileVaryingInput(in_node)) {
          break;
        }
        in_node = NodeUtils::GetParentInput(in_node);
      } else if ((in_node->GetType() == ENTER) || (in_node->GetType() == REFENTER)) {
        bool is_constant = false;
        (void)AttrUtils::GetBool(in_node->GetOpDesc(), ENTER_ATTR_CONSTANT_FLAG, is_constant);
        if (!is_constant) {
          break;
        }
        // Enter node has and only has one input
        if (in_node->GetInDataNodes().size() != 1U) {
          GELOGW("[Get][ConstInput] Check number of input_nodes for Enter node %s failed, input_node_num=%zu.",
                 in_node->GetName().c_str(), in_node->GetInDataNodes().size());
          break;
        }
        in_node = in_node->GetInDataNodes().at(0UL);
      } else {
        break;
      }
    }
  }
  return ret;
}

std::vector<NodeToOutAnchor> OpDescUtils::GetConstInputNodeAndAnchor(const ge::Node &node) {
  std::vector<std::pair<NodePtr, OutDataAnchorPtr>> ret;
  const auto in_nodes_and_anchors = node.GetInDataNodesAndAnchors();
  for (const auto &in_node_2_anchor : in_nodes_and_anchors) {
    auto in_node = in_node_2_anchor.first;
    auto in_node_2_out_anchor = in_node_2_anchor;
    while (true) {
      if (in_node == nullptr) {
        break;
      }
      if (ConstantUtils::IsConstant(in_node)) {
        ret.push_back(in_node_2_out_anchor);
        break;
      } else if (in_node->GetType() == DATA) {
        if (NodeUtils::IsWhileVaryingInput(in_node)) {
          break;
        }
        in_node_2_out_anchor = NodeUtils::GetParentInputAndAnchor(in_node);
        in_node = in_node_2_out_anchor.first;
      } else if ((in_node->GetType() == ENTER) || (in_node->GetType() == REFENTER)) {
        bool is_constant = false;
        (void)AttrUtils::GetBool(in_node->GetOpDesc(), ENTER_ATTR_CONSTANT_FLAG, is_constant);
        if (!is_constant) {
          break;
        }
        // Enter node has and only has one input
        if (in_node->GetInDataNodes().size() != 1U) {
          GELOGW("[Get][ConstInput] Check number of input_nodes for Enter node %s failed, input_node_num=%zu.",
                 in_node->GetName().c_str(), in_node->GetInDataNodes().size());
          break;
        }
        auto peer_out_anchor = in_node->GetInDataAnchor(0)->GetPeerOutAnchor();
        in_node = peer_out_anchor->GetOwnerNode();
        in_node_2_out_anchor = std::make_pair(in_node, peer_out_anchor);
      } else {
        break;
      }
    }
  }
  return ret;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::vector<ConstGeTensorPtr> OpDescUtils::GetInputData(
    const std::vector<ge::NodePtr> &input_nodes) {
  std::vector<ConstGeTensorPtr> ret;

  for (const auto &input_node : input_nodes) {
    const auto temp_weight = MutableWeights(input_node->GetOpDesc());
    if (temp_weight == nullptr) {
      REPORT_CALL_ERROR("E18888", "const op's weight is null, name: %s", input_node->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Invoke][MutableWeights] const op's weight is null, name: %s",
             input_node->GetName().c_str());
      return std::vector<ConstGeTensorPtr>();
    }
    ret.push_back(temp_weight);
  }

  return ret;
}

vector<ConstGeTensorPtr> OpDescUtils::GetWeightsFromNodes(
    const std::vector<NodeToOutAnchor> &input_nodes_2_out_anchors) {
  std::vector<ConstGeTensorPtr> ret;
  for (const auto &input_node_2_anchor : input_nodes_2_out_anchors) {
    const auto input_node = input_node_2_anchor.first;
    GeTensorPtr temp_weight ;
    (void)ConstantUtils::MutableWeight(input_node->GetOpDesc(),
                                       static_cast<uint32_t>(input_node_2_anchor.second->GetIdx()),
                                       temp_weight);
    if (temp_weight == nullptr) {
      REPORT_CALL_ERROR("E18888", "const op's weight is null, name: %s", input_node->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Invoke][MutableWeights] const op's weight is null, name: %s",
             input_node->GetName().c_str());
      return std::vector<ConstGeTensorPtr>();
    }
    ret.push_back(temp_weight);
  }

  return ret;
}
size_t OpDescUtils::GetNonConstInputsSize(const ge::Node &node) {
  if (NodeUtils::IsAnchorStatusSet(node)) {
    size_t input_num = 0UL;
    for (const auto &anchor : node.GetAllInDataAnchors()) {
      if (ge::AnchorUtils::GetStatus(anchor) == ANCHOR_DATA) {
        input_num++;
        continue;
      }
    }
    return input_num;  // lint !e712
  } else {
    GE_IF_BOOL_EXEC(
        node.GetInDataNodes().size() < GetConstInputs(node).size(),
        REPORT_INNER_ERROR("E18888", "InDataNodes size:%zu is smaller than ConstInputs size:%zu",
                           node.GetInDataNodes().size(), GetConstInputs(node).size());
        GELOGE(GRAPH_FAILED, "[Check][Param] %zu is smaller than %zu",
               node.GetInDataNodes().size(), GetConstInputs(node).size());
        return 0UL);
    return node.GetInDataNodes().size() - GetConstInputs(node).size();
  }
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY size_t OpDescUtils::GetNonConstInputsSize(const ge::ConstNodePtr node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E18888", "node is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Node is nullptr");
    return 0UL;
  }
  return GetNonConstInputsSize(*node);
}

GeTensorDesc OpDescUtils::GetNonConstInputTensorDesc(const ge::Node &node, const size_t index_non_const) {
  GE_CHK_BOOL_EXEC(node.GetOpDesc() != nullptr, REPORT_CALL_ERROR("E18888", "node.GetOpDesc() is nullptr!");
                   return GeTensorDesc(), "[Check][Param] node.GetOpDesc() is nullptr!");
  size_t i = 0UL;
  if (NodeUtils::IsAnchorStatusSet(node)) {
    for (const auto &anchor : node.GetAllInDataAnchors()) {
      if (ge::AnchorUtils::GetStatus(anchor) == ANCHOR_DATA) {
        if (index_non_const == i) {
          return node.GetOpDesc()->GetInputDesc(static_cast<uint32_t>(anchor->GetIdx()));
        }
        ++i;
      }
    }
  } else {
    for (const auto &anchor : node.GetAllInDataAnchors()) {
      const auto peer_anchor = anchor->GetPeerOutAnchor();
      if (peer_anchor == nullptr) {
        continue;
      }
      const auto owner_node = peer_anchor->GetOwnerNode();
      if (owner_node == nullptr) {
        continue;
      }
      if (owner_node->GetType() == CONSTANT) {
        continue;
      }
      if (index_non_const == i) {
        return node.GetOpDesc()->GetInputDesc(static_cast<uint32_t>(anchor->GetIdx()));
      }
      ++i;
    }
  }
  return GeTensorDesc();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY GeTensorDesc
OpDescUtils::GetNonConstInputTensorDesc(const ge::ConstNodePtr &node, const size_t index_non_const) {
  CHECK_FALSE_EXEC(node != nullptr, return GeTensorDesc());
  return GetNonConstInputTensorDesc(*node, index_non_const);
}

bool OpDescUtils::GetNonConstInputIndex(const ge::Node &node, const size_t index_non_const, size_t &index) {
  bool ret = false;
  size_t i = 0UL;
  if (NodeUtils::IsAnchorStatusSet(node)) {
    for (const auto &anchor : node.GetAllInDataAnchors()) {
      if (ge::AnchorUtils::GetStatus(anchor) == ANCHOR_DATA) {
        if (index_non_const == i) {
          index = static_cast<size_t>(anchor->GetIdx());
          ret = true;
        }
        ++i;
      }
    }
  } else {
    for (const auto &anchor : node.GetAllInDataAnchors()) {
      const auto peer_anchor = anchor->GetPeerOutAnchor();
      if (peer_anchor == nullptr) {
        continue;
      }
      const auto owner_node = peer_anchor->GetOwnerNode();
      if (owner_node == nullptr) {
        continue;
      }
      if (owner_node->GetType() == CONSTANT) {
        continue;
      }
      if (index_non_const == i) {
        index = static_cast<size_t>(anchor->GetIdx());
        ret = true;
      }
      ++i;
    }
  }
  return ret;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool OpDescUtils::GetNonConstInputIndex(const ge::ConstNodePtr &node,
                                                                                       const size_t index_non_const,
                                                                                       size_t &index) {
  CHECK_FALSE_EXEC(node != nullptr, return false);
  return GetNonConstInputIndex(*node, index_non_const, index);
}

bool OpDescUtils::IsNonConstInput(const ge::Node &node, const size_t index) {
  bool ret = false;
  if (index < node.GetAllInDataAnchors().size()) {
    if (NodeUtils::IsAnchorStatusSet(node)) {
      ret = (ge::AnchorUtils::GetStatus(node.GetInDataAnchor(static_cast<int32_t>(index))) ==
             ANCHOR_DATA); // lint !e712
    } else {
      for (const auto &anchor : node.GetAllInDataAnchors()) {
        if (anchor->GetIdx() != static_cast<int32_t>(index)) {
          continue;
        }
        const auto peer_anchor = anchor->GetPeerOutAnchor();
        if (peer_anchor == nullptr) {
          break;
        }
        const auto owner_node = peer_anchor->GetOwnerNode();
        if (owner_node == nullptr) {
          break;
        }
        ret = (owner_node->GetType() != CONSTANT);
      }
    }
  }

  return ret;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool OpDescUtils::IsNonConstInput(const ge::ConstNodePtr &node,
                                                                                 const size_t index) {
  CHECK_FALSE_EXEC(node != nullptr, return false);
  return IsNonConstInput(*node, index);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::vector<ge::NodePtr> OpDescUtils::GetConstInputs(
    const ge::ConstNodePtr &node) {
  if (node == nullptr) {
    return std::vector<ge::NodePtr>();
  }
  return GetConstInputs(*node);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::vector<ge::GeTensorDesc> OpDescUtils::GetNonConstTensorDesc(
    const ge::ConstNodePtr &node) {
  if ((node == nullptr) || (node->GetOpDesc() == nullptr)) {
    return std::vector<ge::GeTensorDesc>();
  }
  std::vector<ge::GeTensorDesc> ret;
  if (NodeUtils::IsAnchorStatusSet(*node)) {
    for (const auto &in_anchor : node->GetAllInDataAnchors()) {
      if (ge::AnchorUtils::GetStatus(in_anchor) == ANCHOR_DATA) {
        ret.push_back(node->GetOpDesc()->GetInputDesc(static_cast<uint32_t>(in_anchor->GetIdx())));
      }
    }
  } else {
    for (const auto &in_anchor : node->GetAllInDataAnchors()) {
      const auto out_anchor = in_anchor->GetPeerOutAnchor();
      if ((out_anchor == nullptr) || (out_anchor->GetOwnerNode()->GetOpDesc() == nullptr)) {
        continue;
      }
      if (out_anchor->GetOwnerNode()->GetOpDesc()->GetType() != CONSTANT) {
        ret.push_back(node->GetOpDesc()->GetInputDesc(static_cast<uint32_t>(in_anchor->GetIdx())));
      }
    }
  }
  return ret;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
std::vector<ge::NodePtr> OpDescUtils::GetConstInputs(const ge::Node &node, const uint32_t depth) {
  std::vector<ge::NodePtr> ret;
  if (depth == 0U) {
    return ret;
  }

  const auto in_anchors = node.GetAllInDataAnchors();
  for (const auto &in_anchor : in_anchors) {
    const auto out_anchor = in_anchor->GetPeerOutAnchor();
    if (out_anchor == nullptr) {
      continue;
    }

    const auto in_node = out_anchor->GetOwnerNode();
    if (in_node->GetType() == CONSTANT) {
      ret.push_back(in_node);
    } else if ((in_node->GetType() == SWITCH) && (node.GetType() == MATMUL)) {
      // const --> switch --> matmul
      auto switch_input = GetConstInputs(*in_node, depth - 1U);
      if (switch_input.size() > 0U) {
        (void)ret.insert(ret.end(), switch_input.begin(), switch_input.end());
      }
    } else if (in_node->GetType() == DATA) {
      const auto parent = NodeUtils::GetParentInput(in_node);
      if ((parent != nullptr) && (parent->GetType() == CONSTANT)) {
        ret.push_back(parent);
      }
    } else {
      // do nothing
    }
  }
  return ret;
}


graphStatus OpDescUtils::SetNoneConstNodeWeights(ge::Node &node, const std::vector<ge::GeTensorPtr> &weights) {
  const auto input_nodes = GetConstInputs(node);
  if (weights.size() < input_nodes.size()) {
    REPORT_INNER_ERROR("E18888", "weights count:%zu can't be less than const input count:%zu, node:%s(%s)",
                       weights.size(), input_nodes.size(), node.GetName().c_str(), node.GetType().c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] weights count:%zu can't be less than const input count:%zu",
           weights.size(), input_nodes.size());
    return GRAPH_PARAM_INVALID;
  }

  ge::NamedAttrs named_attrs;
  (void)ge::AttrUtils::SetListTensor(named_attrs, "key", weights);
  std::vector<ge::GeTensorPtr> copy_weights;
  (void)ge::AttrUtils::MutableListTensor(named_attrs, "key", copy_weights);

  for (size_t i = 0UL; i < input_nodes.size(); ++i) {
    if (input_nodes[i]->GetOpDesc() != nullptr) {
      if (SetWeights(input_nodes[i]->GetOpDesc(), copy_weights[i]) != GRAPH_SUCCESS) {
        REPORT_INNER_ERROR("E18888", "set weights failed, node:%s(%s)",
                           input_nodes[i]->GetName().c_str(), input_nodes[i]->GetType().c_str());
        GELOGE(GRAPH_FAILED, "[Set][Weights] failed, node:%s(%s)",
               input_nodes[i]->GetName().c_str(), input_nodes[i]->GetType().c_str());
        return GRAPH_FAILED;
      }
    }
  }

  // If set more weights than constop, need to add constop
  for (size_t i = input_nodes.size(); i < copy_weights.size(); ++i) {
    // Use org weight before SetWeights Overwrite
    const auto const_opdesc = CreateConstOp(copy_weights[i]);
    GE_CHECK_NOTNULL(const_opdesc);

    const auto owner_graph = node.GetOwnerComputeGraph();
    if (owner_graph == nullptr) {
      REPORT_CALL_ERROR("E18888", "node's graph is empty, node name: %s", node.GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Get][Graph] node's graph is empty, name: %s", node.GetName().c_str());
      return GRAPH_PARAM_INVALID;
    }
    const auto const_node = owner_graph->AddNodeFront(const_opdesc);
    GE_CHK_BOOL_EXEC(node.AddLinkFrom(const_node) == GRAPH_SUCCESS,
                     REPORT_CALL_ERROR("E18888", "node:%s add link failed.", node.GetName().c_str());
                     GELOGE(GRAPH_FAILED, "[Invoke][AddLinkFrom] graph add link failed! node:%s",
                            node.GetName().c_str());
                     return GRAPH_FAILED);
    const std::vector<ge::NodePtr> original_nodes;
    ge::GraphUtils::RecordOriginalNames(original_nodes, const_node);
  }
  return GRAPH_SUCCESS;
}

graphStatus OpDescUtils::SetNoneConstNodeWeights(ge::Node &node, const std::map<int, ge::GeTensorPtr> &weights_map) {
  for (const auto &pair:weights_map) {
    const auto idx = pair.first;
    // idx = in data anchor size is valid, it meant to add a new const node
    if ((idx < 0) || (static_cast<size_t>(idx) > node.GetAllInDataAnchorsSize())) {
      REPORT_CALL_ERROR("E18888", "Invalid map key: %d of node[%s].", idx, node.GetName().c_str());
      GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] Invalid map key: %d of node[%s].", idx, node.GetName().c_str());
      return GRAPH_PARAM_INVALID;
    }
    const auto peer_node = NodeUtils::GetInDataNodeByIndex(node, idx);
    if (peer_node != nullptr) {
      // a. update const input node
      if (peer_node->GetType() != CONSTANT) {
        REPORT_INNER_ERROR("E18888", "op %s [%d]'s input node should be const, but is %s type:%s ",
                           node.GetName().c_str(), pair.first,
                           peer_node->GetName().c_str(), peer_node->GetType().c_str());
        GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] op %s [%d]'s input node should be const, but is %s type:%s ",
               node.GetName().c_str(), pair.first, peer_node->GetName().c_str(), peer_node->GetType().c_str());
      }
      if (SetWeights(peer_node->GetOpDesc(), pair.second) != GRAPH_SUCCESS) {
        REPORT_INNER_ERROR("E18888", "set weights failed, node:%s(%s)",
                           peer_node->GetName().c_str(), peer_node->GetType().c_str());
        GELOGE(GRAPH_FAILED, "[Set][Weights] failed, node:%s(%s)",
               peer_node->GetName().c_str(), peer_node->GetType().c_str());
        return GRAPH_FAILED;
      }
    } else {
      // b. create new const input node
      const auto const_opdesc = CreateConstOp(pair.second);
      GE_CHECK_NOTNULL(const_opdesc);
      const auto owner_graph = node.GetOwnerComputeGraph();
      if (owner_graph == nullptr) {
        REPORT_CALL_ERROR("E18888", "node's graph is empty, node name: %s", node.GetName().c_str());
        GELOGE(GRAPH_PARAM_INVALID, "[Get][Graph] node's graph is empty, name: %s", node.GetName().c_str());
        return GRAPH_PARAM_INVALID;
      }
      const auto const_node = owner_graph->AddNodeFront(const_opdesc);
      if (node.AddLinkFrom(static_cast<uint32_t>(pair.first), const_node) != GRAPH_SUCCESS) {
        REPORT_CALL_ERROR("E18888", "op %s add const to input index[%d] failed", node.GetName().c_str(), pair.first);
        GELOGE(GRAPH_FAILED, "[Invoke][AddLinkFrom] op %s add const to input index[%d] failed",
               node.GetName().c_str(), pair.first);
        return GRAPH_FAILED;
      }
    }
  }
  return GRAPH_SUCCESS;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
std::vector<GeTensorPtr> OpDescUtils::MutableWeights(const ge::Node &node) {
  std::vector<GeTensorPtr> ret;
  auto op_desc = node.GetOpDesc();
  GE_CHK_BOOL_EXEC(op_desc != nullptr, REPORT_INNER_ERROR("E18888", "param node's op_desc is nullptr.");
                   return ret, "[Check][Param] op_desc is nullptr!");
  // Const operator, take the weight directly
  if ((op_desc->GetType() == CONSTANT) || (op_desc->GetType() == CONSTANTOP)) {
    const auto weight = MutableWeights(op_desc);
    if (weight == nullptr) {
      GELOGD("op type %s has no weight, op name:%s", node.GetType().c_str(), node.GetName().c_str());
      return ret;
    }
    ret.push_back(weight);
    return ret;
  }
  // Place holder operator, try to get the weight from parent node
  // when parent node is const operator
  if (node.GetType() == PLACEHOLDER) {
    ConstGeTensorPtr ge_tensor = nullptr;
    if (NodeUtils::TryGetWeightByPlaceHolderNode(std::const_pointer_cast<Node>(node.shared_from_this()), ge_tensor) ==
            GRAPH_SUCCESS &&
        ge_tensor != nullptr) {
      ret.push_back(std::const_pointer_cast<GeTensor>(ge_tensor));
    }
    return ret;
  }

  if (node.GetType() == DATA) {
    ConstGeTensorPtr ge_tensor = nullptr;
    if (NodeUtils::TryGetWeightByDataNode(std::const_pointer_cast<Node>(node.shared_from_this()), ge_tensor) ==
            GRAPH_SUCCESS &&
        ge_tensor != nullptr) {
      ret.push_back(std::const_pointer_cast<GeTensor>(ge_tensor));
    }
    return ret;
  }

  // Other operators, get weights from connected constop
  const auto input_nodes = GetConstInputs(node);
  for (const auto &input_node : input_nodes) {
    const auto temp_weight = MutableWeights(input_node->GetOpDesc());
    if (temp_weight == nullptr) {
      REPORT_INNER_ERROR("E18888", "const op's weight is null, name: %s", input_node->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Invoke][MutableWeights] const op's weight is null, name: %s",
             input_node->GetName().c_str());
      return std::vector<GeTensorPtr>();
    }
    ret.push_back(temp_weight);
  }

  return ret;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
std::vector<GeTensorPtr> OpDescUtils::MutableWeights(const ge::NodePtr node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E18888", "node is nullptr, check invalid");
    GELOGE(GRAPH_FAILED, "[Check][Param] Node is nullptr");
    return std::vector<ge::GeTensorPtr>();
  }
  return MutableWeights(*node);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
OpDescUtils::SetWeights(ge::Node &node, const std::vector<ge::GeTensorPtr> &weights) {
  GE_CHK_BOOL_EXEC(node.GetOpDesc() != nullptr, REPORT_CALL_ERROR("E18888", "opdesc of node is nullptr.");
                   return GRAPH_PARAM_INVALID, "[Check][Param] node.GetOpDesc is nullptr!");
  if (node.GetOpDesc()->GetType() == CONSTANT) {
    if (weights.size() == CONST_OP_NORMAL_WEIGHT_SIZE) {
      return SetWeights(node.GetOpDesc(), weights[0UL]);
    }
    GELOGI("const op weight size %zu should be 1", weights.size());
    return GRAPH_PARAM_INVALID;
  }

  return SetNoneConstNodeWeights(node, weights);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
OpDescUtils::SetWeights(ge::Node &node, const std::map<int, ge::GeTensorPtr> &weights_map) {
  GE_CHECK_NOTNULL(node.GetOpDesc());
  // 1. node is const
  if (node.GetOpDesc()->GetType() == CONSTANT) {
    if (weights_map.size() == CONST_OP_NORMAL_WEIGHT_SIZE) {
      return SetWeights(node.GetOpDesc(), weights_map.begin()->second);
    }
    REPORT_INNER_ERROR("E18888", "const op %s weight size %zu should be 1", node.GetName().c_str(), weights_map.size());
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] const op %s weight size %zu should be 1",
           node.GetName().c_str(), weights_map.size());
    return GRAPH_PARAM_INVALID;
  }
  // 2. node is not const
  auto const ret = SetNoneConstNodeWeights(node, weights_map);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  NodeUtils::UpdateIsInputConst(node);
  return GRAPH_SUCCESS;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY OpDescPtr OpDescUtils::CloneOpDesc(const ConstOpDescPtr &org_op_desc) {
  return GraphUtils::CloneOpDesc(org_op_desc);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY OpDescPtr OpDescUtils::CopyOpDesc(const ConstOpDescPtr &org_op_desc) {
  return GraphUtils::CopyOpDesc(org_op_desc);
}

OpDescPtr OpDescUtils::CreateConstOp(const GeTensorPtr &tensor_ptr) {
  GE_CHK_BOOL_EXEC(tensor_ptr != nullptr, REPORT_INNER_ERROR("E18888", "tensor_ptr is nullptr, check invalid.");
                   return nullptr, "[Check][Param] tensor_ptr is nullptr!");
  const shared_ptr<OpDesc> const_opdesc = ComGraphMakeShared<OpDesc>();
  if (const_opdesc == nullptr) {
    REPORT_CALL_ERROR("E18888", "create OpDesc failed.");
    GELOGE(GRAPH_FAILED, "[Create][OpDesc] failed to make_shared ");
    return nullptr;
  }

  CHECK_FALSE_EXEC(SetWeights(const_opdesc, tensor_ptr) == ge::GRAPH_SUCCESS, return nullptr);

  const_opdesc->SetType(CONSTANT);

  std::string op_name;
  GetConstantOpName(op_name);
  const_opdesc->SetName(op_name);
  GELOGI("add const op: %s", const_opdesc->GetName().c_str());

  (void)const_opdesc->AddOutputDesc("y", tensor_ptr->GetTensorDesc());

  GELOGI("after add const op: %s", const_opdesc->GetName().c_str());

  return const_opdesc;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
OpDescUtils::AddConstOpToAnchor(const InDataAnchorPtr in_anchor, const GeTensorPtr &tensor_ptr) {
  GE_CHECK_NOTNULL(in_anchor);
  GE_CHECK_NOTNULL(tensor_ptr);
  const auto const_opdesc = CreateConstOp(tensor_ptr);
  GE_CHECK_NOTNULL(const_opdesc);
  const auto in_node = in_anchor->GetOwnerNode();
  GE_CHECK_NOTNULL(in_node);
  const auto owner_graph = in_node->GetOwnerComputeGraph();
  if (owner_graph == nullptr) {
    REPORT_CALL_ERROR("E18888", "node's graph is empty, name: %s", in_node->GetName().c_str());
    GELOGE(GRAPH_PARAM_INVALID, "[Get][Graph] node's graph is empty, name: %s", in_node->GetName().c_str());
    return GRAPH_PARAM_INVALID;
  }
  const auto const_node = in_node->GetOwnerComputeGraph()->AddNodeFront(const_opdesc);
  GE_CHECK_NOTNULL(const_node);
  if (GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), in_anchor) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E18888", "AddEdge const %s to node %s failed", const_node->GetName().c_str(),
                      in_node->GetName().c_str());
    GELOGE(GRAPH_PARAM_INVALID, "[Add][Edge] const %s to node %s failed.", const_node->GetName().c_str(),
           in_node->GetName().c_str());
    return GRAPH_PARAM_INVALID;
  }
  return GRAPH_SUCCESS;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
OpDescUtils::SetWeights(ge::NodePtr node, const std::vector<ge::GeTensorPtr> &weights) {
  GE_CHECK_NOTNULL(node);
  return SetWeights(*node, weights);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus OpDescUtils::ClearWeights(const ge::NodePtr node) {
  GE_CHECK_NOTNULL(node);
  const auto const_ops = GetConstInputs(node);
  const auto graph = node->GetOwnerComputeGraph();
  if (graph == nullptr) {
    REPORT_CALL_ERROR("E18888", "GetOwnerComputeGraph failed, graph is nullptr, node:%s", node->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Graph] Graph is nullptr");
    return GRAPH_PARAM_INVALID;
  }
  for (const auto &const_op : const_ops) {
    GE_CHK_STATUS_RET(GraphUtils::IsolateNode(const_op, {}), "[Isolate][Node] %s, type:%s failed",
                      const_op->GetName().c_str(), const_op->GetType().c_str());
    GE_CHK_STATUS_RET(GraphUtils::RemoveNodeWithoutRelink(graph, const_op),
                      "[Remove][Node] %s, type: %s without relink failed", const_op->GetName().c_str(),
                      const_op->GetType().c_str());
  }
  return GRAPH_SUCCESS;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
graphStatus OpDescUtils::SetSubgraphInstanceName(const std::string &subgraph_name,
                                                 const std::string &subgraph_instance_name,
                                                 OpDescPtr &op_desc) {
  const auto &subgraph_names_to_index = op_desc->GetSubgraphNameIndexes();
  const auto iter = subgraph_names_to_index.find(subgraph_name);
  if (iter == subgraph_names_to_index.end()) {
    REPORT_INNER_ERROR("E18888",
                       "Failed to set subgraph instance %s for node %s type %s, the subgraph name %s does not exists",
                       subgraph_instance_name.c_str(), op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                       subgraph_name.c_str());
    GELOGE(GRAPH_PARAM_INVALID,
        "[Check][Param] Failed to set subgraph instance %s for node %s type %s, the subgraph name %s does not exists",
        subgraph_instance_name.c_str(), op_desc->GetName().c_str(), op_desc->GetType().c_str(), subgraph_name.c_str());
    return GRAPH_PARAM_INVALID;
  }

  return op_desc->SetSubgraphInstanceName(iter->second, subgraph_instance_name);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
ConstGeTensorBarePtr OpDescUtils::GetInputConstData(const Operator &op, const uint32_t idx) {
  if (op.operator_impl_ == nullptr) {
    AscendString op_name;
    (void)op.GetName(op_name);
    GELOGW("[Check][Param] Op(%s) operator_impl_ is nullptr.", op_name.GetString());
    return nullptr;
  }

  ConstGeTensorPtr ge_tensor = nullptr;
  if (op.operator_impl_->GetInputConstData(idx, ge_tensor) == GRAPH_SUCCESS) {
    return ge_tensor.get();
  }
  AscendString name;
  (void) op.GetName(name);
  GELOGW("[Get][ConstInput] Op(%s) %u get input const data failed", name.GetString(), idx);
  return nullptr;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
void OpDescUtils::SetRuntimeContextToOperator(const Operator &op, RuntimeInferenceContext *const context) {
  op.operator_impl_->runtime_context_ = context;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
void OpDescUtils::SetCallbackGetConstInputFuncToOperator(const Operator &op,
                                                         GetConstInputOnRuntimeFun get_const_input_func) {
  op.operator_impl_->get_const_input_runtime_ = get_const_input_func;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY
bool OpDescUtils::HasCallbackGetConstInputFunc(const Operator &op) {
  return (op.operator_impl_->get_const_input_runtime_ != nullptr);
}

ge::graphStatus IrInputRequiredCall(const OpDescPtr &op_desc, size_t ir_index, size_t start_index, size_t all_ins_num,
                                    const std::string &ir_name,
                                    const std::map<uint32_t, std::string> &valid_index_2_names, size_t &instance_num) {
  (void)all_ins_num;
  const auto max_index = valid_index_2_names.rbegin()->first;
  if (start_index > max_index) {
    GELOGW("Failed to get instance num for node %s, current name %s current index %zu out of range %u",
           op_desc->GetName().c_str(), ir_name.c_str(), start_index, max_index);
    instance_num = 1U;
    return ge::SUCCESS;
  }
  const auto name = valid_index_2_names.at(start_index);
  if (name != ir_name) {
    GELOGW("Failed to get instance num for node %s, can not find the input for ir name %s, current index %zu, "
           "current name %s",
           op_desc->GetName().c_str(), ir_name.c_str(), start_index, name.c_str());
    if (FindSubsequentMatches(valid_index_2_names, start_index + 1U, ir_name)) {
      GELOGE(ge::FAILED, "Find another input name that match ir name. ir_index:%zu, ir_name:%s, inputs names:%s",
             ir_index, ir_name.c_str(), InputsNamesStr(op_desc).c_str());
      return FAILED;
    }
  }
  instance_num = 1U;
  GELOGD("Get instance num %zu for node %s, current name %s current ir index %zu, start_index %zu", instance_num,
         op_desc->GetName().c_str(), ir_name.c_str(), ir_index, start_index);
  return ge::SUCCESS;
}

ge::graphStatus IrInputOptionalCall(const OpDescPtr &op_desc, size_t ir_index, size_t start_index, size_t all_ins_num,
                                    const std::string &ir_name,
                                    const std::map<uint32_t, std::string> &valid_index_2_names, size_t &instance_num) {
  (void)all_ins_num;
  const auto max_index = valid_index_2_names.rbegin()->first;
  // ooooooxxx
  // o : required input
  // x : option input
  if (start_index > max_index) {
    instance_num = 0U;
    return ge::SUCCESS;
  }
  const auto name = valid_index_2_names.at(start_index);
  if (name == ir_name) {
    instance_num = 1U;
  } else {
    instance_num = 0U;
  }
  GELOGD("Get instance num %zu for node %s, current name %s current ir index %zu, start_index %zu", instance_num,
         op_desc->GetName().c_str(), ir_name.c_str(), ir_index, start_index);
  return ge::SUCCESS;
}

ge::graphStatus IrDynamicCall(const OpDescPtr &op_desc, size_t ir_index, size_t start_index, size_t all_ins_num,
                                   const std::string &ir_name,
                                   const std::map<uint32_t, std::string> &valid_index_2_names, size_t &instance_num) {
  size_t dyn_i = 0;
  const auto max_index = valid_index_2_names.rbegin()->first;
  for (size_t i = start_index; i < all_ins_num; ++i, ++dyn_i) {
    if (i > max_index) {
      break;
    }
    const auto name = valid_index_2_names.at(i);
    if (name != ir_name + std::to_string(dyn_i)) {
      break;
    }
  }
  instance_num = dyn_i;
  GELOGD("Get instance num %zu for node %s, current name %s current ir index %zu, start_index %zu", instance_num,
         op_desc->GetName().c_str(), ir_name.c_str(), ir_index, start_index);
  return ge::SUCCESS;
}

ge::graphStatus GetOutputInstanceNum(const OpDescPtr &op_desc, size_t ir_index, size_t start_index,
                                     const std::map<uint32_t, std::string> &valid_index_2_names, size_t &instance_num) {
  GE_CHECK_NOTNULL(op_desc);
  if (valid_index_2_names.empty()) {
    GELOGD("Node %s has not any outputs, just return", op_desc->GetName().c_str());
    return ge::SUCCESS;
  }
  const auto &ir_outputs = op_desc->GetIrOutputs();
  const auto ir_type = ir_outputs[ir_index].second;
  const auto ir_name = ir_outputs[ir_index].first;
  using GetInstanceCall = std::function<Status(
      const OpDescPtr &op_desc, const size_t ir_index, const size_t start_index, const size_t all_ins_num,
      const std::string &ir_name, const std::map<uint32_t, std::string> &valid_index_2_names, size_t &instance_num)>;
  static std::map<IrOutputType, GetInstanceCall> get_instance_calls = {{kIrOutputRequired, &IrInputRequiredCall},
                                                                       {kIrOutputDynamic, &IrDynamicCall}};
  const auto it = get_instance_calls.find(ir_type);
  if (it != get_instance_calls.end()) {
    const size_t all_ins_num = op_desc->GetAllOutputsDescSize();
    return (it->second)(op_desc, ir_index, start_index, all_ins_num, ir_name, valid_index_2_names, instance_num);
  }
  GELOGE(ge::FAILED, "Failed to get instance num for node %s, unknown ir output type %d, ir name %s",
         op_desc->GetName().c_str(), ir_type, ir_name.c_str());
  return ge::FAILED;
}

ge::graphStatus OpDescUtils::GetInstanceNum(const OpDescPtr &op_desc, size_t ir_index, size_t start_index,
                                            const std::map<uint32_t, std::string> &valid_index_2_names,
                                            size_t &instance_num) {
  GE_CHECK_NOTNULL(op_desc);
  if (valid_index_2_names.empty()) {
    GELOGD("Node %s has not any inputs, just return", op_desc->GetName().c_str());
    return ge::SUCCESS;
  }
  const auto &ir_inputs = op_desc->GetIrInputs();
  const auto ir_type = ir_inputs[ir_index].second;
  const auto ir_name = ir_inputs[ir_index].first;
  using GetInstanceCall = std::function<Status(
      const OpDescPtr &op_desc, const size_t ir_index, const size_t start_index, const size_t all_ins_num,
      const std::string &ir_name, const std::map<uint32_t, std::string> &valid_index_2_names, size_t &instance_num)>;
  static std::map<IrInputType, GetInstanceCall> get_instance_calls = {{kIrInputRequired, &IrInputRequiredCall},
                                                                      {kIrInputOptional, &IrInputOptionalCall},
                                                                      {kIrInputDynamic, &IrDynamicCall}};
  const auto it = get_instance_calls.find(ir_type);
  if (it != get_instance_calls.end()) {
    const size_t all_ins_num = op_desc->GetAllInputsSize();
    return (it->second)(op_desc, ir_index, start_index, all_ins_num, ir_name, valid_index_2_names, instance_num);
  }
  GELOGE(ge::FAILED, "Failed to get instance num for node %s, unknown ir input type %d, ir name %s",
         op_desc->GetName().c_str(), ir_type, ir_name.c_str());
  return ge::FAILED;
}

std::map<size_t, std::pair<size_t, size_t>> OpDescUtils::GetInputIrIndexes2InstanceIndexesPairMap(
    const OpDescPtr &op_desc) {
  std::map<size_t, std::pair<size_t, size_t>> ir_index_to_instance_index_pair_map;

  if (GetIrInputInstanceDescRange(op_desc, ir_index_to_instance_index_pair_map) == GRAPH_SUCCESS) {
    return ir_index_to_instance_index_pair_map;
  }

  return {};
}

std::map<size_t, std::pair<size_t, size_t>> OpDescUtils::GetOutputIrIndexes2InstanceIndexesPairMap(
    const OpDescPtr &op_desc) {
  std::map<size_t, std::pair<size_t, size_t>> ir_index_to_instance_index_pair_map;

  if (GetIrOutputDescRange(op_desc, ir_index_to_instance_index_pair_map) == GRAPH_SUCCESS) {
    return ir_index_to_instance_index_pair_map;
  }

  return {};
}

ge::graphStatus OpDescUtils::GetInputIrIndexByInstanceIndex(const OpDescPtr &op_desc,
                                                            size_t instance_index, size_t &ir_index) {
  GE_CHECK_NOTNULL(op_desc);
  auto ir_index_to_instance_index_pair_map = GetInputIrIndexes2InstanceIndexesPairMap(op_desc);
  if (ir_index_to_instance_index_pair_map.empty()) {
    GELOGE(ge::GRAPH_FAILED, "node [%s(%s)] get ir indexes to instance indexes list failed, instance_index[%zu]",
           op_desc->GetName().c_str(), op_desc->GetType().c_str(), instance_index);
    return ge::GRAPH_FAILED;
  }
  size_t input_index = 0;
  for (size_t i = 0; i < op_desc->GetIrInputs().size(); ++i) {
    const size_t instance_num = ir_index_to_instance_index_pair_map[i].second;
    if (instance_num == 0) {
      continue;
    }
    if (instance_index < input_index + instance_num) {
      ir_index = i;
      return GRAPH_SUCCESS;
    }
    input_index += instance_num;
  }
  ir_index = std::numeric_limits<size_t>::max();
  GELOGW("node [%s(%s)] failed to get ir index by instance index[%zu], input_index[%zi], set ir_index to %zu",
         op_desc->GetName().c_str(), op_desc->GetType().c_str(), instance_index, input_index, ir_index);
  return GRAPH_SUCCESS;
}

namespace {
class DescEnv {
 public:
  DescEnv(const OpDescPtr &op, bool for_input) : op_(op), for_input_(for_input) {}
  ~DescEnv() = default;

  bool IsDescValid(uint32_t index) const {
    return for_input_ ? (op_->MutableInputDesc(index) != nullptr) : (op_->MutableOutputDesc(index) != nullptr);
  }

  size_t NumDescs() const {
    return for_input_ ? op_->GetAllInputsSize() : op_->GetOutputsSize();
  }

  std::string DebugString() const {
    std::string str = "Env for ";
    str += op_->GetName() + " ";
    str += op_->GetType() + " ";
    str += for_input_ ? "input" : "output";
    return str;
  }

 private:
  const OpDescPtr &op_;
  bool for_input_;
};
class IrIOSpec {
 public:
  IrIOSpec(const std::string &name, const IrInputType &type) {
    name_ = name;
    is_input_ = true;
    if (type == kIrInputDynamic) {
      is_dynamic_ = true;
    } else if (type == kIrInputOptional) {
      is_optional_ = true;
    } else if (type == kIrInputRequired) {
      is_required_ = true;
    } else {
      is_valid_ = false;
    }
  }

  IrIOSpec(const std::string &name, const IrOutputType &type) {
    name_ = name;
    if (type == kIrOutputDynamic) {
      is_dynamic_ = true;
    } else if (type == kIrOutputRequired) {
      is_required_ = true;
    } else {
      is_valid_ = false;
    }
  }
  ~IrIOSpec() = default;

  const std::string &GetName() const {
    return name_;
  }
  std::string DebugString() const {
    std::string str = (is_dynamic_ ? "Dynamic " : is_optional_ ? "Optional " : is_required_ ? "Required " : "Invalid ");
    str += is_input_ ? "input " : "output ";
    str += name_;
    return str;
  }
  bool IsValid() const {
    return is_valid_;
  }
  bool IsDynamic() const {
    return is_dynamic_;
  }
  bool IsOptional() const {
    return is_optional_;
  }
  bool IsRequired() const {
    return is_required_;
  }

 private:
  std::string name_;
  bool is_input_ = false;
  bool is_valid_ = true;
  bool is_dynamic_ = false;
  bool is_optional_ = false;
  bool is_required_ = false;
};

// 对于空的Dynamic输入和未传值的Optional输入，计算其起始index以展示更为友好
size_t GetIrDescStartIndex(std::map<size_t, std::pair<size_t, size_t>> &ir_2_range, size_t ir_index) {
  if (ir_index == 0U) {
    return 0U;
  }

  auto iter = ir_2_range.find(ir_index - 1U);
  if (iter == ir_2_range.end()) {
    return 0U;
  }
  return iter->second.first + iter->second.second;
}

graphStatus MappingDynamicIrDesc(const std::vector<IrIOSpec> &ir_specs, const DescEnv &desc_env,
                                 const std::map<std::string, uint32_t> &name2idx,
                                 std::map<size_t, std::pair<size_t, size_t>> &ir_2_range) {
  GELOGD("Start mapping dynamic ir desc for %s", desc_env.DebugString().c_str());
  for (size_t ir_io_idx = 0U; ir_io_idx < ir_specs.size(); ir_io_idx++) {
    const auto &ir_spec = ir_specs[ir_io_idx];
    GE_ASSERT(ir_spec.IsValid(), "Invalid ir spec %s", ir_spec.DebugString().c_str());
    if (!ir_spec.IsDynamic()) {  // 优先处理Dynamic类型的IR输入
      continue;
    }
    std::set<size_t> indexes;  // Dynamic类型的IR输入对应的多个index
    size_t num_instances = 0U;
    for (; num_instances < name2idx.size(); num_instances++) {
      auto iter = name2idx.find(ir_spec.GetName() + std::to_string(num_instances));
      if (iter == name2idx.end()) {
        break;
      }
      indexes.insert(iter->second);
    }
    // 校验Dynamic类型的IR IO对应的多个index连续
    GE_ASSERT((indexes.size() <= 1U) || (*indexes.rbegin() - *indexes.begin() == (indexes.size() - 1U)));
    if (indexes.empty()) {
      GELOGD("Dynamic ir spec %s has no instance", ir_spec.DebugString().c_str());
      ir_2_range.emplace(ir_io_idx, std::make_pair(GetIrDescStartIndex(ir_2_range, ir_io_idx), 0U));
    } else {
      ir_2_range.emplace(ir_io_idx, std::make_pair(*indexes.begin(), indexes.size()));
      GELOGD("Mapping %s to desc[%zu, %zu)", ir_spec.DebugString().c_str(), *indexes.begin(),
             *indexes.begin() + indexes.size());
    }
  }
  return GRAPH_SUCCESS;
}

void UpdateRawDescInstanceShifts(std::vector<size_t> &desc_instance_shifts, size_t elim_index) {
  if (elim_index >= desc_instance_shifts.size()) {
    return;
  }
  auto iter = desc_instance_shifts.begin() + elim_index + 1U;
  for (; iter != desc_instance_shifts.end(); iter++) {
    (*iter)++;
  }
}

graphStatus MappingNonDynamicIrDesc(const std::vector<IrIOSpec> &ir_specs, const DescEnv &desc_env,
                                    const std::vector<std::pair<std::string, uint32_t>> &name2index_left,
                                    const bool &require_raw_index,
                                    std::map<size_t, std::pair<size_t, size_t>> &ir_2_range) {
  GELOGD("Start mapping non-dynamic ir desc for %s", desc_env.DebugString().c_str());
  std::vector<size_t> desc_instance_shifts;
  desc_instance_shifts.resize(desc_env.NumDescs(), 0U);

  auto iter = name2index_left.begin();
  for (size_t ir_io_idx = 0U; ir_io_idx < ir_specs.size(); ir_io_idx++) {
    const auto &ir_spec = ir_specs[ir_io_idx];
    if (ir_spec.IsDynamic()) {  // 已经处理过Dynamic类型的IR输入
      continue;
    }

    if (iter == name2index_left.end()) {  // 只允许Optional的IR输入没有对应的desc，对应Optional在IR最后且没有Desc信息
      if (!ir_spec.IsOptional()) {
        GELOGW("No desc left for %s", ir_spec.DebugString().c_str());
        return GRAPH_SUCCESS;
      }
      ir_2_range.emplace(ir_io_idx, std::make_pair(GetIrDescStartIndex(ir_2_range, ir_io_idx), 0U));
      continue;
    }

    auto &name = iter->first;
    auto &index = iter->second;

    if (ir_spec.GetName() != name) {  // 如果当前名字和IR不一致，需要确保不是乱序，即没有与IR名字对应的Desc存在
      for (auto &name2index : name2index_left) {
        GE_ASSERT(ir_spec.GetName() != name2index.first, "Found desc for %s index %u, while current name is %s",
                  ir_spec.DebugString().c_str(), name2index.second, name.c_str());
      }
    }

    if (!ir_spec.IsOptional()) {  // 非可选，则认为是自行构造的非标IR
      iter++;
      ir_2_range.emplace(ir_io_idx, std::make_pair(index, 1U));
      GELOGD("Mapping %s to desc %zu named %s", ir_spec.DebugString().c_str(), index, name.c_str());
      continue;
    }

    if (name != ir_spec.GetName()) {  // 对应Optional不在尾部且未传入
      GELOGD("Ir spec %s has no instance as desc[%u] named %s", ir_spec.DebugString().c_str(), index, name.c_str());
      ir_2_range.emplace(ir_io_idx, std::make_pair(index, 0U));
      continue;
    }

    iter++;
    if (desc_env.IsDescValid(index)) {  // 对应Optional传入有效值
      GELOGD("Mapping %s desc[%zu]", ir_spec.DebugString().c_str(), index);
      ir_2_range.emplace(ir_io_idx, std::make_pair(index, 1U));
    } else {  // Optional传入无效值，对实例index进行调整（实例index只会保存非nullptr的输入）
      GELOGD("Skip mapping %s to invalid desc[%zu]", ir_spec.DebugString().c_str(), index);
      ir_2_range.emplace(ir_io_idx, std::make_pair(index, 0U));
      UpdateRawDescInstanceShifts(desc_instance_shifts, index);
    }
  }

  if (!require_raw_index) {
    for (auto &item : ir_2_range) {
      auto &start = item.second.first;
      auto &num = item.second.second;
      size_t shift = (start >= desc_instance_shifts.size() ? 0U : desc_instance_shifts[start]);
      start = (start > shift) ? (start - shift) : 0U;
      GELOGD("Re-mapping %s to desc[%zu, %zu) shift(-%zu)", ir_specs[item.first].DebugString().c_str(), start,
             start + num, shift);
    }
  }

  return GRAPH_SUCCESS;
}

graphStatus GetIrDescRange(const std::vector<IrIOSpec> &ir_specs, const std::map<std::string, uint32_t> &name2idx,
                           const DescEnv &desc_env, const bool &require_raw_index,
                           std::map<size_t, std::pair<size_t, size_t>> &ir_2_range) {
  GELOGD("Start get desc range for %s", desc_env.DebugString().c_str());
  for (auto &ir_spec : ir_specs) {
    GELOGD("  Spec %s", ir_spec.DebugString().c_str());
  }

  std::map<uint32_t, std::string> idx2name;
  for (auto &item : name2idx) {
    GELOGD("  Desc name %s index %d", item.first.c_str(), item.second);
    idx2name.emplace(item.second, item.first);
  }
  GE_ASSERT_EQ(idx2name.size(), name2idx.size());  // 拦截多个name对应到同一index
  if (!idx2name.empty()) {
    GE_ASSERT_EQ(idx2name.rbegin()->first, idx2name.size() - 1U);  // 拦截index不连续
  }

  // 首先确定Dynamic类型的IR IO对应的index范围，对于IR构图场景，用户会通过create_dynmaic_xx接口创建多个输入Desc，
  // 但是Desc在所有desc中的位置，是受调用时的参数决定的，默认情况下，都向尾部追加，会出现先定义的IR输入或输出对应的desc，在后定义的之后
  GE_ASSERT_GRAPH_SUCCESS(MappingDynamicIrDesc(ir_specs, desc_env, name2idx, ir_2_range));

  std::vector<bool> index_consumed;  // index对应的desc是否已经决定对应关系
  index_consumed.resize(name2idx.size(), false);
  for (auto &item : ir_2_range) {
    auto &range = item.second;
    for (size_t i = range.first; i < range.first + range.second; i++) {
      index_consumed[i] = true;
    }
  }

  std::vector<std::pair<std::string, uint32_t>> name2index_left;
  for (size_t i = 0U; i < index_consumed.size(); i++) {
    if (!index_consumed[i]) {  // 未被使用的index顺序排列
      name2index_left.emplace_back(idx2name[i], static_cast<uint32_t>(i));
    }
  }

  // 确定非Dynamic类型的IR IO对应的index范围
  GE_ASSERT_GRAPH_SUCCESS(MappingNonDynamicIrDesc(ir_specs, desc_env, name2index_left, require_raw_index, ir_2_range));

  // 不校验所有的index都决定了对应的IR输入（存在算子追加非IR输入的场景，CCB裁决框架适配支持）

  return GRAPH_SUCCESS;
}

graphStatus GetIrInputDescRange(const OpDescPtr &op, const bool &require_raw_index,
                                std::map<size_t, std::pair<size_t, size_t>> &ir_input_2_range) {
  GE_ASSERT_NOTNULL(op);
  std::vector<IrIOSpec> ir_specs;
  for (auto &item : op->GetIrInputs()) {
    ir_specs.emplace_back(item.first, item.second);
  }
  const std::map<std::string, uint32_t> &name2idx = op->GetAllInputName();
  DescEnv desc_env(op, true);

  return GetIrDescRange(ir_specs, name2idx, desc_env, require_raw_index, ir_input_2_range);
}
}  // namespace

// 获取输入IR对应的实例Desc的index范围，实例Desc中会去除未传值的Optional输入Desc
graphStatus OpDescUtils::GetIrInputInstanceDescRange(const OpDescPtr &op,
                                                     std::map<size_t, std::pair<size_t, size_t>> &ir_input_2_range) {
  return GetIrInputDescRange(op, false, ir_input_2_range);
}

// 获取输入IR对应的全部Desc的index范围，包含未传值的Optional输入Desc
graphStatus OpDescUtils::GetIrInputRawDescRange(const OpDescPtr &op,
                                                std::map<size_t, std::pair<size_t, size_t>> &ir_input_2_range) {
  return GetIrInputDescRange(op, true, ir_input_2_range);
}

graphStatus OpDescUtils::GetIrOutputDescRange(const OpDescPtr &op,
                                              std::map<size_t, std::pair<size_t, size_t>> &ir_output_2_range) {
  GE_ASSERT_NOTNULL(op);
  std::vector<IrIOSpec> ir_specs;
  for (auto &item : op->GetIrOutputs()) {
    ir_specs.emplace_back(item.first, item.second);
  }
  const std::map<std::string, uint32_t> &name2idx = op->GetAllOutputName();
  DescEnv desc_env(op, false);

  return GetIrDescRange(ir_specs, name2idx, desc_env, true, ir_output_2_range);
}
}  // namespace ge
/*lint +e512 +e737 +e752*/
