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

#include <gtest/gtest.h>

#define protected public
#define private public

#include "graph/utils/node_utils.h"
#include "graph/node_impl.h"
#include "graph/op_desc_impl.h"
#include "graph_builder_utils.h"

#define protected public
#define private public

namespace ge {
class UtestNodeUtils : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestNodeUtils, GetInputConstData) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  const auto &data = builder.AddNode("Data", "Data", 0, 1);
  const auto &data2 = builder.AddNode("Data2", "Data", 0, 1);
  const auto &enter = builder.AddNode("Enter", "Enter", 1, 1);
  const auto &transdata = builder.AddNode("Transdata", "Transdata", 2, 1);
  const auto &netoutput = builder.AddNode("Netoutput", "NetOutput", 1, 0);
  builder.AddDataEdge(data2, 0, enter, 0);
  builder.AddDataEdge(data, 0, transdata, 0);
  builder.AddDataEdge(enter, 0, transdata, 1);
  builder.AddDataEdge(transdata, 0, netoutput, 0);
  auto graph = builder.GetGraph();

  auto tensor = std::make_shared<GeTensor>();
  auto op_desc = transdata->GetOpDesc();
  op_desc->impl_->input_name_idx_["Data"] = 0;
  op_desc->impl_->input_name_idx_["Enter"] = 1;
  auto tensor_desc = op_desc->MutableInputDesc(0);
  AttrUtils::SetTensor(tensor_desc, "_value", tensor);

  GeTensorPtr ge_tensor;
  ASSERT_EQ(NodeUtils::GetInputConstData(*transdata, "Data", ge_tensor), GRAPH_SUCCESS);
  ASSERT_EQ(NodeUtils::GetInputConstData(*transdata, "Enter", ge_tensor), GRAPH_FAILED);
}

TEST_F(UtestNodeUtils, GetInputConstData_subgraph) {
  auto ge_tensor = std::make_shared<GeTensor>();
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  const auto &const_node = builder.AddNode("Const", "Const", 0, 1);
  AttrUtils::SetTensor(const_node->GetOpDesc(), "value", ge_tensor);
  const auto &case_node = builder.AddNode("Case", "Case", 1, 1);
  const auto &netoutput = builder.AddNode("Netoutput", "NetOutput", 1, 0);
  builder.AddDataEdge(const_node, 0, case_node, 0);
  builder.AddDataEdge(case_node, 0, netoutput, 0);
  auto parent_graph = builder.GetGraph();

  ut::GraphBuilder sub_builder = ut::GraphBuilder("subgraph_graph");
  const auto &sub_data = sub_builder.AddNode("sub_data", "Data", 0, 1);
  const auto &sub_const = sub_builder.AddNode("sub_const", "Const", 0, 1);
  AttrUtils::SetTensor(sub_const->GetOpDesc(), "value", ge_tensor);
  const auto &add = sub_builder.AddNode("Add", "Add", 2, 1);
  const auto &sub_netoutput = sub_builder.AddNode("sub_netoutput", "NetOutput", 1, 0);
  sub_builder.AddDataEdge(sub_data, 0, add, 0);
  sub_builder.AddDataEdge(sub_const, 0, add, 1);
  sub_builder.AddDataEdge(add, 0, sub_netoutput, 0);

  auto subgraph = sub_builder.GetGraph();
  subgraph->SetParentNode(case_node);
  subgraph->SetParentGraph(parent_graph);
  parent_graph->AddSubgraph(subgraph->GetName(), subgraph);
  AttrUtils::SetInt(sub_data->GetOpDesc(), "_parent_node_index", 0);

  auto op_desc = add->GetOpDesc();
  op_desc->impl_->input_name_idx_["sub_data"] = 0;
  op_desc->impl_->input_name_idx_["sub_const"] = 1;

  GeTensorPtr tensor;
  ASSERT_EQ(NodeUtils::GetInputConstData(*add, "sub_const", tensor), GRAPH_SUCCESS);
  ASSERT_EQ(NodeUtils::GetInputConstData(*add, "sub_data", tensor), GRAPH_SUCCESS);
}

TEST_F(UtestNodeUtils, UpdateOriginShapeAndShape) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto data1 = builder.AddNode("Data1", "Data", 1, 1);
  auto data2 = builder.AddNode("Data2", "Data", 1, 1);

  vector<int64_t> dims = {1, 2};
  GeShape data_shape(dims);
  ASSERT_EQ(NodeUtils::UpdateInputOriginalShapeAndShape(*data1, 0, data_shape), GRAPH_SUCCESS);
  ASSERT_EQ(NodeUtils::UpdateOutputOriginalShapeAndShape(*data1, 0, data_shape), GRAPH_SUCCESS);
  ASSERT_EQ(NodeUtils::UpdateInputOriginalShapeAndShape(*data2, 0, data_shape), GRAPH_SUCCESS);
  ASSERT_EQ(NodeUtils::UpdateOutputOriginalShapeAndShape(*data2, 0, data_shape), GRAPH_SUCCESS);
  ASSERT_EQ(data1->GetOpDesc()->GetInputDesc(0).GetShape() == data1->GetOpDesc()->GetInputDesc(0).GetShape(), true);
  ASSERT_EQ(data1->GetOpDesc()->GetInputDesc(0).IsOriginShapeInitialized(), true);
}

TEST_F(UtestNodeUtils, GetSubgraphs) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &case0 = root_builder.AddNode("case0", "Case", 0, 0);
  const auto &root_graph = root_builder.GetGraph();

  auto sub_builder1 = ut::GraphBuilder("sub1");
  const auto &case1 = sub_builder1.AddNode("case1", "Case", 0, 0);
  const auto &sub_graph1 = sub_builder1.GetGraph();
  root_graph->AddSubGraph(sub_graph1);
  sub_graph1->SetParentNode(case0);
  sub_graph1->SetParentGraph(root_graph);
  case0->GetOpDesc()->AddSubgraphName("branch1");
  case0->GetOpDesc()->SetSubgraphInstanceName(0, "sub1");

  std::vector<ComputeGraphPtr> subgraphs0;
  ASSERT_EQ(NodeUtils::GetDirectSubgraphs(case0, subgraphs0), GRAPH_SUCCESS);
  ASSERT_EQ(subgraphs0.size(), 1);
  std::vector<ComputeGraphPtr> subgraphs1;
  ASSERT_EQ(NodeUtils::GetDirectSubgraphs(case1, subgraphs1), GRAPH_SUCCESS);
  ASSERT_TRUE(subgraphs1.empty());
}

TEST_F(UtestNodeUtils, GetSubgraphs_nullptr_node) {
  std::vector<ComputeGraphPtr> subgraphs;
  ASSERT_NE(NodeUtils::GetDirectSubgraphs(nullptr, subgraphs), GRAPH_SUCCESS);
  ASSERT_TRUE(subgraphs.empty());
}

TEST_F(UtestNodeUtils, GetSubgraphs_nullptr_root_graph) {
  auto builder = ut::GraphBuilder("graph");
  const auto &node = builder.AddNode("node", "node", 0, 0);
  node->impl_->owner_graph_.reset();

  std::vector<ComputeGraphPtr> subgraphs;
  ASSERT_NE(NodeUtils::GetDirectSubgraphs(node, subgraphs), GRAPH_SUCCESS);
  ASSERT_TRUE(subgraphs.empty());
}

TEST_F(UtestNodeUtils, GetSubgraphs_nullptr_sub_graph) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("node", "node", 0, 0);
  const auto &root_graph = root_builder.GetGraph();

  auto sub_builder = ut::GraphBuilder("sub");
  const auto &sub_graph = sub_builder.GetGraph();
  sub_graph->SetParentNode(node);
  sub_graph->SetParentGraph(root_graph);
  node->GetOpDesc()->AddSubgraphName("branch1");
  node->GetOpDesc()->SetSubgraphInstanceName(0, "sub");

  std::vector<ComputeGraphPtr> subgraphs;
  ASSERT_EQ(NodeUtils::GetDirectSubgraphs(node, subgraphs), GRAPH_SUCCESS);
  ASSERT_TRUE(subgraphs.empty());
}

TEST_F(UtestNodeUtils, GetNodeUnknownShapeStatus_success) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &case0 = root_builder.AddNode("case0", "Case", 0, 0);
  const auto &root_graph = root_builder.GetGraph();
  auto sub_builder1 = ut::GraphBuilder("sub1");
  const auto &case1 = sub_builder1.AddNode("case1", "Case", 0, 0);
  const auto &sub_graph1 = sub_builder1.GetGraph();
  root_graph->AddSubGraph(sub_graph1);
  sub_graph1->SetParentNode(case0);
  sub_graph1->SetParentGraph(root_graph);
  case0->GetOpDesc()->AddSubgraphName("branch1");
  case0->GetOpDesc()->SetSubgraphInstanceName(0, "sub1");
  bool is_known = false;
  ASSERT_EQ(NodeUtils::GetNodeUnknownShapeStatus(*case0, is_known), GRAPH_SUCCESS);
}
}  // namespace ge
