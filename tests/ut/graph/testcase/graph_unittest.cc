/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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
#include <iostream>
#define private public
#include "graph/graph.h"
#include "graph/operator.h"
#include "compute_graph.h"
#include "op_desc.h"
#include "node.h"
#include "graph/utils/graph_utils.h"

class UtestGraph : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestGraph, copy_graph_01) {
  ge::OpDescPtr add_op(new ge::OpDesc("add1", "Add"));
  add_op->AddDynamicInputDesc("input", 2);
  add_op->AddDynamicOutputDesc("output", 1);
  std::shared_ptr<ge::ComputeGraph> compute_graph(new ge::ComputeGraph("test_graph"));
  auto add_node = compute_graph->AddNode(add_op);
  auto graph = ge::GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  ge::Graph copy_graph("copy_graph");
  ASSERT_EQ(copy_graph.CopyFrom(graph), ge::GRAPH_SUCCESS);

  auto cp_compute_graph = ge::GraphUtils::GetComputeGraph(copy_graph);
  ASSERT_NE(cp_compute_graph, nullptr);
  ASSERT_NE(cp_compute_graph, compute_graph);
  ASSERT_EQ(cp_compute_graph->GetDirectNodesSize(), 1);
  auto cp_add_node = cp_compute_graph->FindNode("add1");
  ASSERT_NE(cp_add_node, nullptr);
  ASSERT_NE(cp_add_node, add_node);
}


TEST_F(UtestGraph, copy_graph_02) {
  ge::OpDescPtr if_op(new ge::OpDesc("if", "If"));
  if_op->AddDynamicInputDesc("input", 1);
  if_op->AddDynamicOutputDesc("output", 1);
  std::shared_ptr<ge::ComputeGraph> compute_graph(new ge::ComputeGraph("test_graph"));
  auto if_node = compute_graph->AddNode(if_op);
  auto graph = ge::GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  ge::Graph copy_graph("copy_graph");

  if_op->AddSubgraphName("then_branch");
  if_op->AddSubgraphName("else_branch");
  if_op->SetSubgraphInstanceName(0, "then");
  if_op->SetSubgraphInstanceName(1, "else");

  ge::OpDescPtr add_op1(new ge::OpDesc("add1", "Add"));
  add_op1->AddDynamicInputDesc("input", 2);
  add_op1->AddDynamicOutputDesc("output", 1);
  std::shared_ptr<ge::ComputeGraph> then_compute_graph(new ge::ComputeGraph("then"));
  auto add_node1 = then_compute_graph->AddNode(add_op1);
  then_compute_graph->SetParentNode(if_node);
  then_compute_graph->SetParentGraph(compute_graph);
  compute_graph->AddSubgraph(then_compute_graph);

  ge::OpDescPtr add_op2(new ge::OpDesc("add2", "Add"));
  add_op2->AddDynamicInputDesc("input", 2);
  add_op2->AddDynamicOutputDesc("output", 1);
  std::shared_ptr<ge::ComputeGraph> else_compute_graph(new ge::ComputeGraph("else"));
  auto add_node2 = else_compute_graph->AddNode(add_op2);
  else_compute_graph->SetParentNode(if_node);
  else_compute_graph->SetParentGraph(compute_graph);
  compute_graph->AddSubgraph(else_compute_graph);

  ASSERT_EQ(copy_graph.CopyFrom(graph), ge::GRAPH_SUCCESS);

  auto cp_compute_graph = ge::GraphUtils::GetComputeGraph(copy_graph);
  ASSERT_NE(cp_compute_graph, nullptr);
  ASSERT_NE(cp_compute_graph, compute_graph);
  ASSERT_EQ(cp_compute_graph->GetDirectNodesSize(), 1);
  auto cp_if_node = cp_compute_graph->FindNode("if");
  ASSERT_NE(cp_if_node, nullptr);
  ASSERT_NE(cp_if_node, if_node);

  auto cp_then_compute_graph = cp_compute_graph->GetSubgraph("then");
  ASSERT_NE(cp_then_compute_graph, nullptr);
  ASSERT_NE(cp_then_compute_graph, then_compute_graph);
  ASSERT_EQ(cp_then_compute_graph->GetDirectNodesSize(), 1);
  auto cp_add_node1 = cp_then_compute_graph->FindNode("add1");
  ASSERT_NE(cp_add_node1, nullptr);
  ASSERT_NE(cp_add_node1, add_node1);

  auto cp_else_compute_graph = cp_compute_graph->GetSubgraph("else");
  ASSERT_NE(cp_else_compute_graph, nullptr);
  ASSERT_NE(cp_else_compute_graph, else_compute_graph);
  ASSERT_EQ(cp_else_compute_graph->GetDirectNodesSize(), 1);
  auto cp_add_node2 = cp_else_compute_graph->FindNode("add2");
  ASSERT_NE(cp_add_node2, nullptr);
  ASSERT_NE(cp_add_node2, add_node2);
}
