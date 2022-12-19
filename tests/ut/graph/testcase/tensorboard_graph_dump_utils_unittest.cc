/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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
#include "graph/utils/tensorboard_graph_dump_utils.h"
#include <vector>
#include <gtest/gtest.h>

#include "mmpa/mmpa_api.h"
#include "graph_builder_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/graph_utils_inner.h"
#include "graph/utils/node_utils.h"
#include "proto/tensorflow/graph.pb.h"
#include "proto/tensorflow/types.pb.h"

namespace ge {
namespace {
  const char_t *const kDumpPath = "./graph_dump";

// construct graph which contains subgraph
 ge::ComputeGraphPtr BuildComputeGraph(const std::string &name) {
  ut::GraphBuilder builder = ut::GraphBuilder(name);
  auto data = builder.AddNode("Data", "Data", 0, 1);

  auto transdata = builder.AddNode("Transdata", "Transdata", 1, 1);
  auto opdesc = transdata->GetOpDesc();
  opdesc->AddSubgraphName("subgraph");
  opdesc->SetSubgraphInstanceName(0, "subgraph");
  ge::AttrUtils::SetInt(opdesc, "attr_int", 1);
  ge::AttrUtils::SetFloat(opdesc, "attr_float", 1234.1234);
  ge::AttrUtils::SetBool(opdesc, "attr_bool", true);
  ge::AttrUtils::SetStr(opdesc, "attr_str", "good boy");
  ge::AttrUtils::SetListInt(opdesc, "attr_list_int", std::vector<int64_t>({10, 400, 3000, 8192}));
  ge::AttrUtils::SetListFloat(opdesc, "attr_list_float", std::vector<float>({1.1, 2.2, 3.3, 4.4}));
  ge::AttrUtils::SetListBool(opdesc, "attr_list_bool", std::vector<bool>({true, false, true, true}));
  ge::AttrUtils::SetListStr(opdesc, "attr_list_string", std::vector<std::string>({"aaaa", "bbbb"}));
  // unsupported attr
  ge::AttrUtils::SetDataType(opdesc, "dtype", ge::DT_INT32);

  auto netoutput = builder.AddNode("Netoutput", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, transdata, 0);
  builder.AddDataEdge(transdata, 0, netoutput, 0);

  //add control edge
  const auto &node1 = builder.AddNode("scope1/node1", "node1", 1, 1);
  const auto &node2 = builder.AddNode("scope1/node2", "node2", 1, 1);
  const auto &node3 = builder.AddNode("scope1/node3", "node3", 1, 1);
  builder.AddControlEdge(node1, node2);
  builder.AddControlEdge(node3, node1);

  auto graph = builder.GetGraph();
  return graph;
}

ge::ComputeGraphPtr BuildSubGraph(const std::string &name) {
  ge::ut::GraphBuilder builder(name);
  auto data1 = builder.AddNode(name + "data1", "Data", 1, 1);
  auto data2 = builder.AddNode(name + "data2", "Data", 1, 1);
  auto add = builder.AddNode(name + "sub", "Sub", 2, 1);
  auto netoutput = builder.AddNode(name + "_netoutput", "NetOutput", 1, 1);

  ge::AttrUtils::SetInt(data1->GetOpDesc(), "_parent_node_index", static_cast<int>(0));
  ge::AttrUtils::SetInt(data2->GetOpDesc(), "_parent_node_index", static_cast<int>(1));
  ge::AttrUtils::SetInt(netoutput->GetOpDesc()->MutableInputDesc(0), "_parent_node_index", static_cast<int>(0));

  builder.AddDataEdge(data1, 0, add, 0);
  builder.AddDataEdge(data2, 0, add, 1);
  builder.AddDataEdge(add, 0, netoutput, 0);

  return builder.GetGraph();
}
/*
 *   netoutput
 *       |
 *      if
 *     /   \
 * data1   data2
 */
ge::ComputeGraphPtr BuildMainGraphWithIf(const std::string &name) {
  ge::ut::GraphBuilder builder(name);
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto if1 = builder.AddNode("if", "If", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput", "NetOutput", 1, 1);

  builder.AddDataEdge(data1, 0, if1, 0);
  builder.AddDataEdge(data2, 0, if1, 1);
  builder.AddDataEdge(if1, 0, netoutput1, 0);

  auto main_graph = builder.GetGraph();

  auto sub1 = BuildSubGraph("sub1");
  sub1->SetParentGraph(main_graph);
  sub1->SetParentNode(main_graph->FindNode("if"));
  main_graph->FindNode("if")->GetOpDesc()->AddSubgraphName("sub1");
  main_graph->FindNode("if")->GetOpDesc()->SetSubgraphInstanceName(0, "sub1");
  main_graph->AddSubgraph("sub1", sub1);

  auto sub2 = BuildSubGraph("sub2");
  sub2->SetParentGraph(main_graph);
  sub2->SetParentNode(main_graph->FindNode("if"));
  main_graph->FindNode("if")->GetOpDesc()->AddSubgraphName("sub2");
  main_graph->FindNode("if")->GetOpDesc()->SetSubgraphInstanceName(1, "sub2");
  main_graph->AddSubgraph("sub2", sub2);

  return main_graph;
}
}

class TensorBoardGraphDumpUtilsUT : public testing::Test {
 protected:
  void SetUp() {
    unsetenv(kNpuCollectPath);
  }
  void TearDown() {
    unsetenv(kDumpGeGraph);
    unsetenv(kDumpGraphPath);
    rmdir(kDumpPath);
  }
};

TEST_F(TensorBoardGraphDumpUtilsUT, DumpGraph_NoNeedDump) {
  mmSetEnv(kDumpGeGraph, "0", 1);
  auto compute_graph = BuildComputeGraph("graph");
  EXPECT_EQ(TensorBoardGraphDumpUtils::DumpGraph(*compute_graph, "dumpgraph"), GRAPH_NOT_CHANGED);
}

TEST_F(TensorBoardGraphDumpUtilsUT, DumpGraph_DumpGraphWithoutSubGraph) {
  mmSetEnv(kDumpGeGraph, "1", 1);
  mmSetEnv(kDumpGraphPath, kDumpPath, 1);
  auto compute_graph = BuildComputeGraph("graph");
  EXPECT_EQ(TensorBoardGraphDumpUtils::DumpGraph(*compute_graph, "dumpgraph"), GRAPH_SUCCESS);

  domi::tensorflow::GraphDef graph_def;
  std::string dump_file_name = std::string(kDumpPath) + "/ge_tensorboard_00000000_graph_0_dumpgraph.pbtxt";
  EXPECT_EQ(GraphUtils::ReadProtoFromTextFile(dump_file_name.c_str(), &graph_def), true);

  ASSERT_EQ(graph_def.node_size(), 6);

  //check node with attr
  auto node_def_1 = graph_def.node(1);
  ASSERT_EQ(node_def_1.name(), "Transdata");
  ASSERT_EQ(node_def_1.op(), "Transdata");
  ASSERT_EQ(node_def_1.input_size(), 1);
  ASSERT_EQ(node_def_1.input(0), "Data:0");
  //check attr
  ASSERT_EQ(node_def_1.attr_size(),  10);
  auto mutable_attr = node_def_1.mutable_attr();
  ASSERT_EQ(mutable_attr->at("attr_int").i(),  1);
  ASSERT_FLOAT_EQ(mutable_attr->at("attr_float").f(),  1234.1234);
  ASSERT_EQ(mutable_attr->at("attr_bool").b(),  true);
  ASSERT_EQ(mutable_attr->at("attr_str").s(),  "good boy");
  ASSERT_EQ(mutable_attr->at("subgraph").s(),  "subgraph");
  std::vector<int64_t> list_int = {10, 400, 3000, 8192};
  auto list_i = mutable_attr->at("attr_list_int").list().i();
  auto list_valu_i = std::vector<int64_t>{list_i.begin(), list_i.end()};
  ASSERT_EQ(list_valu_i, list_int);
  std::vector<float> list_float = {1.1, 2.2, 3.3, 4.4};
  auto list_f = mutable_attr->at("attr_list_float").list().f();
  auto list_value_f = std::vector<float>{list_f.begin(), list_f.end()};
  ASSERT_EQ(list_value_f, list_float);
  std::vector<bool> list_bool = {true, false, true, true};
  auto list_b = mutable_attr->at("attr_list_bool").list().b();
  //auto list_value_b = std::vector<bool>{list_b.begin(), list_b.end()};
  ASSERT_EQ(list_b.size(), list_bool.size());

  std::vector<std::string> list_str = {"aaaa", "bbbb"};
  auto list_s = mutable_attr->at("attr_list_string").list().s();
  auto list_value_s = std::vector<std::string>{list_s.begin(), list_s.end()};
  ASSERT_EQ(list_value_s, list_str);

  //unsupported attr with empty value
  ASSERT_EQ(mutable_attr->at("dtype").type(), domi::tensorflow::DT_INVALID);

  //check node with ctrl input
  auto node_def_4 = graph_def.node(4);
  ASSERT_EQ(node_def_4.name(), "scope1/node1");
  ASSERT_EQ(node_def_4.op(), "node1");
  ASSERT_EQ(node_def_4.input_size(), 1);
  ASSERT_EQ(node_def_4.input(0), "^scope1/node3");

  unlink(dump_file_name.c_str());
}

TEST_F(TensorBoardGraphDumpUtilsUT, DumpGraph_DumpGraphWithSubGraph) {
  mmSetEnv(kDumpGeGraph, "1", 1);
  mmSetEnv(kDumpGraphPath, kDumpPath, 1);
  auto compute_graph = BuildMainGraphWithIf("subgraph");
  EXPECT_EQ(TensorBoardGraphDumpUtils::DumpGraph(*compute_graph, "dumpsubgraph"), GRAPH_SUCCESS);

  domi::tensorflow::GraphDef graph_def;
  std::string dump_file_name = std::string(kDumpPath) + "/ge_tensorboard_00000001_graph_0_dumpsubgraph.pbtxt";
  EXPECT_EQ(GraphUtils::ReadProtoFromTextFile(dump_file_name.c_str(), &graph_def), true);

  ASSERT_EQ(graph_def.node_size(), 4);
  ASSERT_EQ(graph_def.library().function_size(), 2);
  ASSERT_EQ(graph_def.library().function(0).signature().name(), "sub1");
  ASSERT_EQ(graph_def.library().function(0).node_def().size(), 4);
  ASSERT_EQ(graph_def.library().function(1).signature().name(), "sub2");
  ASSERT_EQ(graph_def.library().function(1).node_def().size(), 4);

  unlink(dump_file_name.c_str());
}
}  // namespace ge