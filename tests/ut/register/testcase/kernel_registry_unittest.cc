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

#include "register/kernel_registry_impl.h"
#include <gtest/gtest.h>

using namespace gert;
namespace test_gert {
class KernelRegistryTest : public testing::Test {};
namespace {
ge::graphStatus TestFuncCreator(const ge::Node *, KernelContext *) {
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus TestFuncInitializer(const ge::Node *, KernelContext *) {
  return ge::GRAPH_SUCCESS;
}
KernelStatus TestFunc1(KernelContext *context) {
  return 0;
}
}

TEST_F(KernelRegistryTest, RegisterKernelSuccess) {
  REGISTER_KERNEL(KernelRegistryTest1).RunFunc(TestFunc1);
  auto funcs = gert::KernelRegistryImpl::GetInstance().FindKernelFuncs("KernelRegistryTest1");
  ASSERT_NE(funcs, nullptr);
  EXPECT_EQ(funcs->run_func, TestFunc1);
  EXPECT_NE(funcs->outputs_creator, nullptr);
  EXPECT_NE(funcs->outputs_initializer, nullptr);
  EXPECT_EQ(funcs->outputs_creator(nullptr, nullptr), ge::GRAPH_SUCCESS);
  EXPECT_EQ(funcs->outputs_initializer(nullptr, nullptr), ge::GRAPH_SUCCESS);
}

TEST_F(KernelRegistryTest, RegisterKernelWithOutputCreator) {
  REGISTER_KERNEL(KernelRegistryTest2).RunFunc(TestFunc1).OutputsCreator(TestFuncCreator).OutputsInitializer(TestFuncInitializer);
  auto funcs = gert::KernelRegistryImpl::GetInstance().FindKernelFuncs("KernelRegistryTest2");
  ASSERT_NE(funcs, nullptr);
  EXPECT_EQ(funcs->run_func, TestFunc1);
  EXPECT_NE(funcs->outputs_creator, nullptr);
  EXPECT_NE(funcs->outputs_initializer, nullptr);
}
}