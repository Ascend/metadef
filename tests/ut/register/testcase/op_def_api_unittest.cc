
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "register/op_def_registry.h"

namespace ge {

static ge::graphStatus InferShape4AddTik2(gert::InferShapeContext *context) {
  return GRAPH_SUCCESS;
}

static ge::graphStatus InferShapeRange4AddTik2(gert::InferShapeRangeContext *context) {
  return GRAPH_SUCCESS;
}

static ge::graphStatus InferDataType4AddTik2(gert::InferDataTypeContext *context) {
  return GRAPH_SUCCESS;
}

}  // namespace ge

namespace optiling {

static ge::graphStatus TilingTik2Add(gert::TilingContext *context) {
  return ge::GRAPH_SUCCESS;
}

static ge::graphStatus TilingPrepareTik2Add(gert::TilingParseContext *context) {
  return ge::GRAPH_SUCCESS;
}

static int check_op_support(const ge::Operator &op, ge::AscendString &result) {
  return 1;
}

static int get_op_support(const ge::Operator &op, ge::AscendString &result) {
  return 1;
}

static int op_select_format(const ge::Operator &op, ge::AscendString &result) {
  return 1;
}

static int get_op_specific_info(const ge::Operator &op, ge::AscendString &result) {
  return 1;
}

static int generalize_config(const ge::Operator &op, const ge::AscendString &generalize_config,
                             ge::AscendString &generalize_para) {
  return 1;
}

}  // namespace optiling

namespace ops {

namespace {

class OpDefAPIUT : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(OpDefAPIUT, APITest) {
  OpDef opDef("Test");
  opDef.SetInferShape(ge::InferShape4AddTik2);
  opDef.SetInferShapeRange(ge::InferShapeRange4AddTik2);
  opDef.SetInferDataType(ge::InferDataType4AddTik2);
  opDef.AICore().SetTiling(optiling::TilingTik2Add).SetTilingParse(optiling::TilingPrepareTik2Add);
  opDef.AICore()
      .SetCheckSupport(optiling::check_op_support)
      .SetOpSelectFormat(optiling::op_select_format)
      .SetOpSupportInfo(optiling::get_op_support)
      .SetOpSpecInfo(optiling::get_op_specific_info)
      .SetParamGeneralize(optiling::generalize_config);
  EXPECT_EQ(opDef.GetInferShape(), ge::InferShape4AddTik2);
  EXPECT_EQ(opDef.GetInferShapeRange(), ge::InferShapeRange4AddTik2);
  EXPECT_EQ(opDef.GetInferDataType(), ge::InferDataType4AddTik2);
  EXPECT_EQ(opDef.AICore().GetTiling(), optiling::TilingTik2Add);
  EXPECT_EQ(opDef.AICore().GetTilingParse(), optiling::TilingPrepareTik2Add);
  EXPECT_EQ(opDef.AICore().GetCheckSupport(), optiling::check_op_support);
  EXPECT_EQ(opDef.AICore().GetOpSelectFormat(), optiling::op_select_format);
  EXPECT_EQ(opDef.AICore().GetOpSupportInfo(), optiling::get_op_support);
  EXPECT_EQ(opDef.AICore().GetOpSpecInfo(), optiling::get_op_specific_info);
  EXPECT_EQ(opDef.AICore().GetParamGeneralize(), optiling::generalize_config);
}

}  // namespace
}  // namespace ops
