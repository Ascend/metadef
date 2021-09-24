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
#include "graph/op_desc.h"
#include "graph/ge_tensor.h"
#undef private
#undef protected
#include "graph/utils/transformer_utils.h"

namespace ge {
class UtestOpDesc : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestOpDesc, TestCommonVerifyOnDummyShape) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({-3}));
  tensor_desc->SetFormat(FORMAT_NCHW);
  tensor_desc->SetDataType(DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>("test", "Identity");
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddOutputDesc(tensor_desc->Clone());

  EXPECT_EQ(GRAPH_SUCCESS, op_desc->CommonVerify());
}

TEST_F(UtestOpDesc, TestOpDescGetSetTensorDesc) {
  GeTensorDesc desc(GeShape(), FORMAT_NCHW, DT_INT32);
  OpDesc op_desc("foo", "Foo");
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddInputDesc("x", desc));
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddOutputDesc("y", desc));

  EXPECT_EQ(op_desc.GetInputDesc("x"), desc);
  EXPECT_EQ(op_desc.GetOutputDesc("y"), desc);
}

TEST_F(UtestOpDesc, TestNodeShapeTransUtils) {

  NodeShapeTransUtils transformer1(nullptr);
  EXPECT_NE(transformer1.Init(), true);

  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1, 1, 16, 16}));
  tensor_desc->SetFormat(FORMAT_FRACTAL_NZ);
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetOriginFormat(FORMAT_ND);

  auto op_desc = std::make_shared<OpDesc>("test", "Identity");
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddOutputDesc(tensor_desc->Clone());
  NodeShapeTransUtils transformer2(op_desc);
  EXPECT_EQ(transformer2.Init(), true);
  EXPECT_EQ(transformer2.CatchFormatAndShape(), true);
  EXPECT_EQ(transformer2.UpdateFormatAndShape(), true);


  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddOutputDesc(tensor_desc->Clone());

  NodeShapeTransUtils transformer3(op_desc);
  EXPECT_EQ(transformer3.Init(), true);
  EXPECT_EQ(transformer3.CatchFormatAndShape(), true);
  EXPECT_EQ(transformer3.UpdateFormatAndShape(), true);


  EXPECT_EQ(GRAPH_SUCCESS, op_desc->CommonVerify());
}

TEST_F(UtestOpDesc, IndexOutOfRange) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetFormat(FORMAT_NCHW);
  tensor_desc->SetDataType(DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>("test", "Identity");
  op_desc->AddInputDesc(tensor_desc->Clone());

  EXPECT_NE(nullptr, op_desc->MutableInputDesc(0));
  EXPECT_EQ(nullptr, op_desc->MutableInputDesc(1));
  EXPECT_EQ(nullptr, op_desc->MutableInputDesc(999));
}

}
