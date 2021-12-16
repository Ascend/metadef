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
#include "graph/compute_graph.h"
#include "graph/op_desc.h"
#include "graph/op_desc_impl.h"
#include "graph/ge_tensor.h"
#include "graph/utils/ge_ir_utils.h"
#undef private
#undef protected
#include "debug/ge_op_types.h"
#include "inc/graph/debug/ge_attr_define.h"
#include "graph/utils/transformer_utils.h"
#include "graph/utils/node_utils.h"
namespace ge
{
  class UtestComputeGraph : public testing::Test {
    protected:
    void SetUp() {}
    void TearDown() {}
  };

TEST_F(UtestComputeGraph, GetAllNodes_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetFormat(FORMAT_CHWN);

  auto op_desc = std::make_shared<OpDesc>();
  op_desc->AddInputDesc(tensor_desc->Clone());
  graph->AddNode(op_desc);
  graph->AddNode(op_desc);

  auto node_filter = [](const Node &node){ return true;};
  auto graph_filter = [](const Node &node, const char *str, const ComputeGraphPtr &graph){ return true;};
  auto out_nodes = graph->GetAllNodes(node_filter, graph_filter);
  EXPECT_EQ(out_nodes.size(), 2);
}

TEST_F(UtestComputeGraph, GetNodes_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetFormat(FORMAT_CHWN);

  auto op_desc = std::make_shared<OpDesc>();
  op_desc->AddInputDesc(tensor_desc->Clone());
  graph->AddNode(op_desc);
  graph->AddNode(op_desc);
  auto node_filter = [](const Node &node){ return true;};
  auto graph_filter = [](const Node &node, const char *str, const ComputeGraphPtr &graph){ return true;};
  auto out_nodes = graph->GetNodes(true, node_filter, graph_filter);
  EXPECT_EQ(out_nodes.size(), 2);
}

TEST_F(UtestComputeGraph, AddNodeFront_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetFormat(FORMAT_CHWN);

  auto op_desc = std::make_shared<OpDesc>("node1", "node1");
  op_desc->AddInputDesc(tensor_desc->Clone());
  auto node = graph->AddNode(op_desc);
  
  auto op_desc1 = std::make_shared<OpDesc>("add_front", "add_front");
  op_desc1->AddInputDesc(tensor_desc->Clone());
  auto nodeptr = graph->AddNodeFront(node);
  EXPECT_EQ(node, nodeptr);
}

TEST_F(UtestComputeGraph, RemoveNode_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetFormat(FORMAT_CHWN);
  auto op_desc = std::make_shared<OpDesc>("node1", "node1");
  op_desc->AddInputDesc(tensor_desc->Clone());
  auto node = graph->AddNode(op_desc);

  EXPECT_EQ(graph->RemoveNode(node), GRAPH_SUCCESS);
}

TEST_F(UtestComputeGraph, GraphMembersAreEqual_success) {
  auto graph1 = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetFormat(FORMAT_CHWN);

  auto op_desc = std::make_shared<OpDesc>("node1", "node1");
  op_desc->AddInputDesc(tensor_desc->Clone());
  graph1->AddNode(op_desc);
  graph1->AddNode(op_desc);

  auto graph2 = std::make_shared<ComputeGraph>("graph");
  graph2->AddNode(op_desc);
  EXPECT_EQ(graph1->GraphMembersAreEqual(*(graph2)), false);
  graph2->AddNode(op_desc);
  EXPECT_EQ(graph1->GraphMembersAreEqual(*(graph2)), true);
}

TEST_F(UtestComputeGraph, GraphAttrsAreEqual_success) {
  auto graph1 = std::make_shared<ComputeGraph>("graph1");

  int64_t val = 0;
  AnyValue anyvalue;
  anyvalue.SetValue(val);
  graph1->SetAttr("test", anyvalue);

  auto graph2 = std::make_shared<ComputeGraph>("graph2");
  EXPECT_EQ(graph1->GraphAttrsAreEqual(*(graph2)), false);

  graph2->SetAttr("test", anyvalue);
  EXPECT_EQ(graph1->GraphAttrsAreEqual(*(graph2)), true);
}

TEST_F(UtestComputeGraph, VectorInputNodePtrIsEqual_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetFormat(FORMAT_CHWN);

  auto op_desc = std::make_shared<OpDesc>("node1", "node1");
  op_desc->AddInputDesc(tensor_desc->Clone());
  auto node = graph->AddNode(op_desc);

  std::vector<NodePtr> leftnodes{node};
  std::vector<NodePtr> rightnodes{node};
  EXPECT_EQ(graph->VectorInputNodePtrIsEqual(leftnodes, rightnodes), true);
  rightnodes.push_back(node);
  EXPECT_EQ(graph->VectorInputNodePtrIsEqual(leftnodes, rightnodes), false);
}

TEST_F(UtestComputeGraph, RemoveConstInput_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetFormat(FORMAT_CHWN);

  auto op_desc = std::make_shared<OpDesc>("node1", CONSTANT);
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddOutputDesc(tensor_desc->Clone());
  
  auto node1 = graph->AddNode(op_desc);
  auto node2 = graph->AddNode(op_desc);
  GraphUtils::AddEdge(node1->GetOutControlAnchor(), node2->GetInControlAnchor());
  EXPECT_EQ(graph->RemoveConstInput(node1), GRAPH_SUCCESS);
}

TEST_F(UtestComputeGraph, RemoveSubGraph_success) {
  auto rootgraph = std::make_shared<ComputeGraph>("rootgraph");
  auto subgraph = std::make_shared<ComputeGraph>("subgraph");
  rootgraph->AddSubGraph(subgraph);
  EXPECT_EQ(rootgraph->RemoveSubGraph(subgraph), GRAPH_SUCCESS);
}

TEST_F(UtestComputeGraph, Set_GetShareParamLayer_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  std::map<std::vector<std::string>, std::vector<std::string>> params_share_map{{{"test"},{"test"}}};
  graph->SetShareParamLayer(params_share_map);
  EXPECT_EQ(graph->GetShareParamLayer().size(), 1);
}

TEST_F(UtestComputeGraph, Set_GetGraphOutNodes_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  std::map<std::string, std::vector<int32_t>> out_nodes_map{{"test",{1}}};
  auto opdesc = std::make_shared<OpDesc>();
  graph->SetGraphOutNodes(out_nodes_map);
  EXPECT_EQ(graph->GetGraphOutNodes().size(), 1);
  std::map<std::string, std::vector<int32_t>> append_out_nodes_map{{"test2",{2}}};
  graph->AppendGraphOutNodes(append_out_nodes_map);
  EXPECT_EQ(graph->GetGraphOutNodes().size(), 2);
}

TEST_F(UtestComputeGraph, Set_GetOrigGraph_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto origin_graph = std::make_shared<ComputeGraph>("origin_graph");
  graph->SetOrigGraph(origin_graph);
  EXPECT_NE(graph->GetOrigGraph(), nullptr);
}

TEST_F(UtestComputeGraph, GetOutputSize_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto nodes = std::make_shared<Node>();
  graph->AddOutputNode(nodes);
  EXPECT_EQ(graph->GetOutputSize(), 1);
}

TEST_F(UtestComputeGraph, GetInputSize_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto nodes = std::make_shared<Node>();
  graph->AddInputNode(nodes);
  EXPECT_EQ(graph->GetInputSize(), 1);
}

TEST_F(UtestComputeGraph, Set_GetNeedIteration_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  graph->SetNeedIteration(true);
  EXPECT_EQ(graph->GetNeedIteration(), true);
}

TEST_F(UtestComputeGraph, UpdateInputMapping_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetFormat(FORMAT_NCHW);
  tensor_desc->SetDataType(DT_FLOAT);
  auto opdesc = std::make_shared<OpDesc>(ATTR_NAME_PARENT_NODE_INDEX, DATA);
  opdesc->AddInputDesc("name1", tensor_desc->Clone());
  opdesc->AddOutputDesc("name2", tensor_desc->Clone());
  auto node = graph->AddNode(opdesc);
  ge::AttrUtils::SetInt(opdesc, ATTR_NAME_PARENT_NODE_INDEX, 1);
  
  graph->AddInputNode(node);
  std::map<uint32_t, uint32_t> input_mapping{{0,1}};
  EXPECT_EQ(graph->UpdateInputMapping(input_mapping), GRAPH_SUCCESS);
}

TEST_F(UtestComputeGraph, UpdateOutputMapping_success) {
  auto graph = std::make_shared<ComputeGraph>("graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetFormat(FORMAT_NCHW);
  tensor_desc->SetDataType(DT_FLOAT);
  auto opdesc = std::make_shared<OpDesc>(ATTR_NAME_PARENT_NODE_INDEX, NETOUTPUT);
  opdesc->AddInputDesc("name1", tensor_desc->Clone());
  opdesc->AddOutputDesc("name2", tensor_desc->Clone());
  auto node = graph->AddNode(opdesc);
  ge::AttrUtils::SetInt(opdesc, ATTR_NAME_PARENT_NODE_INDEX, 1);
  graph->AddOutputNode(node);
  std::map<uint32_t, uint32_t> output_mapping{{0,1}};
  EXPECT_EQ(graph->UpdateOutputMapping(output_mapping), GRAPH_SUCCESS);
}

}