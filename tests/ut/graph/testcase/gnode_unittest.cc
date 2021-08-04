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

#include "graph/gnode.h"
#include "graph/node_impl.h"
#include "graph/utils/node_adapter.h"
#include "graph_builder_utils.h"

#define protected public
#define private public

namespace ge {
class GNodeTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(GNodeTest, GetALLSubgraphs) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("node", "node", 0, 0);
  const auto &root_graph = root_builder.GetGraph();

  auto sub_builder = ut::GraphBuilder("sub");
  const auto &sub_graph = sub_builder.GetGraph();
  root_graph->AddSubGraph(sub_graph);
  sub_graph->SetParentNode(node);
  sub_graph->SetParentGraph(root_graph);
  node->GetOpDesc()->AddSubgraphName("branch1");
  node->GetOpDesc()->SetSubgraphInstanceName(0, "sub");

  std::vector<GraphPtr> subgraphs;
  ASSERT_EQ(NodeAdapter::Node2GNode(node).GetALLSubgraphs(subgraphs), GRAPH_SUCCESS);
  ASSERT_EQ(subgraphs.size(), 1);
}

TEST_F(GNodeTest, GetALLSubgraphs_nullptr_root_graph) {
  auto builder = ut::GraphBuilder("graph");
  const auto &node = builder.AddNode("node", "node", 0, 0);
  node->impl_->owner_graph_.reset();

  std::vector<GraphPtr> subgraphs;
  ASSERT_NE(NodeAdapter::Node2GNode(node).GetALLSubgraphs(subgraphs), GRAPH_SUCCESS);
  ASSERT_TRUE(subgraphs.empty());
}
}  // namespace ge
