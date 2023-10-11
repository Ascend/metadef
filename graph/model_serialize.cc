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

#include "graph/model_serialize.h"
#include <google/protobuf/text_format.h>
#include <queue>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "graph/debug/ge_attr_define.h"
#include "proto/ge_ir.pb.h"
#include "debug/ge_log.h"
#include "debug/ge_util.h"
#include "graph/utils/file_utils.h"
#include "graph/detail/model_serialize_imp.h"
#include "graph/op_desc_impl.h"
#include "graph/ge_tensor.h"
#include "graph/ge_tensor_impl.h"
#include "graph/compute_graph_impl.h"
#include "graph/serialization/attr_serializer_registry.h"
#include "graph/utils/graph_utils.h"
#include "debug/ge_op_types.h"
#include "common/util/mem_utils.h"
#include "common/checker.h"

namespace {
const std::string kTmpWeight = "air_weight/";
const std::string kSrcOutPeerIndex = "_src_out_peer_index_for_ge_txt_load";  // only exist in dump file
constexpr int64_t kInvalidIndex = -1;
constexpr int32_t kDecimal = 10;
}

namespace ge {
bool ModelSerializeImp::ParseNodeIndex(const std::string &node_index, std::string &node_name, int32_t &index) const {
  const auto sep = node_index.rfind(":");
  if (sep == std::string::npos) {
    GELOGD("[Parse][CheckParam] Separator \":\" is not found in node_index.");
    return false;
  }
  node_name = node_index.substr(0UL, sep);
  const auto index_str = node_index.substr(sep + 1UL);
  index = static_cast<int32_t>(std::strtol(index_str.c_str(), nullptr, kDecimal));
  return true;
}

int64_t ModelSerializeImp::GenDataInputInfo(const OutDataAnchorPtr &src_anchor,
                                            const InDataAnchorPtr &dst_anchor) const {
  const auto peer_in_data_anchors = src_anchor->GetPeerInDataAnchors();
  for (size_t i = 0U; i < peer_in_data_anchors.size(); ++i) {
    if (peer_in_data_anchors.at(i) == dst_anchor) {
      return static_cast<int64_t>(i);
    }
  }
  return kInvalidIndex;
}

int64_t ModelSerializeImp::GenCtrlInputInfo(const OutControlAnchorPtr &src_anchor,
                                            const InControlAnchorPtr &dst_anchor) const {
  const auto peer_in_ctrl_anchors = src_anchor->GetPeerInControlAnchors();
  for (size_t i = 0U; i < peer_in_ctrl_anchors.size(); ++i) {
    if (peer_in_ctrl_anchors.at(i) == dst_anchor) {
      return static_cast<int64_t>(i);
    }
  }
  return kInvalidIndex;
}

bool ModelSerializeImp::SerializeEdge(const NodePtr &node, proto::OpDef *const op_def_proto,
                                      const bool is_dump_graph) const {
  GE_CHK_BOOL_EXEC(node != nullptr, REPORT_INNER_ERROR("E18888", "param node is nullptr, check invalid.");
                   return false, "[Check][Param] node is null.");
  GE_CHK_BOOL_EXEC(op_def_proto != nullptr, REPORT_INNER_ERROR("E18888", "param op_def_proto is null, check invalid.");
                   return false, "[Check][Param] op_def_proto is null.");

  op_def_proto->clear_input();
  proto::AttrDef src_out_peer_index;
  // Inputs
  for (const auto &in_data_anchor : node->GetAllInDataAnchors()) {
    if (in_data_anchor != nullptr) {
      const auto peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
      if ((peer_out_anchor != nullptr) && peer_out_anchor->GetOwnerNode()) {
        op_def_proto->add_input(peer_out_anchor->GetOwnerNode()->GetName() + ":" +
                                std::to_string(peer_out_anchor->GetIdx()));
        src_out_peer_index.mutable_list()->add_i(GenDataInputInfo(peer_out_anchor, in_data_anchor));
      } else {
        op_def_proto->add_input("");
        src_out_peer_index.mutable_list()->add_i(kInvalidIndex);
      }
    }
  }
  // Control edge
  const auto in_control_anchor = node->GetInControlAnchor();
  if (in_control_anchor != nullptr) {
    const auto peer_out_anchors = in_control_anchor->GetPeerOutControlAnchors();
    for (const auto &peer_out_anchor : peer_out_anchors) {
      if ((peer_out_anchor != nullptr) && peer_out_anchor->GetOwnerNode()) {
        op_def_proto->add_input(peer_out_anchor->GetOwnerNode()->GetName() + ":-1");
        src_out_peer_index.mutable_list()->add_i(GenCtrlInputInfo(peer_out_anchor, in_control_anchor));
      }
    }
  }
  if (is_dump_graph) {
    GELOGD("Save src out peer index for %s.", node->GetName().c_str());
    auto const op_desc_attr = op_def_proto->mutable_attr();
    (void)op_desc_attr->insert({kSrcOutPeerIndex, src_out_peer_index});
  }

  return true;
}

void ModelSerializeImp::FixOpDefSubgraphInstanceName(const ConstOpDescPtr &op_desc) const {
  op_desc->impl_->meta_data_.ClearSubgraphNames();
  for (const std::string &name : op_desc->GetSubgraphInstanceNames()) {
    op_desc->impl_->meta_data_.AddSubGraphName(name);
  }
}

bool ModelSerializeImp::SerializeOpDesc(const ConstOpDescPtr &op_desc, proto::OpDef *const op_def_proto,
                                        const bool not_dump_all) const {
  GE_CHK_BOOL_EXEC(op_desc != nullptr, REPORT_INNER_ERROR("E18888", "param op_desc is nullptr. check invalid.");
                   return false, "[Check][Param] op_desc is null.");
  GE_CHK_BOOL_EXEC(op_def_proto != nullptr, REPORT_INNER_ERROR("E18888", "param op_def_proto is null, check invalid.");
                   return false, "[Check][Param] op_def_proto is null.");
  GE_CHK_BOOL_EXEC(op_desc->impl_ != nullptr,
                   REPORT_INNER_ERROR("E18888", "param op_desc impl is null, check invalid.");
                   return false, "[Check][Param] op_desc impl is null.");

  FixOpDefSubgraphInstanceName(op_desc);
  op_desc->impl_->SerializeMetaDataToOpDef(op_def_proto);
  // Delete unnecessary attr
  op_def_proto->clear_input_desc();
  op_def_proto->clear_output_desc();
  // Input descs
  if (op_desc->GetAllInputsSize() > 0UL) {
    const auto size = static_cast<uint32_t>(op_desc->GetAllInputsSize());
    for (uint32_t i = 0U; i < size; i++) {
      const auto tensor_desc = op_desc->GetInputDescPtrDfault(i);
      if ((tensor_desc != nullptr) && (tensor_desc->impl_ != nullptr)) {
        GeTensorSerializeUtils::GeTensorDescAsProto(*tensor_desc, op_def_proto->add_input_desc());
      }
    }
  }
  // Output descs
  if (op_desc->GetOutputsSize() > 0UL) {
    const auto size = static_cast<uint32_t>(op_desc->GetOutputsSize());
    for (uint32_t i = 0U; i < size; i++) {
      const auto tensor_desc = op_desc->GetOutputDescPtr(i);
      if ((tensor_desc != nullptr) && (tensor_desc->impl_ != nullptr)) {
        GeTensorSerializeUtils::GeTensorDescAsProto(*tensor_desc, op_def_proto->add_output_desc());
      }
    }
  }

  op_def_proto->set_id(op_desc->GetId());
  OpDescToAttrDef(op_desc, op_def_proto, not_dump_all);

  return true;
}

void ModelSerializeImp::OpDescIrDefToAttrDef(const ConstOpDescPtr &op_desc,
    google::protobuf::Map<std::string, ge::proto::AttrDef> *op_desc_attr) const {
  if (!op_desc->impl_->GetIRMeta().GetIrAttrNames().empty()) {
    proto::AttrDef ir_attr_names;
    for (const auto &item : op_desc->impl_->GetIRMeta().GetIrAttrNames()) {
      ir_attr_names.mutable_list()->add_s(item);
    }
    (*op_desc_attr)["_ir_attr_names"] = ir_attr_names;
  }
  if (!op_desc->impl_->GetIRMeta().GetIrInputs().empty()) {
    proto::AttrDef key;
    proto::AttrDef value;
    for (const auto &input : op_desc->impl_->GetIRMeta().GetIrInputs()) {
      key.mutable_list()->add_s(input.first);
      value.mutable_list()->add_i(static_cast<int64_t>(input.second));
    }
    (*op_desc_attr)["_ir_inputs_key"] = key;
    (*op_desc_attr)["_ir_inputs_value"] = value;
  }
}

void ModelSerializeImp::OpDescToAttrDef(const ConstOpDescPtr &op_desc, proto::OpDef *const op_def_proto,
                                        const bool not_dump_all) const {
  auto const op_desc_attr = op_def_proto->mutable_attr();
  if ((op_desc == nullptr) || (op_desc->impl_ == nullptr)) {
    GELOGE(FAILED, "[Check][Param] op desc or impl is nullptr.");
    return;
  }
  if (!op_desc->impl_->input_name_idx_.empty()) {
    proto::AttrDef key_in;
    proto::AttrDef value_in;
    for (auto &item : op_desc->impl_->input_name_idx_) {
      key_in.mutable_list()->add_s(item.first);
      value_in.mutable_list()->add_i(static_cast<int64_t>(item.second));
    }
    (void) op_desc_attr->insert({"_input_name_key", key_in});
    (void) op_desc_attr->insert({"_input_name_value", value_in});
  }
  if (!op_desc->impl_->output_name_idx_.empty()) {
    proto::AttrDef key_out;
    proto::AttrDef value_out;
    for (auto &item : op_desc->impl_->output_name_idx_) {
      key_out.mutable_list()->add_s(item.first);
      value_out.mutable_list()->add_i(static_cast<int64_t>(item.second));
    }
    (void) op_desc_attr->insert({"_output_name_key", key_out});
    (void) op_desc_attr->insert({"_output_name_value", value_out});
  }
  if (!op_desc->impl_->GetIRMeta().GetOptionalInputName().empty()) {
    proto::AttrDef opt_input;
    for (auto &item : op_desc->impl_->GetIRMeta().GetOptionalInputName()) {
      opt_input.mutable_list()->add_s(item);
    }
    (*op_desc_attr)["_opt_input"] = opt_input;
  }
  OpDescIrDefToAttrDef(op_desc, op_desc_attr);

  if (!SerializeAllAttrsFromAnyMap(op_desc->GetAllAttrs(), op_desc_attr)) {
    GELOGE(GRAPH_FAILED, "OpDesc [%s] attr serialize failed.", op_desc->GetName().c_str());
    return;
  }

  if (not_dump_all) {
    (void) op_desc_attr->erase(ATTR_NAME_FRAMEWORK_NODE_DEF);
    (void) op_desc_attr->erase(ATTR_NAME_FRAMEWORK_OP_DEF);
    (void) op_desc_attr->erase(ATTR_NAME_FRAMEWORK_FUNC_DEF);
    GE_IF_BOOL_EXEC(((op_def_proto->type() == CONSTANT) || (op_def_proto->type() == CONSTANTOP)),
                    (void) op_desc_attr->erase(ATTR_NAME_WEIGHTS));
  }
}

bool ModelSerializeImp::SerializeNode(const NodePtr &node, proto::OpDef *const op_def_proto,
                                      const bool not_dump_all) const {
  return SerializeNode(node, false, op_def_proto, not_dump_all);
}

bool ModelSerializeImp::SerializeNode(const NodePtr &node, const bool is_dump_graph, proto::OpDef *const op_def_proto,
                                      const bool not_dump_all) const {
  if ((node == nullptr) || (op_def_proto == nullptr)) {
    REPORT_INNER_ERROR("E18888", "param node or op_def_proto is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] param node or op_def_proto is nullptr, check invalid.");
    return false;
  }
  if (!SerializeOpDesc(node->GetOpDesc(), op_def_proto, not_dump_all)) {
    GELOGE(GRAPH_FAILED, "[Serialize][OpDesc] failed, node:%s", node->GetName().c_str());
    return false;
  }
  return SerializeEdge(node, op_def_proto, is_dump_graph);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ModelSerializeImp::SerializeGraph(const ConstComputeGraphPtr &graph,
    proto::GraphDef *const graph_proto, const bool not_dump_all) const {
  return SerializeGraph(graph, false, graph_proto, not_dump_all);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ModelSerializeImp::SerializeGraph(const ConstComputeGraphPtr &graph,
    const bool is_dump_graph, proto::GraphDef *const graph_proto, const bool not_dump_all) const {
  if ((graph == nullptr) || (graph_proto == nullptr)) {
    REPORT_INNER_ERROR("E18888", "param graph or graph_proto is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] param graph or graph_proto is nullptr, check invalid.");
    return false;
  }
  graph_proto->set_name(graph->GetName());
  // Inputs
  for (const auto &input : graph->GetInputNodes()) {
    if (input != nullptr) {
      graph_proto->add_input(input->GetName() + ":0");
    }
  }
  // Outputs
  for (const auto &output : graph->GetGraphOutNodesInfo()) {
    if (output.first != nullptr) {
      graph_proto->add_output(output.first->GetName() + ":" + std::to_string(output.second));
      GELOGI("Add output to graph proto, node name:%s, index:%d", output.first->GetName().c_str(), output.second);
    }
  }
  // ComputeGraph中的属性序列化
  if (!SerializeAllAttrsFromAnyMap(graph->GetAllAttrs(), graph_proto->mutable_attr())) {
    GELOGE(GRAPH_FAILED, "ComputeGraph [%s] serialize attr failed.", graph->GetName().c_str());
    return false;
  }

  for (const auto &node : graph->GetDirectNode()) {
    if (!SerializeNode(node, is_dump_graph, graph_proto->add_op(), not_dump_all)) {
      return false;
    }
  }
  return true;
}

bool ModelSerializeImp::SerializeModel(const Model &model, proto::ModelDef *const model_proto,
                                       const bool not_dump_all) const {
  return SerializeModel(model, false, model_proto, not_dump_all);
}

bool ModelSerializeImp::SerializeModel(const Model &model, const bool is_dump_graph, proto::ModelDef *const model_proto,
                                       const bool not_dump_all) const {
  if (model_proto == nullptr) {
    REPORT_INNER_ERROR("E18888", "param model_proto is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] param model_proto is nullptr, check Invalid");
    return false;
  }
  model_proto->set_name(model.GetName());
  model_proto->set_custom_version(model.GetPlatformVersion());
  model_proto->set_version(model.GetVersion());

  // Model属性序列化
  if (!SerializeAllAttrsFromAnyMap(model.GetAllAttrs(), model_proto->mutable_attr())) {
    GELOGE(GRAPH_FAILED, "Model [%s] serialize attr failed.", model.GetName().c_str());
    return false;
  }

  const auto compute_graph = model.graph_;
  if (compute_graph == nullptr) {
    REPORT_CALL_ERROR("E18888", "get compute graph from graph failed as graph is invalid.");
    GELOGE(GRAPH_FAILED, "[Get][ComputeGraph] return nullptr");
    return false;
  }
  if (!SerializeGraph(compute_graph, is_dump_graph, model_proto->add_graph(), not_dump_all)) {
    GELOGE(GRAPH_FAILED, "[Serialize][Graph] failed");
    return false;
  }

  const auto root_graph = GraphUtils::FindRootGraph(compute_graph);
  GE_RT_FALSE_CHECK_NOTNULL(root_graph);
  std::vector<std::shared_ptr<ComputeGraph>> subgraphs;
  if (compute_graph == root_graph) {
    subgraphs = compute_graph->GetAllSubgraphs();
  } else {
    GELOGD("[Serialize][Subgraph] compute_graph[%s] is not root graph[%s], get all subgraphs recursively",
           compute_graph->GetName().c_str(), root_graph->GetName().c_str());
    if (ge::GraphUtils::GetSubgraphsRecursively(compute_graph, subgraphs) != SUCCESS) {
      GELOGE(GRAPH_FAILED, "[Serialize][Subgraph] failed");
      return false;
    }
  }
  for (const auto &subgraph : subgraphs) {
    if (!SerializeGraph(subgraph, is_dump_graph, model_proto->add_graph(), not_dump_all)) {
      GELOGE(GRAPH_FAILED, "[Serialize][Subgraph] failed");
      return false;
    }
  }

  return true;
}

void ModelSerializeImp::AttrDefToOpDescIn(OpDescPtr &op_desc, std::vector<std::string> &key_in,
                                          std::vector<uint32_t> &value_in) const {
  if ((op_desc == nullptr) || (op_desc->impl_ == nullptr)) {
    GELOGE(FAILED, "[Serialize][Opdesc] op desc or impl is nullptr.");
    return;
  }
  if (!key_in.empty()) {
    if (key_in.size() != value_in.size()) {
      GELOGW("[ParseAttrDef][CheckParam] Input key and value vector size is different. key_size=%zu, value_size=%zu.",
             key_in.size(), value_in.size());
    } else {
      for (size_t i = 0UL; i < key_in.size(); ++i) {
        (void) op_desc->impl_->input_name_idx_.insert(std::pair<std::string, uint32_t>(key_in.at(i), value_in.at(i)));
      }
    }
  }
}

void ModelSerializeImp::AttrDefToOpDesc(OpDescPtr &op_desc, std::vector<std::string> &key_out,
                                        std::vector<uint32_t> &value_out,
                                        const std::vector<std::string> &opt_input) const {
  if ((op_desc == nullptr) || (op_desc->impl_ == nullptr)) {
    GELOGE(FAILED, "[Serialize][Opdesc] op desc or impl is nullptr.");
    return;
  }
  if (!key_out.empty()) {
    if (key_out.size() != value_out.size()) {
      GELOGW("[ParseAttrDef][CheckParam] Output key and value vector size is different. key_size=%zu, value_size=%zu.",
             key_out.size(), value_out.size());
    } else {
      for (size_t i = 0UL; i < key_out.size(); ++i) {
        (void)op_desc->impl_->output_name_idx_.insert(std::pair<std::string, uint32_t>(key_out.at(i), value_out.at(i)));
      }
    }
  }
  if (!opt_input.empty()) {
    for (const auto &i : opt_input) {
      (void) op_desc->impl_->MutableIRMeta().AddRegisterOptionalInputName(i);
    }
  }
}

void ModelSerializeImp::AttrDefToOpDescIrDef(OpDescPtr &op_desc, proto::OpDef &op_def_proto) const {
  if (op_def_proto.attr().count("_ir_attr_names") > 0UL) {
    const auto &name_list = op_def_proto.attr().at("_ir_attr_names").list();
    for (const auto &item_s : name_list.s()) {
      op_desc->impl_->MutableIRMeta().AppendIrAttrName(item_s);
    }
    (void) op_def_proto.mutable_attr()->erase("_ir_attr_names");
  }

  std::vector<std::string> keys;
  if (op_def_proto.attr().count("_ir_inputs_key") > 0UL) {
    const auto &key_list = op_def_proto.attr().at("_ir_inputs_key").list();
    for (const auto &key : key_list.s()) {
        keys.emplace_back(key);
    }
    (void) op_def_proto.mutable_attr()->erase("_ir_inputs_key");
  }
  std::vector<IrInputType> values;
  if (op_def_proto.attr().count("_ir_inputs_value") > 0UL) {
    const auto &value_list = op_def_proto.attr().at("_ir_inputs_value").list();
    for (const auto &value : value_list.i()) {
      if (value >= kIrInputTypeEnd) {
        GELOGW("[ParseAttrDef][CheckParam] ir inputs value[%" PRId64 "] is invalid, valid range is [%d-%d)",
               value, kIrInputRequired, kIrInputTypeEnd);
        return;
      }
      values.emplace_back(static_cast<IrInputType>(value));
    }
    (void) op_def_proto.mutable_attr()->erase("_ir_inputs_value");
  }
  if (keys.size() != values.size()) {
    GELOGW("[ParseAttrDef][CheckParam] ir inputs key and value vector size is different. key_size=%zu, value_size=%zu.",
           keys.size(), values.size());
    return;
  }
  for (size_t i = 0U; i < keys.size(); ++i) {
      op_desc->impl_->MutableIRMeta().AppendIrInput(std::move(keys[i]), values[i]);
  }
}

bool ModelSerializeImp::UnserializeOpDesc(OpDescPtr &op_desc, proto::OpDef &op_def_proto) const {
  std::vector<std::string> opt_input;
  std::vector<std::string> key_in;
  std::vector<uint32_t> value_in;
  std::vector<std::string> key_out;
  std::vector<uint32_t> value_out;

  ExtractMetaDataAttrIn(op_def_proto, opt_input, key_in, value_in);
  ExtractMetaDataAttr(op_def_proto, key_out, value_out);
  if (op_def_proto.type() == CONSTANT || op_def_proto.type() == CONSTANTOP) {
    if (!SetWeightForModel(op_def_proto)) {
      GELOGE(GRAPH_FAILED, "[Unserialize][Model] Set const weight failed");
      return false;
    }
  }

  op_desc = ComGraphMakeShared<OpDesc>(op_def_proto);
  GE_CHK_BOOL_EXEC(op_desc != nullptr, REPORT_CALL_ERROR("E18888", "create OpDesc failed.");
                   return false, "[Create][OpDesc] op_desc is nullptr.");
  GE_CHK_BOOL_EXEC(op_desc->impl_ != nullptr, REPORT_CALL_ERROR("E18888", "create OpDesc impl failed.");
                   return false, "[Create][OpDesc] op_desc impl is nullptr.");
  // Input tensor
  for (auto &input_desc : *op_def_proto.mutable_input_desc()) {
    const std::shared_ptr<GeTensorDesc> temp_value = ComGraphMakeShared<GeTensorDesc>(&input_desc);
    GE_CHK_BOOL_EXEC(temp_value != nullptr, REPORT_CALL_ERROR("E18888", "create GeTensorDesc failed.");
                     return false, "[Create][GeTensorDesc] temp_value is nullptr.");
    op_desc->impl_->inputs_desc_.push_back(temp_value);
  }
  // Output tensor
  for (auto &output_desc : *op_def_proto.mutable_output_desc()) {
    const std::shared_ptr<GeTensorDesc> temp_value = ComGraphMakeShared<GeTensorDesc>(&output_desc);
    GE_CHK_BOOL_EXEC(temp_value != nullptr, REPORT_CALL_ERROR("E18888", "create GeTensorDesc failed.");
                     return false, "[Create][GeTensorDesc] temp_value is nullptr.");
    op_desc->impl_->outputs_desc_.push_back(temp_value);
  }

  op_desc->SetId(op_def_proto.id());
  uint32_t graph_index = 0U;
  for (const std::string &name : op_def_proto.subgraph_name()) {
    if (!name.empty()) {
      (void) op_desc->AddSubgraphName(name);
      (void) op_desc->SetSubgraphInstanceName(graph_index++, name);
    }
  }

  // insert name index by key and value
  AttrDefToOpDescIn(op_desc, key_in, value_in);
  AttrDefToOpDesc(op_desc, key_out, value_out, opt_input);
  AttrDefToOpDescIrDef(op_desc, op_def_proto);
  if (!DeserializeAllAttrsToAttrHolder(op_def_proto.attr(), op_desc.get())) {
    GELOGE(GRAPH_FAILED, "Opdesc [%s] attr deserialize failed", op_def_proto.name().c_str());
    return false;
  }

  return true;
}

void ModelSerializeImp::ExtractMetaDataAttrIn(proto::OpDef &op_def_proto, std::vector<std::string> &opt_input,
                                              std::vector<std::string> &key_in, std::vector<uint32_t> &value_in) const {
  if (op_def_proto.attr().count("_opt_input") > 0UL) {
    const auto &name_list = op_def_proto.attr().at("_opt_input").list();
    for (const auto &item_s : name_list.s()) {
      opt_input.push_back(item_s);
    }
    (void) op_def_proto.mutable_attr()->erase("_opt_input");
  }
  if (op_def_proto.attr().count("_input_name_key") > 0UL) {
    const auto &output_name_key_list = op_def_proto.attr().at("_input_name_key").list();
    for (const auto &item_s : output_name_key_list.s()) {
      key_in.push_back(item_s);
    }
    (void) op_def_proto.mutable_attr()->erase("_input_name_key");
  }
  if (op_def_proto.attr().count("_input_name_value") > 0UL) {
    const auto &input_name_value_list = op_def_proto.attr().at("_input_name_value").list();
    for (const auto &item_i : input_name_value_list.i()) {
      value_in.push_back(static_cast<uint32_t>(item_i));
    }
    (void) op_def_proto.mutable_attr()->erase("_input_name_value");
  }
}

void ModelSerializeImp::ExtractMetaDataAttr(proto::OpDef &op_def_proto, std::vector<std::string> &key_out,
                                            std::vector<uint32_t> &value_out) const {
  if (op_def_proto.attr().count("_output_name_key") > 0UL) {
    const auto &output_name_key_list = op_def_proto.attr().at("_output_name_key").list();
    for (const auto &item_s : output_name_key_list.s()) {
      key_out.push_back(item_s);
    }
    (void) op_def_proto.mutable_attr()->erase("_output_name_key");
  }
  if (op_def_proto.attr().count("_output_name_value") > 0UL) {
    const auto &output_name_value_list = op_def_proto.attr().at("_output_name_value").list();
    for (const auto &item_i : output_name_value_list.i()) {
      value_out.push_back(static_cast<uint32_t>(item_i));
    }
    (void) op_def_proto.mutable_attr()->erase("_output_name_value");
  }
}

bool ModelSerializeImp::UnserializeNode(ComputeGraphPtr &graph, proto::OpDef &op_def_proto) {
  GE_RT_FALSE_CHECK_NOTNULL(graph);
  OpDescPtr op_desc = nullptr;
  if (!UnserializeOpDesc(op_desc, op_def_proto)) {
    GELOGE(ge::INTERNAL_ERROR, "[Unserialize][OpDesc] error.");
    return false;
  }

  const NodePtr node = graph->AddNode(op_desc, op_desc->GetId());
  GE_CHK_BOOL_EXEC(node != nullptr,
                   REPORT_CALL_ERROR("E18888", "add node to graph:%s failed", graph->GetName().c_str());
                   return false, "[Add][Node] to graph:%s failed.", graph->GetName().c_str());

  std::vector<int32_t> src_out_peer_index;
  if (op_def_proto.attr().count(kSrcOutPeerIndex) > 0UL) {
    const auto &src_out_peer_index_list = op_def_proto.attr().at(kSrcOutPeerIndex).list();
    for (const auto &item_i : src_out_peer_index_list.i()) {
      src_out_peer_index.push_back(static_cast<int32_t>(item_i));
    }
    (void)op_def_proto.mutable_attr()->erase(kSrcOutPeerIndex);
  }

  // Inputs
  int32_t dst_index = 0;
  int32_t cur_index = 0;
  const size_t input_size = op_def_proto.input().size();
  for (const auto &input : op_def_proto.input()) {
    std::string node_name;
    int32_t index = 0;
    if (ParseNodeIndex(input, node_name, index)) {
      int32_t peer_index = static_cast<int32_t>(kInvalidIndex);
      if (src_out_peer_index.size() == input_size) {
        peer_index = src_out_peer_index[cur_index];
      }
      node_input_node_names_.push_back(
          NodeNameNodeReq{node_name, index, peer_index, node, dst_index, op_def_proto.name()});
    }
    if (index >= 0) {
      dst_index++;
    }
    ++cur_index;
  }
  node_map_[op_def_proto.name()] = node;
  return true;
}

void ModelSerializeImp::SaveEdgeInfo(const AnchorPtr &src_anchor, const AnchorPtr &dst_anchor,
                                     const int64_t src_out_peer_index, const int64_t cur_index,
                                     std::unordered_map<AnchorPtr, DstAnchors> &edges) const {
  // old version would be -1
  if (src_out_peer_index >= 0) {
    edges[src_anchor].emplace(dst_anchor, src_out_peer_index);
  } else {
    edges[src_anchor].emplace(dst_anchor, cur_index);
  }
}

bool ModelSerializeImp::LinkEdges(const std::unordered_map<AnchorPtr, DstAnchors> &edges) const {
  for (const auto &edge : edges) {
    for (const auto &out_anchor_index : edge.second) {
      GE_ASSERT_SUCCESS(GraphUtils::AddEdge(edge.first, out_anchor_index.first));
    }
  }
  return true;
}

bool ModelSerializeImp::HandleNodeNameRef() {
  // Edges
  std::unordered_map<AnchorPtr, DstAnchors> edges;
  int64_t cur_index = 0;
  for (auto &item : node_input_node_names_) {
    ++cur_index;
    const auto src_node_it = node_map_.find(item.src_node_name);
    GE_ASSERT_TRUE(src_node_it != node_map_.end());
    GE_IF_BOOL_EXEC((src_node_it->second == nullptr) || (item.dst_node == nullptr), continue);
    if (item.src_out_index >= 0) {
      const auto src_anchor = src_node_it->second->GetOutDataAnchor(item.src_out_index);
      const auto dst_anchor = item.dst_node->GetInDataAnchor(item.dst_in_index);
      GE_ASSERT_NOTNULL(src_anchor);
      GE_ASSERT_NOTNULL(dst_anchor);
      SaveEdgeInfo(src_anchor, dst_anchor, item.src_out_peer_index, cur_index, edges);
    } else {
      // Control edge
      const auto src_anchor = src_node_it->second->GetOutControlAnchor();
      const auto dst_anchor = item.dst_node->GetInControlAnchor();
      if ((src_anchor != nullptr) && (dst_anchor != nullptr)) {
        SaveEdgeInfo(src_anchor, dst_anchor, item.src_out_peer_index, cur_index, edges);
      }
    }
  }
  GE_ASSERT_TRUE(LinkEdges(edges));
  // Graph input
  for (auto &item : graph_input_node_names_) {
    const std::map<std::string, ge::NodePtr>::const_iterator node_it = node_map_.find(item.node_name);
    GE_ASSERT_TRUE(node_it != node_map_.cend());
    GE_IF_BOOL_EXEC(item.graph == nullptr, continue);
    GE_ASSERT_NOTNULL(item.graph->AddInputNode(node_it->second));
  }
  // Graph output
  for (auto &item : graph_output_node_names_) {
    const std::map<std::string, ge::NodePtr>::const_iterator node_it = node_map_.find(item.node_name);
    GE_ASSERT_TRUE(node_it != node_map_.cend());

    GE_IF_BOOL_EXEC(item.graph == nullptr, continue);
    const auto ret = item.graph->AddOutputNodeByIndex(node_it->second, item.index);
    GELOGI("node name:%s, item.index:%d", node_it->second->GetName().c_str(), item.index);
    GE_ASSERT_NOTNULL(ret);
  }
  node_input_node_names_.clear();
  graph_input_node_names_.clear();
  graph_output_node_names_.clear();
  node_map_.clear();
  return true;
}

bool ModelSerializeImp::RebuildOwnership(ComputeGraphPtr &compute_graph,
                                         std::map<std::string, ComputeGraphPtr> &subgraphs) const {
  std::queue<ComputeGraphPtr> all_graphs;
  all_graphs.emplace(compute_graph);
  while (!all_graphs.empty()) {
    const ComputeGraphPtr graph = all_graphs.front();
    all_graphs.pop();

    for (const NodePtr &node : graph->GetDirectNode()) {
      const OpDescPtr op_desc = node->GetOpDesc();
      for (const std::string &name : op_desc->GetSubgraphInstanceNames()) {
        if (name.empty()) {
          continue;
        }
        const auto it = subgraphs.find(name);
        if (it == subgraphs.end()) {
          REPORT_INNER_ERROR("E18888", "Node:%s, Subgraph:%s not found, num:%zu.",
                             op_desc->GetName().c_str(), name.c_str(), subgraphs.size());
          GELOGE(GRAPH_FAILED, "[Check][Param] Node:%s, Subgraph:%s not found, num:%zu.",
                 op_desc->GetName().c_str(), name.c_str(), subgraphs.size());
          return false;
        }

        ComputeGraphPtr &subgraph = it->second;
        subgraph->SetParentGraph(graph);
        subgraph->SetParentNode(node);
        (void) compute_graph->AddSubgraph(subgraph->GetName(), subgraph);
        all_graphs.emplace(subgraph);
      }
    }
  }

  return true;
}

bool ModelSerializeImp::UnserializeModel(Model &model, proto::ModelDef &model_proto) {
  model.name_ = model_proto.name();
  model.version_ = model_proto.version();
  model.platform_version_ = model_proto.custom_version();

  // Model属性反序列化
  if (!DeserializeAllAttrsToAttrHolder(model_proto.attr(), &model)) {
    GELOGE(GRAPH_FAILED, "Model [%s] deserialize attr failed.", model.GetName().c_str());
    return false;
  }

  auto &graphs_proto = *model_proto.mutable_graph();
  if (!graphs_proto.empty()) {
    auto &graph_proto = graphs_proto[0];
    ComputeGraphPtr compute_graph_ptr;
    if (!UnserializeGraphWithoutEdge(compute_graph_ptr, graph_proto)) {
      GELOGE(GRAPH_FAILED, "[Call][UnserializeGraphWithoutEdge] failed");
      return false;
    }
    model.graph_ = compute_graph_ptr;
    // 0 is main graph, following is subgraph.
    std::map<std::string, ComputeGraphPtr> subgraphs;
    for (auto idx = 1; idx < graphs_proto.size(); ++idx) {
      ComputeGraphPtr subgraph;
      ModelSerializeImp impl;
      impl.SetAirModelPath(air_path_);
      if (!impl.UnserializeGraphWithoutEdge(subgraph, graphs_proto[idx])) {
        GELOGE(GRAPH_FAILED, "[Call][UnserializeGraphWithoutEdge] failed");
        return false;
      }

      if (!impl.HandleNodeNameRef()) {
        GELOGE(GRAPH_FAILED, "[Call][HandleNodeNameRef] failed");
        return false;
      }

      subgraphs[subgraph->GetName()] = subgraph;
    }

    if (!subgraphs.empty()) {
      if (!RebuildOwnership(compute_graph_ptr, subgraphs)) {
        GELOGE(GRAPH_FAILED, "[Rebuild][GraphOwnerShip] failed");
        return false;
      }
    }
  }

  if (!HandleNodeNameRef()) {
    GELOGE(GRAPH_FAILED, "[Call][HandleNodeNameRef] failed");
    return false;
  }
  return true;
}

bool ModelSerializeImp::UnserializeGraphWithoutEdge(ComputeGraphPtr &graph, proto::GraphDef &graph_proto) {
  graph = ComGraphMakeShared<ComputeGraph>(graph_proto.name());
  if ((graph == nullptr) || (graph->impl_ == nullptr)) {
    REPORT_CALL_ERROR("E18888", "create ComputeGraph failed.");
    GELOGE(GRAPH_FAILED, "[Create][ComputeGraph] ComputeGraph make shared failed");
    return false;
  }

  // Inputs
  for (const auto &input : graph_proto.input()) {
    std::string node_name;
    int32_t index;
    if (ParseNodeIndex(input, node_name, index)) {
      graph_input_node_names_.push_back(NodeNameGraphReq{node_name, index, graph});
    }
  }
  // Outputs
  for (const auto &output : graph_proto.output()) {
    std::string node_name;
    int32_t index;
    if (ParseNodeIndex(output, node_name, index)) {
      graph_output_node_names_.push_back(NodeNameGraphReq{node_name, index, graph});
    }
  }
  // ComputeGraph 属性反序列化
  if (!DeserializeAllAttrsToAttrHolder(graph_proto.attr(), graph.get())) {
    GELOGE(GRAPH_FAILED, "ComputeGraph [%s] deserialize attr failed.", graph->GetName().c_str());
    return false;
  }

  for (auto &op_def_proto : *graph_proto.mutable_op()) {
    if (!UnserializeNode(graph, op_def_proto)) {
      GELOGE(GRAPH_FAILED, "[Unserialize][Node] failed");
      return false;
    }
  }
  return true;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ModelSerializeImp::UnserializeGraph(ComputeGraphPtr &graph,
                                                                                        proto::GraphDef &graph_proto) {
  if (!UnserializeGraphWithoutEdge(graph, graph_proto)) {
    GELOGW("[Deserialize][Graph] Deserialize graph without edges failed");
  }
  if (!HandleNodeNameRef()) {
    GELOGE(GRAPH_FAILED, "[Call][HandleNodeNameRef] Link Anchor or set graph input or output fail");
    return false;
  }
  return true;
}

static bool ReadProtoFromBinaryFile(const uint8_t *const data, const size_t len,
                                    google::protobuf::Message *const proto) {
  GE_CHK_BOOL_EXEC(data != nullptr, REPORT_INNER_ERROR("E18888", "param data is nullptr, check invalid.");
                   return false, "[Check][Param] data is null.");
  GE_CHK_BOOL_EXEC(proto != nullptr, REPORT_INNER_ERROR("E18888", "param proto is nullptr, check invalid.");
                   return false, "[Check][Param] proto is null.");

  google::protobuf::io::CodedInputStream coded_stream(data, static_cast<int32_t>(len));
  // 2048M -1
  coded_stream.SetTotalBytesLimit(INT32_MAX);
  if (!proto->ParseFromCodedStream(&coded_stream)) {
    REPORT_CALL_ERROR("E18888", "Read proto from BinaryFile failed, len %zu", len);
    GELOGE(GRAPH_FAILED, "[Read][Proto] from BinaryFile failed, len %zu", len);
    return false;
  }

  return true;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ModelSerializeImp::SerializeAllAttrsFromAnyMap(
    const std::map<std::string, AnyValue> &attr_map,
    google::protobuf::Map<std::string, ::ge::proto::AttrDef> *const mutable_attr) {
  if (mutable_attr == nullptr) {
    GELOGE(GRAPH_FAILED, "mutable_attr is nullptr.");
    return false;
  }

  for (const auto &attr : attr_map) {
    const AnyValue attr_value = attr.second;
    const auto value_serializer = AttrSerializerRegistry::GetInstance().GetSerializer(attr_value.GetValueTypeId());
    if (value_serializer == nullptr) {
      GELOGE(GRAPH_FAILED, "Get serialized failed,name:[%s] value type:%u.",
             attr.first.c_str(), attr_value.GetValueType());
      return false;
    }
    proto::AttrDef attr_def;
    if (value_serializer->Serialize(attr_value, attr_def) != GRAPH_SUCCESS) {
      GELOGE(GRAPH_FAILED, "Attr serialized failed, name:[%s].", attr.first.c_str());
      return false;
    }
    (*mutable_attr)[attr.first] = attr_def;
  }
  return true;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool ModelSerializeImp::DeserializeAllAttrsToAttrHolder(
    const google::protobuf::Map<std::string, ::ge::proto::AttrDef> &proto_attr_map, AttrHolder *const attr_holder) {
  if (attr_holder == nullptr) {
    return false;
  }
  for (const auto &iter : proto_attr_map) {
    // skip not set attribute
    if ((iter.second.value_case() == proto::AttrDef::VALUE_NOT_SET) ||
        ((iter.second.value_case() == proto::AttrDef::kList) &&
            (iter.second.list().val_type() == ge::proto::AttrDef::ListValue::VT_LIST_NONE))) {
      continue;
    }

    const auto deserializer =
        AttrSerializerRegistry::GetInstance().GetDeserializer(iter.second.value_case());
    if (deserializer == nullptr) {
      GELOGE(GRAPH_FAILED, "Get deserialize failed, attr type:[%d].", static_cast<int32_t>(iter.second.value_case()));
      return false;
    }
    AnyValue attr_value;
    if (deserializer->Deserialize(iter.second, attr_value) != GRAPH_SUCCESS) {
      GELOGE(FAILED, "Attr deserialized failed, name:[%s].", iter.first.c_str());
      return false;
    }

    if (attr_holder->SetAttr(iter.first, attr_value) != GRAPH_SUCCESS) {
      GELOGE(GRAPH_FAILED, "Set attr [%s] failed.", iter.first.c_str());
      return false;
    }
  }
  return true;
}

bool ModelSerializeImp::SeparateModelDef(Buffer &buffer, const std::string &path,
                                         proto::ModelDef &model_def, const bool is_need_separate) const {
  if (SerializeToBuffer(model_def, buffer)) {
    return true;
  }
  if (!is_need_separate) {
    GELOGE(GRAPH_FAILED, "[Serialize][Model] Model is larger than 2G, "
           "but can not separate in this scenario, you can use external_weight instead");
    return false;
  }
  GELOGW("[Serialize][Model] Model is larger than 2G, need separate");
  uint64_t constant_op_id = 0UL;
  for (auto &graph_def : *model_def.mutable_graph()) {
    for (auto &op_def : *graph_def.mutable_op()) {
      if (op_def.type() != CONSTANT && op_def.type() != CONSTANTOP) {
        continue;
      }
      auto attr_map = op_def.mutable_attr();
      auto iter = attr_map->find(ATTR_NAME_WEIGHTS);
      if (iter == attr_map->end()) {
        GELOGE(GRAPH_FAILED, "Find attr [%s] of op[%s] failed.", ATTR_NAME_WEIGHTS.c_str(), op_def.name().c_str());
        return false;
      }
      auto tensor_def = iter->second.mutable_t();
      if (tensor_def->data().empty()) {
        GELOGW("Weight attr of node: %s is empty", op_def.name().c_str());
        continue;
      }
      std::string dir_path;
      std::string file_name;
      SplitFilePath(path, dir_path, file_name);
      std::string model_name = GetRegulatedName(model_def.name());
      if (!dir_path.empty()) {
        dir_path += "/";
      }
      if (!model_name.empty()) {
        model_name += "/";
      }
      std::string file_path = kTmpWeight + model_name +
                              op_def.type() + "_" + std::to_string(constant_op_id) + "_file";
      std::string real_file_path = dir_path + file_path;
      constant_op_id++;
      if (SaveBinToFile(tensor_def->data().c_str(), tensor_def->data().length(), real_file_path) != GRAPH_SUCCESS) {
        GELOGE(GRAPH_FAILED, "Write data of attr [%s] of op[%s] to file failed.",
               ATTR_NAME_WEIGHTS.c_str(), op_def.name().c_str());
        return false;
      }
      int64_t length = static_cast<int64_t>(tensor_def->data().length());
      proto::AttrDef file_attr;
      file_attr.set_s(file_path);
      attr_map->insert({ATTR_NAME_LOCATION, file_attr});
      proto::AttrDef length_attr;
      length_attr.set_i(length);
      attr_map->insert({ATTR_NAME_LENGTH, length_attr});
      tensor_def->set_data("");
    }
  }
#if !defined(__ANDROID__) && !defined(ANDROID)
  Buffer temp_buffer(model_def.ByteSizeLong());
#else
  Buffer temp_buffer(model_def.ByteSize());
#endif
  buffer = temp_buffer;
  return SerializeToBuffer(model_def, buffer);
}

bool ModelSerializeImp::SerializeToBuffer(const proto::ModelDef &model_def, Buffer &buffer) const {
  google::protobuf::io::ArrayOutputStream array_stream(buffer.GetData(), static_cast<int32_t>(buffer.GetSize()));
  google::protobuf::io::CodedOutputStream output_stream(&array_stream);
  output_stream.SetSerializationDeterministic(true);
  return model_def.SerializeToCodedStream(&output_stream);
}

Buffer ModelSerialize::SerializeModel(const Model &model, const bool not_dump_all) const {
  std::string path;
  return SerializeModel(model, path, true, not_dump_all);
}

Buffer ModelSerialize::SerializeModel(const Model &model, const std::string &path,
                                      const bool is_need_separate, const bool not_dump_all) const {
  proto::ModelDef model_def;
  ModelSerializeImp model_imp;
  if (!model_imp.SerializeModel(model, &model_def, not_dump_all)) {
    return Buffer();
  }
#if !defined(__ANDROID__) && !defined(ANDROID)
  Buffer buffer(model_def.ByteSizeLong());
#else
  Buffer buffer(model_def.ByteSize());
#endif
  GE_CHK_BOOL_ONLY_LOG(buffer.GetSize() != 0UL, "get size failed");
  GE_CHK_BOOL_ONLY_LOG((buffer.GetData() != nullptr), "get size failed");
  if (!model_imp.SeparateModelDef(buffer, path, model_def, is_need_separate)) {
    GELOGW("[Serialize][Model] Serialize to binary failed");
    return Buffer();
  }
  return buffer;
}

Status ModelSerialize::SerializeModel(const Model &model, const bool not_dump_all, proto::ModelDef &model_def) const {
  ModelSerializeImp model_imp;
  if (!model_imp.SerializeModel(model, true, &model_def, not_dump_all)) {
    return FAILED;
  }
  return SUCCESS;
}

bool ModelSerializeImp::LoadWeightFromFile(const std::string &file_path,
                                           const int64_t &length,
                                           std::string &weight) const {
  if (length <= 0L) {
    GELOGE(GRAPH_FAILED, "Value length is less than 0.");
    return false;
  }
  auto bin_data = std::unique_ptr<char_t[]>(new(std::nothrow) char_t[length]);
  if (bin_data == nullptr) {
    GELOGE(FAILED, "[Allocate][Mem]Allocate mem failed");
    return false;
  }
  std::string air_directory;
  std::string air_filename;
  SplitFilePath(air_path_, air_directory, air_filename);
  std::string weight_path;
  if (!air_directory.empty()) {
    weight_path = air_directory + "/" + file_path;
  } else {
    weight_path = file_path;
  }
  size_t data_len = static_cast<size_t>(length);
  if (GetBinFromFile(weight_path, static_cast<char_t *>(bin_data.get()), data_len) != GRAPH_SUCCESS) {
    GELOGE(GRAPH_FAILED, "Get bin from file failed.");
    return false;
  }
  weight = std::string(bin_data.get(), length);
  return true;
}

bool ModelSerializeImp::SetWeightForModel(proto::OpDef &op_def) const {
  auto attr_map = op_def.mutable_attr();
  auto iter = attr_map->find(ATTR_NAME_LOCATION);
  if (iter == attr_map->end()) {
    return true;
  }
  const std::string file_path = iter->second.s();
  iter = attr_map->find(ATTR_NAME_LENGTH);
  if (iter == attr_map->end()) {
    return true;
  }
  const int64_t length = iter->second.i();
  std::string weight;
  if (!LoadWeightFromFile(file_path, length, weight)) {
    GELOGE(GRAPH_FAILED, "Load weight from path %s failed.", file_path.c_str());
    return false;
  }
  iter = attr_map->find(ATTR_NAME_WEIGHTS);
  if (iter == attr_map->end()) {
    GELOGE(GRAPH_FAILED, "find attr [%s] of op[%s] failed.", ATTR_NAME_WEIGHTS.c_str(), op_def.name().c_str());
    return false;
  }
  iter->second.mutable_t()->set_data(weight);
  attr_map->erase(ATTR_NAME_LOCATION);
  attr_map->erase(ATTR_NAME_LENGTH);
  return true;
}

bool ModelSerialize::UnserializeModel(const uint8_t *const data, const size_t len, Model &model) const {
  if (data == nullptr) {
    REPORT_INNER_ERROR("E18888", "param data is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] data is nullptr");
    return false;
  }

  std::shared_ptr<proto::ModelDef> model_proto_ptr;
  model_proto_ptr = ComGraphMakeShared<proto::ModelDef>();
  if (model_proto_ptr == nullptr) {
    REPORT_CALL_ERROR("E18888", "create ModelDef failed.");
    GELOGE(GRAPH_FAILED, "[Create][ModelDef] proto::ModelDef make shared failed");
    return false;
  }

  auto &model_proto = *model_proto_ptr;
  if (!ReadProtoFromBinaryFile(data, len, &model_proto)) {
    GELOGE(GRAPH_FAILED, "[Read][Proto] from binaryfile failed.");
    return false;
  }

  ModelSerializeImp model_imp;
  model_imp.SetProtobufOwner(model_proto_ptr);
  if (!model_imp.UnserializeModel(model, model_proto)) {
    GELOGE(GRAPH_FAILED, "[Unserialize][Model] failed");
    return false;
  }
  return model.IsValid();
}

bool ModelSerialize::UnserializeModel(ge::proto::ModelDef &model_def, Model &model) const {
  std::string path;
  return UnserializeModel(model_def, model, path);
}

bool ModelSerialize::UnserializeModel(ge::proto::ModelDef &model_def, Model &model, const std::string &path) const {
  const std::shared_ptr<proto::ModelDef> model_def_ptr = ComGraphMakeShared<proto::ModelDef>(model_def);
  GE_CHK_BOOL_EXEC(model_def_ptr != nullptr, REPORT_CALL_ERROR("E18888", "create ModelDef failed.");
                   return false, "[Create][ModelDef] mode_def make shared failed");

  ModelSerializeImp model_imp;
  model_imp.SetAirModelPath(path);
  model_imp.SetProtobufOwner(model_def_ptr);
  if (!model_imp.UnserializeModel(model, *model_def_ptr)) {
    GELOGE(GRAPH_FAILED, "[Unserialize][Model] fail");
    return false;
  }
  return model.IsValid();
}
}  // namespace ge
