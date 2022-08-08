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
#include "exe_graph/lowering/lowering_global_data.h"
#include <gtest/gtest.h>
#include "checker/bg_test.h"
namespace gert {
namespace {
ge::NodePtr BuildTestNode() {
  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  return graph->AddNode(op_desc);
}
}
class LoweringGlobalDataUT : public BgTest {};
TEST_F(LoweringGlobalDataUT, SetGetStreamOk) {
  LoweringGlobalData gd;

  EXPECT_EQ(gd.GetStream(), nullptr);

  auto holder = bg::ValueHolder::CreateFeed(0);
  auto holder1 = holder;
  gd.SetStream(std::move(holder1));
  EXPECT_EQ(gd.GetStream(), holder);
}
TEST_F(LoweringGlobalDataUT, SetGetCompileResultOk) {
  LoweringGlobalData gd;

  auto node = BuildTestNode();
  ASSERT_NE(node, nullptr);

  EXPECT_EQ(gd.FindCompiledResult(node), nullptr);

  gd.AddCompiledResult(node, {});
  ASSERT_NE(gd.FindCompiledResult(node), nullptr);
  EXPECT_TRUE(gd.FindCompiledResult(node)->GetTaskDefs().empty());
}

TEST_F(LoweringGlobalDataUT, SetGetKnownSubgraphModelOk) {
  LoweringGlobalData gd;

  auto node = BuildTestNode();
  ASSERT_NE(node, nullptr);

  EXPECT_EQ(gd.FindKnownSubgraphModel(node), nullptr);

  gd.AddKnownSubgraphModel(node, reinterpret_cast<void *>(0x123));
  EXPECT_EQ(gd.FindKnownSubgraphModel(node), reinterpret_cast<void *>(0x123));
}

TEST_F(LoweringGlobalDataUT, SetGetAllocatorOk) {
  LoweringGlobalData gd;
  EXPECT_EQ(gd.GetAllocator({kOnDeviceHbm, AllocatorUsage::kAllocNodeOutput}), nullptr);
  auto holder = bg::ValueHolder::CreateFeed(0);
  ASSERT_NE(holder, nullptr);
  gd.SetAllocator({kOnDeviceHbm, AllocatorUsage::kAllocNodeOutput}, holder);
  EXPECT_EQ(gd.GetAllocator({kOnDeviceHbm, AllocatorUsage::kAllocNodeOutput}), holder);
}

TEST_F(LoweringGlobalDataUT, GetOrCreateAllocatorOk) {
  LoweringGlobalData gd;
  auto allocator1 = gd.GetOrCreateAllocator({kOnDeviceHbm, AllocatorUsage::kAllocNodeOutput});
  EXPECT_NE(allocator1, nullptr);
  EXPECT_EQ(allocator1, gd.GetOrCreateAllocator({kOnDeviceHbm, AllocatorUsage::kAllocNodeOutput}));
}
TEST_F(LoweringGlobalDataUT, GetOrCreateAllocator_AlwaysCreateOnRootFrame_CallInSubgraph) {
  LoweringGlobalData gd;

  auto data0 = bg::ValueHolder::CreateFeed(0);
  auto foo1 = bg::ValueHolder::CreateSingleDataOutput("Foo", {data0});

  bg::ValueHolder::PushGraphFrame(foo1, "FooGraph");
  auto allocator1 = gd.GetOrCreateAllocator({kOnDeviceHbm, AllocatorUsage::kAllocNodeOutput});
  EXPECT_NE(allocator1, nullptr);
  ASSERT_NE(bg::ValueHolder::PopGraphFrame(), nullptr);

  auto root_frame = bg::ValueHolder::PopGraphFrame();
  ASSERT_NE(root_frame, nullptr);

  ASSERT_EQ(allocator1->GetNode()->GetOwnerComputeGraph(), root_frame->GetExeGraph());
}
}  // namespace gert