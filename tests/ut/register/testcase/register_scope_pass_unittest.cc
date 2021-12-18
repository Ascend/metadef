/**
 * Copyright 2021-2021 Huawei Technologies Co., Ltd
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
#include <gmock/gmock.h>
#include <vector>
#include <iostream>

#define private public
#define protected public
#include "external/register/scope/scope_fusion_pass_register.h"
#include "register/scope/scope_graph_impl.h"
#include "register/scope/scope_pass_impl.h"
#include "register/scope/scope_pass_registry_impl.h"
#undef private
#undef protected

using namespace ge;
class UtestScopePass : public testing::Test {
protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestScopePass, ScopesResultImplFail) {
  ScopesResult scopeRstOri;
  scopeRstOri.impl_.reset();

  std::vector<OperatorPtr> nodes;
  scopeRstOri.SetNodes(nodes);

  ScopesResult scopeRst1(scopeRstOri);
  EXPECT_EQ(scopeRst1.impl_->GetScopes().empty(), true);

  ScopesResult scopeRst2;
  scopeRst2 = scopeRst2;
  EXPECT_EQ(scopeRst2.impl_->GetScopes().empty(), true);

  ScopesResult scopeRst3;
  scopeRst3 = scopeRstOri;
  EXPECT_EQ(scopeRst3.impl_->GetScopes().empty(), true);
}

TEST_F(UtestScopePass, ScopesResultRegister) {
  ScopesResult scopeRstOri;
  std::vector<Scope *> scopes;
  std::vector<OperatorPtr> nodes;
  // test for initialized
  std::vector<Scope *> ScopeList = scopeRstOri.impl_->GetScopes();
  EXPECT_EQ(ScopeList.empty(), true);
  std::vector<OperatorPtr> NodeList = scopeRstOri.impl_->GetNodes();
  EXPECT_EQ(NodeList.empty(), true);

  // add scope
  Scope scope1;
  scope1.Init("scope1", "type1");
  scopes.push_back(&scope1);
  Scope scope2;
  scope2.Init("scope2", "type2");
  scopes.push_back(&scope2);
  scopeRstOri.SetScopes(scopes);
  // add node
  OperatorPtr node1(new (std::nothrow) ge::Operator("add", "Add"));
  nodes.push_back(node1);
  OperatorPtr node2(new (std::nothrow) ge::Operator("sub", "Sub"));
  nodes.push_back(node2);
  OperatorPtr node3(new (std::nothrow) ge::Operator("mul", "Mul"));
  nodes.push_back(node3);
  scopeRstOri.SetNodes(nodes);

  ScopesResult scopeRst1(scopeRstOri);
  ScopeList = scopeRst1.impl_->GetScopes();
  EXPECT_EQ(ScopeList.empty(), false);
  NodeList = scopeRst1.impl_->GetNodes();
  EXPECT_EQ(NodeList.empty(), false);

  ScopesResult scopeRst2;
  scopeRst2 = scopeRst1;
  ScopeList = scopeRst2.impl_->GetScopes();
  EXPECT_EQ(ScopeList.empty(), false);
  NodeList = scopeRst2.impl_->GetNodes();
  EXPECT_EQ(NodeList.empty(), false);
}

namespace {
class ScopePass : public ScopeBasePass {
public:
  ScopePattern *scoPattern1;
  ScopePattern *scoPattern2;

protected:
  std::vector<ScopeFusionPatterns> DefinePatterns() {
    std::vector<std::vector<std::vector<ScopePattern *>>> scoPattern;
    std::vector<std::vector<ScopePattern *>> scoPatternSub;
    std::vector<ScopePattern *> scoPatternSubSub1;
    std::vector<ScopePattern *> scoPatternSubSub2;

    scoPattern1 = new ScopePattern();
    scoPattern2 = new ScopePattern();
    scoPatternSubSub1.push_back(scoPattern1);
    scoPatternSubSub2.push_back(scoPattern2);

    scoPatternSub.push_back(scoPatternSubSub1);
    scoPatternSub.push_back(scoPatternSubSub2);

    scoPattern.push_back(scoPatternSub);
    return scoPattern;
  }
  std::string PassName() {
    return std::string("passName");
  }
  Status LastMatchScopesAndOPs(std::shared_ptr<ScopeGraph> &scope_graph,
                               std::vector<ScopesResult> &results) {
    return SUCCESS;
  }
  void GenerateFusionResult(const std::vector<Scope *> &scopes,
                            FusionScopesResult *fusion_rlt) {
    return;
  }
};

void CreateGraph(domi::tensorflow::GraphDef &graph_def) {
  // 1. add node
  auto placeholder0 = graph_def.add_node();
  auto placeholder1 = graph_def.add_node();
  auto add0 = graph_def.add_node();
  auto add1 = graph_def.add_node();
  auto mul0 = graph_def.add_node();
  auto mul1 = graph_def.add_node();
  auto mul2 = graph_def.add_node();
  auto add2 = graph_def.add_node();
  auto retval0 = graph_def.add_node();
  auto retval1 = graph_def.add_node();
  auto retval2 = graph_def.add_node();

  // 2. set info
  placeholder0->set_name("placeholder0");
  placeholder0->set_op("PlaceHolder");
  placeholder1->set_name("placeholder1");
  placeholder1->set_op("PlaceHolder");

  add0->set_name("add0");
  add0->set_op("Add");
  add1->set_name("add1");
  add1->set_op("Add");
  add2->set_name("add2");
  add2->set_op("Add");

  mul0->set_name("mul0");
  mul0->set_op("Mul");
  mul1->set_name("mul1");
  mul1->set_op("Mul");
  mul2->set_name("mul1/mul2");
  mul2->set_op("Mul");

  retval0->set_name("retval0");
  retval0->set_op("_RetVal");
  retval1->set_name("retval1");
  retval1->set_op("_RetVal");
  retval2->set_name("retval2");
  retval2->set_op("_RetVal");

  // 3. add edges
  add0->add_input("placeholder0");
  add0->add_input("placeholder1");

  mul0->add_input("placeholder0");
  mul0->add_input("placeholder1");

  mul1->add_input("placeholder0");
  mul1->add_input("add0");
  mul1->add_input("^mul0");

  mul2->add_input("mul0");
  mul2->add_input("add0");

  add1->add_input("mul0");
  add1->add_input("placeholder1");

  add2->add_input("mul1");
  add2->add_input("mul0");

  retval0->add_input("add2:0");
  retval1->add_input("add1:0");
  retval2->add_input("mul2:0");
}
}

TEST_F(UtestScopePass, ScopePassRun1) {
  // no scope match
  Status ret;
  domi::tensorflow::GraphDef graph_def;
  CreateGraph(graph_def);

  std::shared_ptr<ScopeGraph> scope_graph = std::make_shared<ScopeGraph>();
  ASSERT_NE(scope_graph, nullptr);
  ret = scope_graph->Init();
  ASSERT_EQ(ret, SUCCESS);
  auto &impl = scope_graph->impl_;
  impl->BuildScopeGraph(&graph_def);

  ScopePass scoBasePass;
  ret = scoBasePass.impl_->Run(scope_graph);
  EXPECT_EQ(ret, domi::SUCCESS);
}

