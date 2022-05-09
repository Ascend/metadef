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

REG_OP(MultiAdd)
    .INPUT(x1, TensorType({DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_INT16,
                           DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                           DT_COMPLEX64, DT_STRING}))
    .INPUT(x2, TensorType({DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_INT16,
                           DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                           DT_COMPLEX64, DT_STRING}))
    .INPUT(x3, TensorType({DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_INT16,
                           DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                           DT_COMPLEX64, DT_STRING}))
    .INPUT(x4, TensorType({DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_INT16,
                           DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                           DT_COMPLEX64, DT_STRING}))
    .OUTPUT(y, TensorType({DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_INT16,
                           DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                           DT_COMPLEX64, DT_STRING}))
    .OP_END_FACTORY_REG(MultiAdd)

REG_OP(Relu)
    .INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16, DT_DOUBLE,
                          DT_INT8, DT_INT32, DT_INT16, DT_INT64,
                          DT_UINT8, DT_UINT16, DT_QINT8}))
    .OUTPUT(y, TensorType({DT_FLOAT, DT_FLOAT16, DT_DOUBLE,
                           DT_INT8, DT_INT32, DT_INT16, DT_INT64,
                           DT_UINT8, DT_UINT16, DT_QINT8}))
    .OP_END_FACTORY_REG(Relu)

class UTestAccelerator2 : public testing::Test {
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

    unique_ptr<int32_t[]> data(new(std::nothrow) int32_t[4096]);
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

    unique_ptr<int32_t[]> data(new(std::nothrow) int32_t[4096]);
    WeightInfo w(tensor_desc_a, data.get());
    acc.AddWeight(relu1_front, 0, w);
    acc.AddWeight(relu2_front, 0, w);

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

TEST_F(UTestAccelerator2, test_case_01) {
  auto graph = CreateComplexGraph();
  FusionTurbo acc(graph);
  string name = "add_new";
  string type = "Add";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu1 = GetNode(graph, "relu1");
  auto relu2 = GetNode(graph, "relu2");
  auto add = GetNode(graph, "add");
  auto out = GetNode(graph, "output");

  auto relu1_input = relu1->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_input = relu2->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

  Relations input_relations = {{0, {relu1_input, 0}},
                               {1, {relu2_input, 0}}};
  Relations output_relations = {0, {out, 0}};
  Status ret = acc.MultiInOne(node, input_relations, output_relations, {relu1, relu2, add}, true);
  EXPECT_EQ(ret, SUCCESS);

  auto relu1_out_nodes = relu1_input->GetOutDataNodes();
  auto relu2_out_nodes = relu2_input->GetOutDataNodes();
  ASSERT_EQ(relu1_out_nodes.size(), 1);
  ASSERT_EQ(relu2_out_nodes.size(), 1);
  EXPECT_EQ(relu1_out_nodes.at(0)->GetName(), "add_new");

  EXPECT_EQ(relu2_out_nodes.at(0)->GetName(), "add_new");
  auto out_in_nodes = out->GetInDataNodes();
  EXPECT_EQ(out_in_nodes.size(), 1);
  EXPECT_EQ(out_in_nodes.at(0)->GetName(), "add_new");
  EXPECT_EQ(graph->GetDirectNodesSize(), 4);
}

TEST_F(UTestAccelerator2, test_case_01_1) {
  auto graph = CreateComplexGraph();
  FusionTurbo acc(graph);
  string name = "add_new";
  string type = "Add";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu1 = GetNode(graph, "relu1");
  auto relu2 = GetNode(graph, "relu2");
  auto add = GetNode(graph, "add");
  auto out = GetNode(graph, "output");

  auto relu1_input = relu1->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_input = relu2->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

  Relations input_relations = {{0, {relu1_input, 0}},
                               {1, {relu2_input, 0}}};
  Relations output_relations = {0, {out, 0}};
  Status ret = acc.MultiInOne(node, input_relations, output_relations, {}, false);
  EXPECT_EQ(ret, SUCCESS);

  auto relu1_out_nodes = relu1_input->GetOutDataNodes();
  auto relu2_out_nodes = relu2_input->GetOutDataNodes();
  ASSERT_EQ(relu1_out_nodes.size(), 2);
  ASSERT_EQ(relu2_out_nodes.size(), 2);
  EXPECT_EQ(relu1_out_nodes.at(0)->GetName(), "relu1");
  EXPECT_EQ(relu1_out_nodes.at(1)->GetName(), "add_new");

  EXPECT_EQ(relu2_out_nodes.at(0)->GetName(), "relu2");
  EXPECT_EQ(relu2_out_nodes.at(1)->GetName(), "add_new");

  auto out_in_nodes = out->GetInDataNodes();
  EXPECT_EQ(out_in_nodes.size(), 2);
  EXPECT_EQ(out_in_nodes.at(0)->GetName(), "add_new");
  EXPECT_EQ(out_in_nodes.at(1)->GetName(), "add");

  auto add_in_nodes = add->GetInDataNodes();
  EXPECT_EQ(add_in_nodes.size(), 2);
  EXPECT_EQ(add_in_nodes.at(0)->GetName(), "relu1");
  EXPECT_EQ(add_in_nodes.at(1)->GetName(), "relu2");
}

TEST_F(UTestAccelerator2, test_case_01_2) {
  auto graph = CreateComplexGraph();
  FusionTurbo acc(graph);
  string name = "add_new";
  string type = "Add";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu1 = GetNode(graph, "relu1");
  auto relu2 = GetNode(graph, "relu2");
  auto add = GetNode(graph, "add");
  auto out = GetNode(graph, "output");

  auto relu1_input = relu1->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_input = relu2->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

  Relations input_relations = {{0, {relu1_input, 0}},
                               {0, {relu1_input, 0}},
                               {1, {relu2_input, 0}}};
  Relations output_relations = {0, {out, 0}};
  Status ret = acc.MultiInOne(node, input_relations, output_relations, {}, false);
  EXPECT_EQ(ret, SUCCESS);

  auto relu1_out_nodes = relu1_input->GetOutDataNodes();
  auto relu2_out_nodes = relu2_input->GetOutDataNodes();
  ASSERT_EQ(relu1_out_nodes.size(), 2);
  ASSERT_EQ(relu2_out_nodes.size(), 2);
  EXPECT_EQ(relu1_out_nodes.at(0)->GetName(), "relu1");
  EXPECT_EQ(relu1_out_nodes.at(1)->GetName(), "add_new");

  EXPECT_EQ(relu2_out_nodes.at(0)->GetName(), "relu2");
  EXPECT_EQ(relu2_out_nodes.at(1)->GetName(), "add_new");

  auto out_in_nodes = out->GetInDataNodes();
  EXPECT_EQ(out_in_nodes.size(), 2);
  EXPECT_EQ(out_in_nodes.at(0)->GetName(), "add_new");
  EXPECT_EQ(out_in_nodes.at(1)->GetName(), "add");

  auto add_in_nodes = add->GetInDataNodes();
  EXPECT_EQ(add_in_nodes.size(), 2);
  EXPECT_EQ(add_in_nodes.at(0)->GetName(), "relu1");
  EXPECT_EQ(add_in_nodes.at(1)->GetName(), "relu2");
}

TEST_F(UTestAccelerator2, test_case_01_3) {
  auto graph = CreateComplexGraph();
  FusionTurbo acc(graph);
  string name = "add_new";
  string type = "Add";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu1 = GetNode(graph, "relu1");
  auto relu2 = GetNode(graph, "relu2");
  auto add = GetNode(graph, "add");
  auto out = GetNode(graph, "output");

  auto relu1_input = relu1->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_input = relu2->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

  Relations input_relations = {{0, {relu1_input, 0}},
                               {2, {relu1_input, 0}},
                               {1, {relu2_input, 0}}};
  Relations output_relations = {0, {out, 0}};
  Status ret = acc.MultiInOne(node, input_relations, output_relations, {}, false);
  EXPECT_EQ(ret, FAILED);

  auto relu1_out_nodes = relu1_input->GetOutDataNodes();
  auto relu2_out_nodes = relu2_input->GetOutDataNodes();
  ASSERT_EQ(relu1_out_nodes.size(), 1);
  ASSERT_EQ(relu2_out_nodes.size(), 1);
  EXPECT_EQ(relu1_out_nodes.at(0)->GetName(), "relu1");

  EXPECT_EQ(relu2_out_nodes.at(0)->GetName(), "relu2");

  auto out_in_nodes = out->GetInDataNodes();
  ASSERT_EQ(out_in_nodes.size(), 1);
  EXPECT_EQ(out_in_nodes.at(0)->GetName(), "add");

  auto add_in_nodes = add->GetInDataNodes();
  ASSERT_EQ(add_in_nodes.size(), 2);
  EXPECT_EQ(add_in_nodes.at(0)->GetName(), "relu1");
  EXPECT_EQ(add_in_nodes.at(1)->GetName(), "relu2");
}


TEST_F(UTestAccelerator2, test_case_01_4) {
  auto graph = CreateComplexGraph();
  FusionTurbo acc(graph);
  string name = "add_new";
  string type = "Add";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu1 = GetNode(graph, "relu1");
  auto relu2 = GetNode(graph, "relu2");
  auto add = GetNode(graph, "add");
  auto out = GetNode(graph, "output");

  auto relu1_input = relu1->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_input = relu2->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

  Relations input_relations = {{0, {relu1_input, 0}},
                               {1, {relu2_input, 0}}};
  Relations output_relations = {{0, {out, 0}},
                                {1, {out, 1}}};
  Status ret = acc.MultiInOne(node, input_relations, output_relations, {}, false);
  EXPECT_EQ(ret, FAILED);

  auto relu1_out_nodes = relu1_input->GetOutDataNodes();
  auto relu2_out_nodes = relu2_input->GetOutDataNodes();
  ASSERT_EQ(relu1_out_nodes.size(), 1);
  ASSERT_EQ(relu2_out_nodes.size(), 1);
  EXPECT_EQ(relu1_out_nodes.at(0)->GetName(), "relu1");

  EXPECT_EQ(relu2_out_nodes.at(0)->GetName(), "relu2");

  auto out_in_nodes = out->GetInDataNodes();
  ASSERT_EQ(out_in_nodes.size(), 1);
  EXPECT_EQ(out_in_nodes.at(0)->GetName(), "add");

  auto add_in_nodes = add->GetInDataNodes();
  ASSERT_EQ(add_in_nodes.size(), 2);
  EXPECT_EQ(add_in_nodes.at(0)->GetName(), "relu1");
  EXPECT_EQ(add_in_nodes.at(1)->GetName(), "relu2");
  EXPECT_EQ(graph->GetDirectNodesSize(), 7);
}

TEST_F(UTestAccelerator2, test_case_2) {
  auto graph = CreateComplexGraph2();
  FusionTurbo acc(graph);
  string name = "add_new";
  string type = "Add";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu1 = GetNode(graph, "relu1");
  auto relu2 = GetNode(graph, "relu2");
  auto relu2_front = GetNode(graph, "relu2_front");
  auto add = GetNode(graph, "add");
  auto out = GetNode(graph, "output");

  auto relu1_input = relu1->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_front_input = relu2_front->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

  Relations input_relations = {{0, {relu1_input, 0}},
                               {1, {relu2_front_input, 0}}};
  Relations output_relations = {{0, {add, 0}},
                                {0, {relu2, 0}}};
  Status ret = acc.MultiInOne(node, input_relations, output_relations, {relu1, relu2_front}, true);
  EXPECT_EQ(ret, SUCCESS);

  auto out_nodes1 = relu1_input->GetOutDataNodes();
  auto out_nodes2 = relu2_front_input->GetOutDataNodes();
  ASSERT_EQ(out_nodes1.size(), 1);
  ASSERT_EQ(out_nodes2.size(), 1);

  EXPECT_EQ(out_nodes1.at(0)->GetName(), "add_new");
  EXPECT_EQ(out_nodes2.at(0)->GetName(), "add_new");
  auto out_in_nodes = out->GetInDataNodes();
  EXPECT_EQ(out_in_nodes.size(), 1);
  EXPECT_EQ(out_in_nodes.at(0)->GetName(), "add");

  auto add_new_out_nodes = node->GetOutDataNodes();
  EXPECT_EQ(add_new_out_nodes.size(), 2);
  EXPECT_EQ(add_new_out_nodes.at(0)->GetName(), "add");
  EXPECT_EQ(add_new_out_nodes.at(1)->GetName(), "relu2");
}


TEST_F(UTestAccelerator2, test_case_3) {
  auto graph = CreateComplexGraph2();
  FusionTurbo acc(graph);
  string name = "add_new";
  string type = "MultiAdd";
  auto node = acc.AddNodeOnly(name, type);
  ASSERT_NE(node, nullptr);

  auto relu1 = GetNode(graph, "relu1");
  auto relu1_front = GetNode(graph, "relu1_front");
  auto relu2 = GetNode(graph, "relu2");
  auto relu2_front = GetNode(graph, "relu2_front");
  auto add = GetNode(graph, "add");
  auto out = GetNode(graph, "output");

  auto relu1_input = relu1->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu1_front_input = relu1_front->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_input = relu2->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_front_input = relu2_front->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

  Relations input_relations = {{0, {relu1_input, 0}},
                               {1, {relu1_front_input, 0}},
                               {2, {relu2_input, 0}},
                               {3, {relu2_front_input, 0}}};
  Relations output_relations = {{0, {add, 0}},
                                {0, {add, 1}}};
  Status ret = acc.MultiInOne(node, input_relations, output_relations,
                              {relu1, relu1_front, relu2, relu2_front}, true);
  EXPECT_EQ(ret, SUCCESS);

  auto out_nodes1 = relu1_front_input->GetOutDataNodes();
  auto out_nodes2 = relu2_front_input->GetOutDataNodes();
  ASSERT_EQ(out_nodes1.size(), 1);
  ASSERT_EQ(out_nodes2.size(), 1);

  EXPECT_EQ(out_nodes1.at(0)->GetName(), "add_new");
  EXPECT_EQ(out_nodes2.at(0)->GetName(), "add_new");
  auto out_in_nodes = out->GetInDataNodes();
  EXPECT_EQ(out_in_nodes.size(), 1);
  EXPECT_EQ(out_in_nodes.at(0)->GetName(), "add");

  auto add_new_out_nodes = node->GetOutDataNodes();
  EXPECT_EQ(add_new_out_nodes.size(), 2);
  EXPECT_EQ(add_new_out_nodes.at(0)->GetName(), "add");
  EXPECT_EQ(add_new_out_nodes.at(1)->GetName(), "add");
  EXPECT_EQ(graph->GetDirectNodesSize(), 5);
}

TEST_F(UTestAccelerator2, test_case_4) {
  auto graph = CreateComplexGraph2();
  FusionTurbo acc(graph);
  string name = "add_new";
  string type = "MultiAdd";

  auto relu1 = GetNode(graph, "relu1");
  auto relu1_front = GetNode(graph, "relu1_front");
  auto relu2 = GetNode(graph, "relu2");
  auto relu2_front = GetNode(graph, "relu2_front");
  auto add = GetNode(graph, "add");
  auto out = GetNode(graph, "output");

  auto relu1_input = relu1->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu1_front_input = relu1_front->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_input = relu2->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  auto relu2_front_input = relu2_front->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

  Relations input_relations = {{0, {relu1_input, 0}},
                               {1, {relu1_front_input, 0}},
                               {2, {relu2_input, 0}},
                               {3, {relu2_front_input, 0}}};
  Relations output_relations = {{0, {add, 0}},
                                {0, {add, 1}}};
  auto node = acc.MultiInOne(name, type, input_relations, output_relations,
                              {relu1, relu1_front, relu2, relu2_front}, true);
  EXPECT_NE(node, nullptr);

  auto out_nodes1 = relu1_front_input->GetOutDataNodes();
  auto out_nodes2 = relu2_front_input->GetOutDataNodes();
  ASSERT_EQ(out_nodes1.size(), 1);
  ASSERT_EQ(out_nodes2.size(), 1);

  EXPECT_EQ(out_nodes1.at(0)->GetName(), "add_new");
  EXPECT_EQ(out_nodes2.at(0)->GetName(), "add_new");
  auto out_in_nodes = out->GetInDataNodes();
  EXPECT_EQ(out_in_nodes.size(), 1);
  EXPECT_EQ(out_in_nodes.at(0)->GetName(), "add");

  auto add_new_out_nodes = node->GetOutDataNodes();
  EXPECT_EQ(add_new_out_nodes.size(), 2);
  EXPECT_EQ(add_new_out_nodes.at(0)->GetName(), "add");
  EXPECT_EQ(add_new_out_nodes.at(1)->GetName(), "add");
  EXPECT_EQ(graph->GetDirectNodesSize(), 5);
}
}