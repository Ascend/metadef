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

#include <gtest/gtest.h>
#include "external/hcom/hcom_topo_info.h"
namespace ge {
class UtestHcomTopoInfo : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};
TEST_F(UtestHcomTopoInfo, SetGroupTopoInfo) {
  HcomTopoInfo::TopoInfo topo_info;
  topo_info.rank_size = 8;
  EXPECT_EQ(HcomTopoInfo::Instance().SetGroupTopoInfo("group0", topo_info), GRAPH_SUCCESS);
  // add repeated
  const std::string group = "group0";
  EXPECT_EQ(HcomTopoInfo::Instance().SetGroupTopoInfo(group.c_str(), topo_info), GRAPH_FAILED);
}

TEST_F(UtestHcomTopoInfo, GetAndUnsetGroupTopoInfo) {
  int64_t rank_size = -1;
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("group0", rank_size), GRAPH_SUCCESS);
  EXPECT_EQ(rank_size, 8);
  // not added, get failed
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("group1", rank_size), GRAPH_FAILED);
  HcomTopoInfo::Instance().UnsetGroupTopoInfo("group1");
  HcomTopoInfo::Instance().UnsetGroupTopoInfo("group0");
  // removed
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("group0", rank_size), GRAPH_FAILED);
  HcomTopoInfo::TopoInfo topo_info;
  topo_info.rank_size = 16;
  EXPECT_EQ(HcomTopoInfo::Instance().SetGroupTopoInfo("group0", topo_info), GRAPH_SUCCESS);
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("group0", rank_size), GRAPH_SUCCESS);
  EXPECT_EQ(rank_size, 16);
  EXPECT_EQ(HcomTopoInfo::Instance().SetGroupTopoInfo("group0", topo_info), GRAPH_FAILED);
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("", rank_size), GRAPH_FAILED);
}
}