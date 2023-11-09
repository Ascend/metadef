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

#ifndef D_INC_GRAPH_FAST_NODE_H
#define D_INC_GRAPH_FAST_NODE_H

#include <map>
#include <mutex>
#include <set>
#include <vector>
#include "graph/types.h"
#include "graph/op_desc.h"
#include "graph/edge.h"

namespace ge {
struct OutDataEdgeStatisticsInfo {
  size_t total_num = 0UL;             // The total number edge of all inputs or all outputs
  std::vector<size_t> per_edges_num;  // The number of one input or one output.
};

class ExecuteGraph;
class FastNode;
using FastEdge = Edge<FastNode>;

class ExtendInfo {
 public:
  virtual ~ExtendInfo() {}
  /**
   * set the input index of graph.
   */
  void SetInputIndex(int32_t idx);

  /**
   * get the input index of graph.
   * Don`t use this function unless it is explicitly required.
   */
  int32_t GetInputIndex() const;

  /**
   * add the output index of graph.
   * Don`t use this function unless it is explicitly required.
   */
  void AddOneOutputIndex(int32_t idx);

  /**
   * get the output index of graph.
   * Don`t use this function unless it is explicitly required.
   */
  std::vector<int32_t> &GetOutputIndex();

  /**
   * get the owner graph of node.
   */
  ExecuteGraph *GetOwnerGraphBarePtr() const;

  /**
   * set the owner graph of node.
   */
  graphStatus SetOwnerGraph(ExecuteGraph *graph, FastNode *fast_node);

  /**
   * check the extend information is same with r_info.
   */
  bool operator==(const ExtendInfo &r_info) const;

  /**
   * get the flag of host node.
   */
  bool GetHostNode() const;

  /**
   * set the flag of host node.
   */
  void SetHostNode(const bool is_host);

 private:
  ExecuteGraph *execute_graph_ = nullptr;
  std::vector<int32_t> output_index_;
  int32_t input_index_ = kControlEdgeIndex;
  bool host_node_ = false;
};

class FastNode {
 public:
  /**
   * construct a fastnode.
   * please call Init after construction
   */
  FastNode();

  ~FastNode();
  /**
   * The function is used to init node with opdesc.
   */
  graphStatus Init(const OpDescPtr &op);

  /**
   * get the bare pointer of op desc.
   */
  OpDesc *GetOpDescBarePtr() const;

  /**
   * get the shared pointer of op desc.
   */
  OpDescPtr GetOpDescPtr() const;

  /**
   * get the type of node.
   */
  std::string GetType() const;

  /**
   * get the type of node.
   */
  const char *GetTypePtr() const;

  /**
   * get the name of node.
   */
  std::string GetName() const;

  /**
   * get the name of node.
   */
  const char *GetNamePtr() const;

  /**
   * record the edge info to node.
   * The funcion is not recommended, please used the AddEdge funcion istead.
   */
  graphStatus RecordEdge(Edge<FastNode> *edge, DirectionType type);

  /**
   * clear the edge info to node.
   * The funcion is not recommended, please used the RemoveEdge funcion istead.
   */
  graphStatus EraseEdge(Edge<FastNode> *edge, DirectionType type);

  /**
   * adjust the position of the edge in the node record.
   */
  graphStatus MoveEdge(DirectionType type, int32_t idx, int32_t cur_index, int32_t replace_index);

  /**
   * get a unique identifier of node.
   * please notes the unique identifier is in the graph.
   */
  size_t GetNodeToken() const;

  /**
   * get the number of input data in the node.
   */
  size_t GetDataInNum() const;

  /**
   * get the number of output data in the node.
   */
  size_t GetDataOutNum() const;

  /**
   * update the number of data input in the node.
   */
  void UpdateDataInNum(size_t new_num);

  /**
   * update the number of data ouput in the node.
   */
  void UpdateDataOutNum(size_t new_num);

  /**
   * get the total number of output edges from the node.
   */
  size_t GetAllOutEdgesSize() const;
  size_t GetAllOutDataEdgesSize() const;
  size_t GetAllOutControlEdgesSize() const;

  /**
   * get the total number of input edges from the node.
   */
  size_t GetAllInDataEdgesSize() const;
  size_t GetAllInControlEdgesSize() const;

  /**
   * check the node is same with r_node.
   */
  bool operator==(const FastNode &r_node) const;

  /**
   * get the first peer node which is the output data to this.
   * the idx is the input index.
   */
  FastEdge *GetPeerOutDataEdge(int32_t idx) const;

  /**
   * get the total number of in edge from the node.
   * the number include data edge and control edge.
   */
  size_t GetAllInEdgeSize() const;

  /**
   * collecting all input edge.
   * please check the item, the item from vector may be nullptr.
   * if the item is nullptr, it just continue to get next, no error handing is required.
   */
  const std::vector<Edge<FastNode> *> &GetAllInDataEdgesRef() const;
  const std::vector<Edge<FastNode> *> &GetAllInControlEdgesRef() const;

  /**
   * collecting all output edge.
   * please check the item, the item from vector may be nullptr.
   * if the item is nullptr, it just continue to get next, no error handing is required.
   */
  const std::vector<Edge<FastNode> *> &GetAllOutControlEdgesRef() const;
  const std::vector<std::vector<Edge<FastNode> *>> &GetAllOutDataEdgesRef() const;

  /**
   * collecting all output or input edge.
   * it already filter the nullptr item.
   */
  std::vector<Edge<FastNode> *> GetAllOutDataEdges() const;
  std::vector<Edge<FastNode> *> GetAllOutControlEdges() const;
  std::vector<Edge<FastNode> *> GetAllInDataEdges() const;
  std::vector<Edge<FastNode> *> GetAllInControlEdges() const;

  /**
   * collecting the peer nodes which is input data edge.
   */
  std::vector<FastNode *> GetPeerNodesInDataEdge(const int32_t idx) const;

  /**
   * collecting the peer nodes which is input control edge.
   */
  std::vector<FastNode *> GetPeerNodesInControlEdge(const int32_t idx) const;

  /**
   * collecting the peer nodes which input information from this.
   */
  std::vector<FastNode *> GetAllOutNodes() const;

  /**
   * Check the number of out edge is zero.
   */
  bool OutNodesIsEmpty() const;

  /**
   * Set the relative node information.
   * Don`t use this function unless it is explicitly required.
   */
  void SetNodePtr(const std::shared_ptr<Node> &node);

  /**
   * clear the relative node information.
   * Don`t use this function unless it is explicitly required.
   */
  void ClearNodePtr();
  void ClearNodeBarePtr();

  /**
   * get the relative node information.
   * Don`t use this function unless it is explicitly required.
   */
  std::shared_ptr<Node> GetNodePtr() const;
  const std::shared_ptr<Node> &GetNodePtrRef() const;
  Node *GetNodeBarePtr() const;

  /**
   * get peer node which input control information from this.
   */
  std::vector<FastNode *> GetOutControlNodes() const;

  /**
   * get peer node which output control information to this.
   */
  std::vector<FastNode *> GetInControlNodes() const;

  /**
   * get the total number of edge with input index or output index.
   */
  size_t GetAllPeerEdgesSizeByIndex(DirectionType type, int32_t idx_) const;

  /**
   * get the total number of edge with input index.
   */
  size_t GetInEdgesSizeByIndex(int32_t idx) const;

  /**
   * get the total number of edge with output index.
   */
  size_t GetOutEdgesSizeByIndex(int32_t idx) const;

  /**
   * collecting input data edge with input index.
   * please check the item, the item from vector may be nullptr.
   * if the item is nullptr, it just continue to get next, no error handing is required.
   */
  Edge<FastNode> *GetInDataEdgesByIndex(int32_t idx) const;

  /**
   * collecting input control edges with input index.
   * please check the item, the item from vector may be nullptr.
   * if the item is nullptr, it just continue to get next, no error handing is required.
   */
  std::vector<Edge<FastNode> *> GetInControlEdgesByIndex() const;
  const std::vector<Edge<FastNode> *> &GetInControlEdgesRefByIndex() const;

  /**
   * collecting all output edge with output index.
   * please check the item, the item from vector may be nullptr.
   * if the item is nullptr, it just continue to get next, no error handing is required.
   */
  std::vector<Edge<FastNode> *> GetOutEdgesByIndex(int32_t idx) const;
  const std::vector<Edge<FastNode> *> &GetOutEdgesRefByIndex(int32_t idx) const;

  /**
   * remove all of edge in the node.
   * please define remove_edge_func to delete edge.
   * The general process is as follow:
   *    1. clear the edge information in src node and dst node (use EraseEdge);
   *    2. remove edge in container.
   *    3. add the free edge to free container.
   * example:
   *   node[1]->RemoveAllEdge([&compute_graph](FastEdge *e) {
   *     auto src_node = e->src;
   *     auto dst_node = e->dst;
   *     Utils::GetNode(src_node).EraseEdge(e, DirectionType::kDirectionOutType);
   *     Utils::GetNode(dst_node).EraseEdge(e, DirectionType::kDirectionInType);
   *     if (Utils::GetListElementAddr(e)->owner != nullptr) {
   *       Utils::GetListElementAddr(e)->owner->erase(Utils::GetListElementAddr(e));
   *     }
   *     auto ret = compute_graph->RecycleQuickEdge(e);
   *     if ((ret != GRAPH_SUCCESS) && (e != nullptr)) {
   *       delete e;
   *     }
   *   });
   */
  void RemoveAllEdge(std::function<void(Edge<FastNode> *)> const &remove_edge_func);

  /**
   * get the extend info of node.
   */
  ExtendInfo *GetExtendInfo() const;

  /**
   * get the numbers input edges which peer node is not NEXTITERATION or REFNEXTITERATION.
   */
  size_t GetInEdgeSize() const;

 private:
  graphStatus CheckAllInputParamter(DirectionType type, int32_t idx, int32_t cur_index, int32_t replace_index) const;
  inline bool CheckDataIndexIsValid(int32_t index, DirectionType type) const;
  graphStatus Reset();
  void UpdateDataForIoNumChange();
  graphStatus RecordInControlEdge(FastEdge *edge);
  graphStatus RecordOutControlEdge(FastEdge *edge);
  graphStatus RecordInDataEdge(FastEdge *edge, int32_t index);
  graphStatus RecordOutDataEdge(FastEdge *edge, int32_t index);
  graphStatus EraseInControlEdge(FastEdge *edge);
  graphStatus EraseOutControlEdge(FastEdge *edge);
  graphStatus EraseInDataEdge(int32_t index);
  graphStatus EraseOutDataEdge(FastEdge *edge, int32_t index);
  graphStatus ModifySizeByNodeType(FastEdge *fast_edge, size_t &in_edge_size) const;

 private:
  std::string name_;
  size_t node_token_ = 0UL;
  OpDescPtr opdesc_ = nullptr;
  std::shared_ptr<Node> self_ptr_ = nullptr;
  Node *node_ptr_ = nullptr;

  size_t data_in_num_ = 0UL;
  size_t data_out_num_ = 0UL;

  std::vector<Edge<FastNode> *> in_data_edges_;
  std::vector<Edge<FastNode> *> in_control_edges_;
  std::vector<Edge<FastNode> *> out_control_edges_;
  std::vector<std::vector<Edge<FastNode> *>> out_data_edges_;

  size_t in_data_edges_count_ = 0UL;
  size_t in_control_edge_count_ = 0UL;
  size_t out_control_edges_count_ = 0UL;
  OutDataEdgeStatisticsInfo out_data_edges_info_;

  std::unique_ptr<ExtendInfo> extend_info_ = nullptr;
};

}  // namespace ge
#endif  // D_INC_GRAPH_FAST_NODE_H
