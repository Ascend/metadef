/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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
#include "register/graph_optimizer/fusion_common/accelerator.h"
#include "graph/operator_factory.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/graph_utils.h"

#include <array>

namespace fe {
WeightInfo::WeightInfo(const ge::GeTensorDesc &tensor_desc, void *data_p)
    : data(reinterpret_cast<uint8_t*>(data_p)) {
  shape = tensor_desc.GetShape();
  ori_shape = tensor_desc.GetOriginShape();
  datatype = tensor_desc.GetDataType();
  ori_datatype = tensor_desc.GetOriginDataType();
  format = tensor_desc.GetFormat();
  ori_format = tensor_desc.GetOriginFormat();
  CalcTotalDataSize();
}

WeightInfo::WeightInfo(const ge::NodePtr &node, int32_t index, void *data_p)
    : data(reinterpret_cast<uint8_t*>(data_p)) {
  if (node == nullptr) {
    return;
  }
  auto tensor = node->GetOpDesc()->MutableInputDesc(index);
  if (tensor == nullptr) {
    return;
  }
  shape = tensor->GetShape();
  ori_shape = tensor->GetOriginShape();
  datatype = tensor->GetDataType();
  ori_datatype = tensor->GetOriginDataType();
  format = tensor->GetFormat();
  ori_format = tensor->GetOriginFormat();
  CalcTotalDataSize();
}

WeightInfo::WeightInfo(const ge::GeShape &shape_p, const ge::GeShape &ori_shape_p,
                       ge::DataType datatype_p, ge::DataType ori_datatype_p,
                       ge::Format format_p, ge::Format ori_format_p, void *data_p)
    : shape(shape_p),
      ori_shape(ori_shape_p),
      datatype(datatype_p),
      ori_datatype(ori_datatype_p),
      format(format_p),
      ori_format(ori_format_p),
      data(reinterpret_cast<uint8_t*>(data_p)) {
  CalcTotalDataSize();
}

WeightInfo::WeightInfo(ge::GeShape &&shape_p, ge::GeShape &&ori_shape_p,
                       ge::DataType datatype_p, ge::DataType ori_datatype_p,
                       ge::Format format_p, ge::Format ori_format_p, void *data_p)
    : shape(std::move(shape_p)),
      ori_shape(std::move(ori_shape_p)),
      datatype(datatype_p),
      ori_datatype(ori_datatype_p),
      format(format_p),
      ori_format(ori_format_p),
      data(reinterpret_cast<uint8_t*>(data_p)) {
  CalcTotalDataSize();
}

WeightInfo::WeightInfo(const ge::GeShape &shape_p, ge::DataType datatype_p,
                       ge::Format format_p, void *data_p)
    : shape(shape_p),
      ori_shape(shape_p),
      datatype(datatype_p),
      ori_datatype(datatype_p),
      format(format_p),
      ori_format(format_p),
      data(reinterpret_cast<uint8_t*>(data_p)) {
  CalcTotalDataSize();
}

WeightInfo::WeightInfo(ge::GeShape &&shape_p, ge::DataType datatype_p,
                       ge::Format format_p, void *data_p)
    :shape(std::move(shape_p)),
     ori_shape(shape_p),
     datatype(datatype_p),
     ori_datatype(datatype_p),
     format(format_p),
     ori_format(format_p),
     data(reinterpret_cast<uint8_t*>(data_p)) {
  CalcTotalDataSize();
}

Accelerator::Accelerator(const ge::ComputeGraphPtr &graph) : graph_(graph) {}

Accelerator::Accelerator(ge::ComputeGraph &graph) : graph_(graph.shared_from_this()) {}

Accelerator::~Accelerator() {}

Status Accelerator::BreakInput(const ge::NodePtr &node,
                               const vector<int32_t> &input_index) {
  for (auto index : input_index) {
    auto in_anchor = node->GetInDataAnchor(index);
    if (in_anchor == nullptr) {
      continue;
    }

    in_anchor->UnlinkAll();
  }
  return SUCCESS;
}

Status Accelerator::BreakOutput(const ge::NodePtr &node,
                                const vector<int32_t> &output_index) {
  for (auto index : output_index) {
    auto out_anchor = node->GetOutDataAnchor(index);
    if (out_anchor == nullptr) {
      continue;
    }

    out_anchor->UnlinkAll();
  }
  return SUCCESS;
}

/* Remove single input and output node, then relink the peer nodes.
 * A->B-->C      ---->    A-->C
 *     \->D    remove B    \->D */
Status Accelerator::RemoveSingleInOutNode(const ge::NodePtr &node) {
  ACCLRT_NOTNULL(node, PARAM_INVALID);
  if (ge::GraphUtils::IsolateNode(node, {}) != ge::GRAPH_SUCCESS) {
    return FAILED;
  }

  if (ge::GraphUtils::RemoveNodeWithoutRelink(graph_, node) != ge::GRAPH_SUCCESS) {
    return FAILED;
  }

  return SUCCESS;
}

/* Just remove the node and all its relative data and control anchors. */
Status Accelerator::RemoveNodeOnly(const ge::NodePtr &node) {
  ACCLRT_NOTNULL(node, PARAM_INVALID);
  ge::NodeUtils::UnlinkAll(*node);

  if (ge::GraphUtils::RemoveNodeWithoutRelink(graph_, node) != ge::GRAPH_SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

ge::GeTensorPtr GenerateWeightTensor(const WeightInfo &w_info) {
  ge::GeTensorDesc new_weight_tensor;
  new_weight_tensor.SetShape(w_info.shape);
  new_weight_tensor.SetDataType(w_info.datatype);
  new_weight_tensor.SetFormat(w_info.format);
  new_weight_tensor.SetOriginShape(w_info.ori_shape);
  new_weight_tensor.SetOriginDataType(w_info.ori_datatype);
  new_weight_tensor.SetOriginFormat(w_info.ori_format);
  if (w_info.total_data_size == 0) {
    return nullptr;
  }
  ge::GeTensorPtr w = nullptr;
  GE_MAKE_SHARED(w = std::make_shared<ge::GeTensor>(
      new_weight_tensor,
      reinterpret_cast<uint8_t *>(w_info.data), w_info.total_data_size),
                 return nullptr);
  return w;
}

static inline ge::NodePtr GetPeerOutNode(const ge::NodePtr &node,
                                         int32_t index) {
  auto in_anchor = node->GetInDataAnchor(index);
  ACCLRT_NOTNULL(in_anchor, nullptr);
  auto peer_anchor = in_anchor->GetPeerOutAnchor();
  ACCLRT_NOTNULL(in_anchor, nullptr);
  auto peer_node = peer_anchor->GetOwnerNode();
  return peer_node;
}

void UpdateTensor(const ge::GeTensorDescPtr &tensor, const WeightInfo &w_info) {
  tensor->SetDataType(w_info.datatype);
  tensor->SetOriginDataType(w_info.ori_datatype);
  tensor->SetFormat(w_info.format);
  tensor->SetOriginFormat(w_info.ori_format);
  tensor->SetShape(w_info.shape);
  tensor->SetOriginShape(w_info.ori_shape);
}

ge::NodePtr Accelerator::AddConstNode(const ge::NodePtr &node, int32_t index,
                                      const WeightInfo &w_info) {
  auto node_in_tensor = node->GetOpDesc()->MutableInputDesc(index);
  ACCLRT_NOTNULL(node_in_tensor, nullptr);
  UpdateTensor(node_in_tensor, w_info);

  ge::GeTensorPtr const_out_tenosr = nullptr;
  GE_MAKE_SHARED(const_out_tenosr = std::make_shared<ge::GeTensor>(*node_in_tensor), return nullptr);

  Status ret = const_out_tenosr->SetData(reinterpret_cast<uint8_t *>(w_info.data),
                                         w_info.total_data_size);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "[GraphAcclrt][AddWeight][AddConstNode] Failed to set data.");
    return nullptr;
  }
  ge::OpDescPtr const_op_desc = ge::OpDescUtils::CreateConstOp(const_out_tenosr);

  auto const_node = graph_->AddNode(const_op_desc);
  if (const_node == nullptr) {
    GELOGE(FAILED, "[GraphAcclrt][AddWeight][AddConstNode] Failed to add const node.");
    return nullptr;
  }

  GELOGD("Successfully create const input [%s] for node [%s].", const_op_desc->GetName().c_str(),
         node->GetName().c_str());
  if (ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node->GetInDataAnchor(index)) != SUCCESS) {
    GELOGE(FAILED, "[GraphAcclrt][AddWeight][AddConstNode] Failed to add edge between const %s and index %d of %s.",
           const_node->GetName().c_str(), index, node->GetName().c_str());
  }

  return const_node;
}

ge::NodePtr Accelerator::UpdateConst(const ge::NodePtr &node, int32_t index,
                                     const WeightInfo &w_info) {
  auto const_node = AcceleratorUtils::GetConstInput(node, index);
  if (const_node == nullptr) {
    return nullptr;
  }
  auto const_op = const_node->GetOpDesc();
  auto const_out_tensor_desc = const_op->MutableOutputDesc(0);
  UpdateTensor(const_out_tensor_desc, w_info);

  auto node_in_tensor = node->GetOpDesc()->MutableInputDesc(index);
  ACCLRT_NOTNULL(node_in_tensor, nullptr);
  UpdateTensor(node_in_tensor, w_info);

  std::vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(const_node);
  /* Substitute the const value with a new one. */
  Status ret;
  if (weights.empty()) {
    GELOGD("The weight of %s is empty, create a new one.", const_node->GetName().c_str());
    ge::GeTensorPtr const_out = nullptr;
    GE_MAKE_SHARED(const_out = std::make_shared<ge::GeTensor>(*const_out_tensor_desc), return nullptr);
    if (w_info.data == nullptr) {
      GELOGE(FAILED, "[GraphAcclrt][AddWeight][UpdateConst] Data is null.");
      return nullptr;
    }
    ret = const_out->SetData(reinterpret_cast<uint8_t *>(w_info.data), w_info.total_data_size);
  } else {
    GELOGD("The weight of %s is not empty, update data.", const_node->GetName().c_str());
    ge::GeTensorPtr &const_out = weights.at(0);
    const_out->SetTensorDesc(*const_out_tensor_desc);
    ret = const_out->SetData(reinterpret_cast<uint8_t *>(w_info.data), w_info.total_data_size);
  }
  if (ret != SUCCESS) {
    GELOGE(FAILED, "[GraphAcclrt][AddWeight][UpdateConst] Failed to set data.");
    return nullptr;
  }

  return const_node;
}

ge::NodePtr Accelerator::AddWeight(const ge::NodePtr &node, const WeightInfo &w_info, int32_t index) {
  ACCLRT_NOTNULL(node, nullptr);
  size_t input_size = node->GetAllInDataAnchorsSize();
  if (static_cast<size_t>(index) >= input_size) {
    return AddWeight(node, w_info);
  } else {
    auto in_anchor = node->GetInDataAnchor(index);
    /* 1. If the peer node of this input index is nullptr, we add a const node
     *    as input and update tensor desc. */
    if (in_anchor->GetPeerOutAnchor() == nullptr) {
      auto const_node = AddConstNode(node, index, w_info);
      return const_node;
    }

    /* 2. If the peer node of this input index is Const, we substitute the data
     *    of current Const and update tensor desc. */
    return UpdateConst(node, index, w_info);
  }
}

ge::NodePtr Accelerator::AddWeight(const ge::NodePtr &node, const WeightInfo &w_info, const string& tensor_name) {
  ACCLRT_NOTNULL(node, nullptr);
  auto index = node->GetOpDesc()->GetInputIndexByName(tensor_name);
  if (index == -1) {
    return nullptr;
  }
  return AddWeight(node, w_info, index);
}

ge::NodePtr Accelerator::AddWeight(const ge::NodePtr &node,
                                   const WeightInfo &w_info) {
  ACCLRT_NOTNULL(node, nullptr);
  std::vector<ge::NodePtr> ret;
  /* 1. Collect all existing weights. */
  vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(node);

  /* 2. Create new weight and link edges. */
  ge::GeTensorPtr w = GenerateWeightTensor(w_info);
  if (w == nullptr) {
    GELOGE(FAILED, "[GraphAcclrt][AddWeight]Failed to generate weight for node %s.", node->GetName().c_str());
    return nullptr;
  }
  weights.emplace_back(w);
  if (ge::OpDescUtils::SetWeights(node, weights) != ge::GRAPH_SUCCESS) {
    return nullptr;
  }

  /* 3. Return new weight node. */
  auto in_size = static_cast<int32_t>(node->GetAllInDataAnchorsSize());
  auto i = in_size - 1;
  return GetPeerOutNode(node, i);
}
                                                 
std::vector<ge::NodePtr> Accelerator::AddWeights(const ge::NodePtr &node,
                                                 const vector<WeightInfo> &w_infos) {
  std::vector<ge::NodePtr> ret;
  ACCLRT_NOTNULL(node, ret);
  /* 1. Colloect all existing weights. */
  vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(node);

  /* 2. Create new weights and link edges. */
  for (auto &w_info : w_infos) {
    ge::GeTensorPtr w = GenerateWeightTensor(w_info);
    if (w == nullptr) {
      GELOGE(FAILED, "[GraphAcclrt][AddWeights]Failed to generate weight for node %s.", node->GetName().c_str());
      return ret;
    }
    weights.emplace_back(w);
  }
  if (ge::OpDescUtils::SetWeights(node, weights) != ge::GRAPH_SUCCESS) {
    return ret;
  }

  /* 3. Return new weight nodes. */
  auto in_size = static_cast<size_t>(node->GetAllInDataAnchorsSize());
  GELOGD("in_size %zu, w_info size %zu", in_size, w_infos.size());
  for (size_t i = in_size - w_infos.size(); i < in_size; i++) {
    auto peer_node = GetPeerOutNode(node, i);
    ret.emplace_back(peer_node);
  }
    
  return ret;
}

ge::GeTensorPtr Accelerator::MutableWeight(const ge::NodePtr &node, int32_t index) {
  ACCLRT_NOTNULL(node, nullptr);
  auto const_node = AcceleratorUtils::GetConstInput(node, index);
  if (const_node == nullptr) {
    return nullptr;
  }

  std::vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(const_node);
  if (weights.empty()) {
    return nullptr;
  }

  return weights.at(0);
}

ge::NodePtr Accelerator::AddNodeOnly(const string &op_name, const string &op_type) {
  auto op = ge::OperatorFactory::CreateOperator(op_name.c_str(), op_type.c_str());
  if (op.IsEmpty()) {
    return nullptr;
  }
  auto op_desc = ge::OpDescUtils::GetOpDescFromOperator(op);
  auto ret_node = graph_->AddNode(op_desc);
  return ret_node;
}

ge::NodePtr Accelerator::InsertNodeBefore(const string &op_name, const string &op_type,
                                          const ge::NodePtr &base_node, int32_t base_input_index,
                                          int32_t input_index, int32_t output_index) {
  ACCLRT_NOTNULL(base_node, nullptr);
  auto base_desc = base_node->GetOpDesc();
  auto base_input = base_desc->MutableInputDesc(base_input_index);
  ACCLRT_NOTNULL(base_input, nullptr);

  /* 1. Create new operator, OpDesc and Node. */
  auto op = ge::OperatorFactory::CreateOperator(op_name.c_str(), op_type.c_str());
  if (op.IsEmpty()) {
    GELOGE(FAILED, "[GraphAcclrt][InstNodeBefore]Cannot find this op %s in op factory.", op_type.c_str());
    return nullptr;
  }

  auto op_desc = ge::OpDescUtils::GetOpDescFromOperator(op);
  auto ret_node = graph_->AddNode(op_desc);

  auto base_in_anchor = base_node->GetInDataAnchor(base_input_index);
  auto peer_out_anchor = base_in_anchor->GetPeerOutAnchor();
  /* 2. Update Output desc using base node's successor node. */
  if (op_desc->UpdateOutputDesc(output_index, *base_input) != ge::GRAPH_SUCCESS) {
    GELOGE(FAILED, "[GraphAcclrt][InstNodeBefore]Failed to update output %d of node %s", output_index, op_name.c_str());
    goto failed_process;
  }

  if (peer_out_anchor != nullptr) {
    auto peer_out_index = peer_out_anchor->GetIdx();
    auto peer_output = peer_out_anchor->GetOwnerNode()->GetOpDesc()->MutableOutputDesc(peer_out_index);
    /* 3. Update input desc using base node's father node. */
    if (op_desc->UpdateInputDesc(input_index, *peer_output) != ge::GRAPH_SUCCESS) {
      GELOGE(FAILED, "[GraphAcclrt][InstNodeBefore]Failed to update input %d of node %s", input_index, op_name.c_str());
      goto failed_process;
    }

    /* 4.1. Insert new op into graph and between peer-out and base-in anchors. */
    if (ge::GraphUtils::InsertNodeAfter(peer_out_anchor, {base_in_anchor}, ret_node,
                                        static_cast<uint32_t>(input_index), static_cast<uint32_t>(output_index)) !=
        ge::GRAPH_SUCCESS) {
      goto failed_process;
    }
  } else {
    GELOGD("Input %d of base node %s does not have peer out node.", base_input_index, base_node->GetName().c_str());
    /* 4.2. Just insert new op before base-in anchor. */
    auto out_anchor = ret_node->GetOutDataAnchor(output_index);
    ACCLRT_NOTNULL(out_anchor, nullptr);
    if (ge::GraphUtils::AddEdge(out_anchor, base_in_anchor) != ge::GRAPH_SUCCESS) {
      goto failed_process;
    }
  }
  GELOGD("Succeed inserting %s before %s.", op_name.c_str(), base_node->GetName().c_str());
  return ret_node;

failed_process:
  graph_->RemoveNode(ret_node);
  return nullptr;
}

ge::NodePtr Accelerator::InsertNodeAfter(const string &op_name, const string &op_type, const ge::NodePtr &base_node,
                                         int32_t base_output_index, int32_t input_index, int32_t output_index) {
  ACCLRT_NOTNULL(base_node, nullptr);
  auto base_desc = base_node->GetOpDesc();
  auto base_output = base_desc->MutableOutputDesc(base_output_index);
  ACCLRT_NOTNULL(base_output, nullptr);

  auto base_out_anchor = base_node->GetOutDataAnchor(base_output_index);
  auto peer_in_anchors = base_out_anchor->GetPeerInDataAnchors();

  /* 1. Create new operator, OpDesc and Node. */
  auto op = ge::OperatorFactory::CreateOperator(op_name.c_str(), op_type.c_str());
  if (op.IsEmpty()) {
    GELOGE(FAILED, "[GraphAcclrt][InstNodeAfter]Cannot find this op %s in op factory.", op_type.c_str());
    return nullptr;
  }
  auto op_desc = ge::OpDescUtils::GetOpDescFromOperator(op);
  auto ret_node = graph_->AddNode(op_desc);

  /* 2. Update input desc using base_node. */
  if (op_desc->UpdateInputDesc(input_index, *base_output) != ge::GRAPH_SUCCESS) {
    GELOGE(FAILED, "[GraphAcclrt][InstNodeAfter]Failed to update input %d of node %s", input_index, op_name.c_str());
    goto failed_process;
  }

  if (!peer_in_anchors.empty()) {
    /* 3. Update output desc by peer input. */
    auto peer_in_anchor = peer_in_anchors.at(0);
    auto peer_in_index = peer_in_anchor->GetIdx();
    auto peer_node = peer_in_anchor->GetOwnerNode();
    auto peer_input = peer_node->GetOpDesc()->MutableInputDesc(peer_in_index);
    if (op_desc->UpdateOutputDesc(output_index, *peer_input) != ge::GRAPH_SUCCESS) {
      GELOGE(FAILED, "[GraphAcclrt][InstNodeAfter]Failed to update output %d of node %s",
             output_index, op_name.c_str());
      goto failed_process;
    }

    /* 4.1. Insert new op between base-out anchor and every peer-in anchor. */
    auto peer_in_anchors_vec = std::vector<ge::InDataAnchorPtr>(peer_in_anchors.begin(), peer_in_anchors.end());
    if (ge::GraphUtils::InsertNodeAfter(base_out_anchor, peer_in_anchors_vec, ret_node,
                                        static_cast<uint32_t>(input_index),
                                        static_cast<uint32_t>(output_index)) != ge::GRAPH_SUCCESS) {
      GELOGE(FAILED, "[GraphAcclrt][InstNodeAfter]Failed to insert node after output %d of node %s",
             base_output_index, base_node->GetName().c_str());
      goto failed_process;
    }
  } else {
    GELOGD("output %d of base node %s does not have peer in nodes.", base_output_index, base_node->GetName().c_str());
    auto in_anchor = ret_node->GetInDataAnchor(input_index);
    /* 4.2. Just insert new op after base-out anchor. */
    if (ge::GraphUtils::AddEdge(base_out_anchor, in_anchor) != ge::GRAPH_SUCCESS) {
      GELOGE(FAILED, "[GraphAcclrt][InstNodeAfter]Failed to add edge between %d of %s and %d of %s",
             base_output_index, base_node->GetName().c_str(), input_index, op_name.c_str());
      goto failed_process;
    }
  }
  GELOGD("Succeed inserting %s after %s.", op_name.c_str(), base_node->GetName().c_str());
  return ret_node;
failed_process:
  graph_->RemoveNode(ret_node);
  return nullptr;
}

/* If relations is empty and relations_by_name is not empty,
 * interpreter the relations_by_name to relations. */
void PreprocessRelation(const ge::NodePtr &node,
                        bool is_input,
                        Relations &relations) {
  auto op_desc = node->GetOpDesc();
  if (relations.relations.empty() &&
      !relations.relations_by_name.empty()) {
    for (auto &relation : relations.relations_by_name) {
      auto name = relation.first;
      auto index = is_input ? op_desc->GetInputIndexByName(name) :
                              op_desc->GetOutputIndexByName(name);
      relations.relations.emplace(std::make_pair(index, relation.second));
    }
  }
}

Status Accelerator::LinkInput(Relations &input_relations,
                              const ge::NodePtr &dst_node,
                              bool update_tensor) {
  ACCLRT_NOTNULL(dst_node, PARAM_INVALID);
  auto dst_op_desc = dst_node->GetOpDesc();
  if (input_relations.relations.empty() &&
      input_relations.relations_by_name.empty()) {
    GELOGD("dst_node %s's input relations is empty.", dst_node->GetName().c_str());
    return PARAM_INVALID;
  }
  PreprocessRelation(dst_node, true, input_relations);

  auto dst_input_size = dst_node->GetAllInDataAnchorsSize();
  for (const auto &relation : input_relations.relations) {
    auto dst_in_index = static_cast<uint32_t>(relation.first);
    if (dst_in_index >= dst_input_size) {
      GELOGW("Dst input index %u is larger than dst node %s's input size %u.",
             dst_in_index, dst_node->GetName().c_str(), dst_input_size);
      continue;
    }

    if (relation.second.empty()) {
      continue;
    }

    auto src_node = relation.second.at(0).first;
    auto src_out_index = relation.second.at(0).second;
    ACCLRT_NOTNULL(src_node, PARAM_INVALID);
    auto out_anchor = src_node->GetOutDataAnchor(src_out_index);
    ACCLRT_NOTNULL(out_anchor, PARAM_INVALID);

    /* 1. Update tensor descs. We assume the input desc of src node is correct. */
    if (update_tensor) {
      auto src_out_tensor_desc = src_node->GetOpDesc()->MutableOutputDesc(src_out_index);
      if (dst_op_desc->UpdateInputDesc(dst_in_index, *src_out_tensor_desc) != ge::GRAPH_SUCCESS) {
        GELOGE(FAILED, "[GraphAcclrt][LinkInput]Failed to update input %d of node %s",
               dst_in_index, dst_node->GetName().c_str());
        return FAILED;
      }
    }

    /* 2. Link anchors. */
    auto dst_in_anchor = dst_node->GetInDataAnchor(dst_in_index);
    if (ge::GraphUtils::AddEdge(out_anchor, dst_in_anchor) != ge::GRAPH_SUCCESS) {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status Accelerator::LinkOutput(Relations &out_relations, const ge::NodePtr &src_node, bool update_tensor) {
  ACCLRT_NOTNULL(src_node, PARAM_INVALID);
  auto dst_op_desc = src_node->GetOpDesc();
  if (out_relations.relations.empty() && out_relations.relations_by_name.empty()) {
    GELOGD("src_node %s's output relations is empty.", src_node->GetName().c_str());
    return PARAM_INVALID;
  }

  PreprocessRelation(src_node, false, out_relations);

  auto src_op_desc = src_node->GetOpDesc();
  auto src_output_size = src_node->GetAllOutDataAnchorsSize();
  for (auto &relation : out_relations.relations) {
    auto src_out_index = static_cast<uint32_t>(relation.first);
    if (src_out_index >= src_output_size) {
      GELOGW("Source output index %u is larger than src node %s's output size %u.",
             src_out_index, src_node->GetName().c_str(), src_output_size);
      continue;
    }

    if (relation.second.empty()) {
      continue;
    }
    /* 1. Update tensor descs. We assume the input desc of first dst node is correct.
     * We only update once, */
    if (update_tensor) {
      auto first_dst_node = relation.second.at(0).first;
      auto first_dst_index = relation.second.at(0).second;
      auto dst_in_tensor_desc = first_dst_node->GetOpDesc()->MutableInputDesc(first_dst_index);
      if (src_op_desc->UpdateOutputDesc(src_out_index, *dst_in_tensor_desc) != ge::GRAPH_SUCCESS) {
        GELOGE(FAILED, "[GraphAcclrt][LinkOutput]Failed to update output %d of node %s",
               src_out_index, src_node->GetName().c_str());
        return FAILED;
      }
    }

    /* 2. Link all peer in anchors. */
    for (const auto &peer_index: relation.second) {
      auto dst_node = peer_index.first;
      auto dst_index = peer_index.second;
      if (dst_node == nullptr) {
        continue;
      }
      auto in_anchor = dst_node->GetInDataAnchor(dst_index);
      ACCLRT_NOTNULL(in_anchor, PARAM_INVALID);
      auto peer_out = in_anchor->GetPeerOutAnchor();
      if (peer_out != nullptr) {
        GELOGW("Dst node %s's input %d already has peer out [%s].", dst_node->GetName().c_str(), dst_index,
               peer_out->GetOwnerNode()->GetName().c_str());
        in_anchor->UnlinkAll();
      }

      auto src_out_anchor = src_node->GetOutDataAnchor(src_out_index);
      if (ge::GraphUtils::AddEdge(src_out_anchor, in_anchor) != ge::GRAPH_SUCCESS) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status Accelerator::UpdateInputByPeer(const ge::NodePtr &node, int32_t index,
                                      const ge::NodePtr &peer_node, int32_t peer_index) {
  ACCLRT_NOTNULL(node, PARAM_INVALID);
  ACCLRT_NOTNULL(peer_node, PARAM_INVALID);

  auto peer_output_desc = peer_node->GetOpDesc()->MutableOutputDesc(peer_index);
  ACCLRT_NOTNULL(peer_output_desc, PARAM_INVALID);

  auto input_desc = node->GetOpDesc()->MutableInputDesc(index);
  ACCLRT_NOTNULL(input_desc, PARAM_INVALID);

  *input_desc = *peer_output_desc;

  return SUCCESS;
}

Status Accelerator::UpdateOutputByPeer(const ge::NodePtr &node, int32_t index,
                                       const ge::NodePtr &peer_node, int32_t peer_index) {
  ACCLRT_NOTNULL(node, PARAM_INVALID);
  ACCLRT_NOTNULL(peer_node, PARAM_INVALID);

  auto peer_input_desc = peer_node->GetOpDesc()->MutableInputDesc(peer_index);
  ACCLRT_NOTNULL(peer_input_desc, PARAM_INVALID);

  auto output_desc = node->GetOpDesc()->MutableOutputDesc(index);
  ACCLRT_NOTNULL(output_desc, PARAM_INVALID);

  *output_desc = *peer_input_desc;
  return SUCCESS;
}

bool Accelerator::IsUnknownShape(const ge::NodePtr &node, int32_t index, bool is_input) {
  ge::GeTensorDescPtr tensor;
  if (is_input) {
    tensor = node->GetOpDesc()->MutableInputDesc(index);
  } else {
    tensor = node->GetOpDesc()->MutableOutputDesc(index);
  }
  ACCLRT_NOTNULL(tensor, false);
  const auto &shape = tensor->MutableShape();
  return shape.IsUnknownShape();
}

bool Accelerator::IsUnknownOriShape(const ge::NodePtr &node, int32_t index, bool is_input) {
  ge::GeTensorDescPtr tensor;
  if (is_input) {
    tensor = node->GetOpDesc()->MutableInputDesc(index);
  } else {
    tensor = node->GetOpDesc()->MutableOutputDesc(index);
  }
  ACCLRT_NOTNULL(tensor, false);
  const auto &shape = tensor->GetOriginShape();
  return shape.IsUnknownShape();
}

Status Accelerator::TransferOutCtrlEdges(const std::vector<ge::NodePtr> &nodes,
                                         const ge::NodePtr &new_node) {
  ACCLRT_NOTNULL(new_node, FAILED);
  for (const auto &node : nodes) {
    if (node == nullptr) {
      continue;
    }
    auto peer_in_ctrl_nodes = node->GetOutControlNodes();
    if (peer_in_ctrl_nodes.empty()) {
      continue;
    }

    for (const auto &in_node : peer_in_ctrl_nodes) {
      ge::GraphUtils::AddEdge(new_node->GetOutControlAnchor(), in_node->GetInControlAnchor());
    }
  }
  return SUCCESS;
}

Status Accelerator::TransferInCtrlEdges(const std::vector<ge::NodePtr> &nodes,
                                        const ge::NodePtr &new_node) {
  ACCLRT_NOTNULL(new_node, FAILED);
  for (const auto &node : nodes) {
    if (node == nullptr) {
      continue;
    }
    auto peer_out_ctrl_nodes = node->GetInControlNodes();
    if (peer_out_ctrl_nodes.empty()) {
      continue;
    }

    for (const auto &out_node : peer_out_ctrl_nodes) {
      if (ge::GraphUtils::AddEdge(out_node->GetOutControlAnchor(), new_node->GetInControlAnchor()) !=
          ge::GRAPH_SUCCESS) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

ge::NodePtr Accelerator::MultiInOne(const string &node_name, const string &node_type,
                                    Relations &input_relations,
                                    Relations &output_relations,
                                    const std::vector<ge::NodePtr> &old_nodes,
                                    bool remove_old) {
  auto node = AddNodeOnly(node_name, node_type);
  if (MultiInOne(node, input_relations, output_relations, old_nodes, remove_old) != SUCCESS) {
    graph_->RemoveNode(node);
    return nullptr;
  }
  return node;
}

Status Accelerator::MultiInOne(const ge::NodePtr &new_node,
                               Relations &input_relations,
                               Relations &output_relations,
                               const std::vector<ge::NodePtr> &old_nodes,
                               bool remove_old) {
  ACCLRT_NOTNULL(new_node, FAILED);
  GELOGD("Merge multiple nodes into %s.", new_node->GetName().c_str());
  PeerIndices dst_list;
  std::vector<int32_t> output_index;
  /* Check params. */
  if (input_relations.relations.size() > new_node->GetAllInDataAnchorsSize()) {
    GELOGE(FAILED, "[GraphAcclrt][MultiInOne][ChkInput]Input relation size %zu is larger than %s's input size %u.",
           input_relations.relations.size(), new_node->GetName().c_str(), new_node->GetAllInDataAnchorsSize());
    return FAILED;
  }
  if (output_relations.relations.size() > new_node->GetAllOutDataAnchorsSize()) {
    GELOGE(FAILED, "[GraphAcclrt][MultiInOne][ChkOutput]Output relation size %zu is larger than %s's output size %u.",
           output_relations.relations.size(), new_node->GetName().c_str(), new_node->GetAllOutDataAnchorsSize());
    return FAILED;
  }

  /* Link data edges. */
  if (LinkInput(input_relations, new_node, true) != SUCCESS) {
    GELOGE(FAILED, "[GraphAcclrt][MultiInOne][LnkIn]Failed to link input for node %s.", new_node->GetName().c_str());
    return FAILED;
  }

  if (LinkOutput(output_relations, new_node, true) != SUCCESS) {
    GELOGE(FAILED, "[GraphAcclrt][MultiInOne][LnkOut]Failed to link output for node %s.", new_node->GetName().c_str());
    return FAILED;
  }

  /* Link control edges. */
  if (TransferInCtrlEdges(old_nodes, new_node) != SUCCESS) {
    return FAILED;
  }

  if (TransferOutCtrlEdges(old_nodes, new_node) != SUCCESS) {
    return FAILED;
  }

  if (remove_old) {
    for (auto &old_node : old_nodes) {
      RemoveNodeOnly(old_node);
    }
  }
  return SUCCESS;
}
}