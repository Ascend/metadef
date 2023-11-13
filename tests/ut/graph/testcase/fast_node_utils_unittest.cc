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
#include "fast_graph/fast_node_utils.h"
#include "graph_builder_utils.h"
#include "graph/debug/ge_op_types.h"
#include "graph/debug/ge_attr_define.h"

namespace ge {
class UtestFastNodeUtils : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestFastNodeUtils, GetConstOpType_CONST) {
  ut::ExecuteGraphBuilder builder = ut::ExecuteGraphBuilder("graph");
  auto data = builder.AddNode("const1", CONSTANT, 0, 1);
  std::cout << data->GetType() << std::endl;
  auto ret = FastNodeUtils::GetConstOpType(data);
  ASSERT_EQ(ret, true);
}

TEST_F(UtestFastNodeUtils, GetConstOpType_DATA) {
  ut::ExecuteGraphBuilder builder = ut::ExecuteGraphBuilder("graph");
  auto data = builder.AddNode("Data", "Data", 0, 1);
  std::cout << data->GetType() << std::endl;
  std::string op_type;
  auto ret = FastNodeUtils::GetConstOpType(data);
  ASSERT_EQ(ret, false);
}

TEST_F(UtestFastNodeUtils, GetConstOpType) {
  ut::ExecuteGraphBuilder builder = ut::ExecuteGraphBuilder("graph");
  auto data = builder.AddNode("nt", NETOUTPUT, 0, 1);
  EXPECT_EQ(FastNodeUtils::GetConstOpType(data), false);
}

TEST_F(UtestFastNodeUtils, GetParentInput_invalid) {
  auto builder = ut::ExecuteGraphBuilder("test_graph0");
  const auto &data_node = builder.AddNode("data", DATA, 0, 0);
  auto graph = builder.GetGraph();
  AttrUtils::SetInt(data_node->GetOpDescPtr(), ge::ATTR_NAME_PARENT_NODE_INDEX, 1);
  EXPECT_EQ(FastNodeUtils::GetParentInput(data_node), nullptr);
}

TEST_F(UtestFastNodeUtils, GetParentInput) {
  auto node_num = 10;
  auto io_num = 10;
  auto subgraph_num = 1;
  auto subgraph_node_num = 10;
  std::shared_ptr<OpDesc> op_desc[node_num] = {nullptr};
  for (int j = 0; j < node_num; j++) {
    if (j == 1) {
      op_desc[j] = std::make_shared<OpDesc>("op", DATA);
    } else {
      op_desc[j] = std::make_shared<OpDesc>("op", "op");
    }

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

  auto root_graph = std::make_shared<ExecuteGraph>("root_graph");
  for (int i = 0; i < node_num; i++) {
    node[i] = root_graph->AddNode(op_desc[i]);
    ASSERT_NE(node[i], nullptr);
  }

  for (int i = 1; i < node_num; i++) {
    edge[i] = root_graph->AddEdge(node[i], 1, node[i - 1], 0);
    ASSERT_NE(edge[i], nullptr);
  }

  FastNode *sub_graph_node[subgraph_node_num] = {};
  std::string name = "subgraph_" + std::to_string(0);
  sub_graph[0] = std::make_shared<ExecuteGraph>(name);

  for (int j = 0; j < subgraph_node_num; j++) {
    sub_graph_node[j] = sub_graph[0]->AddNode(op_desc[j]);
    AttrUtils::SetInt(sub_graph_node[j]->GetOpDescPtr(), ge::ATTR_NAME_PARENT_NODE_INDEX, 0);
    ASSERT_NE(sub_graph_node[j], nullptr);
  }

  sub_graph[0]->SetParentGraph(root_graph.get());
  sub_graph[0]->SetParentNode(node[0]);
  quick_graph[0] = root_graph->AddSubGraph(sub_graph[0], name);
  ASSERT_NE(quick_graph[0], nullptr);

  EXPECT_EQ(FastNodeUtils::GetParentInput(sub_graph_node[1]), nullptr);
}

}  // namespace ge
