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

#include "graph/ir_definitions_recover.h"
#include <cinttypes>
#include <ostream>
#include <sstream>
#include "graph/operator_factory.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/debug/ge_op_types.h"
#include "framework/common/debug/ge_log.h"

namespace {
struct IrDefinition {
  bool inited;
  bool has_ir_definition;
  std::vector<std::string> attr_names;
  std::map<std::string, ge::AnyValue> attr_value;
  std::vector<std::pair<std::string, ge::IrInputType>> inputs;
};
void InitIrDefinitionsIfNeed(const std::string &op_type, IrDefinition &ir_def) {
  if (!ir_def.inited) {
    auto op = ge::OperatorFactory::CreateOperator("temp", op_type.c_str());
    op.BreakConnect();
    auto op_desc = ge::OpDescUtils::GetOpDescFromOperator(op);
    if (op_desc == nullptr) {
      GELOGW("Failed to construct operator from type %s", op_type.c_str());
      ir_def.has_ir_definition = false;
      ir_def.inited = true;
      return;
    }
    ir_def.attr_names = op_desc->GetIrAttrNames();
    ir_def.inputs = op_desc->GetIrInputs();
    ir_def.attr_value = ge::AttrUtils::GetAllAttrs(op_desc);
    ir_def.has_ir_definition = true;
    ir_def.inited = true;
  }
}

std::string IrInputsToString(const std::vector<std::pair<std::string, ge::IrInputType>> &ir_inputs) {
  std::ostringstream oss;
  for (const auto &pair : ir_inputs) {
    oss << "[" << pair.first << ", " << pair.second << "], ";
  }
  return oss.str();
}

std::string IrAttrNamesToString(const std::vector<std::string> &attr_names) {
  std::ostringstream oss;
  for (const auto &attr : attr_names) {
    oss << attr << ", ";
  }
  return oss.str();
}

ge::graphStatus RecoverIrInputs(const ge::NodePtr &node, IrDefinition &ir_def) {
  const auto ir_inputs_in_node = node->GetOpDesc()->GetIrInputs();
  // 输入个数和顺序校验针对单算子离线流程当前未实现so进om时版本兼容性校验，实现后校验逻辑可去除
  // 当前运行版本中，算子输入个数减少了（相对于导出模型的版本）
  if (ir_def.inputs.size() < ir_inputs_in_node.size()) {
    GELOGE(ge::FAILED, "In the current running version, the number of operator[%s][%s] inputs has been reduced, "
            "ir_def.inputs size[%zu] is less than ir_inputs_in_node size[%zu], ir_def.inputs is [%s], "
           "ir_inputs_in_node is [%s]", node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str(),
           ir_def.inputs.size(), ir_inputs_in_node.size(), IrInputsToString(ir_def.inputs).c_str(),
           IrInputsToString(ir_inputs_in_node).c_str());
    return ge::FAILED;
  }
  // 算子输入顺序或者输入类型变化了
  for (size_t i = 0U; i < ir_inputs_in_node.size(); ++i) {
    if (ir_inputs_in_node[i] != ir_def.inputs[i]) {
      GELOGE(ge::FAILED, "In the current running version, the order or type of operator[%s][%s] inputs may "
             "have changed, ir_def.inputs[%zu] is [%s, %u], ir_inputs_in_node[%zu] is [%s, %u], "
             "ir_def.inputs is [%s], ir_inputs_in_node is [%s]", node->GetOpDesc()->GetName().c_str(),
             node->GetOpDesc()->GetType().c_str(), i, ir_def.inputs[i].first.c_str(), ir_def.inputs[i].second,
             i, ir_inputs_in_node[i].first.c_str(), ir_inputs_in_node[i].second,
             IrInputsToString(ir_def.inputs).c_str(), IrInputsToString(ir_inputs_in_node).c_str());
      return ge::FAILED;
    }
  }
  // 当前运行版本中，算子输入个数在后面增加了，需要添加到node中，或者 ir_inputs_in_node 为空，全部拷贝到node中
  for (size_t i = ir_inputs_in_node.size(); i < ir_def.inputs.size(); ++i) {
    node->GetOpDesc()->AppendIrInput(ir_def.inputs[i].first, ir_def.inputs[i].second);
    GELOGD("Append ir input:%s for node[%s(%s)]",
           ir_def.inputs[i].first.c_str(), node->GetName().c_str(), node->GetType().c_str());
  }

  return ge::GRAPH_SUCCESS;
}

ge::graphStatus RecoverIrAttrNames(const ge::NodePtr &node, IrDefinition &ir_def) {
  const auto &ir_attr_names_in_node = node->GetOpDesc()->GetIrAttrNames();
  // 输入个数和顺序校验针对单算子离线流程当前未实现so进om时版本兼容性校验，实现后校验逻辑可去除
  // 当前运行版本中，算子属性个数减少了（相对于导出模型的版本）
  if (ir_def.attr_names.size() < ir_attr_names_in_node.size()) {
    GELOGE(ge::FAILED, "In the current running version, the number of operator[%s][%s] attributes has been reduced, "
           "ir_def.attr_names size[%zu] is less than ir_attr_names_in_node size[%zu], ir_def.attr_names is [%s], "
           "ir_attr_names_in_node is [%s]",
           node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str(),
           ir_def.attr_names.size(), ir_attr_names_in_node.size(), IrAttrNamesToString(ir_def.attr_names).c_str(),
           IrAttrNamesToString(ir_attr_names_in_node).c_str());
    return ge::FAILED;
  }
  // 算子属性顺序变化了
  for (size_t i = 0U; i < ir_attr_names_in_node.size(); ++i) {
    if (ir_attr_names_in_node[i] != ir_def.attr_names[i]) {
      GELOGE(ge::FAILED, "In the current running version, the order of operator[%s][%s] attributes may have changed,"
             "ir_def.attr_names[%zu] is [%s], ir_attr_names_in_node[%zu] is [%s], ir_def.attr_names is [%s], "
             "ir_attr_names_in_node is [%s]",
             node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str(), i,
             ir_def.attr_names[i].c_str(), i, ir_attr_names_in_node[i].c_str(),
             IrAttrNamesToString(ir_def.attr_names).c_str(), IrAttrNamesToString(ir_attr_names_in_node).c_str());
      return ge::FAILED;
    }
  }
  // 当前运行版本中，算子属性在后面增加了，需要拷贝到node中，或者 ir_attr_names_in_node 为空，全部拷贝到node中
  for (size_t i = ir_attr_names_in_node.size(); i < ir_def.attr_names.size(); ++i) {
    node->GetOpDesc()->AppendIrAttrName(ir_def.attr_names[i]);
    GELOGD("Append ir attr name:%s for node[%s(%s)]",
           ir_def.attr_names[i].c_str(), node->GetName().c_str(), node->GetType().c_str());
  }
  return ge::SUCCESS;
}
}

namespace ge {
ge::graphStatus RecoverNodeIrDefinitions(const ge::NodePtr &node, std::string &op_type, IrDefinition &ir_def) {
  if (node->GetType() == ge::NETOUTPUT) {
    return ge::GRAPH_SUCCESS;
  }
  InitIrDefinitionsIfNeed(op_type, ir_def);

  if (!ir_def.has_ir_definition) {
    GELOGI("Op type:%s has no registered IR, maybe no need to recover.", op_type.c_str());
    return ge::GRAPH_SUCCESS;
  }

  // ir_attr_names
  if (RecoverIrAttrNames(node, ir_def) != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "recover ir attr names failed.");
    return ge::FAILED;
  }
  // ir_inputs
  if (RecoverIrInputs(node, ir_def) != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "recover ir inputs failed.");
    return ge::FAILED;
  }
  // attr
  const auto node_all_attrs = ge::AttrUtils::GetAllAttrs(node->GetOpDesc());
  for (const auto &name : ir_def.attr_names) {
    if (node_all_attrs.find(name) != node_all_attrs.cend()) {
      continue;
    }
    const std::map<std::string, ge::AnyValue>::const_iterator iter =
        ir_def.attr_value.find(name);
    if (iter == ir_def.attr_value.cend()) {
      GELOGI("node[%s(%s)] missing attr name[%s], and can not find default value for the attr,"
             " it may be REQUIRED_ATTR.", node->GetName().c_str(), op_type.c_str(), name.c_str());
      continue;
    }
    GELOGD("node[%s(%s)] missing attr name[%s], set default value.",
           node->GetName().c_str(), op_type.c_str(), name.c_str());
    (void)node->GetOpDesc()->SetAttr(name, iter->second);
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus RecoverIrDefinitions(const ge::ComputeGraphPtr &graph, const vector<std::string> &attr_names) {
  GELOGD("Start to recover all ir definitions for graph:%s.", graph->GetName().c_str());
  std::map<std::string, IrDefinition> op_type_to_ir_def;
  for (const auto &node : graph->GetAllNodes()) {
    std::string op_type = ge::NodeUtils::GetNodeType(node);
    auto &ir_def = op_type_to_ir_def[op_type];
    if (RecoverNodeIrDefinitions(node, op_type, ir_def)  != ge::GRAPH_SUCCESS) {
      GELOGE(ge::GRAPH_FAILED, "[Recover][NodeIrDefinitions] failed, node[%s], type[%s]",
             node->GetName().c_str(), node->GetType().c_str());
      return ge::GRAPH_FAILED;
    }
    for (const auto &attr_name : attr_names) {
      ge::ComputeGraphPtr graph_ptr = nullptr;
      (void) ge::AttrUtils::GetGraph(node->GetOpDesc(), attr_name, graph_ptr);
      if (graph_ptr == nullptr) {
        continue;
      }
      if (RecoverIrDefinitions(graph_ptr) != ge::GRAPH_SUCCESS) {
        GELOGE(ge::GRAPH_FAILED, "[Recover][IrDefinitions] failed, graph[%s]", graph_ptr->GetName().c_str());
        return ge::GRAPH_FAILED;
      }
      (void) ge::AttrUtils::SetGraph(node->GetOpDesc(), attr_name, graph_ptr);
      GELOGD("Success to recover definitions for graph:%s with node:%s and attr:%s.",
             graph->GetName().c_str(), node->GetName().c_str(), attr_name.c_str());
    }
  }
  GELOGD("Success to recover all ir definitions for graph:%s.", graph->GetName().c_str());
  return ge::GRAPH_SUCCESS;
}
} // namespace ge
