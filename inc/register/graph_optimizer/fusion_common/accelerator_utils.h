/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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
#ifndef INC_REGISTER_GRAPH_OPTIMIZER_FUSION_COMMON_ACCELERATOR_UTILS_H
#define INC_REGISTER_GRAPH_OPTIMIZER_FUSION_COMMON_ACCELERATOR_UTILS_H
#include "graph/utils/op_desc_utils.h"
#include "graph/debug/ge_log.h"

#define ACCLRT_NOTNULL(val, ret)                       \
  do {                                                  \
    if ((val) == nullptr) {                             \
      GELOGD("Parameter[%s] must not be null.", #val); \
      return ret;                         \
    }                                                   \
  } while (0)

namespace fe {
using PeerIndex = std::pair<ge::NodePtr, int32_t>;
using PeerIndices = std::vector<PeerIndex>;
using ThisIndex = int32_t;
struct Relations {
  /* 如果key是输出的index，那么vector里存放的就是对端输入的index；在单输出多引用场景，
   * 对端输入的index可能有多个。
   * 如果key是输入的index，那么vector里存放的就是对端输出的index。对端输出只会有一个。 */
  std::map<ThisIndex, PeerIndices> relations;

  std::map<std::string, PeerIndices> relations_by_name;
  Relations();

  Relations(const std::initializer_list<PeerIndex> &peer_output_relations);

  explicit Relations(const std::map<ThisIndex, PeerIndices> &relations_param);

  explicit Relations(std::map<ThisIndex, PeerIndices> &&relations_param);

  Relations(const Relations &relations_param);

  Relations(Relations &&relations_param) noexcept;

  explicit Relations(const std::map<std::string, PeerIndices> &relations_param);

  explicit Relations(std::map<std::string, PeerIndices> &&relations_param);

  Relations(ThisIndex this_index, const PeerIndex &peer_index);

  Relations(ThisIndex this_index, PeerIndex &&peer_index);

  Relations(const std::initializer_list<std::pair<ThisIndex, PeerIndex>> &peer_indices);

  Relations(const std::initializer_list<std::pair<ThisIndex, std::initializer_list<PeerIndex>>> &peer_indices_vec);

  Relations& AddPeerIndex(ThisIndex this_index, const PeerIndex &peer_index);

  Relations& AddPeerIndex(ThisIndex this_index, const std::initializer_list<PeerIndex> &peer_indices);

  Relations& AddPeerIndex(const std::string &this_name, const PeerIndex &peer_index);

  Relations& AddPeerIndex(const std::string &this_name, const std::initializer_list<PeerIndex> &peer_indices);

  Relations& AddPeerIndex(ThisIndex this_index, PeerIndex &&peer_index);

  Relations& AddPeerIndex(const std::string &this_name, PeerIndex &&peer_index);
};

extern const std::array<size_t, static_cast<size_t>(ge::DT_MAX + 1)> data_type_size;
class AcceleratorUtils {
 public:
  static ge::NodePtr GetConstInput(const ge::NodePtr &node, int32_t index);
};
}
#endif // INC_REGISTER_GRAPH_OPTIMIZER_FUSION_COMMON_ACCELERATOR_UTILS_H
