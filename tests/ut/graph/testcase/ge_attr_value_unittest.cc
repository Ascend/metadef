/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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
#include <iostream>
#include "graph/op_desc.h"
#include "graph/ge_attr_value.h"
#include "graph/utils/attr_utils.h"

namespace ge {
class UtestGeAttrValue : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestGeAttrValue, GetAllAttrsStr) {
  string name = "const";
  string type = "Constant";
  OpDescPtr op_desc = std::make_shared<OpDesc>(name, type);
  EXPECT_TRUE(op_desc);
  static const string value = "value";
  op_desc->SetAttr(value, GeAttrValue::CreateFrom<GeAttrValue::INT>(0));
  string attr = AttrUtils::GetAllAttrsStr(op_desc);
  string res = value + ":" + "i: 0\n;";
  EXPECT_EQ(res, attr);
}
}
