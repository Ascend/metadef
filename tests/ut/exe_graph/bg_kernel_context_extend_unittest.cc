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
#include "exe_graph/lowering/bg_kernel_context_extend.h"
#include <gtest/gtest.h>
#include <memory>
#include "graph/compute_graph.h"
#include "exe_graph/runtime/context_extend.h"
#include "exe_graph/runtime/continuous_vector.h"
#include "exe_graph/runtime/tensor.h"

namespace gert {
class BgKernelContextExtendUT : public testing::Test {};
TEST_F(BgKernelContextExtendUT, BuildRequiredInput) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AppendIrInput("x", ge::kIrInputRequired);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 0);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 1);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 0);
  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), nullptr);
}
TEST_F(BgKernelContextExtendUT, BuildEmptyRequiredInput) {
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
  op_desc->AppendIrInput("x2", ge::kIrInputRequired);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  EXPECT_EQ(ret, nullptr);
}
TEST_F(BgKernelContextExtendUT, UknownInputFailed) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x2", tensor_desc);
  op_desc->AppendIrInput("x1", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  // TODO checkout except kernel context extend
//  EXPECT_EQ(ret, nullptr);
}
TEST_F(BgKernelContextExtendUT, BuildWithOptionalInputs) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AppendIrInput("x", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 0);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 1);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 0);
  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), nullptr);
}
TEST_F(BgKernelContextExtendUT, BuildWithOptionalInputsNotExists) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AppendIrInput("y", ge::kIrInputOptional);
  op_desc->AppendIrInput("x", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 0);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 2);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 0);
  ins_info = compute_node_info->GetInputInstanceInfo(1);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 0);
  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), nullptr);
}
TEST_F(BgKernelContextExtendUT, BuildWithMultipleOptionalInputsIns) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AddInputDesc("y", tensor_desc);
  op_desc->AppendIrInput("x", ge::kIrInputOptional);
  op_desc->AppendIrInput("y", ge::kIrInputOptional);
  op_desc->AppendIrInput("z", ge::kIrInputOptional);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 2);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 0);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 3);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);
  ins_info = compute_node_info->GetInputInstanceInfo(1);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 1);
  ins_info = compute_node_info->GetInputInstanceInfo(2);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 0);
  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), nullptr);
}
TEST_F(BgKernelContextExtendUT, BuildWithDynamicInputs) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x0", tensor_desc);
  op_desc->AppendIrInput("x", ge::kIrInputDynamic);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 0);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 1);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 0);
  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), nullptr);
}
TEST_F(BgKernelContextExtendUT, BuildWithMultiInstanceDynamicInputs) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x0", tensor_desc);
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AddInputDesc("y0", tensor_desc);
  op_desc->AddInputDesc("y1", tensor_desc);
  op_desc->AddInputDesc("y2", tensor_desc);
  op_desc->AppendIrInput("x", ge::kIrInputDynamic);
  op_desc->AppendIrInput("y", ge::kIrInputDynamic);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 5);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 0);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 2);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 2);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);
  ins_info = compute_node_info->GetInputInstanceInfo(1);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 3);
  EXPECT_EQ(ins_info->GetInstanceStart(), 2);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 0);
  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), nullptr);
}
TEST_F(BgKernelContextExtendUT, BuildWithEmptyDynamicInputs) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x0", tensor_desc);
  op_desc->AddInputDesc("x1", tensor_desc);
  op_desc->AppendIrInput("y", ge::kIrInputDynamic);
  op_desc->AppendIrInput("x", ge::kIrInputDynamic);

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 2);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 0);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 2);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 0);
  ins_info = compute_node_info->GetInputInstanceInfo(1);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 2);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 0);
  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), nullptr);
}
TEST_F(BgKernelContextExtendUT, BuildWithOneAttr) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AppendIrInput("x", ge::kIrInputRequired);
  op_desc->AddOutputDesc("y", tensor_desc);

  ge::AttrUtils::SetStr(op_desc, "a1", "Hello");
  op_desc->AppendIrAttrName("a1");

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 1);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 1);
  ASSERT_NE(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), nullptr);
  EXPECT_STREQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), "Hello");
}
TEST_F(BgKernelContextExtendUT, BuildWithAttrs) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AppendIrInput("x", ge::kIrInputRequired);
  op_desc->AddOutputDesc("y", tensor_desc);

  ge::GeTensorDesc ge_td;
  ge_td.SetOriginFormat(ge::FORMAT_NHWC);
  ge_td.SetFormat(ge::FORMAT_NC1HWC0);
  ge_td.SetDataType(ge::DT_FLOAT16);
  ge_td.SetOriginShape(ge::GeShape({8,224,224,3}));
  ge_td.SetShape(ge::GeShape({8,1,224,224,16}));
  ge::GeTensor ge_tensor(ge_td);
  std::vector<uint16_t> fake_data(8*224*224*16);
  for (size_t i = 0; i < fake_data.size(); ++i) {
    fake_data[i] = static_cast<uint16_t>(rand() % std::numeric_limits<uint16_t>::max());
  }
  ge_tensor.SetData(reinterpret_cast<uint8_t *>(fake_data.data()), fake_data.size() * 2);

  ge::AttrUtils::SetStr(op_desc, "a1", "Hello");
  ge::AttrUtils::SetInt(op_desc, "a2", 10240);
  ge::AttrUtils::SetBool(op_desc, "a3", true);
  ge::AttrUtils::SetFloat(op_desc, "a4", 1024.0021);
  ge::AttrUtils::SetListInt(op_desc, "a5", std::vector<int32_t>({10,200,3000,4096}));
  ge::AttrUtils::SetTensor(op_desc, "a6", ge_tensor);
  ge::AttrUtils::SetStr(op_desc, "b1", "World");
  ge::AttrUtils::SetInt(op_desc, "b2", 1024000);
  ge::AttrUtils::SetBool(op_desc, "b3", false);
  ge::AttrUtils::SetFloat(op_desc, "b4", 1024.1);
  ge::AttrUtils::SetListInt(op_desc, "b5", std::vector<int64_t>({10,400,3000,8192}));
  ge::AttrUtils::SetTensor(op_desc, "b6", ge_tensor);

  op_desc->AppendIrAttrName("b1");
  op_desc->AppendIrAttrName("b2");
  op_desc->AppendIrAttrName("b3");
  op_desc->AppendIrAttrName("b4");
  op_desc->AppendIrAttrName("b5");
  op_desc->AppendIrAttrName("b6");
  op_desc->AppendIrAttrName("a1");
  op_desc->AppendIrAttrName("a2");
  op_desc->AppendIrAttrName("a3");
  op_desc->AppendIrAttrName("a4");
  op_desc->AppendIrAttrName("a5");
  op_desc->AppendIrAttrName("a6");

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 1);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 12);

  EXPECT_STREQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), "World");
  EXPECT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<int64_t>(1), 1024000);
  EXPECT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<bool>(2), false);
  EXPECT_FLOAT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<float>(3), 1024.1);
  auto list_int = compute_node_info->GetAttrs()->GetAttrPointer<gert::ContinuousVector>(4);
  ASSERT_NE(list_int, nullptr);
  ASSERT_EQ(list_int->GetSize(), 4);
  EXPECT_EQ(memcmp(list_int->GetData(), std::vector<int64_t>({10,400,3000,8192}).data(), 4 * sizeof(int64_t)), 0);
  auto gert_tensor = compute_node_info->GetAttrs()->GetAttrPointer<gert::Tensor>(5);
  ASSERT_NE(gert_tensor, nullptr);
  EXPECT_EQ(gert_tensor->storage_shape.GetOriginShape(), gert::Shape({8,224,224,3}));
  EXPECT_EQ(gert_tensor->storage_shape.GetStorageShape(), gert::Shape({8,1,224,224,16}));
  EXPECT_EQ(gert_tensor->storage_format.GetOriginFormat(), ge::FORMAT_NHWC);
  EXPECT_EQ(gert_tensor->storage_format.GetStorageFormat(), ge::FORMAT_NC1HWC0);
  EXPECT_EQ(gert_tensor->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(memcmp(gert_tensor->GetData<uint16_t>(), fake_data.data(), fake_data.size() * sizeof(uint16_t)), 0);

  EXPECT_STREQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(6), "Hello");
  EXPECT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<int64_t>(7), 10240);
  EXPECT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<bool>(8), true);
  EXPECT_FLOAT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<float>(9), 1024.0021);
  list_int = compute_node_info->GetAttrs()->GetAttrPointer<gert::ContinuousVector>(10);
  ASSERT_NE(list_int, nullptr);
  ASSERT_EQ(list_int->GetSize(), 4);
  EXPECT_EQ(memcmp(list_int->GetData(), std::vector<int64_t>({10,200,3000,4096}).data(), 4 * sizeof(int64_t)), 0);
  gert_tensor = compute_node_info->GetAttrs()->GetAttrPointer<gert::Tensor>(11);
  ASSERT_NE(gert_tensor, nullptr);
  EXPECT_EQ(gert_tensor->storage_shape.GetOriginShape(), gert::Shape({8,224,224,3}));
  EXPECT_EQ(gert_tensor->storage_shape.GetStorageShape(), gert::Shape({8,1,224,224,16}));
  EXPECT_EQ(gert_tensor->storage_format.GetOriginFormat(), ge::FORMAT_NHWC);
  EXPECT_EQ(gert_tensor->storage_format.GetStorageFormat(), ge::FORMAT_NC1HWC0);
  EXPECT_EQ(gert_tensor->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(memcmp(gert_tensor->GetData<uint16_t>(), fake_data.data(), fake_data.size() * sizeof(uint16_t)), 0);
}
TEST_F(BgKernelContextExtendUT, IgnoreNoneIrAttr) {
  auto op_desc = std::make_shared<ge::OpDesc>("node", "node");
  ge::GeTensorDesc tensor_desc;
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetFormat(ge::FORMAT_NC1HWC0);
  tensor_desc.SetDataType(ge::DT_FLOAT16);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  tensor_desc.SetShape(ge::GeShape({8,1,224,224,16}));
  tensor_desc.SetOriginShape(ge::GeShape({8,3,224,224}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AppendIrInput("x", ge::kIrInputRequired);
  op_desc->AddOutputDesc("y", tensor_desc);

  ge::AttrUtils::SetStr(op_desc, "a1", "Hello");
  ge::AttrUtils::SetInt(op_desc, "a2", 10240);
  ge::AttrUtils::SetBool(op_desc, "a3", true);
  ge::AttrUtils::SetFloat(op_desc, "a4", 1024.0021);
  ge::AttrUtils::SetStr(op_desc, "b1", "World");
  ge::AttrUtils::SetInt(op_desc, "b2", 1024000);
  ge::AttrUtils::SetBool(op_desc, "b3", false);
  ge::AttrUtils::SetFloat(op_desc, "b4", 1024.1);

  op_desc->AppendIrAttrName("b1");
  op_desc->AppendIrAttrName("b3");
  op_desc->AppendIrAttrName("a2");
  op_desc->AppendIrAttrName("a4");

  auto graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  bg::BufferPool buffer_pool;
  auto ret = bg::CreateComputeNodeInfo(node, buffer_pool);
  ASSERT_NE(ret, nullptr);
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(ret.get());
  ASSERT_EQ(compute_node_info->GetInputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetOutputsNum(), 1);
  ASSERT_EQ(compute_node_info->GetIrInputsNum(), 1);
  auto td = compute_node_info->GetInputTdInfo(0);
  ASSERT_NE(td, nullptr);
  EXPECT_EQ(td->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(td->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(td->GetStorageFormat(), ge::FORMAT_NC1HWC0);

  auto ins_info = compute_node_info->GetInputInstanceInfo(0);
  ASSERT_NE(ins_info, nullptr);
  EXPECT_EQ(ins_info->GetInstanceNum(), 1);
  EXPECT_EQ(ins_info->GetInstanceStart(), 0);

  EXPECT_EQ(compute_node_info->GetAttrs()->GetAttrNum(), 4);
  EXPECT_STREQ(compute_node_info->GetAttrs()->GetAttrPointer<char>(0), "World");
  EXPECT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<bool>(1), false);
  EXPECT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<int64_t>(2), 10240);
  EXPECT_FLOAT_EQ(*compute_node_info->GetAttrs()->GetAttrPointer<float>(3), 1024.0021);
}

// todo lowering时，不需要构造attr
// todo infershape、tiling utils重新看一下输入是否正确
// todo kernel中获取attr的方式变化
}  // namespace gert