/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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
#ifndef INC_REGISTER_GRAPH_OPTIMIZER_GRAPH_FUSION_CONNECTION_MATRIX_H_
#define INC_REGISTER_GRAPH_OPTIMIZER_GRAPH_FUSION_CONNECTION_MATRIX_H_

#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "graph/graph.h"
#include "graph/compute_graph.h"
#include "common/large_bm.h"
#include "register/graph_optimizer/graph_optimize_register_error_codes.h"

namespace fe {
class ConnectionMatrix {
public:
#ifndef ONLY_COMPILE_OPEN_SRC
  ConnectionMatrix();
  ConnectionMatrix(bool enable_data_flow);
#endif

  explicit ConnectionMatrix(const ge::ComputeGraph &graph);

  ~ConnectionMatrix();

  bool IsConnected(const ge::NodePtr &a, const ge::NodePtr &b) const;

  // inputs are all input nodes of parameter node.
  // if there is a path between A->B, then B will own A's
  // connectivity. The reason is ---
  // If some node can reach A, than it can also reach B.
  void SetConnectivity(const ge::Node::Vistor<ge::NodePtr> &inputs, const ge::NodePtr &node);

#ifndef ONLY_COMPILE_OPEN_SRC
  bool IsDataConnected(const ge::NodePtr &a, const ge::NodePtr &b) const;
#endif

  /* Computes the connectivity between two nodes in the
   * computation. The returned ConnectivityMatrix is constructed such that
   * ConnectivityMatrix::IsConnected(a, b) returns true iff there exists a
   * directed path (from producer to consumer) from 'a' to 'b'. Both data
   * connection and control connection are considered for connectivity.
   * A node is connected to itself. */
#ifndef ONLY_COMPILE_OPEN_SRC
  void Generate(const ge::ComputeGraph &graph);
#else
  Status Generate(const ge::ComputeGraph &graph);
#endif

  // update reachablity map for fused nodes.
  void Update(const ge::ComputeGraph &graph, const std::vector<ge::NodePtr> &fusion_nodes);

  void BackupBitMap();

  void RestoreBitMap();

private:
  int64_t GetIndex(const ge::NodePtr &node) const;

  const ge::LargeBitmap &GetBitMap(const ge::NodePtr &node) const;

  ge::LargeBitmap &GetBitMap(const ge::NodePtr &node);

  ge::LargeBitmap &GetBitMap(uint64_t index);

#ifndef ONLY_COMPILE_OPEN_SRC
  const ge::LargeBitmap &GetDataBitMap(const ge::NodePtr &node) const;

  ge::LargeBitmap &GetDataBitMap(const ge::NodePtr &node);

  ge::LargeBitmap &GetDataBitMap(uint64_t index);

  void SetDataConnectivity(const ge::Node::Vistor<ge::NodePtr> &inputs, const ge::NodePtr &node);

  bool enable_data_flow_;
#endif
  size_t size_;
  std::vector<ge::LargeBitmap> bit_maps;
#ifndef ONLY_COMPILE_OPEN_SRC
  std::vector<ge::LargeBitmap> bit_maps_back_up_;
  std::vector<ge::LargeBitmap> data_bit_maps_;
  std::vector<ge::LargeBitmap> data_bit_maps_back_up_;
#endif
  std::map<std::string, int64_t> name_to_index_;
};
}
#endif  // INC_REGISTER_GRAPH_OPTIMIZER_GRAPH_FUSION_CONNECTION_MATRIX_H_
