
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "register/op_def_registry.h"

namespace ops {

namespace {

class OpDefAICoreUT : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(OpDefAICoreUT, AICoreTest) {
  OpAICoreDef aicoreDef;
  OpAICoreConfig config;
  config.AsyncFlag(true)
      .DynamicCompileStaticFlag(true)
      .DynamicFormatFlag(true)
      .DynamicRankSupportFlag(true)
      .DynamicShapeSupportFlag(true)
      .HeavyOpFlag(true)
      .NeedCheckSupportFlag(true)
      .OpPattern("reduce")
      .PrecisionReduceFlag(true)
      .RangeLimitValue("limited")
      .SlicePatternValue("broadcast");
  std::map<ge::AscendString, ge::AscendString> cfgs = config.GetCfgInfo();
  EXPECT_EQ(cfgs["async.flag"], "true");
  EXPECT_EQ(cfgs["dynamicCompileStatic.flag"], "true");
  EXPECT_EQ(cfgs["dynamicFormat.flag"], "true");
  EXPECT_EQ(cfgs["dynamicRankSupport.flag"], "true");
  EXPECT_EQ(cfgs["dynamicShapeSupport.flag"], "true");
  EXPECT_EQ(cfgs["heavyOp.flag"], "true");
  EXPECT_EQ(cfgs["needCheckSupport.flag"], "true");
  EXPECT_EQ(cfgs["op.pattern"], "reduce");
  EXPECT_EQ(cfgs["precision_reduce.flag"], "true");
  EXPECT_EQ(cfgs["rangeLimit.value"], "limited");
  EXPECT_EQ(cfgs["slicePattern.value"], "broadcast");
  config.AsyncFlag(false)
      .DynamicCompileStaticFlag(false)
      .DynamicFormatFlag(false)
      .DynamicRankSupportFlag(false)
      .DynamicShapeSupportFlag(false)
      .HeavyOpFlag(false)
      .NeedCheckSupportFlag(false)
      .OpPattern("reduced")
      .PrecisionReduceFlag(false)
      .RangeLimitValue("limiteded")
      .SlicePatternValue("broadcasted");
  cfgs = config.GetCfgInfo();
  EXPECT_EQ(cfgs["async.flag"], "false");
  EXPECT_EQ(cfgs["dynamicCompileStatic.flag"], "false");
  EXPECT_EQ(cfgs["dynamicFormat.flag"], "false");
  EXPECT_EQ(cfgs["dynamicRankSupport.flag"], "false");
  EXPECT_EQ(cfgs["dynamicShapeSupport.flag"], "false");
  EXPECT_EQ(cfgs["heavyOp.flag"], "false");
  EXPECT_EQ(cfgs["needCheckSupport.flag"], "false");
  EXPECT_EQ(cfgs["op.pattern"], "reduced");
  EXPECT_EQ(cfgs["precision_reduce.flag"], "false");
  EXPECT_EQ(cfgs["rangeLimit.value"], "limiteded");
  EXPECT_EQ(cfgs["slicePattern.value"], "broadcasted");
  aicoreDef.AddConfig("ascend310p", config);
  aicoreDef.AddConfig("ascend910", config);
  aicoreDef.AddConfig("ascend310p", config);
  aicoreDef.OpCheckPost("Test");
  std::map<ge::AscendString, OpAICoreConfig> aicfgs = aicoreDef.GetAICoreConfigs();
  EXPECT_TRUE(aicfgs.find("ascend310p") != aicfgs.end());
  EXPECT_EQ(aicfgs.size(), 2);
  aicoreDef.AddConfig("ascend310p");
  aicfgs = aicoreDef.GetAICoreConfigs();
  config = aicfgs["ascend310p"];
  cfgs = config.GetCfgInfo();
  EXPECT_EQ(cfgs["dynamicCompileStatic.flag"], "true");
  EXPECT_EQ(cfgs["dynamicFormat.flag"], "true");
  EXPECT_EQ(cfgs["dynamicRankSupport.flag"], "true");
  EXPECT_EQ(cfgs["dynamicShapeSupport.flag"], "true");
  EXPECT_EQ(cfgs["needCheckSupport.flag"], "false");
  EXPECT_EQ(cfgs["precision_reduce.flag"], "true");
  EXPECT_EQ(cfgs["rangeLimit.value"], "limited");
}

}  // namespace
}  // namespace ops
