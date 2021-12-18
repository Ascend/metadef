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
#include "inc/external/register/scope/scope_fusion_pass_register.h"
#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {


class ScopeUtilUt : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(ScopeUtilUt, StringReplaceAll) {
  AscendString str = ScopeUtil::StringReplaceAll("123456", "123", "abc");
  EXPECT_EQ(str.GetString(), string("abc456"));
}


}  // namespace ge