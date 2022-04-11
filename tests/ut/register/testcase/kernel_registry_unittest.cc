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

#include <gtest/gtest.h>
#include "register/kernel_registry_impl.h"

using namespace gert;
namespace test_gert {
class KernelRegistryTest : public testing::Test {};
KernelStatus TestFunc1(KernelContext *context) {
  return 0;
}

TEST_F(KernelRegistryTest, RegisterKernelSuccess) {
  EXPECT_EQ(gert::KernelRegistryImpl::GetInstance().FindKernelFuncs("KernelRegistryTest1"), nullptr);
  REGISTER_KERNEL(KernelRegistryTest1).RunFunc(TestFunc1);
  ASSERT_NE(gert::KernelRegistryImpl::GetInstance().FindKernelFuncs("KernelRegistryTest1"), nullptr);
  EXPECT_EQ(gert::KernelRegistryImpl::GetInstance().FindKernelFuncs("KernelRegistryTest1")->run_func, TestFunc1);
}
}