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

#include "graph/utils/tensorboard_graph_dump_utils.h"

#include <sys/types.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>

#include "graph/utils/graph_utils.h"
#include "graph/utils/graph_utils_inner.h"
#include "inc/common/checker.h"
#include "graph/debug/ge_util.h"
#include "mmpa/mmpa_api.h"
#include "proto/tensorflow/graph.pb.h"
#include "proto/tensorflow/node_def.pb.h"
#include "proto/tensorflow/attr_value.pb.h"

namespace ge {
namespace {
void ConvertNodeInput(const ge::NodePtr &node, domi::tensorflow::NodeDef &node_def) {
  // set data input
  for (const auto &anchor : node->GetAllInDataAnchors()) {
    auto peer_out_anchor = anchor->GetPeerOutAnchor();
    if (peer_out_anchor == nullptr) {
      continue;
    }

    const auto &peer_out_node = peer_out_anchor->GetOwnerNode();
    std::string value = peer_out_node->GetName() + ":" + std::to_string(peer_out_anchor->GetIdx());
    node_def.add_input(value);
  }

  // set control input
  const auto &in_control_anchor = node->GetInControlAnchor();
  if (in_control_anchor != nullptr) {
    for (const auto &peer_out_control_anchor : in_control_anchor->GetPeerOutControlAnchors()) {
      // no need to check null ptr for peer_out_control_anchor
      const auto &peer_out_node = peer_out_control_anchor->GetOwnerNode();
      std::string value = "^" + peer_out_node->GetName();
      node_def.add_input(value);
    }
  }
}

graphStatus ConvertAttr(const std::map<std::string, AnyValue> &attr_map, domi::tensorflow::NodeDef &node_def) {
  for (const auto &attr : attr_map) {
    auto type = attr.second.GetValueType();
    ::domi::tensorflow::AttrValue attr_value;
    switch (type) {
      case ge::AnyValue::ValueType::VT_STRING: {
        std::string value;
        GE_ASSERT_GRAPH_SUCCESS(attr.second.GetValue(value), "[Get][AttrValue] for attr:%s failed.",
                                attr.first.c_str());
        attr_value.set_s(value);
        break;
      }

      case ge::AnyValue::ValueType::VT_FLOAT: {
        float value;
        GE_ASSERT_GRAPH_SUCCESS(attr.second.GetValue(value), "[Get][AttrValue] for attr:%s failed.",
                                attr.first.c_str());
        attr_value.set_f(value);
        break;
      }

      case ge::AnyValue::ValueType::VT_BOOL: {
        bool value;
        GE_ASSERT_GRAPH_SUCCESS(attr.second.GetValue(value), "[Get][AttrValue] for attr:%s failed.",
                                attr.first.c_str());
        attr_value.set_b(value);
        break;
      }

      case ge::AnyValue::ValueType::VT_INT: {
        int64_t value;
        GE_ASSERT_GRAPH_SUCCESS(attr.second.GetValue(value), "[Get][AttrValue] for attr:%s failed.",
                                attr.first.c_str());
        attr_value.set_i(value);
        break;
      }

      case ge::AnyValue::ValueType::VT_LIST_STRING: {
        std::vector<std::string> list_value;
        GE_ASSERT_GRAPH_SUCCESS(attr.second.GetValue(list_value), "[Get][AttrValue] for attr:%s failed.",
                                attr.first.c_str());
        for (const auto &value : list_value) {
          attr_value.mutable_list()->add_s(value);
        }
        break;
      }

      case ge::AnyValue::ValueType::VT_LIST_FLOAT: {
        std::vector<float> list_value;
        GE_ASSERT_GRAPH_SUCCESS(attr.second.GetValue(list_value), "[Get][AttrValue] for attr:%s failed.",
                                attr.first.c_str());
        for (const auto &value : list_value) {
          attr_value.mutable_list()->add_f(value);
        }
        break;
      }

      case ge::AnyValue::ValueType::VT_LIST_BOOL: {
        std::vector<bool> list_value;
        GE_ASSERT_GRAPH_SUCCESS(attr.second.GetValue(list_value), "[Get][AttrValue] for attr:%s failed.",
                                attr.first.c_str());
        for (const auto &value : list_value) {
          attr_value.mutable_list()->add_b(value);
        }
        break;
      }

      case ge::AnyValue::ValueType::VT_LIST_INT: {
        std::vector<int64_t> list_value;
        GE_ASSERT_GRAPH_SUCCESS(attr.second.GetValue(list_value), "[Get][AttrValue] for attr:%s failed.",
                                attr.first.c_str());
        for (const auto &value : list_value) {
          attr_value.mutable_list()->add_i(value);
        }
        break;
      }

      default:
        GELOGW("attr:%s with value type:%d is not supported", attr.first.c_str(), type);
        break;
    }

    (*node_def.mutable_attr())[attr.first] = attr_value;
  }

  return GRAPH_SUCCESS;
}

graphStatus ConvertSubgraphAttr(const ge::NodePtr &node, domi::tensorflow::NodeDef &node_def) {
  const auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);

  const auto &subgraph_instance_names = op_desc->GetSubgraphInstanceNames();
  for (const auto &subgraph_instance_name : subgraph_instance_names) {
    std::string subgraph_name;
    GE_ASSERT_GRAPH_SUCCESS(op_desc->GetSubgraphNameByInstanceName(subgraph_instance_name, subgraph_name),
                            "[Get][SubGraphName] for Graph:%s failed.", subgraph_instance_name.c_str());
    ::domi::tensorflow::AttrValue subgraph_attr;
    subgraph_attr.set_s(subgraph_instance_name);
    (*node_def.mutable_attr())[subgraph_name] = subgraph_attr;
  }

  return GRAPH_SUCCESS;
}

graphStatus ConvertNode(const ge::NodePtr &node, domi::tensorflow::NodeDef &node_def) {
  node_def.set_op(node->GetType());
  node_def.set_name(node->GetName());

  // set input
  ConvertNodeInput(node, node_def);

  // set attr
  auto attr_maps = node->GetOpDesc()->GetAllAttrs();
  GE_ASSERT_GRAPH_SUCCESS(ConvertAttr(attr_maps, node_def), "ConvertAttr for node:%s failed.", node->GetName().c_str());

  // set subgraph attr
  GE_ASSERT_GRAPH_SUCCESS(ConvertSubgraphAttr(node, node_def), "ConvertSubgraphAttr for node:%s failed.",
                          node->GetName().c_str());

  return GRAPH_SUCCESS;
}

void ConvertComputeGraphToGraphDef(const ge::ComputeGraph &compute_graph, domi::tensorflow::GraphDef &graph_def) {
  for (const auto &node : compute_graph.GetDirectNode()) {
    auto *node_def = graph_def.add_node();
    if (ConvertNode(node, *node_def) != GRAPH_SUCCESS) {
      continue;
    }
  }

  for (const auto &subgraph : compute_graph.GetAllSubgraphs()) {
    auto *subgraph_def = graph_def.mutable_library()->add_function();
    subgraph_def->mutable_signature()->set_name(subgraph->GetName());
    for (const auto &node : subgraph->GetDirectNode()) {
      auto *node_def = subgraph_def->add_node_def();
      if (ConvertNode(node, *node_def) != GRAPH_SUCCESS) {
        continue;
      }
    }
  }
}
}  // namespace

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
TensorBoardGraphDumpUtils::DumpGraph(const ge::ComputeGraph &compute_graph, const std::string &suffix) {
  char_t dump_ge_graph[MMPA_MAX_PATH] = {'\0'};
  const INT32 res = mmGetEnv(kDumpGeGraph, &(dump_ge_graph[0]), static_cast<uint32_t>(MMPA_MAX_PATH));
  const ge::DumpLevel dump_ge_graph_level = (res == EN_OK)
      ? static_cast<ge::DumpLevel>(std::strtol(&(dump_ge_graph[0U]), nullptr, kBaseOfIntegerValue))
      : DumpLevel::NO_DUMP;
  if ((dump_ge_graph_level == DumpLevel::NO_DUMP) || (dump_ge_graph_level >= DumpLevel::DUMP_LEVEL_END)) {
    GELOGD("Skip TensorBoard DumpGraph with dump_ge_graph_level %d.", dump_ge_graph_level);
    return GRAPH_NOT_CHANGED;
  }

  // 1.Get graph def from compute graph
  domi::tensorflow::GraphDef graph_def;
  ConvertComputeGraphToGraphDef(compute_graph, graph_def);

  // 2.Set file name
  static std::atomic<int64_t> atomic_file_index(0);
  const auto file_index = atomic_file_index.fetch_add(1);
  GELOGD("Start to dump ge tensorboard file: %ld", file_index);

  std::stringstream file_name_stream;
  GetDumpGraphPrefix(file_name_stream);
  if (mmAccess2(file_name_stream.str().c_str(), M_F_OK) != EN_OK) {
    const int32_t ret = CreateDirectory(file_name_stream.str());
    if (ret != 0) {
      GELOGW("[DumpGraph][CreateDirectory] Create dump graph dir failed, path:%s", file_name_stream.str().c_str());
      file_name_stream.str("");
      file_name_stream << "./";
    }
  }

  file_name_stream << "ge_tensorboard_" << std::setw(kDumpGraphIndexWidth) << std::setfill('0') << file_index;
  file_name_stream << "_graph_" << compute_graph.GetGraphID();
  file_name_stream << "_" << suffix << ".pbtxt";
  const std::string file_name = file_name_stream.str();
  if ((file_name.length()) >= kMaxFileNameLen) {
    GELOGE(GRAPH_FAILED, "[Check][Param] File name is too longer!, file:%s", file_name.c_str());
    return GRAPH_FAILED;
  }
  const auto real_path = ComGraphMakeUnique<char_t[]>(static_cast<size_t>(MMPA_MAX_PATH));
  GE_CHECK_NOTNULL(real_path);

  /// Returning nullptr means 3 case as follows:
  /// a.path is MMPA_MAX_PATH chars or more
  /// b.the file does not exist
  /// c.the path has no permissions
  /// Distinguish between last the two cases in the function WriteProtoToTextFile call open()
  mmRealPath(nullptr, nullptr, 0);
  if (mmRealPath(file_name.c_str(), real_path.get(), MMPA_MAX_PATH) != EN_OK) {
    // Case a has been checked above
    GELOGI("File %s does not exist, it will be created.", file_name.c_str());
  }

  // 3. Serialize to file in current path
  GraphUtils::WriteProtoToTextFile(graph_def, real_path.get());

  return GRAPH_SUCCESS;
}
}  // namespace ge
