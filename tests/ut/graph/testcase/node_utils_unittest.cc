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
#include "graph/debug/ge_op_types.h"

#define protected public
#define private public

namespace ge {
class UtestNodeUtils : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};
namespace {
/*                                  -------------------------
*                                  |  partitioncall_0_const1* |
*     partitioncall_0--------------|             |           |
*           |                      |          netoutput      |
*           |                      --------------------------
*           |                       ------------------         -------------
*           |                      |        data      |       |    data     |
*           |                      |          |       |       |     |       |
*     partitioncall_1--------------|        case -----|-------|   squeeze*  |
*                                  |          |       |       |     |       |
*                                  |      netoutput   |       |  netoutput  |
*                                   ------------------         -------------
*/
ComputeGraphPtr BuildGraphPartitionCall() {
  auto root_builder = ut::GraphBuilder("root");
  const auto &partitioncall_0 = root_builder.AddNode("partitioncall_0", PARTITIONEDCALL, 0, 1);
  const auto &partitioncall_1 = root_builder.AddNode("partitioncall_1", PARTITIONEDCALL, 1, 1);
  root_builder.AddDataEdge(partitioncall_0, 0, partitioncall_1, 0);
  const auto &root_graph = root_builder.GetGraph();

  // 1.build partitioncall_0 sub graph
  auto p1_sub_builder = ut::GraphBuilder("partitioncall_0_sub");
  const auto &partitioncall_0_const1 = p1_sub_builder.AddNode("partitioncall_0_const1", CONSTANT, 0, 1);
  const auto &partitioncall_0_netoutput = p1_sub_builder.AddNode("partitioncall_0_netoutput", NETOUTPUT, 1, 1);
  AttrUtils::SetInt(partitioncall_0_netoutput->GetOpDesc()->MutableInputDesc(0), "_parent_node_index", 0);
  p1_sub_builder.AddDataEdge(partitioncall_0_const1, 0, partitioncall_0_netoutput, 0);
  const auto &sub_graph = p1_sub_builder.GetGraph();
  sub_graph->SetParentNode(partitioncall_0);
  sub_graph->SetParentGraph(root_graph);
  partitioncall_0->GetOpDesc()->AddSubgraphName("f");
  partitioncall_0->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_0_sub");

  // 2.build partitioncall_1 sub graph
  auto p2_sub_builder = ut::GraphBuilder("partitioncall_1_sub");
  const auto &partitioncall_1_data = p2_sub_builder.AddNode("partitioncall_1_data", DATA, 0, 1);
  AttrUtils::SetInt(partitioncall_1_data->GetOpDesc(), "_parent_node_index", 0);
  const auto &partitioncall_1_case = p2_sub_builder.AddNode("partitioncall_1_case", "Case", 1, 1);
  const auto &partitioncall_1_netoutput = p2_sub_builder.AddNode("partitioncall_1_netoutput", NETOUTPUT, 1, 1);
  p2_sub_builder.AddDataEdge(partitioncall_1_data, 0, partitioncall_1_case, 0);
  p2_sub_builder.AddDataEdge(partitioncall_1_case, 0, partitioncall_1_netoutput, 0);
  const auto &sub_graph2 = p2_sub_builder.GetGraph();
  sub_graph2->SetParentNode(partitioncall_1);
  sub_graph2->SetParentGraph(root_graph);
  partitioncall_1->GetOpDesc()->AddSubgraphName("f");
  partitioncall_1->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_1_sub");

  // 2.1 build case sub graph
  auto case_sub_builder = ut::GraphBuilder("case_sub");
  const auto &case_data = case_sub_builder.AddNode("case_data", DATA, 0, 1);
  AttrUtils::SetInt(case_data->GetOpDesc(), "_parent_node_index", 0);
  const auto &case_squeeze = case_sub_builder.AddNode("case_squeeze", SQUEEZE, 1, 1);
  const auto &case_netoutput = case_sub_builder.AddNode("case_netoutput", NETOUTPUT, 1, 1);
  case_sub_builder.AddDataEdge(case_data, 0, case_squeeze, 0);
  case_sub_builder.AddDataEdge(case_squeeze, 0, case_netoutput, 0);
  const auto &case_sub_graph = case_sub_builder.GetGraph();
  case_sub_graph->SetParentNode(partitioncall_1_case);
  case_sub_graph->SetParentGraph(sub_graph2);
  partitioncall_1_case->GetOpDesc()->AddSubgraphName("branches");
  partitioncall_1_case->GetOpDesc()->SetSubgraphInstanceName(0, "case_sub");

  root_graph->AddSubgraph(case_sub_graph->GetName(), case_sub_graph);
  root_graph->AddSubgraph(sub_graph->GetName(), sub_graph);
  root_graph->AddSubgraph(sub_graph2->GetName(), sub_graph2);
  return root_graph;
}
/*                                  -------------------------
*                                  |           data0         |
*         data                     |             |           |
*           |                      |            cast         |
*     partitioncall_0--------------|             |           |
*           |                      |          netoutput      |
*           |                      --------------------------
*           |                       ------------------        
*           |                      |        data1     |    
*           |                      |          |       |    
*     partitioncall_1--------------|        squeeze   |
*                                  |          |       |
*                                  |      netoutput   |
*                                   ------------------ 
*/
ComputeGraphPtr BuildGraphPartitionCall2() {
  auto root_builder = ut::GraphBuilder("root");
  const auto &data = root_builder.AddNode("data", DATA, 1, 1);
  const auto &partitioncall_0 = root_builder.AddNode("partitioncall_0", PARTITIONEDCALL, 3, 3);
  const auto &partitioncall_1 = root_builder.AddNode("partitioncall_1", PARTITIONEDCALL, 1, 1);
  root_builder.AddDataEdge(data, 0, partitioncall_0, 1);
  root_builder.AddDataEdge(partitioncall_0, 1, partitioncall_1, 0);
  const auto &root_graph = root_builder.GetGraph();

  // 1.build partitioncall_0 sub graph
  auto p1_sub_builder = ut::GraphBuilder("partitioncall_0_sub");
  const auto &partitioncall_0_data = p1_sub_builder.AddNode("partitioncall_0_data", DATA, 0, 1);
  AttrUtils::SetInt(partitioncall_0_data->GetOpDesc(), "_parent_node_index", 1);
  const auto &partitioncall_0_cast = p1_sub_builder.AddNode("partitioncall_0_cast", "Cast", 1, 1);
  const auto &partitioncall_0_netoutput = p1_sub_builder.AddNode("partitioncall_0_netoutput", NETOUTPUT, 3, 3);
  AttrUtils::SetInt(partitioncall_0_netoutput->GetOpDesc()->MutableInputDesc(0), "_parent_node_index", 0);
  AttrUtils::SetInt(partitioncall_0_netoutput->GetOpDesc()->MutableInputDesc(1), "_parent_node_index", 1);
  AttrUtils::SetInt(partitioncall_0_netoutput->GetOpDesc()->MutableInputDesc(2), "_parent_node_index", 2);
  p1_sub_builder.AddDataEdge(partitioncall_0_data, 0, partitioncall_0_cast, 0);
  p1_sub_builder.AddDataEdge(partitioncall_0_cast, 0, partitioncall_0_netoutput, 1);
  const auto &sub_graph = p1_sub_builder.GetGraph();
  sub_graph->SetParentNode(partitioncall_0);
  sub_graph->SetParentGraph(root_graph);
  partitioncall_0->GetOpDesc()->AddSubgraphName("f");
  partitioncall_0->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_0_sub");

  // 2.build partitioncall_1 sub graph
  auto p2_sub_builder = ut::GraphBuilder("partitioncall_1_sub");
  const auto &partitioncall_1_data = p2_sub_builder.AddNode("partitioncall_1_data", DATA, 0, 1);
  AttrUtils::SetInt(partitioncall_1_data->GetOpDesc(), "_parent_node_index", 0);
  const auto &partitioncall_1_squeeze = p2_sub_builder.AddNode("partitioncall_1_squeeze", SQUEEZE, 1, 1);
  const auto &partitioncall_1_netoutput = p2_sub_builder.AddNode("partitioncall_1_netoutput", NETOUTPUT, 1, 1);
  p2_sub_builder.AddDataEdge(partitioncall_1_data, 0, partitioncall_1_squeeze, 0);
  p2_sub_builder.AddDataEdge(partitioncall_1_squeeze, 0, partitioncall_1_netoutput, 0);
  const auto &sub_graph2 = p2_sub_builder.GetGraph();
  sub_graph2->SetParentNode(partitioncall_1);
  sub_graph2->SetParentGraph(root_graph);
  partitioncall_1->GetOpDesc()->AddSubgraphName("f");
  partitioncall_1->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_1_sub");

  root_graph->AddSubgraph(sub_graph->GetName(), sub_graph);
  root_graph->AddSubgraph(sub_graph2->GetName(), sub_graph2);
  return root_graph;
}
/*                                  -------------------------                 -------------------
*                                  |      partitioncall_2---------------------|        Mul       |
*     partitioncall_0--------------|             |           |                |         |        |
*           |                      |         netoutput       |                |     netoutput    |
*           |                      --------------------------                  ------------------
*           |                       -------------
*           |                      |    data     |
*           |                      |     |       |
*     partitioncall_1--------------|   squeeze*  |
*                                  |     |       |
*                                  |  netoutput  |
*                                   -------------
*/
ComputeGraphPtr BuildGraphPartitionCall3() {
  auto root_builder = ut::GraphBuilder("root");
  const auto &partitioncall_0 = root_builder.AddNode("partitioncall_0", PARTITIONEDCALL, 0, 1);
  const auto &partitioncall_1 = root_builder.AddNode("partitioncall_1", PARTITIONEDCALL, 1, 1);
  root_builder.AddDataEdge(partitioncall_0, 0, partitioncall_1, 0);
  const auto &root_graph = root_builder.GetGraph();

  // 1.build partitioncall_0 sub graph
  auto p1_sub_builder = ut::GraphBuilder("partitioncall_0_sub");
  const auto &partitioncall_2 = p1_sub_builder.AddNode("partitioncall_2", PARTITIONEDCALL, 0, 1);
  const auto &partitioncall_0_netoutput = p1_sub_builder.AddNode("partitioncall_0_netoutput", NETOUTPUT, 1, 1);
  AttrUtils::SetInt(partitioncall_0_netoutput->GetOpDesc()->MutableInputDesc(0), "_parent_node_index", 0);
  p1_sub_builder.AddDataEdge(partitioncall_2, 0, partitioncall_0_netoutput, 0);
  const auto &sub_graph = p1_sub_builder.GetGraph();
  sub_graph->SetParentNode(partitioncall_0);
  sub_graph->SetParentGraph(root_graph);
  partitioncall_0->GetOpDesc()->AddSubgraphName("sub0");
  partitioncall_0->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_0_sub");

  // 2.build partitioncall_1 sub graph
  auto p2_sub_builder = ut::GraphBuilder("partitioncall_1_sub");
  const auto &partitioncall_1_data = p2_sub_builder.AddNode("partitioncall_1_data", DATA, 0, 1);
  AttrUtils::SetInt(partitioncall_1_data->GetOpDesc(), "_parent_node_index", 0);
  const auto &partitioncall_1_squeeze = p2_sub_builder.AddNode("partitioncall_1_squeeze", SQUEEZE, 1, 1);
  const auto &partitioncall_1_netoutput = p2_sub_builder.AddNode("partitioncall_1_netoutput", NETOUTPUT, 1, 1);
  p2_sub_builder.AddDataEdge(partitioncall_1_data, 0, partitioncall_1_squeeze, 0);
  p2_sub_builder.AddDataEdge(partitioncall_1_squeeze, 0, partitioncall_1_netoutput, 0);
  const auto &sub_graph2 = p2_sub_builder.GetGraph();
  sub_graph2->SetParentNode(partitioncall_1);
  sub_graph2->SetParentGraph(root_graph);
  partitioncall_1->GetOpDesc()->AddSubgraphName("sub1");
  partitioncall_1->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_1_sub");

  // 3 build partitioncall_2 sub graph
  auto p3_sub_builder = ut::GraphBuilder("partitioncall_2_sub");
  const auto &partitioncall_2_mul = p3_sub_builder.AddNode("partitioncall_2_mul", "Mul", 0, 1);
  const auto &partitioncall_2_netoutput = p3_sub_builder.AddNode("partitioncall_2_netoutput", NETOUTPUT, 1, 1);
  AttrUtils::SetInt(partitioncall_2_netoutput->GetOpDesc()->MutableInputDesc(0), "_parent_node_index", 0);
  p3_sub_builder.AddDataEdge(partitioncall_2_mul, 0, partitioncall_2_netoutput, 0);
  const auto &sub_graph3 = p3_sub_builder.GetGraph();
  sub_graph3->SetParentNode(partitioncall_2);
  sub_graph3->SetParentGraph(sub_graph);
  partitioncall_2->GetOpDesc()->AddSubgraphName("sub2");
  partitioncall_2->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_2_sub");

  root_graph->AddSubgraph(sub_graph->GetName(), sub_graph);
  root_graph->AddSubgraph(sub_graph2->GetName(), sub_graph2);
  root_graph->AddSubgraph(sub_graph3->GetName(), sub_graph3);
  return root_graph;
}
}

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

TEST_F(UtestNodeUtils, GetInNodeCrossPartionedCallNode_subgraph_in_partitioncall) {
  auto graph = BuildGraphPartitionCall();
  NodePtr expect_peer_node;
  NodePtr squeeze_node;
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetType() == SQUEEZE) {
      squeeze_node = node;
    }
  }
  auto ret = NodeUtils::GetInNodeCrossPartionedCallNode(squeeze_node, 0 , expect_peer_node);
  ASSERT_EQ(ret, GRAPH_SUCCESS);
  ASSERT_NE(expect_peer_node, nullptr);
  ASSERT_EQ(expect_peer_node->GetName(), "partitioncall_0_const1");
}

  ///       A(PartionedCall_0)->B(PartionedCall_1)
  ///          PartionedCall_0's subgraph: Data->A->Netoutput
  ///          PartionedCall_1's subgraph: Data1->B->Netoutput
  ///          If it is called like GetInNodeCrossPartionCallNode(B,0,peer_node)or(Data1,0,peer_node), peer_node is A
TEST_F(UtestNodeUtils, GetInNodeCrossPartionedCallNode_paritioncall_link_partitioncall) {
  auto graph = BuildGraphPartitionCall2();
  NodePtr expect_peer_node = nullptr;
  NodePtr squeeze_node;
  NodePtr data_in_partition1;
  NodePtr partitioncall_0;
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetType() == SQUEEZE) {
      squeeze_node = node;
    }
    if (node->GetName() == "partitioncall_1_data") {
      data_in_partition1 = node;
    }
    if (node->GetName() == "partitioncall_0") {
      partitioncall_0 = node;
    }
  }
  // test with src node
  auto ret = NodeUtils::GetInNodeCrossPartionedCallNode(squeeze_node, 0 , expect_peer_node);
  ASSERT_EQ(ret, GRAPH_SUCCESS);
  ASSERT_NE(expect_peer_node, nullptr);
  ASSERT_EQ(expect_peer_node->GetName(), "partitioncall_0_cast");

  // test subgraph_data node
  ret = NodeUtils::GetInNodeCrossPartionedCallNode(data_in_partition1, 0 , expect_peer_node);
  ASSERT_EQ(ret, GRAPH_SUCCESS);
  ASSERT_NE(expect_peer_node, nullptr);
  ASSERT_EQ(expect_peer_node->GetName(), "partitioncall_0_cast");

  // test peer_node is root_data node
  ret = NodeUtils::GetInNodeCrossPartionedCallNode(partitioncall_0, 1 , expect_peer_node);
  ASSERT_EQ(ret, GRAPH_SUCCESS);
  ASSERT_EQ(expect_peer_node, nullptr);
}

TEST_F(UtestNodeUtils, GetInNodeCrossPartionedCallNode_multi_partitioncall) {
  auto graph = BuildGraphPartitionCall3();
  NodePtr expect_peer_node;
  NodePtr squeeze_node;
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetType() == SQUEEZE) {
      squeeze_node = node;
    }
  }
  auto ret = NodeUtils::GetInNodeCrossPartionedCallNode(squeeze_node, 0 , expect_peer_node);
  ASSERT_EQ(ret, GRAPH_SUCCESS);
  ASSERT_NE(expect_peer_node, nullptr);
  ASSERT_EQ(expect_peer_node->GetName(), "partitioncall_2_mul");
}
}  // namespace ge
