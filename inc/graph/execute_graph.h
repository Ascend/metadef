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

#ifndef INC_GRAPH_EXECUTE_GRAPH_H
#define INC_GRAPH_EXECUTE_GRAPH_H

#include <deque>
#include <queue>
#include <unordered_map>
#include "graph/fast_node.h"

namespace ge {
template <class T, class G>
class FastGraphImpl;

class ExecuteGraph : public AttrHolder {
 public:
  struct SubGraphInfo {
    std::shared_ptr<ExecuteGraph> sub_graph;
    void *quick_graph;
  };
  explicit ExecuteGraph(const std::string &name);
  ~ExecuteGraph();

  /**
   * The function is shallow copy for ExecuteGraph
   */
  ExecuteGraph &operator=(ge::ExecuteGraph &exec_graph);

  /**
   * The function is deep copy for ExecuteGraph
   */
  ExecuteGraph &CompleteCopy(ge::ExecuteGraph &exec_graph);

  /**
   * The function is used to add node of graph.
   * The node push back to container in graph.
   */
  FastNode *AddNode(const OpDescPtr &op);
  FastNode *AddNode(FastNode *FastNode);
  FastNode *AddNode(const OpDescPtr &op, int64_t id);

  /**
   * The function is used to add node of graph.
   * The node push front to container in graph.
   */
  FastNode *AddNodeFront(const OpDescPtr &op);
  FastNode *AddNodeFront(FastNode *quick_node);

  /**
   * The function is used to remove node of graph.
   * The node don`t release and it will push to free container which is used to store free obj.
   */
  graphStatus RemoveJustNode(FastNode *node_ptr);

  /**
   * The function is used to add input node of graph.
   */
  FastNode *AddInputNode(FastNode *fast_node);

  /**
   * The function is used to remove input node of graph.
   */
  graphStatus RemoveInputNode(FastNode *node);

  /**
   * The function is used to add output node of graph.
   */
  FastNode *AddOutputNodeByIndex(FastNode *node, const int32_t index);

  /**
   * The function is used to remove output node of graph.
   */
  graphStatus RemoveOutputNode(FastNode *node);

  /**
   * The function is used to add edge of graph.
   */
  FastEdge *AddEdge(FastNode *src, int32_t src_index, FastNode *dst, int32_t dst_index);

  /**
   * The function is used to remove edge of graph.
   * The edge don`t release and it will push to free container which is used to store free obj.
   */
  graphStatus RemoveEdge(FastEdge *e);

  const FastNode *GetParentNodeBarePtr() const;
  void SetParentNode(FastNode *node);

  /**
   * The function is used to directly add subgraph of graph without any check.
   * The shared pointer of subgraph will record in graph.
   */
  ExecuteGraph *AddSubGraph(std::shared_ptr<ExecuteGraph> &sub_graph);

  /**
   * The function will add subgraph After strict checking the valid of subgraph.
   * The shared pointer of subgraph will record in graph.
   */
  ExecuteGraph *AddSubGraph(std::shared_ptr<ExecuteGraph> &sub_graph, std::string &name);

  /**
   * The function is used to remove subgraph of graph.
   * The shared pointer of subgraph will clear in graph.
   */
  graphStatus RemoveSubGraph(ExecuteGraph *sub_graph);
  graphStatus RemoveSubGraph(const std::string &name);

  /**
   * The function is used to get subgraph with name.
   */
  ExecuteGraph *GetSubGraph(const std::string &name) const;

  /**
   * remove all subgraph from parent graph.
   */
  void ClearAllSubGraph();

  /**
   * get the number of direct nodes form graph.
   */
  size_t GetDirectNodesSize() const;

  /**
   * get direct nodes from graph (it is convert to vector which is long time).
   * external modifications don`t affect internal nodes.
   */
  std::vector<FastNode *> GetDirectNode() const;

  /**
   * get all edges from graph (it is convert to vector which is long time).
   * external modifications don`t affect internal edges.
   */
  std::vector<FastEdge *> GetAllEdges() const;

  /**
   * get all sub graph from graph (it is convert to vector which is long time).
   * external modifications don`t affect internal edges.
   */
  std::vector<ExecuteGraph *> GetAllSubgraphs() const;

  /**
   * find the node with node token in the graph.
   */
  const FastNode *FindNode(size_t token) const;

  /**
   * is is topo sort (include dfs, bfs, DFS_POSTORDER).
   */
  graphStatus TopologicalSortingGraph(const ExecuteGraph *compute_graph, const bool dfs_reverse);

  /**
   * get name of graph.
   */
  std::string GetName() const;

  /**
   * set name of graph.
   */
  void SetName(const std::string &name);

  void SetParentGraph(ExecuteGraph *parent_graph);

  const ExecuteGraph *GetParentGraphBarePtr(void) const;

  /**
   * topo sort in the graph (include sub graph).
   */
  graphStatus TopologicalSorting();

  /**
   * push edge to free edge.
   */
  graphStatus RecycleQuickEdge(FastEdge *fast_edge);

  /**
   * push node to free edge.
   */
  graphStatus RecycleQuickNode(FastNode *node_ptr);

  /**
   * get all of nodes in graph (include subgraph).
   */
  std::vector<FastNode *> GetAllNodes() const;

  /**
   * It is used to set input order which is used in topo sorting
   */
  void SetInputsOrder(const std::vector<std::string> &inputs_order);

  void ReorderByNodeId();

  void SetGraphId(size_t graph_id);

  size_t GetGraphId() const;

 protected:
  ProtoAttrMap &MutableAttrMap() override;
  ConstProtoAttrMap &GetAttrMap() const override;

 private:
  std::vector<FastNode *> AllGraphNodes(std::vector<ExecuteGraph *> &subgraphs) const;
  void GetAllNodesFromOpdesc(std::vector<ExecuteGraph *> &subgraphs, const OpDesc &op_desc,
                             std::deque<FastNode *> &candidates) const;
  void RemoveNodeFromNodesFree(FastNode *fast_node);
  graphStatus SortNodes(std::vector<FastNode *> &stack, std::map<FastNode *, uint32_t> &map_in_edge_num) const;
  void GetOutNodesFromEdgesToMap(std::map<FastNode *, uint32_t> &map_in_edge_num, FastNode *node,
                                 std::map<std::string, FastNode *> &breadth_node_map) const;
  graphStatus CollectBreadthOutNode(FastNode *&node, std::map<FastNode *, uint32_t> &map_in_edge_num,
                                    std::map<std::string, FastNode *> &breadth_node_map) const;
  graphStatus BFSTopologicalSorting(std::vector<FastNode *> &node_vec, const bool reverse,
                                    const ExecuteGraph *compute_graph) const;
  graphStatus DFSTopologicalSorting(std::vector<FastNode *> &node_vec, const bool reverse,
                                    const ExecuteGraph *compute_graph) const;
  graphStatus RDFSTopologicalSorting(std::vector<FastNode *> &node_vec, const bool reverse,
                                     const ExecuteGraph *compute_graph) const;
  void GetInNodes(FastNode *current, std::vector<FastNode *> &input_nodes) const;

 private:
  std::shared_ptr<FastGraphImpl<FastNode, ExecuteGraph>> graph_shared_;
  std::unordered_map<std::string, SubGraphInfo> names_to_subgraph_;
  std::vector<std::string> inputs_order_;
  AttrStore attrs_;
};

}  // namespace ge
#endif  // INC_GRAPH_EXECUTE_GRAPH_H
