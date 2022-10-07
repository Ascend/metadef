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

#include "exe_graph/lowering/lowering_global_data.h"
#include <memory>
#include "common/checker.h"
#include "graph/debug/ge_log.h"
#include "exe_graph/lowering/frame_selector.h"
namespace gert {
namespace {
bool CurrentOnInitGraph() {
  ge::NodePtr subgraph_node = nullptr;
  auto current_graph = bg::ValueHolder::GetCurrentGraph();
  while (current_graph != nullptr && current_graph->GetParentNode() != nullptr) {
    subgraph_node = current_graph->GetParentNode();
    current_graph = subgraph_node->GetOwnerComputeGraph().get();
  }

  if (subgraph_node != nullptr) {
    return strcmp(subgraph_node->GetType().c_str(), GetExecuteGraphTypeStr(ExecuteGraphType::kInit)) == 0;
  } else {
    return false;
  }
}
}  // namespace
const bg::ValueHolderPtr &LoweringGlobalData::GetStream() const {
  ExecuteGraphType graph_type = ExecuteGraphType::kMain;
  if (CurrentOnInitGraph()) {
    graph_type = ExecuteGraphType::kInit;
  }
  return streams_.holders[static_cast<size_t>(graph_type)];
}
LoweringGlobalData &LoweringGlobalData::SetStream(bg::ValueHolderPtr &&stream) {
  return SetStream(std::move(stream), ExecuteGraphType::kMain);
}
LoweringGlobalData &LoweringGlobalData::SetStream(bg::ValueHolderPtr &&stream, ExecuteGraphType graph_type) {
  if (graph_type >= ExecuteGraphType::kNum) {
    return *this;
  }
  streams_.holders[static_cast<size_t>(graph_type)] = std::move(stream);
  return *this;
}
const LoweringGlobalData::NodeCompileResult *LoweringGlobalData::FindCompiledResult(const ge::NodePtr &node) const {
  auto iter = node_name_to_compile_result_holders_.find(node->GetName());
  if (iter == node_name_to_compile_result_holders_.end()) {
    return nullptr;
  }
  return &iter->second;
}
LoweringGlobalData &LoweringGlobalData::AddCompiledResult(const ge::NodePtr &node,
                                                          LoweringGlobalData::NodeCompileResult compile_result) {
  node_name_to_compile_result_holders_[node->GetName()] = std::move(compile_result);
  return *this;
}

void *LoweringGlobalData::FindKnownSubgraphModel(const ge::NodePtr &node) const {
  const std::map<int64_t, void *>::const_iterator iter =
      node_ids_to_known_subgraph_models_.find(node->GetOpDesc()->GetId());
  if (iter == node_ids_to_known_subgraph_models_.cend()) {
    return nullptr;
  }
  return iter->second;
}

void *LoweringGlobalData::GetGraphStaticCompiledModel(const std::string &graph_name) const {
  auto iter = graph_to_static_models_.find(graph_name);
  if (iter == graph_to_static_models_.cend()) {
    return nullptr;
  }
  return iter->second;
}

LoweringGlobalData &LoweringGlobalData::AddKnownSubgraphModel(const ge::NodePtr &node, void *const model) {
  node_ids_to_known_subgraph_models_[node->GetOpDesc()->GetId()] = model;
  return *this;
}

LoweringGlobalData &LoweringGlobalData::AddStaticCompiledGraphModel(const std::string &graph_name, void *const model) {
  graph_to_static_models_[graph_name] = model;
  return *this;
}

bg::ValueHolderPtr LoweringGlobalData::GetAllocator(AllocatorDesc desc) const {
  if (CurrentOnInitGraph()) {
    return GetUniqueValueHolder(desc.GetKey() + "-Init");
  } else {
    return GetUniqueValueHolder(desc.GetKey());
  }
}
LoweringGlobalData &LoweringGlobalData::SetAllocator(AllocatorDesc desc, bg::ValueHolderPtr allocator) {
  (void)desc;
  (void)allocator;
  throw std::invalid_argument("No longer support SetAllocator, use GetOrCreateAllocator instead");
  return *this;
}
LoweringGlobalData &LoweringGlobalData::SetExternalAllocator(bg::ValueHolderPtr &&allocator) {
  return SetExternalAllocator(std::move(allocator), ExecuteGraphType::kMain);
}
LoweringGlobalData &LoweringGlobalData::SetExternalAllocator(bg::ValueHolderPtr &&allocator,
                                                             ExecuteGraphType graph_type) {
  if (graph_type >= ExecuteGraphType::kNum) {
    return *this;
  }
  external_allocators_.holders[static_cast<size_t>(graph_type)] = std::move(allocator);
  return *this;
}
/*
 * +------------------------------------------------------------------+
 * |Main Graph                                                        |
 * |                 (allocator)                                      |
 * |                     |                                            |
 * |     +------>  SelectAllocator  <-----+                           |
 * |     |           /       \            |                           |
 * | InnerData  InnerData   InnerData   Data(-2)                      |
 * +------------------------------------------------------------------+
 * +------------------------------------------------------------------+
 * |Init Graph                                                        |
 * |                                                                  |
 * |   +------+--->  InnerNetOutput    (allocator)                    |
 * |   |      |              ^            |                           |
 * |   |      |              |     SelectAllocator                    |
 * |   |      |              |    /         ^     \                   |
 * |   |      |     CreateAllocator         |   Data(Allocator)(-2)   |
 * |   |      |          /  \               |                         |
 * |   |  Const(placement)  Const(usage)    |                         |
 * |   |                         |          |                         |
 * |   +-------------------------+----------+                         |
 * +------------------------------------------------------------------+
 */
bg::ValueHolderPtr LoweringGlobalData::GetOrCreateAllocator(AllocatorDesc desc) {
  auto key = desc.GetKey();
  auto from_init = CurrentOnInitGraph();

  bg::ValueHolderPtr allocator_holder;
  if (from_init) {
    auto init_key = key + "-Init";
    allocator_holder = GetUniqueValueHolder(init_key);
  } else {
    allocator_holder = GetUniqueValueHolder(key);
  }

  if (allocator_holder != nullptr) {
    return allocator_holder;
  }

  bg::ValueHolderPtr init_selected_allocator = nullptr;
  auto init_out = bg::FrameSelector::OnInitRoot(
    [&init_selected_allocator, &desc, this]() -> std::vector<bg::ValueHolderPtr> {
    auto placement_holder = bg::ValueHolder::CreateConst(&desc.placement, sizeof(desc.placement));
    auto memory_type_holder = bg::ValueHolder::CreateConst(&desc.usage, sizeof(desc.usage));
    auto created_allocator =
        bg::ValueHolder::CreateSingleDataOutput("CreateAllocator", {placement_holder, memory_type_holder});
    if (external_allocators_.holders[static_cast<size_t>(ExecuteGraphType::kInit)] != nullptr) {
      init_selected_allocator = bg::ValueHolder::CreateSingleDataOutput(
          "SelectAllocator",
          {placement_holder, memory_type_holder,
           external_allocators_.holders[static_cast<size_t>(ExecuteGraphType::kInit)], created_allocator});
    } else {
      init_selected_allocator = created_allocator;
    }

    return {created_allocator, placement_holder, memory_type_holder};
  });
  GE_ASSERT_EQ(init_out.size(), 3U);

  auto allocator = bg::FrameSelector::OnMainRoot([&init_out, this]() -> std::vector<bg::ValueHolderPtr> {
    auto main_selected_allocator = init_out[0];
    if (external_allocators_.holders[static_cast<size_t>(ExecuteGraphType::kMain)] != nullptr) {
      main_selected_allocator = bg::ValueHolder::CreateSingleDataOutput(
          "SelectAllocator",
          {init_out[1], init_out[2], external_allocators_.holders[static_cast<size_t>(ExecuteGraphType::kMain)],
           init_out[0]});
    }
    return {main_selected_allocator};
  });
  GE_ASSERT_EQ(allocator.size(), 1U);

  SetUniqueValueHolder(key + "-Init", init_selected_allocator);
  SetUniqueValueHolder(key, allocator[0]);

  if (from_init) {
    return init_selected_allocator;
  } else {
    return allocator[0];
  }
}

uint64_t LoweringGlobalData::GetSessionId() {
  static std::atomic<uint64_t> global_session_id(0U);
  if (session_id_ == std::numeric_limits<uint64_t>::max()) {
    session_id_ = global_session_id++;
  }
  return session_id_;
}

bg::ValueHolderPtr LoweringGlobalData::GetOrCreateUniqueValueHolder(const std::string &name,
    const std::function<bg::ValueHolderPtr()> &builder) {
  const std::map<std::string, bg::ValueHolderPtr>::const_iterator iter = names_to_unique_value_holder_.find(name);
  if (iter == names_to_unique_value_holder_.cend()) {
    auto holder = builder();
    SetUniqueValueHolder(name, holder);
    return holder;
  }
  return iter->second;
}
void LoweringGlobalData::SetUniqueValueHolder(const string &name, const bg::ValueHolderPtr &holder) {
  names_to_unique_value_holder_.emplace(name, holder);
}
bg::ValueHolderPtr LoweringGlobalData::GetUniqueValueHolder(const string &name) const {
  const auto &iter = names_to_unique_value_holder_.find(name);
  if (iter == names_to_unique_value_holder_.end()) {
    return nullptr;
  }
  return iter->second;
}
}  // namespace gert