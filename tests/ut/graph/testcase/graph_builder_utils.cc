/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include "graph_builder_utils.h"

#include "graph/utils/graph_utils.h"

namespace ge {
namespace ut {

GeTensorDescPtr GetTensorDesc(const std::string &name, const std::string &type, Format format, DataType data_type,
                              std::vector<int64_t> shape) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape(shape));
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);
  tensor_desc->SetOriginFormat(format);
  tensor_desc->SetOriginShape(GeShape(shape));
  tensor_desc->SetOriginDataType(data_type);

  return tensor_desc;
}

NodePtr GraphBuilder::AddNode(const std::string &name, const std::string &type, int in_cnt, int out_cnt, Format format,
                              DataType data_type, std::vector<int64_t> shape) {
  auto tensor_desc = GetTensorDesc(name, type, format, data_type, shape);

  auto op_desc = std::make_shared<OpDesc>(name, type);
  for (int i = 0; i < in_cnt; ++i) {
    op_desc->AddInputDesc(tensor_desc->Clone());
  }
  for (int i = 0; i < out_cnt; ++i) {
    op_desc->AddOutputDesc(tensor_desc->Clone());
  }
  return graph_->AddNode(op_desc);
}

void GraphBuilder::AddDataEdge(const NodePtr &src_node, int src_idx, const NodePtr &dst_node, int dst_idx) {
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(src_idx), dst_node->GetInDataAnchor(dst_idx));
}
void GraphBuilder::AddControlEdge(const NodePtr &src_node, const NodePtr &dst_node) {
  GraphUtils::AddEdge(src_node->GetOutControlAnchor(), dst_node->GetInControlAnchor());
}
NodePtr GraphBuilder::AddNode(const string &name, const string &type, std::initializer_list<std::string> input_names,
                              std::initializer_list<std::string> output_names, Format format, DataType data_type,
                              std::vector<int64_t> shape) {
  auto tensor_desc = GetTensorDesc(name, type, format, data_type, shape);

  auto op_desc_ptr = std::make_shared<OpDesc>(name, type);
  for (auto &input_name : input_names) {
    op_desc_ptr->AddInputDesc(input_name, tensor_desc->Clone());
  }
  for (auto &output_name : output_names) {
    op_desc_ptr->AddOutputDesc(output_name, tensor_desc->Clone());
  }

  return graph_->AddNode(op_desc_ptr);
}

FastNode *ExecuteGraphBuilder::AddNode(const std::string &name, const std::string &type, int in_cnt, int out_cnt,
                                    Format format, DataType data_type, std::vector<int64_t> shape) {
  auto tensor_desc = GetTensorDesc(name, type, format, data_type, shape);

  auto op_desc_ptr = std::make_shared<OpDesc>(name, type);
  for (int i = 0; i < in_cnt; ++i) {
    op_desc_ptr->AddInputDesc(tensor_desc->Clone());
  }
  for (int i = 0; i < out_cnt; ++i) {
    op_desc_ptr->AddOutputDesc(tensor_desc->Clone());
  }

  return graph_->AddNode(op_desc_ptr);
}

FastEdge *ExecuteGraphBuilder::AddDataEdge(FastNode *src_node, int src_idx, FastNode *dst_node, int dst_idx) {
  return graph_->AddEdge(src_node, src_idx, dst_node, dst_idx);
}
FastEdge *ExecuteGraphBuilder::AddControlEdge(FastNode *src_node, FastNode *dst_node) {
  return graph_->AddEdge(src_node, -1, dst_node, -1);
}
FastNode *ExecuteGraphBuilder::AddNode(const string &name, const string &type,
                                    std::initializer_list<std::string> input_names,
                                    std::initializer_list<std::string> output_names, Format format, DataType data_type,
                                    std::vector<int64_t> shape) {
  auto tensor_desc = GetTensorDesc(name, type, format, data_type, shape);

  auto op_desc_ptr = std::make_shared<OpDesc>(name, type);
  for (auto &input_name : input_names) {
    op_desc_ptr->AddInputDesc(input_name, tensor_desc->Clone());
  }
  for (auto &output_name : output_names) {
    op_desc_ptr->AddOutputDesc(output_name, tensor_desc->Clone());
  }

  return graph_->AddNode(op_desc_ptr);
}

}  // namespace ut
}  // namespace ge
