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

#include "graph/utils/op_desc_utils.h"
#include "graph_builder_utils.h"
#include "graph/utils/constant_utils.h"
#include "graph/debug/ge_attr_define.h"

#undef private
#undef protected

namespace ge {
class UtestOpDescUtils : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};
namespace {
///     Data    const1
///        \  /
///        addn
///
ComputeGraphPtr BuildGraph1() {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto data = builder.AddNode("Data", "Data", 1, 1);
  auto const1 = builder.AddNode("const1", "Const", 1, 1);
  auto addn = builder.AddNode("addn", "AddN", 2, 1);

  int32_t weight[1] = {1};
  GeTensorDesc weight_desc(GeShape({1}), FORMAT_NHWC, DT_INT32);
  GeTensorPtr tensor0 = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  OpDescUtils::SetWeights(const1, {tensor0});

  builder.AddDataEdge(data, 0, addn, 0);
  builder.AddDataEdge(const1, 0, addn, 1);
  return builder.GetGraph();
}
///   (p_const)addn    const1
///          /     \   /
///        cast     mul
///
ComputeGraphPtr BuildGraph2() {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto addn = builder.AddNode("addn", "AddN", 0, 2);
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto cast = builder.AddNode("cast", "Cast", 1, 1);
  auto mul = builder.AddNode("mul", "Mul", 2, 1);

  int32_t weight[1] = {1};
  GeTensorDesc weight_desc(GeShape({1}), FORMAT_NHWC, DT_INT32);
  GeTensorPtr tensor0 = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  AttrUtils::SetBool(addn->GetOpDesc(), ATTR_NAME_POTENTIAL_CONST, true);
  AttrUtils::SetListInt(addn->GetOpDesc(), ATTR_NAME_POTENTIAL_WEIGHT_INDICES, {0,1});
  AttrUtils::SetListTensor(addn->GetOpDesc(), ATTR_NAME_POTENTIAL_WEIGHT, {tensor0, tensor0});
  OpDescUtils::SetWeights(const1, {tensor0});

  builder.AddDataEdge(addn, 0, cast, 0);
  builder.AddDataEdge(addn, 1, mul, 0);
  builder.AddDataEdge(const1, 0, mul, 1);
  return builder.GetGraph();
}
///   (p_const)addn    const1
///          /     \   /
///        enter     mul
///         |
///       cast
ComputeGraphPtr BuildGraph3() {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto addn = builder.AddNode("addn", "AddN", 0, 2);
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto enter = builder.AddNode("enter", "Enter", 1, 1);
  auto cast = builder.AddNode("cast", "Cast", 1, 1);
  auto mul = builder.AddNode("mul", "Mul", 2, 1);

  int32_t weight[1] = {1};
  GeTensorDesc weight_desc(GeShape({1}), FORMAT_NHWC, DT_INT32);
  GeTensorPtr tensor0 = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  AttrUtils::SetBool(addn->GetOpDesc(), ATTR_NAME_POTENTIAL_CONST, true);
  AttrUtils::SetListInt(addn->GetOpDesc(), ATTR_NAME_POTENTIAL_WEIGHT_INDICES, {0,1});
  AttrUtils::SetListTensor(addn->GetOpDesc(), ATTR_NAME_POTENTIAL_WEIGHT, {tensor0, tensor0});
  OpDescUtils::SetWeights(const1, {tensor0});

  AttrUtils::SetBool(enter->GetOpDesc(), ENTER_ATTR_CONSTANT_FLAG, true);

  builder.AddDataEdge(addn, 0, enter, 0);
  builder.AddDataEdge(addn, 1, mul, 0);
  builder.AddDataEdge(const1, 0, mul, 1);
  builder.AddDataEdge(enter, 0, cast, 0);
  return builder.GetGraph();
}
}
TEST_F(UtestOpDescUtils, SetWeight) {
  auto graph = BuildGraph1();

  auto addn_node = graph->FindNode("addn");
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);

  map<int, ge::GeTensorPtr> weight0;
  weight0[-1] = tensor;
  auto ret = ge::OpDescUtils::SetWeights(*addn_node, weight0);
  EXPECT_NE(ret, 0);

  map<int, ge::GeTensorPtr> weight1;
  weight1[1] = tensor;
  ret = ge::OpDescUtils::SetWeights(*addn_node, weight1);
  EXPECT_EQ(ret, 0);
  auto const_node = graph->FindNode("const1");
  auto const_tensor = OpDescUtils::MutableWeights(const_node);
  EXPECT_EQ(const_tensor[0]->MutableData().size(), 3);
  auto in_nodes = addn_node->GetInAllNodes();
  EXPECT_EQ(in_nodes.size(), 2);

  map<int, ge::GeTensorPtr> weight2;
  weight2[2] = tensor;
  ret = ge::OpDescUtils::SetWeights(*addn_node, weight2);
  EXPECT_EQ(ret, 0);
  auto in_nodes1 = addn_node->GetInAllNodes();
  EXPECT_EQ(in_nodes1.size(), 3);
}

TEST_F(UtestOpDescUtils, GetRealConstInputNodeAndAnchor) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn");
  auto nodes_2_out_anchor = OpDescUtils::GetConstInputNodeAndAnchor(*add_node);
  EXPECT_EQ(nodes_2_out_anchor.size(), 1);
  EXPECT_EQ(nodes_2_out_anchor[0].first->GetName(), "const1");
  EXPECT_EQ(nodes_2_out_anchor[0].second->GetIdx(), 0);
}
TEST_F(UtestOpDescUtils, GetMixConstInputNodeAndAnchor) {
  auto graph = BuildGraph2();
  auto mul_node = graph->FindNode("mul");
  auto nodes_2_out_anchor = OpDescUtils::GetConstInputNodeAndAnchor(*mul_node);
  EXPECT_EQ(nodes_2_out_anchor.size(), 2);
  EXPECT_EQ(nodes_2_out_anchor[0].first->GetName(), "addn");
  EXPECT_EQ(nodes_2_out_anchor[0].second->GetIdx(), 1);
  EXPECT_EQ(nodes_2_out_anchor[1].first->GetName(), "const1");
  EXPECT_EQ(nodes_2_out_anchor[1].second->GetIdx(), 0);
}
TEST_F(UtestOpDescUtils, GetInputDataByIndexForMixInputConst) {
  auto graph = BuildGraph2();
  auto mul_node = graph->FindNode("mul");
  auto nodes_2_out_anchor = OpDescUtils::GetConstInputNodeAndAnchor(*mul_node);
  EXPECT_EQ(nodes_2_out_anchor.size(), 2);
  EXPECT_EQ(nodes_2_out_anchor[0].first->GetName(), "addn");
  EXPECT_EQ(nodes_2_out_anchor[0].second->GetIdx(), 1);
  EXPECT_EQ(nodes_2_out_anchor[1].first->GetName(), "const1");
  EXPECT_EQ(nodes_2_out_anchor[1].second->GetIdx(), 0);

  auto weights = OpDescUtils::GetWeightsFromNodes(nodes_2_out_anchor);
  EXPECT_EQ(weights.size(), 2);
  EXPECT_EQ(weights[0]->GetTensorDesc().GetDataType(), DT_INT32);
  EXPECT_EQ(weights[1]->GetTensorDesc().GetDataType(), DT_INT32);
}
TEST_F(UtestOpDescUtils, GetPotentailWeightByIndexAccrossEnter) {
  auto graph = BuildGraph3();
  auto cast_node = graph->FindNode("cast");
  auto nodes_2_out_anchor = OpDescUtils::GetConstInputNodeAndAnchor(*cast_node);
  EXPECT_EQ(nodes_2_out_anchor.size(), 1);
  EXPECT_EQ(nodes_2_out_anchor[0].first->GetName(), "addn");
  EXPECT_EQ(nodes_2_out_anchor[0].second->GetIdx(), 0);

  auto weights = OpDescUtils::GetWeightsFromNodes(nodes_2_out_anchor);
  EXPECT_EQ(weights.size(), 1);
  EXPECT_EQ(weights[0]->GetTensorDesc().GetDataType(), DT_INT32);
}
}
