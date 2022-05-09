/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "graph/compute_graph.h"
#include "graph/graph.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/graph_utils.h"

#include "register/graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"
#include "register/graph_optimizer/graph_fusion/graph_fusion_pass_base.h"
#include "register/graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
#include "register/graph_optimizer/fusion_common/fusion_turbo.h"

#define protected public
#define private public
#include "external/graph/operator_factory.h"
#include "external/graph/operator_reg.h"
#include "graph/operator_factory_impl.h"
#include "graph/debug/ge_log.h"
#undef protected
#undef private

using namespace testing;
using namespace ge;
using namespace fe;

namespace fe {
REG_OP(Const)
    .OUTPUT(y,
            TensorType({DT_FLOAT, DT_FLOAT16, DT_INT8, DT_INT16, DT_UINT16, DT_UINT8, DT_INT32, DT_INT64, DT_UINT32,
                        DT_UINT64, DT_BOOL, DT_DOUBLE}))
        .ATTR(value, Tensor, Tensor())
        .OP_END_FACTORY_REG(Const);

REG_OP(Transpose)
    .INPUT(x,
           TensorType({DT_FLOAT, DT_FLOAT16, DT_INT8, DT_INT16, DT_UINT16, DT_UINT8, DT_INT32, DT_INT64, DT_UINT32,
                       DT_UINT64, DT_BOOL, DT_DOUBLE}))
        .INPUT(shape, TensorType({DT_INT32, DT_INT64}))
        .OUTPUT(y,
                TensorType({DT_FLOAT, DT_FLOAT16, DT_INT8, DT_INT16, DT_UINT16, DT_UINT8, DT_INT32, DT_INT64, DT_UINT32,
                            DT_UINT64, DT_BOOL, DT_DOUBLE}))
        .ATTR(axis, Int, 0)
        .ATTR(num_axes, Int, -1)
        .OP_END_FACTORY_REG(Transpose);

REG_OP(Add)
    .INPUT(x1, TensorType({DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_INT16,
                           DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                           DT_COMPLEX64, DT_STRING}))
        .INPUT(x2, TensorType({DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_INT16,
                               DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                               DT_COMPLEX64, DT_STRING}))
        .OUTPUT(y, TensorType({DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_INT16,
                               DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                               DT_COMPLEX64, DT_STRING}))
        .OP_END_FACTORY_REG(Add)

REG_OP(Relu)
    .INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16, DT_DOUBLE,
                          DT_INT8, DT_INT32, DT_INT16, DT_INT64,
                          DT_UINT8, DT_UINT16, DT_QINT8}))
        .OUTPUT(y, TensorType({DT_FLOAT, DT_FLOAT16, DT_DOUBLE,
                               DT_INT8, DT_INT32, DT_INT16, DT_INT64,
                               DT_UINT8, DT_UINT16, DT_QINT8}))
        .OP_END_FACTORY_REG(Relu)

class UTestAccelerator : public testing::Test {
 public:

 protected:


  void SetUp() {
  }

  void TearDown() {
  }

  ge::NodePtr GetNode(ComputeGraphPtr &graph, const string &name) {
    for (auto &node : graph->GetDirectNode()) {
      if (node->GetName() == name) {
        return node;
      }
    }
    return nullptr;
  }

  ComputeGraphPtr CreateGraphSingleInAndOut() {

    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test1");
    OpDescPtr op_desc_cast1 = std::make_shared<OpDesc>("cast1", "Cast");
    OpDescPtr op_desc_relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr op_desc_cast2 = std::make_shared<OpDesc>("cast2", "Cast");
    OpDescPtr op_desc_output = std::make_shared<OpDesc>("output", "NetOutput");
    OpDescPtr op_desc_input = std::make_shared<OpDesc>("other", "Other");

    //add descriptor
    vector<int64_t> dim_a = {8, 4, 16, 16};
    GeShape shape_a(dim_a);
    GeTensorDesc tensor_desc_a(shape_a);
    tensor_desc_a.SetFormat(FORMAT_NCHW);
    tensor_desc_a.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_a.SetDataType(DT_FLOAT16);
    tensor_desc_a.SetOriginDataType(DT_FLOAT);

    vector<int64_t> dim_b = {1, 4, 64, 64};
    GeShape shape_b(dim_b);
    GeTensorDesc tensor_desc_b(shape_b);
    tensor_desc_b.SetFormat(FORMAT_NCHW);
    tensor_desc_b.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_b.SetDataType(DT_FLOAT);
    tensor_desc_b.SetOriginDataType(DT_FLOAT);

    vector<int64_t> dim_c = {1, 4, 64, 64};
    GeShape shape_c(dim_c);
    GeTensorDesc tensor_desc_c(shape_c);
    tensor_desc_c.SetFormat(FORMAT_NCHW);
    tensor_desc_c.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_c.SetDataType(DT_FLOAT);
    tensor_desc_c.SetOriginDataType(DT_FLOAT);

    //vector<int64_t> dim_d;
    GeShape shape_d(dim_a);
    GeTensorDesc tensor_desc_d(shape_d);
    tensor_desc_d.SetFormat(FORMAT_NCHW);
    tensor_desc_d.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_d.SetDataType(DT_FLOAT16);
    tensor_desc_d.SetOriginDataType(DT_FLOAT);

    op_desc_input->AddOutputDesc(tensor_desc_a);

    op_desc_cast1->AddInputDesc(tensor_desc_a);
    op_desc_cast1->AddOutputDesc(tensor_desc_b);

    op_desc_relu->AddInputDesc(tensor_desc_b);
    op_desc_relu->AddOutputDesc(tensor_desc_c);

    op_desc_cast2->AddInputDesc(tensor_desc_c);
    op_desc_cast2->AddOutputDesc(tensor_desc_d);

    op_desc_output->AddInputDesc(tensor_desc_d);

    ge::AttrUtils::SetStr(op_desc_relu, "_op_compile_strategy", "{}");
    ge::AttrUtils::SetInt(op_desc_relu, "_keep_dtype", 1);

    NodePtr node_cast1 = graph->AddNode(op_desc_cast1);
    NodePtr node_relu = graph->AddNode(op_desc_relu);
    NodePtr node_cast2 = graph->AddNode(op_desc_cast2);
    NodePtr node_netoutput = graph->AddNode(op_desc_output);
    NodePtr node_other = graph->AddNode(op_desc_input);

    GraphUtils::AddEdge(node_other->GetOutDataAnchor(0), node_cast1->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_cast1->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_relu->GetOutDataAnchor(0), node_cast2->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_cast2->GetOutDataAnchor(0), node_netoutput->GetInDataAnchor(0));

    return graph;
  }
  ComputeGraphPtr CreateComplexGraph() {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test1");

    OpDescPtr op_desc_relu1 = std::make_shared<OpDesc>("relu1", "Relu");
    OpDescPtr op_desc_relu2 = std::make_shared<OpDesc>("relu2", "Relu");
    OpDescPtr op_desc_output = std::make_shared<OpDesc>("output", "NetOutput");

    //add descriptor
    vector<int64_t> dim_a = {8, 4, 16, 16};
    GeShape shape_a(dim_a);
    GeTensorDesc tensor_desc_a(shape_a);
    tensor_desc_a.SetFormat(FORMAT_NCHW);
    tensor_desc_a.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_a.SetDataType(DT_FLOAT16);
    tensor_desc_a.SetOriginDataType(DT_FLOAT);

    vector<int64_t> dim_b = {1, 4, 64, 64};
    GeShape shape_b(dim_b);
    GeTensorDesc tensor_desc_b(shape_b);
    tensor_desc_b.SetFormat(FORMAT_NCHW);
    tensor_desc_b.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_b.SetDataType(DT_FLOAT);
    tensor_desc_b.SetOriginDataType(DT_FLOAT);

    op_desc_relu1->AddInputDesc(tensor_desc_a);
    op_desc_relu1->AddOutputDesc(tensor_desc_b);

    op_desc_relu2->AddInputDesc(tensor_desc_a);
    op_desc_relu2->AddOutputDesc(tensor_desc_b);

    op_desc_output->AddInputDesc(tensor_desc_b);
    op_desc_output->AddInputDesc(tensor_desc_b);

    NodePtr node_relu1 = graph->AddNode(op_desc_relu1);
    NodePtr node_relu2 = graph->AddNode(op_desc_relu2);
    NodePtr node_netoutput = graph->AddNode(op_desc_output);

    GraphUtils::AddEdge(node_relu2->GetOutDataAnchor(0), node_netoutput->GetInDataAnchor(1));

    FusionTurbo acc(graph);
    auto node_add = acc.InsertNodeAfter("add", "Add", node_relu2, 0, 1);
    EXPECT_NE(node_add, nullptr);
    Relations rl(0, {node_relu1, 0});
    acc.LinkInput(rl, node_add, true);

    std::unique_ptr<int32_t> data(new(std::nothrow) int32_t(30));
    WeightInfo w(tensor_desc_a, data.get());
    acc.AddWeight(node_relu1, 0, w);
    acc.AddWeight(node_relu2, 0, w);

    return graph;
  }

  ComputeGraphPtr CreateComplexGraph2() {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test2");

    OpDescPtr op_desc_relu1 = std::make_shared<OpDesc>("relu1", "Relu");
    OpDescPtr op_desc_relu2 = std::make_shared<OpDesc>("relu2", "Relu");
    OpDescPtr op_desc_output = std::make_shared<OpDesc>("output", "NetOutput");

    //add descriptor
    vector<int64_t> dim_a = {8, 4, 16, 16};
    GeShape shape_a(dim_a);
    GeTensorDesc tensor_desc_a(shape_a);
    tensor_desc_a.SetFormat(FORMAT_NCHW);
    tensor_desc_a.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_a.SetDataType(DT_FLOAT16);
    tensor_desc_a.SetOriginDataType(DT_FLOAT);

    vector<int64_t> dim_b = {1, 4, 64, 64};
    GeShape shape_b(dim_b);
    GeTensorDesc tensor_desc_b(shape_b);
    tensor_desc_b.SetFormat(FORMAT_NCHW);
    tensor_desc_b.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_b.SetDataType(DT_FLOAT);
    tensor_desc_b.SetOriginDataType(DT_FLOAT);

    op_desc_relu1->AddInputDesc(tensor_desc_a);
    op_desc_relu1->AddOutputDesc(tensor_desc_b);

    op_desc_relu2->AddInputDesc(tensor_desc_a);
    op_desc_relu2->AddOutputDesc(tensor_desc_b);

    op_desc_output->AddInputDesc(tensor_desc_b);
    op_desc_output->AddInputDesc(tensor_desc_b);

    NodePtr node_relu1 = graph->AddNode(op_desc_relu1);
    NodePtr node_relu2 = graph->AddNode(op_desc_relu2);
    NodePtr node_netoutput = graph->AddNode(op_desc_output);

    GraphUtils::AddEdge(node_relu2->GetOutDataAnchor(0), node_netoutput->GetInDataAnchor(1));

    FusionTurbo acc(graph);
    auto node_add = acc.InsertNodeAfter("add", "Add", node_relu2, 0, 0);
    EXPECT_NE(node_add, nullptr);
    Relations rl(0, {node_relu1, 0});
    acc.LinkInput(rl, node_add, true);

    auto relu1_front = acc.InsertNodeBefore("relu1_front", "Relu", node_relu1, 0);

    auto relu2_front = acc.InsertNodeBefore("relu2_front", "Relu", node_relu2, 0);

    std::unique_ptr<int32_t> data(new(std::nothrow) int32_t(30));
    WeightInfo w(tensor_desc_a, data.get());
    acc.AddWeight(relu1_front, 0, w);
    acc.AddWeight(relu2_front, 0, w);

    DumpGraph(graph, "test2");
    return graph;
  }

  static void DumpGraph(const ge::ComputeGraphPtr graph, string graph_name) {
    printf("start to dump graph %s...\n", graph_name.c_str());
    for (ge::NodePtr node : graph->GetAllNodes()) {
      printf("node name = %s.\n", node->GetName().c_str());
      for (ge::OutDataAnchorPtr anchor : node->GetAllOutDataAnchors()) {
        for (ge::InDataAnchorPtr peer_in_anchor : anchor->GetPeerInDataAnchors()) {
          printf("    node name = %s[%d], out data node name = %s[%d].\n",
                 node->GetName().c_str(),
                 anchor->GetIdx(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str(),
                 peer_in_anchor->GetIdx());
        }
      }
      if (node->GetOutControlAnchor() != nullptr) {
        for (ge::InControlAnchorPtr peer_in_anchor : node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
          printf("    node name = %s, out control node name = %s.\n", node->GetName().c_str(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str());
        }
      }
    }
  }

};

TEST_F(UTestAccelerator, test_case_01) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";

  auto cast2 = GetNode(graph, "cast2");
  auto node = acc.InsertNodeBefore(name, type, cast2, 0);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {1, 4, 64, 64};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName(), "cast2");
}


TEST_F(UTestAccelerator, test_case_01_1) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";

  auto cast2 = GetNode(graph, "cast2");
  acc.BreakInput(cast2, {0});

  auto node = acc.InsertNodeBefore(name, type, cast2, 0);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims_null = {};
  vector<int64_t> dims = {1, 4, 64, 64};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims_null);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_ND);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor(), nullptr);
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName(), "cast2");
}

TEST_F(UTestAccelerator, test_case_02) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";

  auto relu = GetNode(graph, "relu");
  auto node = acc.InsertNodeAfter(name, type, relu, 0);

  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {1, 4, 64, 64};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName(), "cast2");
}


TEST_F(UTestAccelerator, test_case_02_1) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";

  auto relu = GetNode(graph, "relu");
  acc.BreakOutput(relu, {0});
  auto node = acc.InsertNodeAfter(name, type, relu, 0);

  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {1, 4, 64, 64};
  vector<int64_t> dims_null = {};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims_null);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_ND);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(), 0);
}

TEST_F(UTestAccelerator, test_case_03) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node1 = acc.AddNodeOnly(name, type);
  auto node2 = acc.AddNodeOnly(name, type);
  ASSERT_NE(node1, nullptr);
  ASSERT_NE(node2, nullptr);
}

/* cast2 already has input so Transpose will not have peer out. */
TEST_F(UTestAccelerator, test_case_04) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {1, 4, 64, 64};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(), 1);
}

TEST_F(UTestAccelerator, test_case_04_1) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  PeerIndices peerIdx = {{relu, 0}};
  Relations src_list;
  src_list.AddPeerIndex("x", {relu, 0});
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  dst_list.AddPeerIndex("y", {cast2, 0});
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {1, 4, 64, 64};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(), 1);
}

TEST_F(UTestAccelerator, test_case_04_2) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list;
  src_list.AddPeerIndex("x", {{relu, 0}});
  acc.LinkInput(src_list, node);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list;
  dst_list.AddPeerIndex("y", {{cast2, 0}});
  Status ret = acc.LinkOutput(dst_list, node);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {1, 4, 64, 64};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(), 1);
}


TEST_F(UTestAccelerator, test_case_04_3) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node);

  auto cast2 = GetNode(graph, "cast2");
  auto output_node = GetNode(graph, "output");
  Relations dst_list = {{cast2,       0},
                        {output_node, 0}};
  Status ret = FusionTurbo::LinkOutput(dst_list, node);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {1, 4, 64, 64};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(), 1);
}

/* Initialize case */
TEST_F(UTestAccelerator, test_case_04_5) {
  ge::NodePtr relu;

  Relations src_list(0, {relu, 0});

  Relations src_list1({0, {relu, 0}});

  Relations src_list2 = {{0, {relu, 0}},
                         {1, {relu, 0}}};

  Relations src_list3 = {{0, {{relu, 0}, {}}},
                         {1, {{relu, 0}, {relu, 1}}}};

  Relations src_list4(src_list1.relations);

  Relations src_list5((std::map<ThisIndex, PeerIndices>()));

  Relations src_list6(src_list1);

  Relations src_list7(std::move(src_list1));

  Relations src_list8(src_list2.relations_by_name);

  Relations src_list9(std::move(src_list2.relations_by_name));

  PeerIndex pi = {relu, 0};
  Relations src_list10(0, pi);
}

TEST_F(UTestAccelerator, test_case_05) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  acc.BreakInput(cast2, {0});
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);

  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {1, 4, 64, 64};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
  ASSERT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(), 1);
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName(), "cast2");
}

TEST_F(UTestAccelerator, test_case_06) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node, false);

  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);
  vector<int64_t> dims = {};
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  EXPECT_EQ(input->GetShape().GetDims(), dims);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_ND);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);

  EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetName(), "relu");
}

TEST_F(UTestAccelerator, test_case_07) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  acc.BreakInput(cast2, {0});
  Status ret = acc.LinkOutput(dst_list, node, false);
  EXPECT_EQ(ret, SUCCESS);

  EXPECT_EQ(node->GetName(), name);
  EXPECT_EQ(node->GetType(), type);

  vector<int64_t> dims = {};
  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(output->GetShape().GetDims(), dims);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_ND);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT);
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName(), "cast2");
}

TEST_F(UTestAccelerator, test_case_08) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);

  auto cast2 = GetNode(graph, "cast2");
  acc.BreakOutput(cast2, {0});
  acc.BreakOutput(cast2, {1});
  EXPECT_EQ(acc.RemoveNodeWithRelink(cast2, {0}), SUCCESS);
}

TEST_F(UTestAccelerator, test_case_09) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);

  auto cast2 = GetNode(graph, "cast2");
  auto cast1 = GetNode(graph, "cast1");
  EXPECT_EQ(acc.RemoveNodeOnly(cast2), SUCCESS);
  EXPECT_EQ(acc.RemoveNodeWithRelink(cast1, {0}), SUCCESS);
}

TEST_F(UTestAccelerator, test_case_10) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);

  auto cast2 = GetNode(graph, "cast2");
  cast2->GetOpDesc()->MutableOutputDesc(0)->SetShape(ge::GeShape({-1}));
  cast2->GetOpDesc()->MutableInputDesc(0)->SetOriginShape(ge::GeShape({-1}));
  EXPECT_EQ(false, acc.IsUnknownShape(cast2, 0));
  EXPECT_EQ(false, acc.IsUnknownShape(cast2, 0, true));
  EXPECT_EQ(true, acc.IsUnknownShape(cast2, 0, false));

  EXPECT_EQ(true, acc.IsUnknownOriShape(cast2, 0));
  EXPECT_EQ(true, acc.IsUnknownOriShape(cast2, 0, true));
  EXPECT_EQ(false, acc.IsUnknownOriShape(cast2, 0, false));
}

TEST_F(UTestAccelerator, test_case_11) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list;
  src_list.AddPeerIndex("x", {relu, 0});
  src_list.AddPeerIndex("x", {});
  EXPECT_EQ(SUCCESS, acc.LinkInput(src_list, node, false));

  auto cast2 = GetNode(graph, "cast2");

  Relations dst_list;
  dst_list.AddPeerIndex("y", {cast2, 0});
  dst_list.AddPeerIndex("y", {});
  EXPECT_EQ(SUCCESS, acc.LinkOutput(dst_list, node, false));
  auto input = node->GetOpDesc()->MutableInputDesc(0);

  // Update input desc
  vector<int64_t> dims = {};
  vector<int64_t> dims_new = {1, 4, 64, 64};
  EXPECT_EQ(SUCCESS, acc.UpdateInputByPeer(node, 0, relu, 0));
  EXPECT_EQ(input->GetShape().GetDims(), dims_new);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);
  EXPECT_EQ(input->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetOriginDataType(), ge::DT_FLOAT);

  input->SetShape(ge::GeShape(dims));
  input->SetFormat(ge::FORMAT_ND);

  acc.UpdateInputByPeer(node, 0, relu, 0);
  EXPECT_EQ(input->GetShape().GetDims(), dims_new);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);
  EXPECT_EQ(input->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetOriginDataType(), ge::DT_FLOAT);

  input->SetShape(ge::GeShape(dims));
  input->SetFormat(ge::FORMAT_ND);

  acc.UpdateInputByPeer(node, 0 , relu, 0);
  EXPECT_EQ(input->GetShape().GetDims(), dims_new);
  EXPECT_EQ(input->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetDataType(), ge::DT_FLOAT);
  EXPECT_EQ(input->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(input->GetOriginDataType(), ge::DT_FLOAT);

  input->SetShape(ge::GeShape(dims));
  input->SetFormat(ge::FORMAT_ND);
}

TEST_F(UTestAccelerator, test_case_12) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list;
  PeerIndex pi = {relu, 0};
  src_list.AddPeerIndex("x", pi);
  src_list.AddPeerIndex("x", pi);
  acc.LinkInput(src_list, node, false);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list;
  dst_list.AddPeerIndex("y", {{cast2, 0}});
  acc.LinkOutput(dst_list, node, false);

  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  auto cast1 = GetNode(graph, "cast1");
  // Update output desc
  vector<int64_t> dims = {};
  vector<int64_t> dims_new = {8, 4, 16, 16};
  EXPECT_EQ(SUCCESS, acc.UpdateOutputByPeer(node, 0, cast1, 0));
  EXPECT_EQ(output->GetShape().GetDims(), dims_new);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(output->GetOriginDataType(), ge::DT_FLOAT);
  output->SetShape(ge::GeShape(dims));
  output->SetFormat(ge::FORMAT_ND);

  acc.UpdateOutputByPeer(node, 0, cast1, 0);
  EXPECT_EQ(output->GetShape().GetDims(), dims_new);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(output->GetOriginDataType(), ge::DT_FLOAT);

  output->SetShape(ge::GeShape(dims));
  output->SetFormat(ge::FORMAT_ND);

  acc.UpdateOutputByPeer(node, 0, cast1, 0);
  EXPECT_EQ(output->GetShape().GetDims(), dims_new);
  EXPECT_EQ(output->GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(output->GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(output->GetOriginDataType(), ge::DT_FLOAT);

  output->SetShape(ge::GeShape(dims));
  output->SetFormat(ge::FORMAT_ND);
}

TEST_F(UTestAccelerator, test_case_13) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list;
  src_list.AddPeerIndex(0, {relu, 0});
  src_list.AddPeerIndex(0, {relu, 0});
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list;
  PeerIndex pi = {cast2, 0};
  dst_list.AddPeerIndex(0, pi);
  dst_list.AddPeerIndex(0, pi);
  acc.BreakInput(cast2, {0});
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }
  WeightInfo w = {ge::GeShape({1, 2, 3, 4}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  /* coverage code */
  auto shape = ge::GeShape({1, 2, 3, 4});
  WeightInfo w1 = {shape, ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  WeightInfo w2 = {ge::GeShape({1, 2, 3, 4}), ge::GeShape({1, 2, 3, 4}),
                   ge::DT_INT32, ge::DT_INT32, ge::FORMAT_NCHW,  ge::FORMAT_NCHW,
                   value.get()};

  ASSERT_NE(nullptr, acc.AddWeight(node, w));
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 3);
  EXPECT_EQ(node->GetInDataAnchor(2)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  auto new_weight = FusionTurbo::MutableWeight(node, 2);
  ASSERT_NE(nullptr, new_weight);

  EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginShape(), ge::GeShape({1, 2, 3, 4}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT32);
  EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NCHW);
  const uint8_t *data = new_weight->GetData().GetData();
  auto data_size = new_weight->GetData().size();
  EXPECT_EQ(data_size, 96);
  for (size_t i = 0; i < 96; i++) {
    EXPECT_EQ(data[i], i);
  }
}


TEST_F(UTestAccelerator, test_case_13_1) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  acc.BreakInput(cast2, {0});
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }
  WeightInfo w = {ge::GeShape({1, 2, 3, 4}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};

  ASSERT_NE(nullptr, acc.AddWeight(node, 3, w));
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 3);
  EXPECT_EQ(node->GetInDataAnchor(2)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  auto new_weight = FusionTurbo::MutableWeight(node, 2);
  ASSERT_NE(nullptr, new_weight);

  EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginShape(), ge::GeShape({1, 2, 3, 4}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT32);
  EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NCHW);
  const uint8_t *data = new_weight->GetData().GetData();
  auto data_size = new_weight->GetData().size();
  EXPECT_EQ(data_size, 96);
  for (size_t i = 0; i < 96; i++) {
    EXPECT_EQ(data[i], i);
  }
}

TEST_F(UTestAccelerator, test_case_14) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }

  auto input1 = node->GetOpDesc()->MutableInputDesc(1);
  input1->SetDataType(ge::DT_INT32);
  input1->SetOriginDataType(ge::DT_INT16);
  input1->SetFormat(ge::FORMAT_NCHW);
  input1->SetOriginFormat(ge::FORMAT_NHWC);
  input1->SetShape(ge::GeShape({1, 2, 3, 4}));
  input1->SetOriginShape(ge::GeShape({1, 3, 4, 2}));

  WeightInfo w(node, 1, value.get());

  ASSERT_NE(nullptr, acc.AddWeight(node, 1, w));
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 2);
  EXPECT_EQ(node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  auto new_weight = FusionTurbo::MutableWeight(node, 1);
  ASSERT_NE(nullptr, new_weight);

  EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginShape(), ge::GeShape({1, 3, 4, 2}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT16);
  EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NHWC);
  const uint8_t *data = new_weight->GetData().GetData();
  auto data_size = new_weight->GetData().size();
  EXPECT_EQ(data_size, 96);
  for (size_t i = 0; i < 96; i++) {
    EXPECT_EQ(data[i], i);
  }

  w.shape = ge::GeShape({5, 2, 7, 1});
  w.ori_shape = ge::GeShape({5, 1, 2, 7});
  w.datatype = ge::DT_FLOAT16;
  w.ori_datatype = ge::DT_FLOAT;
  w.format = ge::FORMAT_NHWC;
  w.ori_format = ge::FORMAT_NCHW;

  w.total_data_size = 140;
  unique_ptr<int32_t[]> value1(new(std::nothrow) int32_t[140]);
  auto data_ptr1 = (uint8_t *) (value1.get());
  for (size_t i = 0; i < 140; i++) {
    data_ptr1[i] = i + 1;
  }
  w.data = (uint8_t *) value1.get();
  /* Update const value and tensor when const node and weight both exist. */
  ASSERT_NE(nullptr, acc.AddWeight(node, 1, w));
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 2);
  EXPECT_EQ(node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  new_weight = FusionTurbo::MutableWeight(node, 1);
  ASSERT_NE(nullptr, new_weight);

  EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({5, 2, 7, 1}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginShape(), ge::GeShape({5, 1, 2, 7}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_FLOAT);
  EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NHWC);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NCHW);
  data = new_weight->GetData().GetData();
  data_size = new_weight->GetData().size();
  EXPECT_EQ(data_size, 140);
  for (size_t i = 0; i < 140; i++) {
    EXPECT_EQ(data[i], i + 1);
  }
}

TEST_F(UTestAccelerator, test_case_14_1) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }

  auto input1 = node->GetOpDesc()->MutableInputDesc(1);
  input1->SetDataType(ge::DT_INT32);
  input1->SetOriginDataType(ge::DT_INT16);
  input1->SetFormat(ge::FORMAT_NCHW);
  input1->SetOriginFormat(ge::FORMAT_NHWC);
  input1->SetShape(ge::GeShape({1, 2, 3, 4}));
  input1->SetOriginShape(ge::GeShape({1, 3, 4, 2}));

  WeightInfo w(node, 1, value.get());

  ASSERT_NE(nullptr, acc.AddWeight(node, "shape", w));
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 2);
  EXPECT_EQ(node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  auto new_weight = FusionTurbo::MutableWeight(node, 1);
  ASSERT_NE(nullptr, new_weight);

  EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginShape(), ge::GeShape({1, 3, 4, 2}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT16);
  EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NHWC);
  const uint8_t *data = new_weight->GetData().GetData();
  auto data_size = new_weight->GetData().size();
  EXPECT_EQ(data_size, 96);
  for (size_t i = 0; i < 96; i++) {
    EXPECT_EQ(data[i], i);
  }

  w.shape = ge::GeShape({5, 2, 7, 1});
  w.ori_shape = ge::GeShape({5, 1, 2, 7});
  w.datatype = ge::DT_FLOAT16;
  w.ori_datatype = ge::DT_FLOAT;
  w.format = ge::FORMAT_NHWC;
  w.ori_format = ge::FORMAT_NCHW;

  w.total_data_size = 140;
  unique_ptr<int32_t[]> value1(new(std::nothrow) int32_t[140]);
  auto data_ptr1 = (uint8_t *) (value1.get());
  for (size_t i = 0; i < 140; i++) {
    data_ptr1[i] = i + 1;
  }
  w.data = (uint8_t *) value1.get();
  /* Update const value and tensor when const node and weight both exist. */
  ASSERT_NE(nullptr, acc.AddWeight(node, "shape", w));
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 2);
  EXPECT_EQ(node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  new_weight = FusionTurbo::MutableWeight(node, 1);
  ASSERT_NE(nullptr, new_weight);

  EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({5, 2, 7, 1}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginShape(), ge::GeShape({5, 1, 2, 7}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_FLOAT);
  EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NHWC);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NCHW);
  data = new_weight->GetData().GetData();
  data_size = new_weight->GetData().size();
  EXPECT_EQ(data_size, 140);
  for (size_t i = 0; i < 140; i++) {
    EXPECT_EQ(data[i], i + 1);
  }
}


TEST_F(UTestAccelerator, test_case_14_1_1) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }

  auto input1 = node->GetOpDesc()->MutableInputDesc(1);
  input1->SetDataType(ge::DT_INT32);
  input1->SetOriginDataType(ge::DT_INT16);
  input1->SetFormat(ge::FORMAT_NCHW);
  input1->SetOriginFormat(ge::FORMAT_NHWC);
  input1->SetShape(ge::GeShape({1, 2, 3, 4}));
  input1->SetOriginShape(ge::GeShape({1, 3, 4, 2}));

  WeightInfo w(node, 1, value.get());

  ASSERT_NE(nullptr, acc.AddWeight(node, "shape", w));
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 2);
  EXPECT_EQ(node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  auto new_weight = FusionTurbo::MutableWeight(node, 1);
  ASSERT_NE(nullptr, new_weight);

  EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginShape(), ge::GeShape({1, 3, 4, 2}));
  EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT16);
  EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NHWC);
  const uint8_t *data = new_weight->GetData().GetData();
  auto data_size = new_weight->GetData().size();
  EXPECT_EQ(data_size, 96);
  for (size_t i = 0; i < 96; i++) {
    EXPECT_EQ(data[i], i);
  }

  w.shape = ge::GeShape({5, 2, 7, 1});
  w.ori_shape = ge::GeShape({5, 1, 2, 7});
  w.datatype = ge::DT_FLOAT16;
  w.ori_datatype = ge::DT_FLOAT;
  w.format = ge::FORMAT_NHWC;
  w.ori_format = ge::FORMAT_NCHW;

  w.total_data_size = 140;
  unique_ptr<int32_t[]> value1(new(std::nothrow) int32_t[140]);
  auto data_ptr1 = (uint8_t *) (value1.get());
  for (size_t i = 0; i < 140; i++) {
    data_ptr1[i] = i + 1;
  }
  w.data = (uint8_t *) value1.get();
  /* Update const value and tensor when const node and weight both exist. */
  ASSERT_EQ(nullptr, acc.AddWeight(node, "xxxx", w));
}

TEST_F(UTestAccelerator, test_case_14_2) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }
  auto const_node = acc.InsertNodeBefore("const_1", "Const", node, 1);
  ASSERT_NE(nullptr, const_node);
  auto const_out_desc = const_node->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(const_out_desc->GetShape(), ge::GeShape());
  EXPECT_EQ(const_out_desc->GetOriginShape(), ge::GeShape());
  EXPECT_EQ(const_out_desc->GetDataType(), ge::DT_FLOAT);
  EXPECT_EQ(const_out_desc->GetOriginDataType(), ge::DT_UNDEFINED);
  EXPECT_EQ(const_out_desc->GetFormat(), ge::FORMAT_ND);
  EXPECT_EQ(const_out_desc->GetOriginFormat(), ge::FORMAT_ND);

  auto weight = FusionTurbo::MutableWeight(node, 1);
  ASSERT_NE(nullptr, weight);
  auto &weight_tensor = weight->GetTensorDesc();
  EXPECT_EQ(weight_tensor.GetShape(), ge::GeShape());
  EXPECT_EQ(weight_tensor.GetOriginShape(), ge::GeShape());
  EXPECT_EQ(weight_tensor.GetDataType(), ge::DT_FLOAT);
  EXPECT_EQ(weight_tensor.GetOriginDataType(), ge::DT_UNDEFINED);
  EXPECT_EQ(weight_tensor.GetFormat(), ge::FORMAT_ND);
  EXPECT_EQ(weight_tensor.GetOriginFormat(), ge::FORMAT_ND);
  const uint8_t *data = weight->GetData().GetData();
  auto data_size = weight->GetData().size();
  EXPECT_EQ(data_size, 0);
  EXPECT_NE(data, nullptr);


  auto input1 = node->GetOpDesc()->MutableInputDesc(1);
  input1->SetDataType(ge::DT_INT32);
  input1->SetOriginDataType(ge::DT_INT16);
  input1->SetFormat(ge::FORMAT_NCHW);
  input1->SetOriginFormat(ge::FORMAT_NHWC);
  input1->SetShape(ge::GeShape({1, 2, 3, 4}));
  input1->SetOriginShape(ge::GeShape({1, 3, 4, 2}));

  WeightInfo w(node, 1, value.get());
  ASSERT_NE(nullptr, acc.AddWeight(node, 1, w));
  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 2);
  EXPECT_EQ(node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  auto new_weight = FusionTurbo::MutableWeight(node, 1);
  ASSERT_NE(nullptr, new_weight);

  auto &new_weight_tensor = new_weight->GetTensorDesc();
  EXPECT_EQ(new_weight_tensor.GetShape(), ge::GeShape({1, 2, 3, 4}));
  EXPECT_EQ(new_weight_tensor.GetOriginShape(), ge::GeShape({1, 3, 4, 2}));
  EXPECT_EQ(new_weight_tensor.GetDataType(), ge::DT_INT32);
  EXPECT_EQ(new_weight_tensor.GetOriginDataType(), ge::DT_INT16);
  EXPECT_EQ(new_weight_tensor.GetFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(new_weight_tensor.GetOriginFormat(), ge::FORMAT_NHWC);
  data = new_weight->GetData().GetData();
  data_size = new_weight->GetData().size();
  EXPECT_EQ(data_size, 96);
  for (size_t i = 0; i < 96; i++) {
    EXPECT_EQ(data[i], i);
  }
}

TEST_F(UTestAccelerator, test_case_15) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  acc.BreakInput(cast2, {0});
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }
  WeightInfo w = {ge::GeShape({1, 2, 3, 4}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  WeightInfo w2 = {ge::GeShape({1, 3, 2, 4}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  WeightInfo w3 = {ge::GeShape({4, 1, 3, 2}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  std::vector<WeightInfo> weight_all = {std::move(w), std::move(w2), std::move(w3)};

  auto const_nodes = acc.AddWeights(node, weight_all);
  ASSERT_EQ(const_nodes.size(), 3);

  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 5);
  EXPECT_EQ(node->GetInDataAnchor(2)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  EXPECT_EQ(node->GetInDataAnchor(3)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  EXPECT_EQ(node->GetInDataAnchor(4)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");

  for (size_t i = 2; i < 5; i++) {
    auto new_weight = FusionTurbo::MutableWeight(node, i);
    ASSERT_NE(nullptr, new_weight);
    if (i == 2) {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
    } else if (i == 3) {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 3, 2, 4}));
    } else {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({4, 1, 3, 2}));
    }

    EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
    EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT32);
    EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
    EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NCHW);
    const uint8_t *data = new_weight->GetData().GetData();
    auto data_size = new_weight->GetData().size();
    EXPECT_EQ(data_size, 96);
    for (size_t j = 0; j < 96; j++) {
      EXPECT_EQ(data[j], j);
    }
  }
}


TEST_F(UTestAccelerator, test_case_15_1) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  acc.BreakInput(cast2, {0});
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }
  WeightInfo w = {ge::GeShape({1, 2, 3, 4}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  WeightInfo w2 = {ge::GeShape({1, 3, 2, 4}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  WeightInfo w3 = {ge::GeShape({4, 1, 3, 2}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  acc.AddWeight(node, 1, w);
  std::vector<WeightInfo> weight_all = {std::move(w), std::move(w2), std::move(w3)};

  auto const_nodes = acc.AddWeights(node, weight_all);
  ASSERT_EQ(const_nodes.size(), 3);

  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 5);
  EXPECT_EQ(node->GetInDataAnchor(2)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  EXPECT_EQ(node->GetInDataAnchor(3)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  EXPECT_EQ(node->GetInDataAnchor(4)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  auto all_weights = ge::OpDescUtils::MutableWeights(node);
  ASSERT_EQ(all_weights.size(), 4);

  for (size_t i = 1; i < 5; i++) {
    auto new_weight = FusionTurbo::MutableWeight(node, i);
    ASSERT_NE(nullptr, new_weight);
    if (i == 1 || i == 2) {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
    } else if (i == 3) {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 3, 2, 4}));
    } else {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({4, 1, 3, 2}));
    }

    EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
    EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT32);
    EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
    EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NCHW);
    const uint8_t *data = new_weight->GetData().GetData();
    auto data_size = new_weight->GetData().size();
    EXPECT_EQ(data_size, 96);
    for (size_t j = 0; j < 96; j++) {
      EXPECT_EQ(data[j], j);
    }
  }
}

TEST_F(UTestAccelerator, test_case_15_3) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  acc.BreakInput(cast2, {0});
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }

  auto node_input_1 = node->GetOpDesc()->MutableInputDesc(1);
  auto weight_shape = ge::GeShape({1, 2, 3, 4});
  node_input_1->SetOriginShape(weight_shape);
  node_input_1->SetShape(weight_shape);
  node_input_1->SetDataType(ge::DT_INT32);
  node_input_1->SetOriginDataType(ge::DT_INT32);
  node_input_1->SetFormat(ge::FORMAT_NCHW);
  node_input_1->SetOriginFormat(ge::FORMAT_NCHW);

  WeightInfo w = {*node_input_1, value.get()};
  WeightInfo w2 = {ge::GeShape({1, 3, 2, 4}), ge::GeShape({1, 3, 2, 4}),
                   ge::DT_INT32, ge::DT_INT32, ge::FORMAT_NCHW, ge::FORMAT_NCHW, value.get()};
  WeightInfo w3 = {ge::GeShape({4, 1, 3, 2}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  acc.AddWeight(node, 1, w);
  std::vector<WeightInfo> weight_all = {std::move(w), std::move(w2), std::move(w3)};

  auto const_nodes = acc.AddWeights(node, weight_all);
  ASSERT_EQ(const_nodes.size(), 3);

  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 5);
  EXPECT_EQ(node->GetInDataAnchor(2)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  EXPECT_EQ(node->GetInDataAnchor(3)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  EXPECT_EQ(node->GetInDataAnchor(4)->GetPeerOutAnchor()->GetOwnerNode()->GetType(), "Const");
  auto all_weights = ge::OpDescUtils::MutableWeights(node);
  ASSERT_EQ(all_weights.size(), 4);

  for (size_t i = 1; i < 5; i++) {
    auto new_weight = FusionTurbo::MutableWeight(node, i);
    ASSERT_NE(nullptr, new_weight);
    if (i == 1 || i == 2) {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
    } else if (i == 3) {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 3, 2, 4}));
    } else {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({4, 1, 3, 2}));
    }

    EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
    EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT32);
    EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
    EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NCHW);
    const uint8_t *data = new_weight->GetData().GetData();
    auto data_size = new_weight->GetData().size();
    EXPECT_EQ(data_size, 96);
    for (size_t j = 0; j < 96; j++) {
      EXPECT_EQ(data[j], j);
    }
  }
}

TEST_F(UTestAccelerator, test_case_15_4) {
  auto graph = CreateGraphSingleInAndOut();
  FusionTurbo acc(graph);
  string name = "transpose";
  string type = "Transpose";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu = GetNode(graph, "relu");
  Relations src_list = {{relu, 0}};
  acc.LinkInput(src_list, node, true);

  auto cast2 = GetNode(graph, "cast2");
  Relations dst_list = {{cast2, 0}};
  acc.BreakInput(cast2, {0});
  Status ret = acc.LinkOutput(dst_list, node, true);
  EXPECT_EQ(ret, SUCCESS);

  unique_ptr<int32_t[]> value(new(std::nothrow) int32_t[24]);
  auto data_ptr = (uint8_t *) (value.get());
  for (size_t i = 0; i < 96; i++) {
    data_ptr[i] = i;
  }

  auto node_input_1 = node->GetOpDesc()->MutableInputDesc(1);
  auto weight_shape = ge::GeShape({1, 2, 3, 4});
  node_input_1->SetOriginShape(weight_shape);
  node_input_1->SetShape(weight_shape);
  node_input_1->SetDataType(ge::DT_INT32);
  node_input_1->SetOriginDataType(ge::DT_INT32);
  node_input_1->SetFormat(ge::FORMAT_NCHW);
  node_input_1->SetOriginFormat(ge::FORMAT_NCHW);

  WeightInfo w = {*node_input_1, value.get()};
  WeightInfo w2 = {ge::GeShape({1, 3, 2, 4}), ge::GeShape({1, 3, 2, 4}),
                   ge::DT_INT32, ge::DT_INT32, ge::FORMAT_NCHW, ge::FORMAT_NCHW, value.get()};
  WeightInfo w3 = {ge::GeShape({4, 1, 3, 2}), ge::DT_INT32, ge::FORMAT_NCHW, value.get()};
  acc.AddWeight(node, 1, w);
  std::vector<WeightInfo> weight_all = {std::move(w), std::move(w2), std::move(w3)};

  auto const_nodes = acc.AddWeights(node, weight_all);
  ASSERT_EQ(const_nodes.size(), 3);

  ASSERT_EQ(node->GetAllInDataAnchorsSize(), 5);
  auto const_2 = FusionTurbo::GetPeerOutNode(node, 2);
  auto const_2_peer_in = FusionTurbo::GetPeerInNodes(const_2, 0);
  ASSERT_EQ(const_2_peer_in.size(), 1);
  auto node_temp = const_2_peer_in.at(0);

  EXPECT_EQ(node_temp, node);
  EXPECT_EQ(FusionTurbo::CheckConnected(const_2, node), true);
  EXPECT_EQ(FusionTurbo::CheckConnected(const_2, node, 0), true);
  EXPECT_EQ(const_2->GetType(), "Const");
  EXPECT_EQ(FusionTurbo::GetPeerOutNode(node, 3)->GetType(), "Const");
  EXPECT_EQ(FusionTurbo::GetPeerOutNode(node, 4)->GetType(), "Const");
  auto all_weights = ge::OpDescUtils::MutableWeights(node);
  ASSERT_EQ(all_weights.size(), 4);

  for (size_t i = 1; i < 5; i++) {
    auto new_weight = FusionTurbo::MutableWeight(node, i);
    ASSERT_NE(nullptr, new_weight);
    if (i == 1 || i == 2) {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 2, 3, 4}));
    } else if (i == 3) {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({1, 3, 2, 4}));
    } else {
      EXPECT_EQ(new_weight->GetTensorDesc().GetShape(), ge::GeShape({4, 1, 3, 2}));
    }

    EXPECT_EQ(new_weight->GetTensorDesc().GetDataType(), ge::DT_INT32);
    EXPECT_EQ(new_weight->GetTensorDesc().GetOriginDataType(), ge::DT_INT32);
    EXPECT_EQ(new_weight->GetTensorDesc().GetFormat(), ge::FORMAT_NCHW);
    EXPECT_EQ(new_weight->GetTensorDesc().GetOriginFormat(), ge::FORMAT_NCHW);
    const uint8_t *data = new_weight->GetData().GetData();
    auto data_size = new_weight->GetData().size();
    EXPECT_EQ(data_size, 96);
    for (size_t j = 0; j < 96; j++) {
      EXPECT_EQ(data[j], j);
    }
  }
}
}