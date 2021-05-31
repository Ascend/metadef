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

#include "external/graph/operator.h"
#include "external/graph/tensor.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_builder_utils.h"

#undef private
#undef protected

namespace ge {
class UtestOperater : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestOperater, GetInputConstData) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto data = builder.AddNode("Data", "Data", 0, 1);
  auto data2 = builder.AddNode("Data2", "Data", 0, 1);
  auto enter = builder.AddNode("Enter", "Enter", 1, 1);
  auto transdata = builder.AddNode("Transdata", "Transdata", 2, 1);
  auto netoutput = builder.AddNode("Netoutput", "NetOutput", 1, 0);
  builder.AddDataEdge(data2, 0, enter, 0);
  builder.AddDataEdge(data, 0, transdata, 0);
  builder.AddDataEdge(enter, 0, transdata, 1);
  builder.AddDataEdge(transdata, 0, netoutput, 0);
  auto graph = builder.GetGraph();

  auto ge_tensor = std::make_shared<GeTensor>();
  auto op_desc = transdata->GetOpDesc();
  op_desc->input_name_idx_["Data"] = 0;
  op_desc->input_name_idx_["Enter"] = 1;
  auto tensor_desc = op_desc->MutableInputDesc(0);
  AttrUtils::SetTensor(tensor_desc, "_value", ge_tensor);

  Tensor tensor;
  auto op = OpDescUtils::CreateOperatorFromNode(transdata);
  ASSERT_EQ(op.GetInputConstData("Data", tensor), GRAPH_SUCCESS);
  ASSERT_EQ(op.GetInputConstData("Enter", tensor), GRAPH_FAILED);
}
}
