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

#include "external/hcom/hcom_topo_info.h"

#include "graph/debug/ge_log.h"
namespace ge {
Status HcomTopoInfo::SetGroupTopoInfo(const std::string &group, const HcomTopoInfo::TopoInfo &info) {
  if (!(rank_info_.emplace(group, info).second)) {
    REPORT_INNER_ERROR("E18888", "Group key [%s] has already been added.", group.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] group key [%s] has already been added.", group.c_str());
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}
Status HcomTopoInfo::GetGroupRankSize(const std::string &group, int64_t &rank_size) {
  const auto &iter_info = rank_info_.find(group);
  if (iter_info == rank_info_.end()) {
    REPORT_INNER_ERROR("E18888", "Group key [%s] has not been added, get failed.", group.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] group key [%s] has not been added, get failed.", group.c_str());
    return GRAPH_FAILED;
  }
  rank_size = iter_info->second.rank_size;
  return GRAPH_SUCCESS;
}
}