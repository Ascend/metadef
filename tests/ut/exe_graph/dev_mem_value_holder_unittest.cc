/**
 * Copyright 2023 Huawei Technologies Co., Ltd
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
#include "exe_graph/lowering/dev_mem_value_holder.h"
#include "checker/topo_checker.h"
#include "checker/bg_test.h"

namespace gert {
namespace bg {
class DevMemValueHolderUt : public BgTest {};
TEST_F(DevMemValueHolderUt, SetGetLogicStreamOk) {
  int64_t logic_stream_id = 2;
  auto output0 = DevMemValueHolder::CreateSingleDataOutput("test", {}, logic_stream_id);
  output0->SetPlacement(1);
  EXPECT_EQ(output0->GetPlacement(), 1);
  EXPECT_EQ(output0->GetLogicStream(), 2);
}

TEST_F(DevMemValueHolderUt, DevMemCreateErrorOk) {
  auto holder = DevMemValueHolder::CreateError(0, "This is a test error information, int %d, %s", 10240, "Test msg");
  ASSERT_NE(holder, nullptr);
  EXPECT_FALSE(holder->IsOk());
}

TEST_F(DevMemValueHolderUt, DevMemCreateDataOutputOk) {
  ge::Format f1 = ge::FORMAT_NC1HWC0;
  auto const1 = ValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1));
  auto data1 = ValueHolder::CreateFeed(0);
  ASSERT_NE(const1, nullptr);
  ASSERT_NE(data1, nullptr);

  std::vector<ValueHolderPtr> inputs = {data1, const1};
  auto holders = DevMemValueHolder::CreateDataOutput("TestNode", inputs, 3, 0);

  ASSERT_EQ(holders.size(), 3);
  ASSERT_TRUE(holders[0]->IsOk());
  ASSERT_TRUE(holders[1]->IsOk());
  ASSERT_TRUE(holders[2]->IsOk());
  EXPECT_EQ(holders[0]->GetType(), ValueHolder::ValueHolderType::kOutput);
  EXPECT_EQ(holders[1]->GetType(), ValueHolder::ValueHolderType::kOutput);
  EXPECT_EQ(holders[2]->GetType(), ValueHolder::ValueHolderType::kOutput);
}

TEST_F(DevMemValueHolderUt, DevMemCreateConstOk) {
  ge::Format f1 = ge::FORMAT_NC1HWC0;
  auto holder = DevMemValueHolder::CreateConst(reinterpret_cast<const uint8_t *>(&f1), sizeof(f1), 0);
  ASSERT_NE(holder, nullptr);
  ASSERT_TRUE(holder->IsOk());
}

TEST_F(DevMemValueHolderUt, DevMemCreateMateFromNodeOk) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto op_desc = std::make_shared<ge::OpDesc>("FakeNode", "FakeNode");
  auto node = graph->AddNode(op_desc);
  auto holder = std::make_shared<DevMemValueHolder>(2);
  ASSERT_NE(holder, nullptr);
  auto value_holder = holder->CreateMateFromNode(node, 0, ValueHolder::ValueHolderType::kOutput);
  ASSERT_NE(value_holder, nullptr);
  ASSERT_TRUE(value_holder->IsOk());
  EXPECT_EQ(value_holder->GetType(), ValueHolder::ValueHolderType::kOutput);
  auto mem_holder = std::dynamic_pointer_cast<DevMemValueHolder>(value_holder);
  ASSERT_NE(mem_holder, nullptr);
  EXPECT_EQ(mem_holder->GetLogicStream(), 2);
}
TEST_F(DevMemValueHolderUt, NormalGuard_AddFlagToNode) {
  auto data0 = ValueHolder::CreateFeed(0);
  auto allocator0 = ValueHolder::CreateSingleDataOutput("CreateAllocator", {data0});
  auto allocator_destroyer =
      DevMemValueHolder::CreateSingleDataOutputWithGuarder("DestroyAllocator", 0, allocator0, {});
  ASSERT_NE(allocator_destroyer, nullptr);
  auto graph = ValueHolder::PopGraphFrame()->GetExeGraph();

  auto node = graph->FindFirstNodeMatchType("DestroyAllocator");
  ASSERT_NE(node, nullptr);
  int64_t index;
  EXPECT_TRUE(ge::AttrUtils::GetInt(node->GetOpDesc(), kReleaseResourceIndex, index));
  EXPECT_EQ(index, 0);
}
TEST_F(DevMemValueHolderUt, NormalGuarder_AddDependencyAutomately_ConnectDataEdgeToResource) {
  auto data0 = ValueHolder::CreateFeed(0);
  auto allocator0 = ValueHolder::CreateSingleDataOutput("CreateAllocator", {data0});
  auto allocator_destroyer =
      DevMemValueHolder::CreateSingleDataOutputWithGuarder("DestroyAllocator", 0, allocator0, {});
  ASSERT_NE(allocator_destroyer, nullptr);

  size_t alloc_size = 1024;
  auto size = ValueHolder::CreateConst(&alloc_size, sizeof(alloc_size));
  auto alloc_mem0 = ValueHolder::CreateSingleDataOutput("AllocMemory", {allocator0, size});
  auto alloc_mem1 = ValueHolder::CreateSingleDataOutput("AllocMemory", {allocator0, size});
  auto graph = ValueHolder::PopGraphFrame()->GetExeGraph();

  CheckGraphGenerally(*graph);

  ASSERT_NE(alloc_mem0, nullptr);
  ASSERT_NE(alloc_mem1, nullptr);
  HasControlEdge(*graph, *alloc_mem0->GetNode(), *allocator_destroyer->GetNode());
  HasControlEdge(*graph, *alloc_mem1->GetNode(), *allocator_destroyer->GetNode());
}
/*
*     NetOutput
*        |
*      Bar -c-> foo0_guarder
*      / \    /
* data1   foo0
*          |
*        data0
*/
TEST_F(DevMemValueHolderUt, NormalGuarder_AddDependencyFromTheSameLevelNode_ConnectFromSrcToSubgraphNodes) {
  auto data0 = ValueHolder::CreateFeed(0);
  auto foo0 = ValueHolder::CreateSingleDataOutput("Foo", {data0});
  auto guarder = DevMemValueHolder::CreateSingleDataOutputWithGuarder("FooGuarder", 0, foo0, {});
  ASSERT_NE(guarder, nullptr);
  auto data1 = ValueHolder::CreateFeed(1);
  auto bar1 = ValueHolder::CreateSingleDataOutput("Bar", {data1});

  ValueHolder::PushGraphFrame(bar1, "BarGraph");
  auto foo1 = ValueHolder::CreateSingleDataOutput("Foo", {foo0, data1});
  auto bar_frame = ValueHolder::PopGraphFrame({foo1}, {});

  auto frame = ValueHolder::PopGraphFrame({bar1}, {});
  ASSERT_NE(frame, nullptr);
  ASSERT_NE(frame->GetExeGraph(), nullptr);

  EXPECT_EQ(frame->GetExeGraph()->TopologicalSorting(), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(NodeTopoChecker(bar1).OutChecker().CtrlToByType("FooGuarder").IsOk());
  EXPECT_EQ(NodeTopoChecker(bar1).StrictConnectFrom({{"Data"}, {"Foo"}}), "success");
}
TEST_F(DevMemValueHolderUt, NormalGuarder_DoNotAddDependency_ConnectDataEdgeToNetOutput) {
  auto data0 = ValueHolder::CreateFeed(0);
  auto foo0 = ValueHolder::CreateSingleDataOutput("Foo", {data0});
  auto guarder = DevMemValueHolder::CreateSingleDataOutputWithGuarder("FooGuarder", 0, foo0, {});
  ASSERT_NE(guarder, nullptr);

  auto bar0 = ValueHolder::CreateSingleDataOutput("Bar", {foo0});

  auto frame = ValueHolder::PopGraphFrame({foo0}, {});
  ASSERT_NE(frame, nullptr);
  auto graph = frame->GetExeGraph();
  ASSERT_NE(graph, nullptr);

  EXPECT_EQ(NodeTopoChecker(foo0).StrictConnectTo(0, {{"NetOutput"}, {"FooGuarder"}, {"Bar"}}), "success");
  HasControlEdge(*graph, *bar0->GetNode(), *guarder->GetNode());
  ASSERT_EQ(guarder->GetNode()->GetInControlNodes().size(), 1);
}
TEST_F(DevMemValueHolderUt, NormalAddDependencyForGuard_RleaseBy) {
  auto data0 = ValueHolder::CreateFeed(0);
  auto allocator0 = ValueHolder::CreateSingleDataOutput("CreateAllocator", {data0});
  auto allocator_destroyer =
      DevMemValueHolder::CreateSingleDataOutputWithGuarder("DestroyAllocator", 0, allocator0, {});
  ASSERT_NE(allocator_destroyer, nullptr);

  size_t alloc_size = 1024;
  auto size = ValueHolder::CreateConst(&alloc_size, sizeof(alloc_size));
  auto alloc_mem0 = ValueHolder::CreateSingleDataOutput("AllocMemory", {allocator0, size});
  auto free_mem0 = DevMemValueHolder::CreateSingleDataOutputWithGuarder("FreeMemory", 0, {alloc_mem0}, {});
  auto graph = ValueHolder::PopGraphFrame()->GetExeGraph();
  CheckGraphGenerally(*graph);

  ASSERT_NE(free_mem0, nullptr);
  ASSERT_NE(alloc_mem0, nullptr);
  HasControlEdge(*graph, *alloc_mem0->GetNode(), *allocator_destroyer->GetNode());

  allocator0->ReleaseAfter(free_mem0);
  HasControlEdge(*graph, *free_mem0->GetNode(), *allocator_destroyer->GetNode());
}
}  // namespace bg
}  // namespace gert