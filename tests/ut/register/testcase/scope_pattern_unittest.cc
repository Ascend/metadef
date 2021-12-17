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
#include <gmock/gmock.h>
#include <vector>

#define private public
#define protected public
#include "register/scope/scope_pattern_impl.h"
#include "register/scope/scope_graph_impl.h"
#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {

class ScopePatternUt : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(ScopePatternUt, ScopeAttrValue1) {
  ScopeAttrValue scope_attr_value;

  float32_t value = 0.2;
  scope_attr_value.SetFloatValue(value);
  EXPECT_EQ(scope_attr_value.impl_->GetFloatValue(), static_cast<float32_t>(0.2));

  int64_t value2 = 2;
  scope_attr_value.SetIntValue(value2);
  EXPECT_EQ(scope_attr_value.impl_->GetIntValue(), 2);

  scope_attr_value.SetStringValue("abc");
  EXPECT_EQ(scope_attr_value.impl_->GetStrValue(), string("abc"));

  scope_attr_value.SetStringValue(string("def"));
  EXPECT_EQ(scope_attr_value.impl_->GetStrValue(), string("def"));

  scope_attr_value.SetBoolValue(true);
  EXPECT_TRUE(scope_attr_value.impl_->GetBoolValue());
}

TEST_F(ScopePatternUt, ScopeAttrValue2) {
  ScopeAttrValue scope_attr_value;
  scope_attr_value.impl_ = nullptr;

  float32_t value = 0.2;
  scope_attr_value.SetFloatValue(value);

  int64_t value2 = 2;
  scope_attr_value.SetIntValue(value2);
  scope_attr_value.SetStringValue("abc");
  scope_attr_value.SetStringValue(string("def"));
  scope_attr_value.SetBoolValue(true);

  EXPECT_EQ(scope_attr_value.impl_, nullptr);
}

TEST_F(ScopePatternUt, NodeOpTypeFeature) {
  string nodeType = string("add");
  int32_t num = 1;
  int32_t step = 100;
  NodeOpTypeFeature ntf(nodeType, num, step);
  EXPECT_EQ(ntf.impl_->step_, step);

  NodeOpTypeFeature ntf2("edf", num, step);
  EXPECT_EQ(ntf2.impl_->node_type_, string("edf"));
}


}  // namespace ge