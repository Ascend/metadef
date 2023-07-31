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

#include "graph/utils/node_utils_ex.h"

#include "common/ge_common/util.h"
#include "common/util/trace_manager/trace_manager.h"
#include "graph/format_refiner.h"
#include "graph/shape_refiner.h"
#include "graph/operator_impl.h"
#include "graph/operator_factory_impl.h"
#include "graph/common_error_codes.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/util/mem_utils.h"
#include "graph/utils/op_type_utils.h"

namespace ge {
graphStatus NodeUtilsEx::InferShapeAndType(const NodePtr &node) {
  GE_CHECK_NOTNULL(node, ", Node is null for Infer Shape.");
  Operator op = OpDescUtils::CreateOperatorFromNode(node);
  return ShapeRefiner::InferShapeAndType(node, op);
}

graphStatus NodeUtilsEx::InferOriginFormat(const NodePtr &node) {
  GE_CHECK_NOTNULL(node, ", Node is null for Infer Format.");
  const auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc, ", Op is null for Infer Format.");
  Operator op = OpDescUtils::CreateOperatorFromNode(node);
  auto func = op_desc->GetInferFormatFunc();
  if (func != nullptr) {
    return static_cast<graphStatus>(func(op));
  }
  func = OperatorFactoryImpl::GetInferFormatFunc(op_desc->GetType());
  if (func == nullptr) {
    return op_desc->DefaultInferFormat();
  }
  op_desc->AddInferFormatFunc(func);
  return func(op);
}

graphStatus NodeUtilsEx::IsInputsValid(const NodePtr &node) {
  const auto &op_desc = node->GetOpDesc();
  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    if (in_anchor == nullptr) {
      GELOGW("[Verify][CheckParam] In data anchor is null");
      continue;
    }
    const bool valid_anchor = OpTypeUtils::IsDataNode(node->GetType()) ||
                              (node->GetType() == CONSTANT) || (node->GetType() == VARIABLE) ||
                              (node->GetType() == CONSTANTOP) ||
                              (op_desc->MutableInputDesc(static_cast<uint32_t>(in_anchor->GetIdx())) == nullptr) ||
                              (in_anchor->GetPeerAnchors().size() > 0UL);
    if (!valid_anchor) {
      ErrorManager::GetInstance().ATCReportErrMessage("E11019", {"opname", "index"},
                                                      {node->GetName(), std::to_string(in_anchor->GetIdx())});
      GELOGE(GRAPH_FAILED, "[Check][Param] operator %s's input %d is not linked.",
             node->GetName().c_str(), in_anchor->GetIdx());
      return GRAPH_FAILED;
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus NodeUtilsEx::Verify(const NodePtr &node) {
  GE_CHECK_NOTNULL(node, ", Node is null for Infer Verify.");
  const bool is_unknown_graph = node->GetOwnerComputeGraph()->GetGraphUnknownFlag();
  if (!is_unknown_graph) {
    GE_CHK_STATUS_RET_NOLOG(IsInputsValid(node));
  }

  const auto op_desc = node->GetOpDesc();
  const bool need_update_name = (node->GetType() != FRAMEWORKOP) && (!is_unknown_graph);
  if (need_update_name) {
    const auto node_op = ge::OperatorFactoryImpl::CreateOperator("node_op", node->GetType());
    if (node_op.IsEmpty()) {
      GELOGW("[Verify][CheckParam] Get op from OperatorFactory failed, type: %s", node->GetType().c_str());
    } else {
      GELOGD("get op from OperatorFactory success. opType: %s", node->GetType().c_str());
      const auto temp_op_desc = ge::OpDescUtils::GetOpDescFromOperator(node_op);
      if (temp_op_desc == nullptr) {
        REPORT_INNER_ERROR("E18888", "GetOpDescFromOperator failed, as return nullptr, type:%s",
                           node->GetType().c_str());
        GELOGE(GRAPH_FAILED, "[Get][OpDesc] temp op desc is null, type:%s", node->GetType().c_str());
        return GRAPH_FAILED;
      }
      if (!op_desc->UpdateInputName(temp_op_desc->GetAllInputName())) {
        GELOGW("[Verify][Update] Update input name failed");
      }
      if (!op_desc->UpdateOutputName(temp_op_desc->GetAllOutputName())) {
        GELOGW("[Verify][Update] Update output name failed");
      }
    }
    node_op.BreakConnect();
  }
  if (is_unknown_graph) {
    return GRAPH_SUCCESS;
  }
  if (op_desc->CommonVerify() == GRAPH_SUCCESS) {
    Operator op = OpDescUtils::CreateOperatorFromNode(node);
    auto verify_func = op_desc->GetVerifyFunc();
    if (verify_func == nullptr) {
      verify_func = OperatorFactoryImpl::GetVerifyFunc(node->GetType());
    }
    if (verify_func != nullptr) {
      return static_cast<graphStatus>(verify_func(op));
    }
    return GRAPH_SUCCESS;
  } else {
    REPORT_CALL_ERROR("E18888", "%s(%s) Verify failed.", node->GetName().c_str(), node->GetType().c_str());
    GELOGE(GRAPH_FAILED, "[Call][CommonVerify] %s(%s) failed.", node->GetName().c_str(), node->GetType().c_str());
    return GRAPH_FAILED;
  }
}
} // namespace ge

