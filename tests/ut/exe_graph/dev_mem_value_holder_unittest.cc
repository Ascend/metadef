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
}  // namespace bg
}  // namespace gert