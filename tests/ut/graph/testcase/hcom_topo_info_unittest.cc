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
  const std::string group = "group0";

  // add invalid
  EXPECT_EQ(HcomTopoInfo::Instance().SetGroupTopoInfo(nullptr, topo_info), GRAPH_FAILED);

  EXPECT_EQ(HcomTopoInfo::Instance().SetGroupTopoInfo(group.c_str(), topo_info), GRAPH_SUCCESS);
  // add repeated, over write
  topo_info.notify_handle = reinterpret_cast<void *>(0x8000);
  EXPECT_EQ(HcomTopoInfo::Instance().SetGroupTopoInfo(group.c_str(), topo_info), GRAPH_SUCCESS);
  void *handle = nullptr;
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupNotifyHandle(group.c_str(), handle), GRAPH_SUCCESS);
  EXPECT_EQ(handle, reinterpret_cast<void *>(0x8000));
  HcomTopoInfo::TopoInfo topo_info_existed;
  EXPECT_TRUE(HcomTopoInfo::Instance().TryGetGroupTopoInfo(group.c_str(), topo_info_existed));
  EXPECT_TRUE(HcomTopoInfo::Instance().TopoInfoHasBeenSet(group.c_str()));
  EXPECT_EQ(topo_info_existed.notify_handle, reinterpret_cast<void *>(0x8000));
  EXPECT_EQ(topo_info_existed.rank_size, 8);
}

TEST_F(UtestHcomTopoInfo, GetAndUnsetGroupTopoInfo) {
  int64_t rank_size = -1;
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("group0", rank_size), GRAPH_SUCCESS);
  EXPECT_EQ(rank_size, 8);
  // not added, get failed
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("group1", rank_size), GRAPH_FAILED);
  void *handle = nullptr;
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupNotifyHandle("group1", handle), GRAPH_FAILED);
  HcomTopoInfo::Instance().UnsetGroupTopoInfo("group1");
  HcomTopoInfo::Instance().UnsetGroupTopoInfo("group0");
  // removed
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("group0", rank_size), GRAPH_FAILED);
  HcomTopoInfo::TopoInfo topo_info;
  topo_info.rank_size = 16;
  EXPECT_EQ(HcomTopoInfo::Instance().SetGroupTopoInfo("group0", topo_info), GRAPH_SUCCESS);
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("group0", rank_size), GRAPH_SUCCESS);
  EXPECT_EQ(rank_size, 16);
  EXPECT_EQ(HcomTopoInfo::Instance().GetGroupRankSize("", rank_size), GRAPH_FAILED);
}
}