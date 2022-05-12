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
#include "exe_graph/lowering/value_holder.h"
#include "exe_graph/runtime/context_extend.h"
#include <gtest/gtest.h>
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "checker/bg_test.h"
namespace gert {
namespace bg {
class ValueHolderUt : public BgTest {
};
TEST_F(ValueHolderUt, CreateConstOk) {
  ge::Format f1 = ge::FORMAT_NC1HWC0;
  auto c = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  EXPECT_NE(c, nullptr);
  ASSERT_TRUE(c->IsOk());
  ASSERT_NE(c->GetNode(), nullptr);
  EXPECT_EQ(c->GetType(), ValueHolder::kConst);
  EXPECT_EQ(c->GetOutIndex(), 0);
  auto node = c->GetNode();
  EXPECT_EQ(node->GetType(), "Const");
  EXPECT_EQ(node->GetAllOutDataAnchorsSize(), 1);
  EXPECT_EQ(node->GetAllInDataAnchorsSize(), 0);
  ge::Buffer buffer;
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(node->GetOpDesc(), "value", buffer));
  ASSERT_EQ(buffer.GetSize(), sizeof(ge::FORMAT_NC1HWC0));
  EXPECT_EQ(*reinterpret_cast<ge::Format *>(buffer.GetData()), ge::FORMAT_NC1HWC0);
}
TEST_F(ValueHolderUt, CreateVectorConstOk) {
  int64_t shape[5] = {32, 1, 224, 224, 16};
  auto c = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(shape), sizeof(shape));
  EXPECT_NE(c, nullptr);
  ASSERT_TRUE(c->IsOk());
  ASSERT_NE(c->GetNode(), nullptr);
  EXPECT_EQ(c->GetType(), ValueHolder::kConst);
  EXPECT_EQ(c->GetOutIndex(), 0);
  auto node = c->GetNode();
  EXPECT_EQ(node->GetType(), "Const");
  ge::Buffer buffer;
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(node->GetOpDesc(), "value", buffer));
  ASSERT_EQ(buffer.GetSize(), sizeof(shape));
  EXPECT_EQ(memcmp(buffer.GetData(), shape, sizeof(shape)), 0);
}
TEST_F(ValueHolderUt, CreateFeedOk) {
  auto c = ValueHolder::CreateFeed(1);
  EXPECT_NE(c, nullptr);
  ASSERT_TRUE(c->IsOk());
  ASSERT_NE(c->GetNode(), nullptr);
  EXPECT_EQ(c->GetType(), ValueHolder::kFeed);
  EXPECT_EQ(c->GetOutIndex(), 0);
  auto node = c->GetNode();
  EXPECT_EQ(node->GetType(), "Data");
  EXPECT_EQ(node->GetAllOutDataAnchorsSize(), 1);
  EXPECT_EQ(node->GetAllInDataAnchorsSize(), 0);
  int32_t index;
  ASSERT_TRUE(ge::AttrUtils::GetInt(node->GetOpDesc(), "index", index));
  EXPECT_EQ(index, 1);
}
TEST_F(ValueHolderUt, CreateErrorOk) {
  auto holder = ValueHolder::CreateError("This is a test error information, int %d, %s", 10240, "Test msg");
  ASSERT_NE(holder, nullptr);
  EXPECT_FALSE(holder->IsOk());
}
TEST_F(ValueHolderUt, CreateDataOutOk) {
  ge::Format f1 = ge::FORMAT_NC1HWC0;
  auto const1 = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  auto data1 = ValueHolder::CreateFeed(0);
  ASSERT_NE(const1, nullptr);
  ASSERT_NE(data1, nullptr);

  std::vector<ValueHolderPtr> inputs = {data1, const1};
  auto holders = ValueHolder::CreateDataOutput("TestNode", inputs, 3);

  ASSERT_EQ(holders.size(), 3);
  ASSERT_TRUE(holders[0]->IsOk());
  ASSERT_TRUE(holders[1]->IsOk());
  ASSERT_TRUE(holders[2]->IsOk());
  EXPECT_EQ(holders[0]->GetType(), ValueHolder::kOutput);
  EXPECT_EQ(holders[1]->GetType(), ValueHolder::kOutput);
  EXPECT_EQ(holders[2]->GetType(), ValueHolder::kOutput);

  ASSERT_NE(const1->GetGraph(), nullptr);
  ASSERT_NE(const1->GetNode()->GetOwnerComputeGraph(), nullptr);
  ASSERT_NE(data1->GetGraph(), nullptr);
  ASSERT_NE(data1->GetNode()->GetOwnerComputeGraph(), nullptr);
  ASSERT_NE(holders[0]->GetNode(), nullptr);
  ASSERT_NE(holders[0]->GetGraph(), nullptr);
  ASSERT_NE(holders[0]->GetNode()->GetOwnerComputeGraph(), nullptr);
  ASSERT_NE(holders[1]->GetNode(), nullptr);
  ASSERT_NE(holders[1]->GetGraph(), nullptr);
  ASSERT_NE(holders[1]->GetNode()->GetOwnerComputeGraph(), nullptr);
  ASSERT_NE(holders[2]->GetNode(), nullptr);
  ASSERT_NE(holders[2]->GetGraph(), nullptr);
  ASSERT_NE(holders[2]->GetNode()->GetOwnerComputeGraph(), nullptr);

  // check node is ok
  auto node = holders[0]->GetNode();
  ASSERT_EQ(node->GetType(), "TestNode");
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 2);
  ASSERT_EQ(node->GetAllOutDataAnchorsSize(), 3);

  // all holders point to the same node
  ASSERT_EQ(holders[0]->GetNode(), holders[1]->GetNode());
  ASSERT_EQ(holders[0]->GetNode(), holders[2]->GetNode());

  // all holders have correct out-index
  EXPECT_EQ(holders[0]->GetOutIndex(), 0);
  EXPECT_EQ(holders[1]->GetOutIndex(), 1);
  EXPECT_EQ(holders[2]->GetOutIndex(), 2);

  // all nodes(contains data and const) in the same graph
  EXPECT_EQ(holders[0]->GetNode()->GetOwnerComputeGraph(), const1->GetNode()->GetOwnerComputeGraph());
  EXPECT_EQ(holders[0]->GetNode()->GetOwnerComputeGraph(), data1->GetNode()->GetOwnerComputeGraph());

  // all holders holds the same graph
  EXPECT_EQ(holders[0]->GetGraph(), holders[1]->GetGraph());
  EXPECT_EQ(holders[0]->GetGraph(), holders[2]->GetGraph());
  EXPECT_EQ(holders[0]->GetGraph(), const1->GetGraph());
  EXPECT_EQ(holders[0]->GetGraph(), data1->GetGraph());

  // check graph is ok
  auto graph = holders[0]->GetGraph();
  ASSERT_EQ(graph->GetAllNodesSize(), 3);
  CheckGraphGenerally(*graph);

  auto const1_g = graph->FindFirstNodeMatchType("Const");
  auto data1_g = graph->FindFirstNodeMatchType("Data");
  auto node_g = graph->FindFirstNodeMatchType("TestNode");
  ASSERT_NE(const1_g, nullptr);
  ASSERT_NE(data1_g, nullptr);
  ASSERT_NE(node_g, nullptr);

  EXPECT_EQ(node_g->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode(), data1_g);
  EXPECT_EQ(node_g->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode(), const1_g);
}

/*
 *            --------> node3
 *          /           /   \
 *     node1        node2   const3
 *     / \          /   \
 * data1 const1  data2 const2
 */
TEST_F(ValueHolderUt, CreateDataOutOk2) {
  ge::Format fmt = ge::FORMAT_NC1HWC0;
  auto const1 = ValueHolder::CreateConst(&fmt, sizeof(fmt));
  auto data1 = ValueHolder::CreateFeed(0);
  auto node1 = ValueHolder::CreateSingleDataOutput("Node1", {const1, data1});

  auto const2 = ValueHolder::CreateConst(&fmt, sizeof(fmt));
  auto data2 = ValueHolder::CreateFeed(0);
  auto node2 = ValueHolder::CreateSingleDataOutput("Node1", {const2, data2});

  auto const3 = ValueHolder::CreateConst(&fmt, sizeof(fmt));
  auto n2_holder = ValueHolder::CreateVoid("Node2", {node1, node2, const3});

  for (const auto &holder : {const1, data1, node1, const2, data2, node2, const3, n2_holder}) {
    ASSERT_NE(holder, nullptr);
    ASSERT_TRUE(holder->IsOk());
    ASSERT_NE(holder->GetNode(), nullptr);
    ASSERT_NE(holder->GetGraph(), nullptr);
  }
  EXPECT_EQ(node1->GetNode()->GetType(), "Node1");
  EXPECT_EQ(node2->GetNode()->GetType(), "Node1");
  EXPECT_EQ(n2_holder->GetNode()->GetType(), "Node2");

  ASSERT_EQ(node1->GetNode()->GetAllOutDataAnchorsSize(), 1);
  ASSERT_EQ(node1->GetNode()->GetAllInDataAnchorsSize(), 2);
  EXPECT_EQ(node1->GetNode()->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode().get(), const1->GetNode());
  EXPECT_EQ(node1->GetNode()->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode().get(), data1->GetNode());

  ASSERT_EQ(n2_holder->GetNode()->GetAllOutDataAnchorsSize(), 0);
  ASSERT_EQ(n2_holder->GetNode()->GetAllInDataAnchorsSize(), 3);
  EXPECT_EQ(n2_holder->GetNode()->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode().get(), node1->GetNode());
  EXPECT_EQ(n2_holder->GetNode()->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode().get(), node2->GetNode());
  EXPECT_EQ(n2_holder->GetNode()->GetInDataAnchor(2)->GetPeerOutAnchor()->GetOwnerNode().get(), const3->GetNode());
}
TEST_F(ValueHolderUt, MergeIsolateNodeToGraphOk) {
  ge::Format f1 = ge::FORMAT_NC1HWC0;
  auto const1 = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  auto data1 = ValueHolder::CreateFeed(0);
  auto node1 = ValueHolder::CreateDataOutput("Node1", {data1, const1}, 2);
  ASSERT_NE(const1, nullptr);
  ASSERT_NE(data1, nullptr);
  ASSERT_EQ(node1.size(), 2);
  ASSERT_NE(node1[0], nullptr);
  ASSERT_NE(node1[1], nullptr);

  ASSERT_NE(const1->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_EQ(data1->GetNode()->GetOwnerComputeGraph(), const1->GetNode()->GetOwnerComputeGraph());
  EXPECT_EQ(node1[0]->GetNode()->GetOwnerComputeGraph(), const1->GetNode()->GetOwnerComputeGraph());
  EXPECT_EQ(node1[1]->GetNode()->GetOwnerComputeGraph(), const1->GetNode()->GetOwnerComputeGraph());

  ASSERT_NE(const1->GetGraph(), nullptr);
  EXPECT_EQ(const1->GetGraph(), const1->GetNode()->GetOwnerComputeGraph().get());
  EXPECT_EQ(data1->GetGraph(), const1->GetGraph());
  EXPECT_EQ(node1[0]->GetGraph(), const1->GetGraph());
  EXPECT_EQ(node1[1]->GetGraph(), const1->GetGraph());
}

/*
 *
 *           node3
 *          /     \   |
 *     node1       node2
 *     / \         /   \
 * data1 const1  data2 const2
 */
TEST_F(ValueHolderUt, MergeTwoGraphOk1)  {
  ge::Format f1 = ge::FORMAT_NC1HWC0;
  auto const1 = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  auto data1 = ValueHolder::CreateFeed(0);
  auto node1 = ValueHolder::CreateDataOutput("Node1", {data1, const1}, 1);

  auto const2 = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  auto data2 = ValueHolder::CreateFeed(0);
  auto node2 = ValueHolder::CreateDataOutput("Node2", {data2, const2}, 2);

  auto node3 = ValueHolder::CreateSingleDataOutput("Node3", {node1[0], node2[0]});
  ASSERT_NE(node3, nullptr);

  EXPECT_NE(data1->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(const1->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node1[0]->GetNode()->GetOwnerComputeGraph(), nullptr);

  EXPECT_NE(data2->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(const2->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node2[0]->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node2[1]->GetNode()->GetOwnerComputeGraph(), nullptr);

  EXPECT_NE(node3->GetNode()->GetOwnerComputeGraph(), nullptr);

  ASSERT_NE(const1->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_EQ(const1->GetNode()->GetOwnerComputeGraph()->GetAllNodesSize(), 7);

  EXPECT_EQ(const1->GetGraph(), data1->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node1[0]->GetGraph());
  EXPECT_EQ(const1->GetGraph(), data2->GetGraph());
  EXPECT_EQ(const1->GetGraph(), const2->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node2[0]->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node2[1]->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node3->GetGraph());
}
/*
 *
 *                node4
 *               /   \
 *           node3    \
 *          /     \   |
 *     node1       node2
 *     / \         /   \
 * data1 const1  data2 const2
 */
TEST_F(ValueHolderUt, MergeTwoGraphOk)  {
  ge::Format f1 = ge::FORMAT_NC1HWC0;
  auto const1 = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  auto data1 = ValueHolder::CreateFeed(0);
  auto node1 = ValueHolder::CreateDataOutput("Node1", {data1, const1}, 1);
  ASSERT_NE(const1, nullptr);
  ASSERT_NE(data1, nullptr);
  ASSERT_EQ(node1.size(), 1);
  ASSERT_NE(node1[0], nullptr);
  ASSERT_NE(const1->GetNode()->GetOwnerComputeGraph(), nullptr);

  auto const2 = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  auto data2 = ValueHolder::CreateFeed(0);
  auto node2 = ValueHolder::CreateDataOutput("Node2", {data2, const2}, 2);
  ASSERT_NE(const2, nullptr);
  ASSERT_NE(data2, nullptr);
  ASSERT_EQ(node2.size(), 2);
  ASSERT_NE(node2[0], nullptr);
  ASSERT_NE(node2[1], nullptr);

  EXPECT_NE(const2->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node2[0]->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node2[1]->GetNode()->GetOwnerComputeGraph(), nullptr);

  auto node3 = ValueHolder::CreateSingleDataOutput("Node3", {node1[0], node2[0]});
  ASSERT_NE(node3, nullptr);

  auto node4 = ValueHolder::CreateVoid("Node4", {node3, node2[1]});
  ASSERT_NE(node4, nullptr);

  EXPECT_NE(data1->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node1[0]->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(data2->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(const2->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node2[0]->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node2[1]->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node3->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_NE(node4->GetNode()->GetOwnerComputeGraph(), nullptr);

  ASSERT_NE(const1->GetNode(), nullptr);
  ASSERT_NE(const1->GetNode()->GetOwnerComputeGraph(), nullptr);
  EXPECT_EQ(const1->GetNode()->GetOwnerComputeGraph()->GetAllNodesSize(), 8);

  EXPECT_EQ(const1->GetGraph(), data1->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node1[0]->GetGraph());
  EXPECT_EQ(const1->GetGraph(), data2->GetGraph());
  EXPECT_EQ(const1->GetGraph(), const2->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node2[0]->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node2[1]->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node3->GetGraph());
  EXPECT_EQ(const1->GetGraph(), node4->GetGraph());
}
TEST_F(ValueHolderUt, CreateVoidOk) {
  ge::Format f1 = ge::FORMAT_NC1HWC0;
  auto const1 = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  auto data1 = ValueHolder::CreateFeed(0);
  ASSERT_NE(const1, nullptr);
  ASSERT_NE(data1, nullptr);

  std::vector<ValueHolderPtr> inputs = {data1, const1};
  auto holder = ValueHolder::CreateVoid("TestNode", inputs);

  ASSERT_NE(holder, nullptr);

  ASSERT_NE(const1->GetGraph(), nullptr);
  ASSERT_NE(const1->GetNode()->GetOwnerComputeGraph(), nullptr);
  ASSERT_NE(data1->GetGraph(), nullptr);
  ASSERT_NE(data1->GetNode()->GetOwnerComputeGraph(), nullptr);
  ASSERT_NE(holder->GetNode(), nullptr);
  ASSERT_NE(holder->GetGraph(), nullptr);
  ASSERT_NE(holder->GetNode()->GetOwnerComputeGraph(), nullptr);

  // check graph is ok
  auto graph = holder->GetGraph();
  ASSERT_EQ(graph->GetAllNodesSize(), 3);
  CheckGraphGenerally(*graph);

  auto const1_g = graph->FindFirstNodeMatchType("Const");
  auto data1_g = graph->FindFirstNodeMatchType("Data");
  auto node_g = graph->FindFirstNodeMatchType("TestNode");
  ASSERT_NE(const1_g, nullptr);
  ASSERT_NE(data1_g, nullptr);
  ASSERT_NE(node_g, nullptr);

  EXPECT_EQ(node_g->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode(), data1_g);
  EXPECT_EQ(node_g->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode(), const1_g);
}

TEST_F(ValueHolderUt, AddDependencyOk) {
  auto data1 = ValueHolder::CreateFeed(0);
  auto data2 = ValueHolder::CreateFeed(1);
  ValueHolder::AddDependency(data1, data2);

  auto node1 = ValueHolder::CreateSingleDataOutput("Node1", {data1});
  auto node2 = ValueHolder::CreateSingleDataOutput("Node1", {data1});
  ValueHolder::AddDependency(node1, node2);

  ASSERT_NE(data1, nullptr);
  ASSERT_NE(data2, nullptr);
  ASSERT_NE(node1, nullptr);
  ASSERT_NE(node2, nullptr);

  ASSERT_NE(data1->GetNode(), nullptr);
  ASSERT_NE(data2->GetNode(), nullptr);
  ASSERT_NE(node1->GetNode(), nullptr);
  ASSERT_NE(node2->GetNode(), nullptr);

  ASSERT_EQ(data1->GetNode()->GetOwnerComputeGraph(), data2->GetNode()->GetOwnerComputeGraph());
  ASSERT_EQ(data1->GetNode()->GetOwnerComputeGraph(), node1->GetNode()->GetOwnerComputeGraph());
  ASSERT_EQ(data1->GetNode()->GetOwnerComputeGraph(), node2->GetNode()->GetOwnerComputeGraph());

  ASSERT_EQ(data1->GetNode()->GetOutControlNodes().size(), 1);
  ASSERT_EQ(data2->GetNode()->GetInControlNodes().size(), 1);
  EXPECT_EQ(data1->GetNode()->GetOutControlAnchor()->GetPeerInControlAnchors().begin()->get()->GetOwnerNode().get(),
            data2->GetNode());

  ASSERT_EQ(node1->GetNode()->GetOutControlNodes().size(), 1);
  ASSERT_EQ(node2->GetNode()->GetInControlNodes().size(), 1);
  EXPECT_EQ(node1->GetNode()->GetOutControlAnchor()->GetPeerInControlAnchors().begin()->get()->GetOwnerNode().get(),
            node2->GetNode());
}

/*
 *           KernelLaunch
 *               |
 *             Tiling
 *             /    \
 *    InferShape   CompileInfo
 *     /   \          |
 * shape1  shape2   json
 */
TEST_F(ValueHolderUt, CurrentNodeOk) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto shape1 = ValueHolder::CreateFeed(0);
  auto shape2 = ValueHolder::CreateFeed(1);
  auto json1 = ValueHolder::CreateConst("{}", 3);

  ValueHolder::SetCurrentComputeNode(node);
  auto frame = ValueHolder::GetCurrentFrame();
  ASSERT_NE(frame, nullptr);
  ASSERT_EQ(frame->GetCurrentComputeNode(), node);
  auto shape = ValueHolder::CreateSingleDataOutput("InferShape", {shape1, shape2});
  auto compile_info = ValueHolder::CreateSingleDataOutput("TilingParse", {json1});
  auto tiling_ret  = ValueHolder::CreateSingleDataOutput("Tiling", {shape, compile_info});
  auto holder = ValueHolder::CreateVoid("KernelLaunch", {tiling_ret});

  ASSERT_NE(shape1, nullptr);
  ASSERT_NE(shape2, nullptr);
  ASSERT_NE(json1, nullptr);
  ASSERT_NE(shape, nullptr);
  ASSERT_NE(compile_info, nullptr);
  ASSERT_NE(tiling_ret, nullptr);
  ASSERT_NE(holder, nullptr);

  int64_t compute_node_index_none;
  ASSERT_FALSE(ge::AttrUtils::GetInt(shape1->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_none));
  ASSERT_FALSE(ge::AttrUtils::GetInt(shape2->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_none));
  ASSERT_FALSE(ge::AttrUtils::GetInt(json1->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_none));

  int64_t compute_node_index_shape, compute_node_index_compile_ifo, compute_node_index_tiling_ret, compute_node_index_holder;
  ASSERT_TRUE(ge::AttrUtils::GetInt(shape->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_shape));
  ASSERT_TRUE(ge::AttrUtils::GetInt(compile_info->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_compile_ifo));
  ASSERT_TRUE(ge::AttrUtils::GetInt(tiling_ret->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_tiling_ret));
  ASSERT_TRUE(ge::AttrUtils::GetInt(holder->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_holder));
  EXPECT_EQ(compute_node_index_shape, compute_node_index_compile_ifo);
  EXPECT_EQ(compute_node_index_shape, compute_node_index_tiling_ret);
  EXPECT_EQ(compute_node_index_shape, compute_node_index_holder);

  size_t frame_current_node_index;
  frame->GetCurrentNodeIndex(frame_current_node_index);
  EXPECT_EQ(compute_node_index_shape, frame_current_node_index);
  auto compute_node_info = reinterpret_cast<const ComputeNodeInfo *>(frame->GetComputeNodeInfo(frame_current_node_index));
  ASSERT_NE(compute_node_info, nullptr);
  auto name_index = compute_node_info->GetNodeName();
  auto type_index = compute_node_info->GetNodeType();
  auto buffer_pool = frame->GetBufferPool();
  EXPECT_STREQ(buffer_pool.GetBufById(reinterpret_cast<size_t>(name_index)), "node");
  EXPECT_STREQ(buffer_pool.GetBufById(reinterpret_cast<size_t>(type_index)), "node");
}
/*
 *    hello
 *    /  \
 * data0 data1
 */
TEST_F(ValueHolderUt, CreateExeGraphOk) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto hello = ValueHolder::CreateSingleDataOutput("hello", {data0, data1});

  ASSERT_NE(graph, nullptr);
}
/*
 *    hello
 *    /  \
 * data0 data1
 */
TEST_F(ValueHolderUt, CreateExeGraphWithTargetsOk) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto hello = ValueHolder::CreateVoid("hello", {data0, data1});
  ASSERT_NE(graph, nullptr);
}
/*
 *                      c
 * Atomic-LaunchKernel ----> LaunchKernel
 *          |                 /
 *    Atomic-tiling      Tiling
 *        /    \        /    \
 * TilingParse InferShape   TilingParse
 *    |         /   \          |
 *   json1   shape1  shape2   json2
 */
TEST_F(ValueHolderUt, ScopedCurrentNodeOk) {
  auto graph = std::make_shared<ge::ComputeGraph>("graph");

  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);
  auto node = graph->AddNode(op_desc);

  auto clean_op_desc = std::make_shared<ge::OpDesc>("node-AtomicClean", "DynamicAtomicAddrClean");
  clean_op_desc->AddInputDesc("workspace", tensor_desc);
  clean_op_desc->AddInputDesc("clean1", tensor_desc);
  clean_op_desc->AddInputDesc("clean2", tensor_desc);
  clean_op_desc->AppendIrInput("workspace", ge::kIrInputRequired);
  clean_op_desc->AppendIrInput("clean", ge::kIrInputDynamic);
  auto clean_node = graph->AddNode(clean_op_desc);

  auto shape1 = ValueHolder::CreateFeed(0);
  auto shape2 = ValueHolder::CreateFeed(1);
  auto json1 = ValueHolder::CreateConst("{}", 2);
  auto json2 = ValueHolder::CreateConst("{}", 3);

  ValueHolder::SetCurrentComputeNode(node);
  auto frame = ValueHolder::GetCurrentFrame();
  ASSERT_NE(frame, nullptr);
  ASSERT_EQ(frame->GetCurrentComputeNode(), node);
  auto shape = ValueHolder::CreateSingleDataOutput("InferShape", {shape1, shape2});

  size_t node1_index;
  ValueHolderPtr compile_info1, tiling_ret1, holder1;
  {
    auto guarder = ValueHolder::SetScopedCurrentComputeNode(clean_node);
    compile_info1 = ValueHolder::CreateSingleDataOutput("TilingParse", {json1});
    tiling_ret1  = ValueHolder::CreateSingleDataOutput("Tiling", {shape, compile_info1});
    holder1 = ValueHolder::CreateVoid("AtomicKernelLaunch", {tiling_ret1});
    EXPECT_TRUE(frame->GetCurrentNodeIndex(node1_index));
  }

  auto compile_info2 = ValueHolder::CreateSingleDataOutput("TilingParse", {json2});
  auto tiling_ret2  = ValueHolder::CreateSingleDataOutput("Tiling", {shape, compile_info2});
  auto holder2 = ValueHolder::CreateVoid("KernelLaunch", {tiling_ret2});

  ValueHolder::AddDependency(holder1, holder2);

  ASSERT_NE(shape1, nullptr);
  ASSERT_NE(shape2, nullptr);
  ASSERT_NE(json1, nullptr);
  ASSERT_NE(shape, nullptr);
  ASSERT_NE(compile_info1, nullptr);
  ASSERT_NE(tiling_ret1, nullptr);
  ASSERT_NE(holder1, nullptr);
  ASSERT_NE(compile_info2, nullptr);
  ASSERT_NE(tiling_ret2, nullptr);
  ASSERT_NE(holder2, nullptr);

  int64_t compute_node_index_none;
  ASSERT_FALSE(ge::AttrUtils::GetInt(shape1->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_none));
  ASSERT_FALSE(ge::AttrUtils::GetInt(shape2->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_none));
  ASSERT_FALSE(ge::AttrUtils::GetInt(json1->GetNode()->GetOpDesc(), "ComputeNodeIndex", compute_node_index_none));

  int64_t shape_index, compile_info1_index, tiling_ret1_index, holder1_index, compile_info2_index, tiling_ret2_index, holder2_index;
  ASSERT_TRUE(ge::AttrUtils::GetInt(shape->GetNode()->GetOpDesc(), "ComputeNodeIndex", shape_index));

  ASSERT_TRUE(ge::AttrUtils::GetInt(compile_info1->GetNode()->GetOpDesc(), "ComputeNodeIndex", compile_info1_index));
  ASSERT_TRUE(ge::AttrUtils::GetInt(tiling_ret1->GetNode()->GetOpDesc(), "ComputeNodeIndex", tiling_ret1_index));
  ASSERT_TRUE(ge::AttrUtils::GetInt(holder1->GetNode()->GetOpDesc(), "ComputeNodeIndex", holder1_index));

  ASSERT_TRUE(ge::AttrUtils::GetInt(compile_info2->GetNode()->GetOpDesc(), "ComputeNodeIndex", compile_info2_index));
  ASSERT_TRUE(ge::AttrUtils::GetInt(tiling_ret2->GetNode()->GetOpDesc(), "ComputeNodeIndex", tiling_ret2_index));
  ASSERT_TRUE(ge::AttrUtils::GetInt(holder2->GetNode()->GetOpDesc(), "ComputeNodeIndex", holder2_index));

  EXPECT_EQ(shape_index, compile_info2_index);
  EXPECT_EQ(shape_index, tiling_ret2_index);
  EXPECT_EQ(shape_index, holder2_index);

  EXPECT_NE(shape_index, compile_info1_index);
  EXPECT_EQ(compile_info1_index, tiling_ret1_index);
  EXPECT_EQ(compile_info1_index, holder1_index);

  size_t node2_index;
  EXPECT_TRUE(frame->GetCurrentNodeIndex(node2_index));
  EXPECT_EQ(shape_index, node2_index);
  auto compute_node_info = reinterpret_cast<const ComputeNodeInfo *>(frame->GetComputeNodeInfo(node2_index));
  ASSERT_NE(compute_node_info, nullptr);
  auto name_index = compute_node_info->GetNodeName();
  auto type_index = compute_node_info->GetNodeType();
  auto buffer_pool = frame->GetBufferPool();
  EXPECT_STREQ(buffer_pool.GetBufById(reinterpret_cast<size_t>(name_index)), "node");
  EXPECT_STREQ(buffer_pool.GetBufById(reinterpret_cast<size_t>(type_index)), "node");

  EXPECT_NE(node2_index, node1_index);
  EXPECT_EQ(compile_info1_index, node1_index);
  compute_node_info = reinterpret_cast<const ComputeNodeInfo *>(frame->GetComputeNodeInfo(node1_index));
  ASSERT_NE(compute_node_info, nullptr);
  name_index = compute_node_info->GetNodeName();
  type_index = compute_node_info->GetNodeType();
  EXPECT_STREQ(buffer_pool.GetBufById(reinterpret_cast<size_t>(name_index)), "node-AtomicClean");
  EXPECT_STREQ(buffer_pool.GetBufById(reinterpret_cast<size_t>(type_index)), "DynamicAtomicAddrClean");
}

TEST_F(ValueHolderUt, CreateExeGraphNoOutpus) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto hello = ValueHolder::CreateVoid("hello", {data0, data1});
}

TEST_F(ValueHolderUt, CreateExeGraphNoFrame) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto hello = ValueHolder::CreateVoid("hello", {data0, data1});
}
/*
 *    hello  world
 *    /  \   /
 * data0 data1
 */
TEST_F(ValueHolderUt, SetStageOk) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto hello = ValueHolder::CreateVoid("hello", {data0, data1});
  auto world = ValueHolder::CreateVoid("world", {data1});
  world->SetStage(ValueHolder::kExit);

  ASSERT_NE(graph, nullptr);
}
/*
 *    hello
 *    /  \
 * data0 data1
 */
TEST_F(ValueHolderUt, GetCurrentGraphOk) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto hello = ValueHolder::CreateVoid("hello", {data0, data1});

  EXPECT_NE(hello->GetCurrentFrame(), nullptr);
  EXPECT_NE(hello->GetCurrentGraph(), nullptr);
}
/*
 *       ref
 *     +------+
 *     |      |
 *   launch   |
 *     |      |
 *   tiling   |
 *     |      |
 *    alloc----
 *    /  \
 * data0 data1
 */
TEST_F(ValueHolderUt, RefFromOk) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto alloc_outs = ValueHolder::CreateDataOutput("alloc", {data0, data1}, 3);
  auto tiling_outs = ValueHolder::CreateDataOutput("tiling", {data0, data1}, 2);
  tiling_outs[1]->RefFrom(alloc_outs[1]);

  auto launch = ValueHolder::CreateSingleDataOutput("launch", {tiling_outs[0], tiling_outs[1]});
  launch->RefFrom(alloc_outs[2]);
}
/*
 *    hello
 *    /  \
 * data0 data1
 */
TEST_F(ValueHolderUt, AddNullOutputs) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto hello = ValueHolder::CreateSingleDataOutput("hello", {data0, data1});

  EXPECT_NE(hello->GetCurrentFrame(), nullptr);
  EXPECT_NE(hello->GetCurrentGraph(), nullptr);
}
/*
 *    hello
 *    /  \
 * data0 data1
 */
TEST_F(ValueHolderUt, AddNullTargets) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputRequired);
  op_desc->AppendIrInput("x2", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  auto data0 = ValueHolder::CreateFeed(0);
  auto data1 = ValueHolder::CreateFeed(1);

  ValueHolder::SetCurrentComputeNode(node);
  auto hello = ValueHolder::CreateSingleDataOutput("hello", {data0, data1});

  EXPECT_NE(hello->GetCurrentFrame(), nullptr);
  EXPECT_NE(hello->GetCurrentGraph(), nullptr);
}
}  // namespace bg
}  // namespace gert