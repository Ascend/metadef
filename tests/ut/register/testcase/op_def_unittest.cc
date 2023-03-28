
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

namespace ops {

namespace {

class OpDefUT : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(OpDefUT, Construct) {
  OpDef opDef("Test");
  opDef.Input("x1").DataType({ge::DT_FLOAT16});
  opDef.Output("y").DataType({ge::DT_FLOAT16});
  opDef.SetInferShape(ge::InferShape4AddTik2);
  opDef.SetInferShapeRange(ge::InferShapeRange4AddTik2);
  opDef.SetInferDataType(ge::InferDataType4AddTik2);
  opDef.SetWorkspaceFlag(true);
  OpAICoreConfig aicConfig;
  aicConfig.Input("x1")
      .ParamType(OPTIONAL)
      .DataType({ge::DT_FLOAT})
      .Format({ge::FORMAT_ND})
      .UnknownShapeFormat({ge::FORMAT_ND})
      .NeedCompile(false)
      .ValueDepend(REQUIRED)
      .ReshapeType("NC");
  opDef.AICore().AddConfig("ascend310p", aicConfig);
  opDef.OpProtoPost("Test");
  EXPECT_EQ(ge::AscendString("Test"), opDef.GetOpType());
  std::vector<OpParamDef> inputs = opDef.GetMergeInputs(aicConfig);
  EXPECT_EQ(inputs.size(), 1);
  OpParamDef param = inputs[0];
  EXPECT_EQ(param.GetParamName(), ge::AscendString("x1"));
  EXPECT_EQ(param.GetParamType(), OPTIONAL);
  EXPECT_EQ(param.GetDataTypes()[0], ge::DT_FLOAT);
  EXPECT_EQ(param.GetFormats()[0], ge::FORMAT_ND);
  EXPECT_EQ(param.GetUnknownShapeFormats()[0], ge::FORMAT_ND);
  EXPECT_EQ(param.GetNeedCompile(), ge::AscendString("false"));
  EXPECT_EQ(param.GetReshapeType(), ge::AscendString("NC"));
  EXPECT_EQ(param.GetValueDepend(), ge::AscendString("required"));
  std::vector<OpParamDef> outputs = opDef.GetMergeOutputs(aicConfig);
  EXPECT_EQ(outputs.size(), 1);
  OpParamDef paramOut = outputs[0];
  EXPECT_EQ(paramOut.GetParamType(), REQUIRED);
  EXPECT_EQ(paramOut.GetDataTypes()[0], ge::DT_FLOAT16);
  EXPECT_EQ(paramOut.GetFormats()[0], ge::FORMAT_ND);
  EXPECT_EQ(opDef.GetWorkspaceFlag(), true);
}

}  // namespace
}  // namespace ops