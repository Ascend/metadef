/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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

#include "graph/op_desc.h"
#include "register/hidden_input_func_registry.h"
#include <gtest/gtest.h>
#include <memory>
namespace ge {
ge::graphStatus HcomHiddenInputFunc(const ge::OpDescPtr &op_desc, void *&addr) {
  addr = reinterpret_cast<void *>(0xf1);
  return ge::GRAPH_SUCCESS;
}
class HiddenInputFuncRegistryUnittest : public testing::Test {};

TEST_F(HiddenInputFuncRegistryUnittest, HcomHiddenFuncRegisterSuccess_Test) {
  EXPECT_EQ(ge::HiddenInputFuncRegistry::GetInstance().FindHiddenInputFunc(ge::HiddenInputType::HCOM), nullptr);
  REG_HIDDEN_INPUT_FUNC(ge::HiddenInputType::HCOM, HcomHiddenInputFunc);
  ge::GetHiddenAddr func = nullptr;
  func = ge::HiddenInputFuncRegistry::GetInstance().FindHiddenInputFunc(ge::HiddenInputType::HCOM);
  EXPECT_EQ(func, HcomHiddenInputFunc);
  const ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>();
  void *res = nullptr;
  EXPECT_EQ(func(op_desc, res), ge::GRAPH_SUCCESS);
  EXPECT_EQ(reinterpret_cast<uint64_t>(res), 0xf1);
}
}  // namespace ge