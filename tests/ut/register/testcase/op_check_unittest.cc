#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "register/op_check.h"

bool testFunc(const ge::Operator &op, ge::AscendString &result) {
  return true;
}

namespace {

class OpCheckAPIUT : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(OpCheckAPIUT, APITest) {
  ge::AscendString example("test");
  optiling::GEN_SIMPLIFIEDKEY_FUNC pFunc;
  pFunc = testFunc;
  optiling::OpCheckFuncRegistry::RegisterGenSimplifiedKeyFunc(example, pFunc);

  ge::AscendString errName("notExisted");
  auto nullFunc = optiling::OpCheckFuncRegistry::GetGenSimplifiedKeyFun(errName);
  EXPECT_EQ(nullFunc, nullptr);

  auto func = optiling::OpCheckFuncRegistry::GetGenSimplifiedKeyFun(example);
  EXPECT_NE(func, nullptr);
}

}  // namespace
