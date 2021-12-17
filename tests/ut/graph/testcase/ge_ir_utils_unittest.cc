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
#include "graph/utils/ge_ir_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/op_desc.h"
#include "graph/compute_graph.h"
#include "graph_builder_utils.h"
#include "graph/node.h"
#include "graph/node_impl.h"
#include "test_std_structs.h"

namespace ge {
static ComputeGraphPtr CreateGraph_1_1_224_224(float *tensor_data) {
  ut::GraphBuilder builder("graph1");
  auto data1 = builder.AddNode("data1", "Data", {}, {"y"});
  AttrUtils::SetInt(data1->GetOpDesc(), "index", 0);
  auto const1 = builder.AddNode("const1", "Const", {}, {"y"});
  GeTensorDesc const1_td;
  const1_td.SetShape(GeShape({1, 1, 224, 224}));
  const1_td.SetOriginShape(GeShape({1, 1, 224, 224}));
  const1_td.SetFormat(FORMAT_NCHW);
  const1_td.SetOriginFormat(FORMAT_NCHW);
  const1_td.SetDataType(DT_FLOAT);
  const1_td.SetOriginDataType(DT_FLOAT);
  GeTensor tensor(const1_td);
  tensor.SetData(reinterpret_cast<uint8_t *>(tensor_data), sizeof(float) * 224 * 224);
  AttrUtils::SetTensor(const1->GetOpDesc(), "value", tensor);
  auto add1 = builder.AddNode("add1", "Add", {"x1", "x2"}, {"y"});
  add1->impl_->attrs_["test_attr1"] = GeAttrValue::CreateFrom<int64_t>(100);
  add1->impl_->attrs_["test_attr2"] = GeAttrValue::CreateFrom<string>("test");
  auto netoutput1 = builder.AddNode("NetOutputNode", "NetOutput", {"x"}, {});
  ge::AttrUtils::SetListListInt(add1->GetOpDesc()->MutableOutputDesc(0), "list_list_i", {{1, 0, 0, 0}});
  ge::AttrUtils::SetListInt(add1->GetOpDesc(), "list_i", {1});
  ge::AttrUtils::SetListStr(add1->GetOpDesc(), "list_s", {"1"});
  ge::AttrUtils::SetListFloat(add1->GetOpDesc(), "list_f", {1.0});
  ge::AttrUtils::SetListBool(add1->GetOpDesc(), "list_b", {false});
  builder.AddDataEdge(data1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add1, 1);
  builder.AddDataEdge(add1, 0, netoutput1, 0);

  return builder.GetGraph();
}

class GeIrUtilsUt : public testing::Test {};

TEST_F(GeIrUtilsUt, ModelSerialize) {
  ge::Model model1("model", "");
  ut::GraphBuilder builder("void");
  auto data_node = builder.AddNode("data", "Data", {}, {"y"});
  auto add_node = builder.AddNode("add", "Add", {}, {"y"});
  float tensor_data[224 * 224] = {1.0f};
  ComputeGraphPtr compute_graph = CreateGraph_1_1_224_224(tensor_data);
  compute_graph->AddInputNode(data_node);
  compute_graph->AddOutputNode(add_node);
  model1.SetGraph(GraphUtils::CreateGraphFromComputeGraph(compute_graph));
  onnx::ModelProto model_proto;
  EXPECT_TRUE(OnnxUtils::ConvertGeModelToModelProto(model1, model_proto));
  ge::Model model2;
  EXPECT_TRUE(ge::IsEqual("test", "test", "tag"));
  EXPECT_FALSE(ge::IsEqual(300, 20, "tag"));
}

TEST_F(GeIrUtilsUt, EncodeDataTypeUndefined) {
  DataType data_type = DT_DUAL;
  int ret = OnnxUtils::EncodeDataType(data_type);
  EXPECT_EQ(ret, onnx::TensorProto_DataType_UNDEFINED);
}

TEST_F(GeIrUtilsUt, EncodeNodeDescFail) {
  NodePtr node;
  onnx::NodeProto *node_proto;
  bool ret = OnnxUtils::EncodeNodeDesc(node, node_proto);
  EXPECT_EQ(ret, false);
}

TEST_F(GeIrUtilsUt, EncodeGraphFail) {
  ConstComputeGraphPtr graph;
  onnx::GraphProto *graph_proto;
  bool ret = OnnxUtils::EncodeGraph(graph, graph_proto);
  EXPECT_EQ(ret, false);
}

TEST_F(GeIrUtilsUt, EncodeNodeFail) {
  NodePtr node;
  onnx::NodeProto *node_proto;
  bool ret = OnnxUtils::EncodeNode(node, node_proto);
  EXPECT_EQ(ret, false);
}

TEST_F(GeIrUtilsUt, EncodeNodeLinkFail) {
  NodePtr node;
  onnx::NodeProto *node_proto;
  bool ret = OnnxUtils::EncodeNodeLink(node, node_proto);
  EXPECT_EQ(ret, false);
}

TEST_F(GeIrUtilsUt, ConvertGeModelToModelProtoFail) {
  ge::Model model;
  onnx::ModelProto model_proto;
  bool ret = OnnxUtils::ConvertGeModelToModelProto(model, model_proto);
  EXPECT_EQ(ret, false);
}

TEST_F(GeIrUtilsUt, ConvertGeModelToModelProtoGraphProtoIsNull) {
  ge::Model model("model", "");
  ComputeGraphPtr compute_graph;
  model.SetGraph(GraphUtils::CreateGraphFromComputeGraph(compute_graph));
  onnx::ModelProto model_proto;
  bool ret = OnnxUtils::ConvertGeModelToModelProto(model, model_proto);
  EXPECT_EQ(ret, false);
}

TEST_F(GeIrUtilsUt, DecodeNodeLinkImpFail) {
  OnnxUtils::NodeLinkInfo item;
  NodePtr node_ptr;
  bool ret = OnnxUtils::DecodeNodeLinkImp(item, node_ptr);
  EXPECT_EQ(ret, false);
}

TEST_F(GeIrUtilsUt, DecodeNodeDescFail) {
  onnx::NodeProto *node_proto;
  OpDescPtr op_desc;
  bool ret = OnnxUtils::DecodeNodeDesc(node_proto, op_desc);
  EXPECT_EQ(ret, false);
}

TEST_F(GeIrUtilsUt, DecodeGraphFail) {
  int32_t recursion_depth = 20;
  onnx::GraphProto graph_proto;
  ComputeGraphPtr graph;
  bool ret = OnnxUtils::DecodeGraph(recursion_depth, graph_proto, graph);
  EXPECT_EQ(ret, false);
}


}