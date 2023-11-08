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

#ifndef INC_GRAPH_EDGE_H
#define INC_GRAPH_EDGE_H

#include "graph/anchor.h"

namespace ge {
template <class T>
struct Edge {
  T *src = nullptr;                      // src node or output node
  T *dst = nullptr;                      // dst node or input node
  int32_t src_output = -1;               // the output index of output node
  int32_t dst_input = -1;                // the input index of input node
  int32_t in_edge_index = -1;            // the record index of input node, it used to quickly find the edge in node.
  int32_t out_edge_index = -1;           // the record index of output node, it used to quickly find the edge in node.
  std::weak_ptr<Anchor> src_anchor_ptr;  // the reserved information.
  std::weak_ptr<Anchor> dst_anchor_ptr;  // the reserved information.
};

enum class DirectionType {
  kDirectionInType,
  kDirectionOutType,
};

constexpr int32_t kControlEdgeIndex = -1;

}  // namespace ge
#endif  // INC_GRAPH_EDGE_H
