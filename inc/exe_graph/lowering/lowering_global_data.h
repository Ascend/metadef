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

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_LOWERING_GLOBAL_DATA_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_LOWERING_GLOBAL_DATA_H_
#include <map>
#include "proto/task.pb.h"
#include "value_holder.h"
#include "exe_graph/runtime/tensor.h"
#include "exe_graph/runtime/allocator.h"
#include "exe_graph/runtime/execute_graph_types.h"

namespace gert {
class LoweringGlobalData {
 public:
  struct NodeCompileResult {
    const std::vector<domi::TaskDef> &GetTaskDefs() const {
      return task_defs;
    }
    std::vector<domi::TaskDef> task_defs;
  };
  const bg::ValueHolderPtr &GetStream() const;
  LoweringGlobalData &SetStream(bg::ValueHolderPtr &&stream);
  LoweringGlobalData &SetStream(bg::ValueHolderPtr &&stream, ExecuteGraphType graph_type);

  const NodeCompileResult *FindCompiledResult(const ge::NodePtr &node) const;
  LoweringGlobalData &AddCompiledResult(const ge::NodePtr &node, NodeCompileResult compile_result);

  void *FindKnownSubgraphModel(const ge::NodePtr &node) const;
  LoweringGlobalData &AddKnownSubgraphModel(const ge::NodePtr &node, void *const model);
  void *GetGraphStaticCompiledModel(const std::string &graph_name) const;
  LoweringGlobalData &AddStaticCompiledGraphModel(const std::string &graph_name, void *const model);

  bg::ValueHolderPtr GetAllocator(AllocatorDesc desc) const;
  LoweringGlobalData &SetAllocator(AllocatorDesc desc, bg::ValueHolderPtr allocator);
  LoweringGlobalData &SetExternalAllocator(bg::ValueHolderPtr &&allocator);
  LoweringGlobalData &SetExternalAllocator(bg::ValueHolderPtr &&allocator, ExecuteGraphType graph_type);
  bg::ValueHolderPtr GetOrCreateAllocator(AllocatorDesc desc);

  uint64_t GetSessionId();
  bg::ValueHolderPtr GetOrCreateUniqueValueHolder(const std::string &name,
                                                  const std::function<bg::ValueHolderPtr()> &builder);
  bg::ValueHolderPtr GetUniqueValueHolder(const std::string &name) const;
  void SetUniqueValueHolder(const std::string &name, const bg::ValueHolderPtr &holder);

 private:
  struct HoldersByGraph {
    bg::ValueHolderPtr holders[static_cast<size_t>(ExecuteGraphType::kNum)];
  };

 private:
  bg::ValueHolderPtr stream_;  // to be deleted
  std::unordered_map<std::string, NodeCompileResult> node_name_to_compile_result_holders_;
  std::map<int64_t, void *> node_ids_to_known_subgraph_models_;
  std::map<std::string, void *> graph_to_static_models_;
  std::map<AllocatorDesc, bg::ValueHolderPtr> placements_to_allocator_;
  uint64_t session_id_ = std::numeric_limits<uint64_t>::max();
  std::map<std::string, bg::ValueHolderPtr> names_to_unique_value_holder_;
  HoldersByGraph streams_;
  HoldersByGraph external_allocators_;
};
}  // namespace gert
#endif  // AIR_CXX_RUNTIME_V2_LOWERING_LOWERING_GLOBAL_DATA_H_
