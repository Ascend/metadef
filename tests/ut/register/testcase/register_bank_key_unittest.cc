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
#include <iostream>
#include "nlohmann/json.hpp"
#include "register/tuning_bank_key_registry.h"

namespace tuningtiling {
BEGIN_OP_BANK_KEY_DEF(MatmulKy)
OP_BANK_KEY_FIELD_DEF(uint32_t, batch);
OP_BANK_KEY_FIELD_DEF(std::vector<uint32_t>, shape);
END_OP_BANK_KEY_DEF

DECLARE_SCHEMA(MatmulKy, FIELD(MatmulKy, batch), FIELD(MatmulKy, shape));

REGISTER_OP_BANK_KEY_CLASS(matmul, MatmulKy);

bool ConvertForMatmul(const gert::TilingContext *context, OpBankKeyDefPtr &inArgs) {
  std::shared_ptr<MatmulKy> mmKy = std::static_pointer_cast<MatmulKy>(inArgs);
  if (context == nullptr) {
    mmKy->batch = 4;
    mmKy->shape = {1, 2};
    return false;
  }
  mmKy->batch = 8;
  mmKy->shape = {1, 2, 3};
  return true;
}

REGISTER_OP_BANK_KEY_CONVERT_FUN(matmul, ConvertForMatmul);

class RegisterOPBankKeyUT : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(RegisterOPBankKeyUT, convert_tiling_context) {
  MatmulKy mm;
  mm.batch = 1;
  mm.shape = {1, 2};
  nlohmann::json jsonval;
  mm.ToJson(jsonval);
  std::cout << "vec:" << jsonval["shape"].size() << std::endl;
  std::cout << "ori json:" << jsonval.dump() << std::endl;
  auto kydef = OpBankKeyClassFactory::CreateBankKeyInstance(ge::AscendString("unknow"));
  EXPECT_EQ(kydef == nullptr, true);
  kydef = OpBankKeyClassFactory::CreateBankKeyInstance(ge::AscendString("matmul"));
  ;
  EXPECT_EQ(kydef != nullptr, true);
  kydef->FromJson(jsonval);
  nlohmann::json res;
  kydef->ToJson(res);
  EXPECT_EQ(res, jsonval);
  std::cout << "expected json:" << res.dump() << std::endl;
  auto it = OpBankKeyFuncRegistry::RegisteredOpFuncInfo().find("matmul");
  it->second(nullptr, kydef);
  auto rawdata = kydef->GetRawBankKey();
  EXPECT_EQ(rawdata != nullptr, true);
  auto dsize = kydef->GetDataSize();
  EXPECT_EQ(dsize, 12);
  EXPECT_EQ(*((uint32_t *) (rawdata)), 4);
  EXPECT_EQ(*((uint32_t *) (rawdata + 4)), 1);
  EXPECT_EQ(*((uint32_t *) (rawdata + 8)), 2);
}
}  // namespace tuningtiling
