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

#include "inc/external/register/register_pass.h"
#include "graph/debug/ge_log.h"
#include "register/custom_pass_helper.h"

#define protected public
#define private public

namespace ge {
class UtestRegisterPass : public testing::Test { 
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestRegisterPass, GetPriorityTest) {
  ge::PassRegistrationData pass_data;
  pass_data.impl_ = nullptr;
  int32_t ret = pass_data.GetPriority();
  EXPECT_EQ(ret, INT_MAX);
}
} // namespace ge
