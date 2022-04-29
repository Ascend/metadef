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

#include "register/graph_optimizer/fusion_common/accelerator_utils.h"
#include "graph/debug/ge_op_types.h"
#include "graph/anchor.h"
#include "graph/utils/node_utils.h"

namespace fe {
const std::array<size_t, static_cast<size_t>(ge::DT_MAX + 1)> data_type_size = {
    4, // DT_FLOAT = 0,
    2, // DT_FLOAT16 = 1,
    1, // DT_INT8 = 2,
    4, // DT_INT32 = 3,
    1, // DT_UINT8 = 4,
    1, // DT_xxxx = 5,
    2, // DT_INT16 = 6,
    2, // DT_UINT16 = 7,
    4, // DT_UINT32 = 8,
    8, // DT_INT64 = 9,
    8, // DT_UINT64 = 10,
    8, // DT_DOUBLE = 11,
    1, // DT_BOOL = 12,
    8, // DT_STRING = 13,
    1, // DT_DUAL_SUB_INT8 = 14,
    1, // DT_DUAL_SUB_UINT8 = 15,
    8, // DT_COMPLEX64 = 16,
    16, // DT_COMPLEX128 = 17,
    1, // DT_QINT8 = 18,
    2, // DT_QINT16 = 19,
    4, // DT_QINT32 = 20,
    1, // DT_QUINT8 = 21,
    2, // DT_QUINT16 = 22,
    1, // DT_RESOURCE = 23,
    1, // DT_STRING_REF = 24,
    1, // DT_DUAL = 25,
    1, // DT_VARIANT = 26,
    2, // DT_BF16 = 27,
    1, // DT_UNDEFINED = 28,
    1, // DT_INT4 = 29,
    1, // DT_UINT1 = 30,
    1, // DT_INT2 = 31,
    4, // DT_UINT2 = 32,
    0, // DT_MAX = 33
};

ge::NodePtr AcceleratorUtils::GetConstInput(const ge::NodePtr &node, int32_t index) {
  ge::NodePtr ret = nullptr;

  auto in_anchor = node->GetInDataAnchor(index);

  const auto out_anchor = in_anchor->GetPeerOutAnchor();
  if (out_anchor == nullptr) {
    return nullptr;
  }

  const auto in_node = out_anchor->GetOwnerNode();
  if (in_node->GetType() == ge::CONSTANT) {
    ret = in_node;
  } else if (in_node->GetType() == ge::DATA) {
    const auto parent = ge::NodeUtils::GetParentInput(in_node);
    if ((parent != nullptr) && (parent->GetType() == ge::CONSTANT)) {
      ret = parent;
    }
  } else {
    // do nothing
  }

  return ret;
}

Relations::Relations() {}

Relations::Relations(const std::initializer_list<PeerIndex> &peer_indices) {
  int32_t index = 0;
  for (const auto &peer_index : peer_indices) {
    PeerIndices temp = {peer_index};
    relations.emplace(std::make_pair(index, temp));
  }
}

Relations::Relations(const std::map<ThisIndex, PeerIndices> &relations_param) {
  relations = relations_param;
}

Relations::Relations(std::map<ThisIndex, PeerIndices> &&relations_param) {
  relations = std::move(relations_param);
}

Relations::Relations(const Relations &relations_param) {
  relations = relations_param.relations;
  relations_by_name = relations_param.relations_by_name;
}

Relations::Relations(Relations &&relations_param) noexcept {
  relations = std::move(relations_param.relations);
  relations_by_name = std::move(relations_param.relations_by_name);
}

Relations::Relations(const std::map<std::string, PeerIndices> &relations_param) {
  relations_by_name = relations_param;
}

Relations::Relations(std::map<std::string, PeerIndices> &&relations_param) {
  relations_by_name = std::move(relations_param);
}

Relations::Relations(ThisIndex this_index, const PeerIndex &peer_index) {
  AddPeerIndex(this_index, peer_index);
}

Relations::Relations(ThisIndex this_index, PeerIndex &&peer_index) {
  AddPeerIndex(this_index, std::move(peer_index));
}

Relations::Relations(
    const std::initializer_list<std::pair<ThisIndex, PeerIndex>> &peer_indices) {
  for (const auto &index_pair: peer_indices) {
    AddPeerIndex(index_pair.first, index_pair.second);
  }
}

Relations::Relations(
    const std::initializer_list<std::pair<ThisIndex, std::initializer_list<PeerIndex>>> &peer_indices_vec) {
  for (const auto &peer_indices: peer_indices_vec) {
    AddPeerIndex(peer_indices.first, peer_indices.second);
  }
}

Relations& Relations::AddPeerIndex(ThisIndex this_index, const PeerIndex &peer_index) {
  const auto iter = relations.find(this_index);
  if (iter == relations.end()) {
    PeerIndices temp = {peer_index};
    relations.emplace(std::make_pair(this_index, temp));
  } else {
    iter->second.emplace_back(peer_index);
  }
  return *this;
}

Relations& Relations::AddPeerIndex(ThisIndex this_index, PeerIndex &&peer_index) {
  const auto iter = relations.find(this_index);
  if (iter == relations.end()) {
    PeerIndices temp = {std::move(peer_index)};
    relations.emplace(std::make_pair(this_index, std::move(temp)));
  } else {
    iter->second.emplace_back(std::move(peer_index));
  }
  return *this;
}

Relations& Relations::AddPeerIndex(ThisIndex this_index, const std::initializer_list<PeerIndex> &peer_indices) {
  const auto iter = relations.find(this_index);
  if (iter == relations.end()) {
    relations.emplace(std::make_pair(this_index, peer_indices));
  } else {
    for (const auto &peer_index : peer_indices) {
      iter->second.emplace_back(peer_index);
    }
  }
  return *this;
}

Relations& Relations::AddPeerIndex(const std::string &this_name, const PeerIndex &peer_index) {
  const auto iter = relations_by_name.find(this_name);
  if (iter == relations_by_name.end()) {
    PeerIndices temp = {peer_index};
    relations_by_name.emplace(std::make_pair(this_name, temp));
  } else {
    iter->second.emplace_back(peer_index);
  }
  return *this;
}

Relations& Relations::AddPeerIndex(const std::string &this_name, PeerIndex &&peer_index) {
  const auto iter = relations_by_name.find(this_name);
  if (iter == relations_by_name.end()) {
    PeerIndices temp = {std::move(peer_index)};
    relations_by_name.emplace(std::make_pair(this_name, std::move(temp)));
  } else {
    iter->second.emplace_back(std::move(peer_index));
  }
  return *this;
}

Relations& Relations::AddPeerIndex(const std::string &this_name,
                                   const std::initializer_list<PeerIndex> &peer_indices) {
  const auto iter = relations_by_name.find(this_name);
  if (iter == relations_by_name.end()) {
    relations_by_name.emplace(std::make_pair(this_name, peer_indices));
  } else {
    for (const auto &peer_index : peer_indices) {
      iter->second.emplace_back(peer_index);
    }
  }
  return *this;
}
}