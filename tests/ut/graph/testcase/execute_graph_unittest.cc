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
#include <unordered_map>
#include "execute_graph.h"
#include "graph/ge_local_context.h"
#include "graph_builder_utils.h"
#include "graph/fast_node.h"
#include "graph/debug/ge_op_types.h"
#include "fast_graph/fast_graph_impl.h"

namespace {
std::shared_ptr<ge::ExecuteGraph> BuildDelayTopoGraph(const std::string &name) {
  auto builder = ge::ut::ExecuteGraphBuilder(name);
  const auto &variable = builder.AddNode("variable", ge::VARIABLE, 0, 2);
  const auto &node1 = builder.AddNode("node1", "node1", 1, 1);
  const auto &node2 = builder.AddNode("node2", "node2", 1, 1);
  const auto &node3 = builder.AddNode("node3", "node3", 1, 1);
  const auto &node4 = builder.AddNode("node4", "node4", 1, 1);
  const auto &node5 = builder.AddNode("node5", "node5", 3, 0);
  const auto &data = builder.AddNode("data", "DATA", 0, 1);

  builder.AddDataEdge(variable, 0, node1, 0);
  builder.AddDataEdge(variable, 1, node2, 0);
  builder.AddDataEdge(node1, 0, node5, 0);
  builder.AddDataEdge(node2, 0, node5, 1);
  builder.AddDataEdge(data, 0, node3, 0);
  builder.AddDataEdge(node3, 0, node4, 0);

  int dst_idx = 2;
  builder.AddDataEdge(node4, 0, node5, dst_idx);

  builder.AddControlEdge(node2, node3);
  return builder.GetGraph();
}
}  // namespace

namespace ge {
class UtestExecuteGraph : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestExecuteGraph, ExecuteGraph) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  size_t graph_id = 10;
  compute_graph->SetGraphId(graph_id);
  auto ret = compute_graph->GetGraphId();
  ASSERT_EQ(ret, graph_id);
}

TEST_F(UtestExecuteGraph, AddNodeToExecuteGraph) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    std::vector<std::string> inputs_order;
    int num = 10;
    for (int i = 0; i < num; ++i) {
      inputs_order.push_back("test" + std::to_string(i));
    }
    compute_graph->SetInputsOrder(inputs_order);
    auto td = GeTensorDesc();
    op_desc->AddInputDesc(td);
    op_desc->AddOutputDesc(td);
    auto node = compute_graph->AddNode(op_desc);
    ASSERT_NE(node, nullptr);
    auto ret = compute_graph->RemoveJustNode(node);
    ASSERT_EQ(ret, GRAPH_SUCCESS);

    node = compute_graph->AddNode(node);
    ASSERT_NE(node, nullptr);

    auto node_with_idx = compute_graph->AddNode(op_desc, 0);
    ASSERT_NE(node_with_idx, nullptr);

    ret = compute_graph->RemoveJustNode(node);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    op_desc->AddInputDesc(td);
    op_desc->AddOutputDesc(td);
    auto node = compute_graph->AddNodeFront(op_desc);
    ASSERT_NE(node, nullptr);
    auto ret = compute_graph->RemoveJustNode(node);
    ASSERT_EQ(ret, GRAPH_SUCCESS);

    node = compute_graph->AddNodeFront(node);
    ASSERT_NE(node, nullptr);
  }
}

TEST_F(UtestExecuteGraph, RemoveJustNode) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  FastEdge *edge[edge_num] = {};
  for (int i = 0; i < edge_num; ++i) {
    edge[i] = compute_graph->AddEdge(node[0], i, node[1], i);
    ASSERT_NE(edge[i], nullptr);
  }

  for (int i = 0; i < node_num; ++i) {
    auto ret = compute_graph->RemoveJustNode(node[i]);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  for (int i = 0; i < edge_num; ++i) {
    auto ret = compute_graph->RemoveEdge(edge[i]);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }
}

TEST_F(UtestExecuteGraph, AddEdgeToExecuteGraph) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  FastEdge *edge[edge_num] = {};
  for (int i = 0; i < edge_num; ++i) {
    edge[i] = compute_graph->AddEdge(node[0], i, node[1], i);
    ASSERT_NE(edge[i], nullptr);
  }

  ASSERT_EQ(compute_graph->GetAllEdges().size(), edge_num);

  for (int i = 0; i < edge_num; ++i) {
    auto ret = compute_graph->RemoveEdge(edge[i]);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }
}

TEST_F(UtestExecuteGraph, AddSubGraphToExecuteGraph) {
  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = root_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  int subgraph_num = 5;
  std::shared_ptr<ExecuteGraph> sub_graph[subgraph_num] = {nullptr};
  for (int i = 0; i < subgraph_num; i++) {
    sub_graph[i] = std::make_shared<ExecuteGraph>("subgraph_" + std::to_string(i));
    sub_graph[i]->SetParentGraph(root_graph.get());
    sub_graph[i]->SetParentNode(node[i]);
  }

  {
    /* Test Error */
    std::string name = "subgraph_" + std::to_string(0);
    std::shared_ptr<ExecuteGraph> invalid_sub_graph = nullptr;
    auto fast_graph = root_graph->AddSubGraph(invalid_sub_graph, name);
    ASSERT_EQ(fast_graph, nullptr);

    sub_graph[0]->SetParentGraph(nullptr);
    fast_graph = root_graph->AddSubGraph(sub_graph[0], name);
    ASSERT_EQ(fast_graph, nullptr);

    sub_graph[0]->SetParentGraph(root_graph.get());
    sub_graph[0]->SetParentNode(nullptr);
    fast_graph = root_graph->AddSubGraph(sub_graph[0], name);
    ASSERT_EQ(fast_graph, nullptr);

    sub_graph[0]->SetParentGraph(root_graph.get());
    sub_graph[0]->SetParentNode(node[0]);

    auto ok_fast_graph = root_graph->AddSubGraph(sub_graph[0], name);
    ASSERT_NE(ok_fast_graph, nullptr);

    auto bad_graph = root_graph->AddSubGraph(sub_graph[0], name);
    ASSERT_EQ(bad_graph, nullptr);

    auto ret = root_graph->RemoveSubGraph(name);
    ASSERT_EQ(ret, GRAPH_SUCCESS);

    auto root_graph2 = std::make_shared<ExecuteGraph>("root_graph2");
    sub_graph[1]->SetParentGraph(root_graph2.get());
    sub_graph[1]->SetParentNode(node[1]);
    bad_graph = root_graph->AddSubGraph(sub_graph[1], name);
    ASSERT_EQ(bad_graph, nullptr);

    sub_graph[1]->SetParentGraph(root_graph.get());
    sub_graph[1]->SetParentNode(node[1]);
  }

  {
    std::string name = "root_graph2";
    auto root_graph2 = std::make_shared<ExecuteGraph>(name);
    root_graph2->SetParentGraph(root_graph.get());
    root_graph2->SetParentNode(node[0]);

    auto ok_fast_graph = root_graph->AddSubGraph(root_graph2, name);
    ASSERT_NE(ok_fast_graph, nullptr);

    auto find_graph = root_graph2->GetSubGraph(name);
    ASSERT_NE(find_graph, nullptr);

    auto ret = root_graph->RemoveSubGraph(name);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  for (int i = 0; i < subgraph_num; i++) {
    std::string name = "subgraph_" + std::to_string(i);
    auto fast_graph = root_graph->AddSubGraph(sub_graph[i], name);
    ASSERT_NE(fast_graph, nullptr);

    auto find_graph = root_graph->GetSubGraph(name);
    ASSERT_NE(find_graph, nullptr);

    auto ret = root_graph->RemoveSubGraph(name);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  for (int i = 0; i < subgraph_num; i++) {
    std::string name = "subgraph_" + std::to_string(i);
    auto fast_graph = root_graph->AddSubGraph(sub_graph[i], name);
    ASSERT_NE(fast_graph, nullptr);
  }

  auto find_graph = root_graph->GetSubGraph("subgraph_1");
  ASSERT_NE(find_graph, nullptr);

  auto subgraphs = root_graph->GetAllSubgraphs();
  ASSERT_EQ(subgraphs.size(), subgraph_num);
  root_graph->ClearAllSubGraph();
}

TEST_F(UtestExecuteGraph, AddOKSubGraphToExecuteGraph) {
  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = root_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  int subgraph_num = 5;
  std::shared_ptr<ExecuteGraph> sub_graph[subgraph_num] = {nullptr};
  for (int i = 0; i < subgraph_num; i++) {
    sub_graph[i] = std::make_shared<ExecuteGraph>("subgraph_" + std::to_string(i));
    sub_graph[i]->SetParentGraph(root_graph.get());
    sub_graph[i]->SetParentNode(node[i]);
  }

  {
    std::shared_ptr<ExecuteGraph> invalid_fast_graph = nullptr;
    auto fast_graph = root_graph->AddSubGraph(invalid_fast_graph);
    ASSERT_EQ(fast_graph, nullptr);

    auto ret = root_graph->RemoveSubGraph(nullptr);
    ASSERT_EQ(ret, GRAPH_PARAM_INVALID);
  }

  for (int i = 0; i < subgraph_num; i++) {
    std::string name = "subgraph_" + std::to_string(i);
    auto fast_graph = root_graph->AddSubGraph(sub_graph[i]);
    ASSERT_NE(fast_graph, nullptr);

    auto ret = root_graph->RemoveSubGraph(fast_graph);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }
}

TEST_F(UtestExecuteGraph, TestExecuteGraphAssign) {
  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");

  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = root_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  FastEdge *edge[edge_num] = {};
  for (int i = 0; i < edge_num; ++i) {
    edge[i] = root_graph->AddEdge(node[0], i, node[1], i);
    ASSERT_NE(edge[i], nullptr);
  }

  int subgraph_num = 5;
  std::shared_ptr<ExecuteGraph> sub_graph[subgraph_num] = {nullptr};
  for (int i = 0; i < subgraph_num; i++) {
    sub_graph[i] = std::make_shared<ExecuteGraph>("subgraph_" + std::to_string(i));
    ASSERT_NE(sub_graph[i], nullptr);
    sub_graph[i]->SetParentGraph(root_graph.get());
    sub_graph[i]->SetParentNode(node[i]);
  }

  for (int i = 0; i < subgraph_num; i++) {
    std::string name = "subgraph_" + std::to_string(i);
    auto fast_graph = root_graph->AddSubGraph(sub_graph[i], name);
    ASSERT_NE(fast_graph, nullptr);

    auto ret = root_graph->RemoveSubGraph(name);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  auto Assign_graph = std::make_shared<ExecuteGraph>("root_graph");
  *Assign_graph = *root_graph;
}

TEST_F(UtestExecuteGraph, TestRecycle) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  FastEdge *edge[edge_num] = {};
  for (int i = 0; i < edge_num; ++i) {
    edge[i] = compute_graph->AddEdge(node[0], i, node[1], i);
    ASSERT_NE(edge[i], nullptr);
  }

  FastGraphUtils::GetListElementAddr(node[0])->owner->erase(FastGraphUtils::GetListElementAddr(node[0]));
  auto ret = compute_graph->RecycleQuickNode(node[0]);
  ASSERT_EQ(ret, GRAPH_SUCCESS);
}

TEST_F(UtestExecuteGraph, TestNodes) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 10;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  ASSERT_EQ(compute_graph->GetDirectNodesSize(), node_num);
  ASSERT_EQ(compute_graph->GetDirectNode().size(), node_num);

  {
    auto ret = compute_graph->RemoveJustNode(node[0]);
    ASSERT_EQ(ret, GRAPH_SUCCESS);

    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    auto node = compute_graph->AddNode(op_desc);
    ASSERT_NE(node, nullptr);
  }
}

TEST_F(UtestExecuteGraph, TestIONodes) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  FastNode *quick_node[node_num] = {};

  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    op_desc->AddInputDesc(td);
    op_desc->AddOutputDesc(td);
    quick_node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(quick_node[i], nullptr);
  }

  {
    auto add_node = compute_graph->AddNode(nullptr);
    ASSERT_EQ(add_node, nullptr);

    add_node = compute_graph->AddNode(nullptr, 1);
    ASSERT_EQ(add_node, nullptr);

    auto input_node = compute_graph->AddInputNode(nullptr);
    ASSERT_EQ(input_node, nullptr);

    auto ret = compute_graph->RemoveInputNode(nullptr);
    ASSERT_NE(ret, GRAPH_SUCCESS);

    auto node = compute_graph->AddOutputNodeByIndex(nullptr, 0);
    ASSERT_EQ(node, nullptr);

    ret = compute_graph->RemoveOutputNode(nullptr);
    ASSERT_NE(ret, GRAPH_SUCCESS);
  }

  auto input_node = compute_graph->AddInputNode(quick_node[0]);
  ASSERT_NE(input_node, nullptr);

  auto output_node = compute_graph->AddOutputNodeByIndex(quick_node[node_num - 1], 0);
  ASSERT_NE(output_node, nullptr);

  auto ret = compute_graph->RemoveInputNode(quick_node[0]);
  ASSERT_EQ(ret, GRAPH_SUCCESS);
  ret = compute_graph->RemoveOutputNode(quick_node[node_num - 1]);
  ASSERT_EQ(ret, GRAPH_SUCCESS);
}

TEST_F(UtestExecuteGraph, TestSortsNodes) {
  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");
  int node_num = 10;
  auto subgraph_num = 10;
  auto subgraph_node_num = 5;
  auto io_num = 5;

  std::shared_ptr<OpDesc> op_desc[node_num] = {nullptr};
  for (int j = 0; j < node_num; j++) {
    op_desc[j] = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();

    for (int64_t i = 0; i < io_num; ++i) {
      op_desc[j]->AddInputDesc(td);
    }
    for (int64_t i = 0; i < io_num; ++i) {
      op_desc[j]->AddOutputDesc(td);
    }

    op_desc[j]->AddSubgraphName("subgraph_" + std::to_string(j));
    op_desc[j]->SetSubgraphInstanceName(j, "subgraph_" + std::to_string(j));
  }

  FastNode *quick_node[node_num] = {};
  for (int i = 0; i < node_num; i++) {
    quick_node[i] = root_graph->AddNode(op_desc[i]);
    ASSERT_NE(quick_node[i], nullptr);
  }

  for (int j = 1; j < node_num; j++) {
    root_graph->AddEdge(quick_node[j], 1, quick_node[j - 1], 0);
  }

  std::shared_ptr<OpDesc> sub_op_desc[subgraph_num][subgraph_node_num] = {};
  for (int i = 0; i < subgraph_num; i++) {
    for (int j = 0; j < subgraph_node_num; j++) {
      sub_op_desc[i][j] = std::make_shared<OpDesc>("op", "op");
      auto td = GeTensorDesc();

      for (int64_t x = 0; x < io_num; ++x) {
        sub_op_desc[i][j]->AddInputDesc(td);
      }
      for (int64_t x = 0; x < io_num; ++x) {
        sub_op_desc[i][j]->AddOutputDesc(td);
      }
    }
  }

  std::shared_ptr<ExecuteGraph> sub_graph[subgraph_num]{};
  for (int i = 0; i < subgraph_num; i++) {
    sub_graph[i] = std::make_shared<ExecuteGraph>("subgraph_" + std::to_string(i));
    FastNode *sub_graph_node[subgraph_node_num] = {};
    for (int j = 0; j < subgraph_node_num; j++) {
      sub_graph_node[j] = sub_graph[i]->AddNode(sub_op_desc[i][j]);
    }
    for (int j = 1; j < subgraph_node_num; j++) {
      sub_graph[i]->AddEdge(sub_graph_node[j], 1, sub_graph_node[j - 1], 0);
      sub_graph[i]->SetParentGraph(root_graph.get());
      sub_graph[i]->SetParentNode(quick_node[i]);
    }
  }

  for (int64_t i = 0; i < subgraph_num; ++i) {
    std::string name = "subgraph_" + std::to_string(i);
    auto ret = root_graph->AddSubGraph(sub_graph[i], name);
    ASSERT_NE(ret, nullptr);
  }

  /* bfs   reverse      no memory priority */
  {
    static const std::string kTopoSortingBfs = "0";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingBfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "xxx";
    graph_options[ENABLE_SINGLE_STREAM] = "true";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* dfs   reverse      no memory priority */
  {
    static const std::string kTopoSortingDfs = "1";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingDfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "xxx";
    graph_options[ENABLE_SINGLE_STREAM] = "true";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* DfsPostOrder   reverse      no memory priority */
  {
    static const std::string kTopoSortingDfsPostOrder = "2";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingDfsPostOrder;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "xxx";
    graph_options[ENABLE_SINGLE_STREAM] = "true";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* bfs   no reverse      no memory priority */
  {
    static const std::string kTopoSortingBfs = "0";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingBfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "xxxx";
    graph_options[ENABLE_SINGLE_STREAM] = "false";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* dfs   no reverse      no memory priority */
  {
    static const std::string kTopoSortingDfs = "1";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingDfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "xxx";
    graph_options[ENABLE_SINGLE_STREAM] = "false";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* DfsPostOrder   no reverse      no memory priority */
  {
    static const std::string kTopoSortingDfsPostOrder = "2";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingDfsPostOrder;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "xx";
    graph_options[ENABLE_SINGLE_STREAM] = "false";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* bfs   reverse       with memory priority */
  {
    static const std::string kTopoSortingBfs = "0";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingBfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "MemoryPriority";
    graph_options[ENABLE_SINGLE_STREAM] = "true";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* dfs   reverse      with memory priority */
  {
    static const std::string kTopoSortingDfs = "1";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingDfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "MemoryPriority";
    graph_options[ENABLE_SINGLE_STREAM] = "true";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* DfsPostOrder   reverse      with memory priority */
  {
    static const std::string kTopoSortingDfsPostOrder = "2";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingDfsPostOrder;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "MemoryPriority";
    graph_options[ENABLE_SINGLE_STREAM] = "true";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* bfs   no reverse       with memory priority */
  {
    static const std::string kTopoSortingBfs = "0";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingBfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "MemoryPriority";
    graph_options[ENABLE_SINGLE_STREAM] = "false";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  /* dfs   no reverse      with memory priority */
  {
    static const std::string kTopoSortingDfs = "1";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingDfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "MemoryPriority";
    graph_options[ENABLE_SINGLE_STREAM] = "false";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }
}

TEST_F(UtestExecuteGraph, TestSortsNodesError) {
  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");
  int node_num = 10;
  auto subgraph_num = 10;
  auto subgraph_node_num = 5;
  auto io_num = 5;

  std::shared_ptr<OpDesc> op_desc[node_num] = {nullptr};
  for (int j = 0; j < node_num; j++) {
    op_desc[j] = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();

    for (int64_t i = 0; i < io_num; ++i) {
      op_desc[j]->AddInputDesc(td);
    }
    for (int64_t i = 0; i < io_num; ++i) {
      op_desc[j]->AddOutputDesc(td);
    }
  }

  FastNode *quick_node[node_num] = {};
  for (int i = 0; i < node_num; i++) {
    quick_node[i] = root_graph->AddNode(op_desc[i]);
    ASSERT_NE(quick_node[i], nullptr);
  }

  FastEdge *edge[node_num] = {};
  for (int j = 1; j < node_num; j++) {
    edge[j] = root_graph->AddEdge(quick_node[j], 1, quick_node[j - 1], 0);
  }

  std::shared_ptr<OpDesc> sub_op_desc[subgraph_num][subgraph_node_num] = {};
  for (int i = 0; i < subgraph_num; i++) {
    for (int j = 0; j < subgraph_node_num; j++) {
      sub_op_desc[i][j] = std::make_shared<OpDesc>("op", "op");
      auto td = GeTensorDesc();

      for (int64_t x = 0; x < io_num; ++x) {
        sub_op_desc[i][j]->AddInputDesc(td);
      }
      for (int64_t x = 0; x < io_num; ++x) {
        sub_op_desc[i][j]->AddOutputDesc(td);
      }
    }
  }

  std::shared_ptr<ExecuteGraph> sub_graph[subgraph_num]{};
  for (int i = 0; i < subgraph_num; i++) {
    sub_graph[i] = std::make_shared<ExecuteGraph>("subgraph_" + std::to_string(i));
    FastNode *sub_graph_node[subgraph_node_num] = {};
    for (int j = 0; j < subgraph_node_num; j++) {
      sub_graph_node[j] = sub_graph[i]->AddNode(sub_op_desc[i][j]);
      sub_graph[i]->SetParentGraph(root_graph.get());
      sub_graph[i]->SetParentNode(quick_node[i]);
    }
  }

  for (int64_t i = 0; i < subgraph_num; ++i) {
    std::string name = "subgraph_" + std::to_string(i);
    auto ret = root_graph->AddSubGraph(sub_graph[i], name);
    ASSERT_NE(ret, nullptr);
  }

  /* ERROR */
  {
    static const std::string kTopoSortingBfs = "1";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingBfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "MemoryPriority";
    graph_options[ENABLE_SINGLE_STREAM] = "false";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_FAILED);
  }

  for (int j = 1; j < node_num; j++) {
    root_graph->RemoveEdge(edge[j]);
  }

  /* ERROR */
  {
    static const std::string kTopoSortingBfs = "1";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingBfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "MemoryPriority";
    graph_options[ENABLE_SINGLE_STREAM] = "false";
    GetThreadLocalContext().SetGraphOption(graph_options);
    auto ret = root_graph->TopologicalSorting();
    ASSERT_EQ(ret, GRAPH_FAILED);
  }
}

TEST_F(UtestExecuteGraph, TestGraphName) {
  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");
  std::string str = "changetohelloworld";
  root_graph->SetName(str);
  auto ret = root_graph->GetName();
  ASSERT_EQ(ret, str);
}

TEST_F(UtestExecuteGraph, TestGraphParent) {
  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");
  auto sub_graph = std::make_shared<ExecuteGraph>("sub_graph");
  sub_graph->SetParentGraph(root_graph.get());
  auto ret = sub_graph->GetParentGraphBarePtr();
  ASSERT_EQ(ret, root_graph.get());
}

TEST_F(UtestExecuteGraph, TestRecycleNode) {
  auto root_graph = std::make_shared<ComputeGraph>("root_graph");
  int node_num = 10;
  int io_num = 5;
  std::shared_ptr<OpDesc> op_desc[node_num] = {nullptr};
  NodePtr node[node_num] = {};
  for (int j = 0; j < 1; j++) {
    op_desc[j] = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();

    for (int64_t i = 0; i < io_num; ++i) {
      op_desc[j]->AddInputDesc(td);
    }
    for (int64_t i = 0; i < io_num; ++i) {
      op_desc[j]->AddOutputDesc(td);
    }
  }

  for (int i = 0; i < 1; i++) {
    node[i] = root_graph->AddNode(op_desc[i]);
  }

  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    op_desc->AddInputDesc(td);
    op_desc->AddOutputDesc(td);
    auto quick_node = compute_graph->AddNode(op_desc);
    ASSERT_NE(quick_node, nullptr);
    quick_node->SetNodePtr(node[0]);
    auto ret = compute_graph->RemoveJustNode(quick_node);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
    ret = compute_graph->RecycleQuickNode(quick_node);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }
  {
    auto ret = compute_graph->RecycleQuickNode(nullptr);
    ASSERT_NE(ret, GRAPH_SUCCESS);

    ret = compute_graph->RecycleQuickEdge(nullptr);
    ASSERT_NE(ret, GRAPH_SUCCESS);
  }
}

TEST_F(UtestExecuteGraph, ClearEdge) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  FastEdge *edge[edge_num] = {};
  for (int i = 0; i < edge_num; ++i) {
    edge[i] = compute_graph->AddEdge(node[0], i, node[1], i);
    ASSERT_NE(edge[i], nullptr);
  }

  for (int i = 0; i < edge_num; ++i) {
    auto ret = compute_graph->RemoveEdge(edge[i]);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }
}

TEST_F(UtestExecuteGraph, DelayTopologicalSorting) {
  auto graph = BuildDelayTopoGraph("test_delay_topo_graph");
  std::map<std::string, std::string> options_map;
  options_map["ge.topoSortingMode"] = "2";
  options_map["ge.exec.memoryOptimizationPolicy"] = "MemoryPriority";
  GetThreadLocalContext().SetGraphOption(options_map);
  EXPECT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  std::vector<std::string> expected_dfs_names = {"variable", "data", "node2", "node3", "node4", "node1", "node5"};
  std::vector<std::string> dfs_names;
  const auto &graph_dfs_topo = graph->GetAllNodes();
  for (auto &node : graph_dfs_topo) {
    dfs_names.push_back(node->GetName());
  }

  EXPECT_EQ(dfs_names, expected_dfs_names);
}

TEST_F(UtestExecuteGraph, NoDelayTopologicalSorting) {
  auto graph = BuildDelayTopoGraph("test_delay_topo_graph");
  std::map<std::string, std::string> options_map;
  options_map["ge.topoSortingMode"] = "1";
  GetThreadLocalContext().SetGraphOption(options_map);
  EXPECT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  std::vector<std::string> expected_dfs_names = {"variable", "node2", "node1", "data", "node3", "node4", "node5"};
  std::vector<std::string> dfs_names;
  const auto &graph_dfs_topo = graph->GetAllNodes();
  for (auto &node : graph_dfs_topo) {
    dfs_names.push_back(node->GetName());
  }
  EXPECT_EQ(dfs_names, expected_dfs_names);

  {
    /* recovery environment */
    static const std::string kTopoSortingDfs = "1";
    auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
    auto graph_options = ori_graph_options;
    graph_options[OPTION_TOPOSORTING_MODE] = kTopoSortingDfs;
    graph_options[MEMORY_OPTIMIZATION_POLICY] = "xxx";
    graph_options[ENABLE_SINGLE_STREAM] = "false";
    GetThreadLocalContext().SetGraphOption(graph_options);
  }
}

TEST_F(UtestExecuteGraph, TestNodeAttr) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  auto op_desc = std::make_shared<OpDesc>("op", "op");
  auto td = GeTensorDesc();
  op_desc->AddInputDesc(td);
  op_desc->AddOutputDesc(td);
  auto node = compute_graph->AddNode(op_desc);
  ASSERT_NE(node, nullptr);
  ASSERT_NE(node->GetExtendInfo(), nullptr);
  auto name_ptr = node->GetNamePtr();
  ASSERT_NE(name_ptr, nullptr);
  auto name = node->GetName();
  ASSERT_EQ(name, "op");
  auto type = node->GetType();
  ASSERT_EQ(name, "op");
  auto type_ptr = node->GetTypePtr();
  ASSERT_NE(type_ptr, nullptr);
  auto graph = node->GetExtendInfo()->GetOwnerGraphBarePtr();
  ASSERT_EQ(graph, compute_graph.get());

  auto ret = compute_graph->RemoveJustNode(node);
  ASSERT_EQ(ret, GRAPH_SUCCESS);

  auto node1 = compute_graph->AddNode(node);
  ASSERT_EQ(node1, node);

  {
    auto op_desc1 = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    op_desc1->AddInputDesc(td);
    op_desc1->AddOutputDesc(td);
    auto new_node_1 = compute_graph->AddNode(op_desc);
    new_node_1->Init(op_desc);
    ASSERT_NE(*new_node_1 == *node1, true);
  }
}

TEST_F(UtestExecuteGraph, TestNodeEqual) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  auto op_desc = std::make_shared<OpDesc>("op", "op");
  auto td = GeTensorDesc();
  op_desc->AddInputDesc(td);
  op_desc->AddOutputDesc(td);
  auto node = compute_graph->AddNode(op_desc);

  auto ret = compute_graph->RemoveJustNode(node);
  ASSERT_EQ(ret, GRAPH_SUCCESS);

  auto node1 = compute_graph->AddNode(node);
  ASSERT_EQ(*node1 == *node, true);
}

TEST_F(UtestExecuteGraph, TestEdgeError) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  int32_t invalid_index = -2;
  {
    QuickEdge *edge = new QuickEdge;
    FastGraphUtils::GetEdge(edge).dst_input = invalid_index;
    graphStatus ret = node[0]->RecordEdge(&FastGraphUtils::GetEdge(edge), DirectionType::kDirectionInType);
    ASSERT_NE(ret, GRAPH_SUCCESS);
    delete edge;
  }

  {
    QuickEdge *edge = new QuickEdge;
    FastGraphUtils::GetEdgeSrcOutput(edge) = invalid_index;
    graphStatus ret = node[0]->RecordEdge(&FastGraphUtils::GetEdge(edge), DirectionType::kDirectionOutType);
    ASSERT_NE(ret, GRAPH_SUCCESS);
    delete edge;
  }

  {
    QuickEdge *edge = new QuickEdge;
    FastGraphUtils::GetEdgeSrcOutput(edge) = invalid_index;
    graphStatus ret = node[0]->EraseEdge(&FastGraphUtils::GetEdge(edge), DirectionType::kDirectionOutType);
    ASSERT_NE(ret, GRAPH_SUCCESS);
    delete edge;
  }

  {
    QuickEdge *edge = new QuickEdge;
    FastGraphUtils::GetEdgeDstInput(edge) = invalid_index;
    graphStatus ret = node[0]->EraseEdge(&FastGraphUtils::GetEdge(edge), DirectionType::kDirectionInType);
    ASSERT_NE(ret, GRAPH_SUCCESS);
    delete edge;
  }
}

TEST_F(UtestExecuteGraph, TestNodeEdgeError) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }
  {
    auto ret = node[0]->GetPeerNodesInDataEdge(-2);
    ASSERT_EQ(ret.size(), 0);
  }

  {
    auto ret = node[1]->GetPeerNodesInControlEdge(-2);
    ASSERT_EQ(ret.size(), 0);
  }

  {
    auto ret = node[2]->GetPeerOutDataEdge(-2);
    ASSERT_EQ(ret, nullptr);
  }
}

TEST_F(UtestExecuteGraph, TestMoveEdge) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  FastEdge *edge[edge_num] = {};
  for (int i = 0; i < edge_num; ++i) {
    edge[i] = compute_graph->AddEdge(node[0], i, node[1], i);
    ASSERT_NE(edge[i], nullptr);
  }

  for (int i = 0; i < edge_num; ++i) {
    auto ret = compute_graph->RemoveEdge(edge[i]);
    ASSERT_EQ(ret, GRAPH_SUCCESS);
  }

  edge[0] = compute_graph->AddEdge(node[0], 0, node[1], 0);

  auto size = node[0]->GetOutEdgesSizeByIndex(0);
  ASSERT_EQ(size, 1);

  auto edges = node[0]->GetOutEdgesByIndex(0);
  ASSERT_EQ(edges.empty(), false);
  ASSERT_EQ(edges[1], edge[0]);

  size = node[1]->GetInEdgesSizeByIndex(0);
  ASSERT_EQ(size, 1);

  size = node[1]->GetInEdgesSizeByIndex(-1);
  ASSERT_EQ(size, 0);

  auto data_edge = node[1]->GetInDataEdgesByIndex(0);
  ASSERT_EQ(data_edge, edge[0]);

  auto ret = node[0]->MoveEdge(DirectionType::kDirectionOutType, 0, 1, 0);
  ASSERT_EQ(ret, GRAPH_SUCCESS);

  size = node[0]->GetOutEdgesSizeByIndex(0);
  ASSERT_EQ(size, 1);

  size = node[0]->GetOutEdgesSizeByIndex(-1);
  ASSERT_EQ(size, 0);

  edges = node[0]->GetOutEdgesByIndex(0);
  ASSERT_EQ(edges[0], edge[0]);

  ret = node[1]->MoveEdge(DirectionType::kDirectionInType, 0, 0, 0);
  ASSERT_EQ(ret, GRAPH_SUCCESS);

  size = node[1]->GetInEdgesSizeByIndex(0);
  ASSERT_EQ(size, 1);

  data_edge = node[1]->GetInDataEdgesByIndex(0);
  ASSERT_EQ(data_edge, edge[0]);
}

TEST_F(UtestExecuteGraph, GetDataEdgeInfo) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  FastEdge *edge[edge_num] = {};
  for (int i = 0; i < edge_num; ++i) {
    edge[i] = compute_graph->AddEdge(node[0], i, node[1], i);
    ASSERT_NE(edge[i], nullptr);
  }

  auto out_size = node[0]->GetAllOutEdgesSize();
  ASSERT_EQ(out_size, edge_num);

  auto nodes = node[0]->GetPeerNodesInDataEdge(0);
  ASSERT_EQ(nodes.size(), 1);

  auto peer_size = node[0]->GetAllPeerEdgesSizeByIndex(DirectionType::kDirectionOutType, 1);
  ASSERT_EQ(peer_size, 1);

  auto size = node[1]->GetAllInEdgeSize();
  ASSERT_EQ(size, edge_num);
}

TEST_F(UtestExecuteGraph, GetControlEdgeInfo) {
  auto compute_graph = std::make_shared<ge::ExecuteGraph>("graph");
  int node_num = 10;
  int edge_num = 5;
  FastNode *node[node_num] = {};
  for (int i = 0; i < node_num; ++i) {
    auto op_desc = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();
    for (int j = 0; j < edge_num; ++j) {
      op_desc->AddInputDesc(td);
      op_desc->AddOutputDesc(td);
    }
    node[i] = compute_graph->AddNode(op_desc);
    ASSERT_NE(node[i], nullptr);
  }

  FastEdge *edge[node_num] = {};
  for (int i = 1; i < node_num; ++i) {
    edge[i] = compute_graph->AddEdge(node[0], kControlEdgeIndex, node[i], kControlEdgeIndex);
    ASSERT_NE(edge[i], nullptr);
  }

  auto out_edges = node[0]->GetOutEdgesRefByIndex(kControlEdgeIndex);
  ASSERT_EQ(out_edges.size(), node_num - 1);

  auto out_control_nodes = node[0]->GetOutControlNodes();
  ASSERT_EQ(out_control_nodes.size(), node_num - 1);

  auto nodes = node[0]->GetPeerNodesInControlEdge(kControlEdgeIndex);
  ASSERT_EQ(nodes.size(), node_num - 1);

  auto in_control_nodes = node[1]->GetInControlNodes();
  ASSERT_EQ(in_control_nodes.size(), 1);
}

TEST_F(UtestExecuteGraph, deepcopy) {
  auto node_num = 10;
  auto io_num = 10;
  auto subgraph_num = 10;
  auto subgraph_node_num = 10;
  std::shared_ptr<OpDesc> op_desc[node_num] = {nullptr};
  for (int j = 0; j < node_num; j++) {
    op_desc[j] = std::make_shared<OpDesc>("op", "op");
    auto td = GeTensorDesc();

    for (int64_t i = 0; i < io_num; ++i) {
      op_desc[j]->AddInputDesc(td);
    }
    for (int64_t i = 0; i < io_num; ++i) {
      op_desc[j]->AddOutputDesc(td);
    }
  }

  std::shared_ptr<ExecuteGraph> sub_graph[subgraph_num] = {nullptr};
  FastNode *node[node_num] = {};
  FastEdge *edge[node_num] = {};
  ExecuteGraph *quick_graph[subgraph_num] = {nullptr};
  std::shared_ptr<OpDesc> sub_op_desc[subgraph_num][subgraph_node_num] = {};
  for (int i = 0; i < subgraph_num; i++) {
    for (int j = 0; j < subgraph_node_num; j++) {
      sub_op_desc[i][j] = std::make_shared<OpDesc>("op", "op");
      auto td = GeTensorDesc();
      for (int64_t x = 0; x < io_num; ++x) {
        sub_op_desc[i][j]->AddInputDesc(td);
      }
      for (int64_t x = 0; x < io_num; ++x) {
        sub_op_desc[i][j]->AddOutputDesc(td);
      }
    }
  }

  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");
  for (int i = 0; i < node_num; i++) {
    node[i] = root_graph->AddNode(op_desc[i]);
    ASSERT_NE(node[i], nullptr);
  }
  auto input_node = root_graph->AddInputNode(node[0]);
  ASSERT_NE(input_node, nullptr);

  for (int i = 1; i < node_num; i++) {
    edge[i] = root_graph->AddEdge(node[i], 1, node[i - 1], 0);
    ASSERT_NE(edge[i], nullptr);
  }

  for (int i = 0; i < subgraph_num; i++) {
    std::string name = "subgraph_" + std::to_string(i);
    sub_graph[i] = std::make_shared<ExecuteGraph>(name);
    FastNode *sub_graph_node[subgraph_node_num] = {};
    for (int j = 0; j < subgraph_node_num; j++) {
      sub_graph_node[j] = sub_graph[i]->AddNode(sub_op_desc[i][j]);
      ASSERT_NE(sub_graph_node[j], nullptr);
    }
    for (int j = 1; j < subgraph_node_num; j++) {
      auto ret = sub_graph[i]->AddEdge(sub_graph_node[j], 1, sub_graph_node[j - 1], 0);
      ASSERT_NE(ret, nullptr);
    }
  }

  for (int i = 0; i < subgraph_num; ++i) {
    sub_graph[i]->SetParentGraph(root_graph.get());
    sub_graph[i]->SetParentNode(node[i]);
    std::string name = "subgraph_" + std::to_string(i);
    quick_graph[i] = root_graph->AddSubGraph(sub_graph[i], name);
    ASSERT_NE(quick_graph[i], nullptr);
  }

  std::string name = "root_graph";
  auto test1_graph = std::make_shared<ExecuteGraph>(name);
  test1_graph->CompleteCopy(*(root_graph.get()));
}
}  // namespace ge
