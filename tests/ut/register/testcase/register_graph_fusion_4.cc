/**
 * Copyright 2020-2022 Huawei Technologies Co., Ltd
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
#include "register/graph_optimizer/fusion_common/graph_pass_util.h"
#include "graph/debug/ge_log.h"
/**
 *      Input                         
 *        |                           
 *        A
 *        |
 *        B
 *        |  \    \
 *        C   D   E
 *        |                           
 *     NetOutput                      
 */
namespace fe {
using Mapping = std::map<const std::shared_ptr<ge::OpDesc>, std::vector<ge::NodePtr>>;
using namespace ge;
class TestSetOutputsPass1 : public GraphFusionPassBase {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) override;
};

/**
 *      Input
 *        |
 *        A
 *        |
 *        B
 *        |  \
 *        C   D 
 *        |
 *     NetOutput
 */

class TestSetOutputsPass2 : public GraphFusionPassBase {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) override;
};

class TestSetOutputsPass3 : public GraphFusionPassBase {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) override;
};


static const string TEST_SETOUPT_PASS_NAME = "TestSetOutputsFusionPass";
static const string OP_A = "A";
static const string OP_B = "B";
static const string OP_C = "C";
static const string OP_D = "D";
static const string OP_E = "E";
static const string OP_F = "F";
static const string TYPE_A = "TypeA";
static const string TYPE_B = "TypeB";
static const string TYPE_C = "TypeC";
static const string TYPE_D = "TypeD";
static const string TYPE_E = "TypeE";
static const string TYPE_F = "TypeF";


vector<FusionPattern *> TestSetOutputsPass1::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern = new(std::nothrow) FusionPattern("TestSetOutputsFusionPattern");

  pattern->AddOpDesc(OP_A, {TYPE_A})
      .AddOpDesc(OP_B, {TYPE_B})
      .AddOpDesc(OP_C, {TYPE_C})
      .AddOpDesc(OP_D, {TYPE_D})
      .AddOpDesc(OP_E, {TYPE_E})
      .SetInputs(OP_B, {OP_A})
      .SetOutputs(OP_B, {{0, {OP_C.c_str()}}, {1, {OP_D.c_str(), OP_E.c_str()}}})
      .SetInputs(OP_C, {OP_B})
      .SetOutput(OP_C);

  patterns.push_back(pattern);

  return patterns;
}

Status TestSetOutputsPass1::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) {
  return fe::SUCCESS;
}

vector<FusionPattern *> TestSetOutputsPass2::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern = new(std::nothrow) FusionPattern("TestSetOutputsFusionPattern");

  pattern->AddOpDesc(OP_A, {TYPE_A})
      .AddOpDesc(OP_B, {TYPE_B})
      .AddOpDesc(OP_C, {TYPE_C})
      .AddOpDesc(OP_D, {TYPE_D})
      .SetInputs(OP_B, {OP_A})
      .SetOutputs(OP_B, {{0, OP_C.c_str()}, {1, OP_D.c_str()}}, false)
      .SetInputs(OP_C, {OP_B})
      .SetOutput(OP_C);

  patterns.push_back(pattern);

  return patterns;
}

Status TestSetOutputsPass2::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) {
  return fe::SUCCESS;
}

vector<FusionPattern *> TestSetOutputsPass3::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern = new(std::nothrow) FusionPattern("TestSetOutputsFusionPattern");

  pattern->AddOpDesc(OP_A, {TYPE_A})
      .AddOpDesc(OP_B, {TYPE_B})
      .AddOpDesc(OP_C, {TYPE_C})
      .AddOpDesc(OP_D, {TYPE_D})
      .AddOpDesc(OP_F, {TYPE_F})
      .SetOutputs(OP_A, {{0, OP_B.c_str()}, {1, OP_F.c_str()}})
      .SetInputs(OP_B, {OP_A})
      .SetOutputs(OP_B, {{0, OP_C.c_str()}, {1, OP_D.c_str()}}, false)
      .SetInputs(OP_C, {OP_B})
      .SetOutput(OP_C);

  patterns.push_back(pattern);

  return patterns;
}

Status TestSetOutputsPass3::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) {
  return fe::SUCCESS;
}

class UTESTGraphFusionPass4 : public testing::Test {
  protected:
    void SetUp() {
    }
 
    void TearDown() {
 
    }
 
  protected:
    static ge::ComputeGraphPtr CreateTestOutputGraph1() {
      ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test1");
      ge::OpDescPtr op_desc_a = std::make_shared<ge::OpDesc>("A", TYPE_A);
      ge::OpDescPtr op_desc_b = std::make_shared<ge::OpDesc>("B", TYPE_B);
      ge::OpDescPtr op_desc_c = std::make_shared<ge::OpDesc>("C", TYPE_C);
      ge::OpDescPtr op_desc_out = std::make_shared<ge::OpDesc>("NetOut", "NetOut");
 
      //add descriptor
      vector<int64_t> dim_a = {8, 4, 16, 16};
      GeShape shape_a(dim_a);
      GeTensorDesc tensor_desc_a(shape_a);
      tensor_desc_a.SetFormat(FORMAT_NCHW);
      tensor_desc_a.SetOriginFormat(FORMAT_NCHW);
      tensor_desc_a.SetDataType(DT_FLOAT16);
      tensor_desc_a.SetOriginDataType(DT_FLOAT);
 
      op_desc_a->AddOutputDesc(tensor_desc_a);
 
      op_desc_b->AddInputDesc(tensor_desc_a);
      op_desc_b->AddOutputDesc(tensor_desc_a);
 
      op_desc_c->AddInputDesc(tensor_desc_a);
      op_desc_c->AddOutputDesc(tensor_desc_a);

      op_desc_out->AddInputDesc(tensor_desc_a);
 
      ge::NodePtr node_a = graph->AddNode(op_desc_a);
      ge::NodePtr node_b = graph->AddNode(op_desc_b);
      ge::NodePtr node_c = graph->AddNode(op_desc_c);
      ge::NodePtr node_out = graph->AddNode(op_desc_out);
 
      ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_out->GetInDataAnchor(0));
      return graph;
   }
 
    static ge::ComputeGraphPtr CreateTestOutputGraph2() {
      ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test1");
      ge::OpDescPtr op_desc_a = std::make_shared<ge::OpDesc>("A", TYPE_A);
      ge::OpDescPtr op_desc_b = std::make_shared<ge::OpDesc>("B", TYPE_B);
      ge::OpDescPtr op_desc_c = std::make_shared<ge::OpDesc>("C", TYPE_C);
      ge::OpDescPtr op_desc_d = std::make_shared<ge::OpDesc>("D", TYPE_D);
      ge::OpDescPtr op_desc_e = std::make_shared<ge::OpDesc>("E", TYPE_E);
      ge::OpDescPtr op_desc_out = std::make_shared<ge::OpDesc>("NetOut", "NetOut");

      //add descriptor
      vector<int64_t> dim_a = {8, 4, 16, 16};
      GeShape shape_a(dim_a);
      GeTensorDesc tensor_desc_a(shape_a);
      tensor_desc_a.SetFormat(FORMAT_NCHW);
      tensor_desc_a.SetOriginFormat(FORMAT_NCHW);
      tensor_desc_a.SetDataType(DT_FLOAT16);
      tensor_desc_a.SetOriginDataType(DT_FLOAT);
 
      op_desc_a->AddOutputDesc(tensor_desc_a);
 
      op_desc_b->AddInputDesc(tensor_desc_a);
      op_desc_b->AddOutputDesc(tensor_desc_a);
      op_desc_b->AddOutputDesc(tensor_desc_a);
 
      op_desc_c->AddInputDesc(tensor_desc_a);
      op_desc_c->AddOutputDesc(tensor_desc_a);
 
      op_desc_d->AddInputDesc(tensor_desc_a);
      op_desc_e->AddInputDesc(tensor_desc_a);
      
      op_desc_out->AddInputDesc(tensor_desc_a);

      ge::NodePtr node_a = graph->AddNode(op_desc_a);
      ge::NodePtr node_b = graph->AddNode(op_desc_b);
      ge::NodePtr node_c = graph->AddNode(op_desc_c);
      ge::NodePtr node_d = graph->AddNode(op_desc_d);
      ge::NodePtr node_e = graph->AddNode(op_desc_e);
      ge::NodePtr node_out = graph->AddNode(op_desc_out);

      ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(1), node_d->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(1), node_e->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_out->GetInDataAnchor(0));

      return graph;
    }
    
   static ge::ComputeGraphPtr CreateTestOutputGraph3() {
      ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test1");
      ge::OpDescPtr op_desc_a = std::make_shared<ge::OpDesc>("A", TYPE_A);
      ge::OpDescPtr op_desc_b = std::make_shared<ge::OpDesc>("B", TYPE_B);
      ge::OpDescPtr op_desc_c = std::make_shared<ge::OpDesc>("C", TYPE_C);
      ge::OpDescPtr op_desc_d = std::make_shared<ge::OpDesc>("D", TYPE_D);
      ge::OpDescPtr op_desc_e = std::make_shared<ge::OpDesc>("E", TYPE_E);
      ge::OpDescPtr op_desc_f = std::make_shared<ge::OpDesc>("F", TYPE_F);
      ge::OpDescPtr op_desc_out = std::make_shared<ge::OpDesc>("NetOut", "NetOut");

      //add descriptor
      vector<int64_t> dim_a = {8, 4, 16, 16};
      GeShape shape_a(dim_a);
      GeTensorDesc tensor_desc_a(shape_a);
      tensor_desc_a.SetFormat(FORMAT_NCHW);
      tensor_desc_a.SetOriginFormat(FORMAT_NCHW);
      tensor_desc_a.SetDataType(DT_FLOAT16);
      tensor_desc_a.SetOriginDataType(DT_FLOAT);
 
      op_desc_a->AddOutputDesc(tensor_desc_a);
      op_desc_a->AddOutputDesc(tensor_desc_a);

      op_desc_f->AddInputDesc(tensor_desc_a);
 
      op_desc_b->AddInputDesc(tensor_desc_a);
      op_desc_b->AddOutputDesc(tensor_desc_a);
      op_desc_b->AddOutputDesc(tensor_desc_a);
 
      op_desc_c->AddInputDesc(tensor_desc_a);
      op_desc_c->AddOutputDesc(tensor_desc_a);
 
      op_desc_d->AddInputDesc(tensor_desc_a);
      op_desc_e->AddInputDesc(tensor_desc_a);
      
      op_desc_out->AddInputDesc(tensor_desc_a);

      ge::NodePtr node_a = graph->AddNode(op_desc_a);
      ge::NodePtr node_b = graph->AddNode(op_desc_b);
      ge::NodePtr node_c = graph->AddNode(op_desc_c);
      ge::NodePtr node_d = graph->AddNode(op_desc_d);
      ge::NodePtr node_e = graph->AddNode(op_desc_e);
      ge::NodePtr node_f = graph->AddNode(op_desc_f);
      ge::NodePtr node_out = graph->AddNode(op_desc_out);

      ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(1), node_f->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(1), node_d->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(1), node_e->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_out->GetInDataAnchor(0));

      return graph;
    }

  };

TEST_F(UTESTGraphFusionPass4, UTESTGraphFusionPass4_1) {
  ge::ComputeGraphPtr graph = CreateTestOutputGraph1();
  TestSetOutputsPass1 pass;
  Status status = fe::FAILED;

  pass.SetName("test");
  status = pass.Run(*graph);
  EXPECT_EQ(status, fe::NOT_CHANGED);
}

TEST_F(UTESTGraphFusionPass4, UTESTGraphFusionPass4_2) {
  ge::ComputeGraphPtr graph = CreateTestOutputGraph2();
  TestSetOutputsPass1 pass;
  Status status = fe::FAILED;

  pass.SetName("test");
  status = pass.Run(*graph);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTESTGraphFusionPass4, UTESTGraphFusionPass4_3) {
  ge::ComputeGraphPtr graph = CreateTestOutputGraph2();
  TestSetOutputsPass2 pass;
  Status status = fe::FAILED;

  pass.SetName("test");
  status = pass.Run(*graph);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTESTGraphFusionPass4, UTESTGraphFusionPass4_4) {
  ge::ComputeGraphPtr graph = CreateTestOutputGraph3();
  TestSetOutputsPass3 pass;
  Status status = fe::FAILED;

  pass.SetName("test");
  status = pass.Run(*graph);
  EXPECT_EQ(status, fe::SUCCESS);
}

}
