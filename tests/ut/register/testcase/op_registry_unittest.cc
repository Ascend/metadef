/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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
#include "register/op_impl_registry.h"
#include <gtest/gtest.h>
#include "exe_graph/runtime/kernel_context.h"
namespace gert_test {
class OpImplRegistryUT : public testing::Test {};
namespace {
ge::graphStatus TestFunc1(gert::KernelContext *context) {
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus TestInferShapeFunc1(gert::InferShapeContext *context) {
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus TestTilingFunc1(gert::TilingContext *context) {
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus TestTilingFunc2(gert::TilingContext *context) {
  return ge::GRAPH_SUCCESS;
}
}

TEST_F(OpImplRegistryUT, RegisterInferShapeOk) {
  IMPL_OP(TestConv2D).InferShape(TestInferShapeFunc1);

  EXPECT_EQ(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestConv2D").infer_shape, TestInferShapeFunc1);
  EXPECT_EQ(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestConv2D").tiling, nullptr);
  EXPECT_FALSE(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestConv2D").inputs_dependency);
}

TEST_F(OpImplRegistryUT, RegisterInferShapeAndTilingOk) {
  IMPL_OP(TestAdd).InferShape(TestInferShapeFunc1).Tiling(TestTilingFunc1);

  EXPECT_EQ(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestAdd").infer_shape, TestInferShapeFunc1);
  EXPECT_EQ(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestAdd").tiling, TestTilingFunc1);
  EXPECT_FALSE(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestAdd").inputs_dependency);
}

TEST_F(OpImplRegistryUT, InputsDependencyOk) {
  IMPL_OP(TestReshape).InferShape(TestInferShapeFunc1).InputsDataDependency({1});

  EXPECT_EQ(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestReshape").infer_shape, TestInferShapeFunc1);
  EXPECT_EQ(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestReshape").tiling, nullptr);
  EXPECT_EQ(gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl("TestReshape").inputs_dependency, 2);
}

TEST_F(OpImplRegistryUT, NotRegisterOp) {
  EXPECT_EQ(gert::OpImplRegistry::GetInstance().GetOpImpl("Test"), nullptr);
}
TEST_F(OpImplRegistryUT, DefaultImpl) {
  // auto tiling
  IMPL_OP_DEFAULT().Tiling(TestTilingFunc2);
  ASSERT_NE(gert::OpImplRegistry::GetInstance().GetOpImpl("DefaultImpl"), nullptr);
  EXPECT_EQ(gert::OpImplRegistry::GetInstance().GetOpImpl("DefaultImpl")->tiling, TestTilingFunc2);
}
}  // namespace gert_test