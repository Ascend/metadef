
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "register/op_def_registry.h"

namespace ops {

namespace {

class OpAttrDefUT : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(OpAttrDefUT, AttrTest) {
  OpDef opDef("Test");
  OpAttrDef attr("Test");
  OpAttrDef attr2("Test");
  OpAttrDef attr3("Test1");
  EXPECT_EQ(attr == attr2, true);
  EXPECT_EQ(attr == attr3, false);
  attr.Bool();
  EXPECT_EQ(attr.GetCfgDataType(), "bool");
  EXPECT_EQ(attr.GetProtoDataType(), "Bool");
  opDef.Attr("Test");
  EXPECT_EQ(opDef.GetAttrs().size(), 1);
  opDef.Attr("Test");
  EXPECT_EQ(opDef.GetAttrs().size(), 1);
  opDef.Attr("Test1");
  EXPECT_EQ(opDef.GetAttrs().size(), 2);
  attr.AttrType(OPTIONAL).Bool(true);
  EXPECT_EQ(attr.GetAttrDefaultVal("[]"), "true");
  attr.AttrType(OPTIONAL).Int(10);
  EXPECT_EQ(attr.GetAttrDefaultVal("[]"), "10");
  attr.AttrType(OPTIONAL).Str("test");
  EXPECT_EQ(attr.GetAttrDefaultVal("[]"), "test");
  attr.AttrType(OPTIONAL).Float(0.1);
  EXPECT_EQ(attr.GetAttrDefaultVal("[]"), "0.1");
  attr.AttrType(OPTIONAL).ListBool({true, false});
  EXPECT_EQ(attr.GetAttrDefaultVal("[]"), "[true,false]");
  attr.AttrType(OPTIONAL).ListFloat({0.1, 0.1});
  EXPECT_EQ(attr.GetAttrDefaultVal("[]"), "[0.1,0.1]");
  attr.AttrType(OPTIONAL).ListInt({1, 2});
  EXPECT_EQ(attr.GetAttrDefaultVal("[]"), "[1,2]");
  attr.AttrType(OPTIONAL).ListListInt({{1, 2}, {3, 4}});
  EXPECT_EQ(attr.GetAttrDefaultVal("[]"), "[[1,2],[3,4]]");
}

}  // namespace
}  // namespace ops
