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

#include <gtest/gtest.h>
#include "graph/utils/op_type_utils.h"

namespace ge {
class UtestOpTypeUtils : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestOpTypeUtils, TestDataNodeType) {
  std::string test_node_type = "Data";
  EXPECT_TRUE(OpTypeUtils::IsDataNode(test_node_type));
  EXPECT_FALSE(OpTypeUtils::IsVariableNode(test_node_type));
  EXPECT_FALSE(OpTypeUtils::IsVarLikeNode(test_node_type));

  test_node_type = "AnnData";
  EXPECT_TRUE(OpTypeUtils::IsDataNode(test_node_type));

  test_node_type = "AippData";
  EXPECT_TRUE(OpTypeUtils::IsDataNode(test_node_type));

  test_node_type = "QueueData";
  EXPECT_TRUE(OpTypeUtils::IsDataNode(test_node_type));

  test_node_type = "RefData";
  EXPECT_TRUE(OpTypeUtils::IsDataNode(test_node_type));
}

TEST_F(UtestOpTypeUtils, TestVariableNodeType) {
  std::string test_node_type = "Variable";
  EXPECT_TRUE(OpTypeUtils::IsVariableNode(test_node_type));
  EXPECT_TRUE(OpTypeUtils::IsVarLikeNode(test_node_type));

  test_node_type = "VariableV2";
  EXPECT_TRUE(OpTypeUtils::IsVariableNode(test_node_type));
  EXPECT_TRUE(OpTypeUtils::IsVarLikeNode(test_node_type));

  test_node_type = "VarHandleOp";
  EXPECT_TRUE(OpTypeUtils::IsVariableNode(test_node_type));
  EXPECT_TRUE(OpTypeUtils::IsVarLikeNode(test_node_type));
}

TEST_F(UtestOpTypeUtils, TestVariableLikeNodeType) {
  std::string test_node_type = "RefData";
  EXPECT_FALSE(OpTypeUtils::IsVariableNode(test_node_type));
  EXPECT_TRUE(OpTypeUtils::IsVarLikeNode(test_node_type));
}
} // namespace ge
