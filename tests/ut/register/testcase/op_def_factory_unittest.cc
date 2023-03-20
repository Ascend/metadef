
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "register/op_def_registry.h"

namespace ops {
namespace {
class OpDefFactoryUT : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

class AddTik2 : public OpDef {
 public:
  AddTik2(const char *name) : OpDef(name) {}
};

OP_ADD(AddTik2, None);

TEST_F(OpDefFactoryUT, OpDefFactoryTest) {
  auto &ops = OpDefFactory::GetAllOp();
  for (auto &op : ops) {
    OpDef opDef = OpDefFactory::OpDefCreate(op.GetString());
    EXPECT_EQ(opDef.GetOpType(), "AddTik2");
  }
  EXPECT_EQ(ops.size(), 1);
}

}  // namespace
}  // namespace ops
