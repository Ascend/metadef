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

#define private public
#define protected public
#include "graph/tuning_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/node_impl.h"
#include "graph/op_desc_impl.h"
#include "graph_builder_utils.h"
#include "graph/debug/ge_op_types.h"
#include "graph/operator_factory_impl.h"
#include "graph/compute_graph_impl.h"
#include "graph/anchor.h"
#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {

class UtestTuningUtils : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

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
ComputeGraphPtr BuildGraphPartitionCall1() {
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

TEST_F(UtestTuningUtils, ConvertGraphToFile) {
	std::vector<ComputeGraphPtr> tuning_subgraphs;
	std::vector<ComputeGraphPtr> non_tuning_subgraphs;
	auto builder = ut::GraphBuilder("non_tun");
	const auto data0 = builder.AddNode("data_0", DATA, 0, 1);
    const auto data1 = builder.AddNode("data_1", DATA, 1, 1);
    auto nongraph = builder.GetGraph();
	tuning_subgraphs.push_back(BuildGraphPartitionCall1());
	tuning_subgraphs.push_back(nongraph);
	non_tuning_subgraphs.push_back(nongraph);
	EXPECT_EQ(TuningUtils::ConvertGraphToFile(tuning_subgraphs, non_tuning_subgraphs), GRAPH_SUCCESS);
}

TEST_F(UtestTuningUtils, PrintCheckLog) {
	EXPECT_NE(TuningUtils::PrintCheckLog(), "");
}

TEST_F(UtestTuningUtils, GetNodeNameByAnchor) {
	EXPECT_EQ(TuningUtils::GetNodeNameByAnchor(nullptr), "Null");
}

TEST_F(UtestTuningUtils, CreateDataNode) {
	ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node = builder.AddNode("Data", "Data", 1, 1);
    NodePtr data_node;
	EXPECT_EQ(TuningUtils::CreateDataNode(node, data_node), GRAPH_SUCCESS);
}

TEST_F(UtestTuningUtils, AddAttrToDataNodeForMergeGraph) {
	ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    auto node1 = builder.AddNode("Data1", "Data", 1, 1);
	EXPECT_EQ(TuningUtils::AddAttrToDataNodeForMergeGraph(node0, node1), FAILED);
}

TEST_F(UtestTuningUtils, ChangePld2Data) {
	ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    auto node1 = builder.AddNode("Data1", "Data", 1, 1);
	EXPECT_EQ(TuningUtils::ChangePld2Data(node0, node1), FAILED);
}

TEST_F(UtestTuningUtils, HandlePld) {
	ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
	EXPECT_EQ(TuningUtils::HandlePld(node0), FAILED);
}

TEST_F(UtestTuningUtils, CreateNetOutput) {
	ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    auto node1 = builder.AddNode("Data1", "Data", 1, 1);
	EXPECT_EQ(TuningUtils::CreateNetOutput(node0, node1), FAILED);
}

TEST_F(UtestTuningUtils, AddAttrToNetOutputForMergeGraph) {
	ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    auto node1 = builder.AddNode("Data1", "Data", 1, 1);
	EXPECT_EQ(TuningUtils::AddAttrToNetOutputForMergeGraph(node0, node1, 0), SUCCESS);
}

TEST_F(UtestTuningUtils, LinkEnd2NetOutput) {
	ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    auto node1 = builder.AddNode("Data1", "Data", 1, 1);
	EXPECT_EQ(TuningUtils::LinkEnd2NetOutput(node0, node1), PARAM_INVALID);
}

TEST_F(UtestTuningUtils, ChangeEnd2NetOutput) {
    ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    auto node1 = builder.AddNode("Data1", "Data", 1, 1);
    EXPECT_EQ(TuningUtils::ChangeEnd2NetOutput(node0, node1), FAILED);
}

TEST_F(UtestTuningUtils, HandleEnd) {
    ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    EXPECT_EQ(TuningUtils::HandleEnd(node0), FAILED);
}

TEST_F(UtestTuningUtils, ConvertFileToGraph) {
    std::map<int64_t, std::string> options;
    Graph g;
    EXPECT_EQ(TuningUtils::ConvertFileToGraph(options, g), SUCCESS);
}


TEST_F(UtestTuningUtils, MergeSubGraph) {
    ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    auto graph = builder.GetGraph();
    EXPECT_EQ(TuningUtils::MergeSubGraph(graph), SUCCESS);
}

TEST_F(UtestTuningUtils, FindNode) {
    /*ut::GraphBuilder builder = ut::GraphBuilder("graph");
    auto node0 = builder.AddNode("Data0", "Data", 1, 1);
    auto graph = builder.GetGraph();*/
    int64_t in_index;
    EXPECT_EQ(TuningUtils::FindNode("Data0", in_index), nullptr);
}



} // namespace ge