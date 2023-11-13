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

#include "fast_node_utils.h"
#include "graph/execute_graph.h"
#include "graph/debug/ge_log.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"

namespace ge {
FastNode *FastNodeUtils::GetParentInput(const FastNode *node) {
  uint32_t parent_index = 0U;
  if (!AttrUtils::GetInt(node->GetOpDescPtr(), ATTR_NAME_PARENT_NODE_INDEX, parent_index)) {
    return nullptr;
  }

  // Subgraph Data Node, check for constant input.
  GE_CHECK_NOTNULL_EXEC(node->GetExtendInfo(), return nullptr);
  const ExecuteGraph *graph = node->GetExtendInfo()->GetOwnerGraphBarePtr();
  GE_CHECK_NOTNULL_EXEC(graph, return nullptr);

  const FastNode *parent_node = graph->GetParentNodeBarePtr();
  if (parent_node == nullptr) {
    GELOGW("Node {%s %s} has attr %s but has no parent node.", node->GetNamePtr(), node->GetTypePtr(),
           ATTR_NAME_PARENT_NODE_INDEX.c_str());
    return nullptr;
  }

  auto edge = parent_node->GetPeerOutDataEdge(static_cast<int32_t>(parent_index));
  GE_CHECK_NOTNULL_EXEC(edge, return nullptr);

  auto src_node = edge->src;
  GE_CHECK_NOTNULL_EXEC(src_node, return nullptr);

  if (src_node->GetType() == DATA) {
    if (src_node->GetOpDescPtr() == nullptr) {
      return nullptr;
    }
    if (src_node->GetOpDescBarePtr()->HasAttr(ATTR_NAME_PARENT_NODE_INDEX)) {
      return GetParentInput(src_node);
    }
  }
  return src_node;
}

bool FastNodeUtils::GetConstOpType(const FastNode *node) {
  if (node == nullptr) {
    return false;
  }

  const auto node_type = node->GetType();
  if ((node_type == CONSTANT) || (node_type == CONSTANTOP) || (node_type == FILECONSTANT)) {
    return true;
  }

  if (node_type != DATA) {
    return false;  // not subgraph input node
  }

  const auto &parent = GetParentInput(node);
  return GetConstOpType(parent);
}
}  // namespace ge
