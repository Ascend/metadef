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
  while ((current_graph != nullptr) && (current_graph->GetParentNode() != nullptr)) {
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
LoweringGlobalData &LoweringGlobalData::SetStream(bg::ValueHolderPtr &&stream, const ExecuteGraphType graph_type) {
  if (graph_type >= ExecuteGraphType::kNum) {
    return *this;
  }
  streams_.holders[static_cast<size_t>(graph_type)] = std::move(stream);
  return *this;
}
const LoweringGlobalData::NodeCompileResult *LoweringGlobalData::FindCompiledResult(const ge::NodePtr &node) const {
  const auto iter = node_name_to_compile_result_holders_.find(node->GetName());
  if (iter == node_name_to_compile_result_holders_.cend()) {
    return nullptr;
  }
  return &iter->second;
}
LoweringGlobalData &LoweringGlobalData::AddCompiledResult(const ge::NodePtr &node,
                                                          LoweringGlobalData::NodeCompileResult compile_result) {
  node_name_to_compile_result_holders_[node->GetName()] = std::move(compile_result);
  return *this;
}

void *LoweringGlobalData::GetGraphStaticCompiledModel(const std::string &graph_name) const {
  const auto iter = graph_to_static_models_.find(graph_name);
  if (iter == graph_to_static_models_.cend()) {
    return nullptr;
  }
  return iter->second;
}

LoweringGlobalData &LoweringGlobalData::AddStaticCompiledGraphModel(const std::string &graph_name, void *const model) {
  graph_to_static_models_[graph_name] = model;
  return *this;
}

bg::ValueHolderPtr LoweringGlobalData::GetAllocator(const AllocatorDesc &desc) const {
  if (CurrentOnInitGraph()) {
    return GetUniqueValueHolder(desc.GetKey() + "-Init");
  } else {
    return GetUniqueValueHolder(desc.GetKey());
  }
}
LoweringGlobalData &LoweringGlobalData::SetExternalAllocator(bg::ValueHolderPtr &&allocator) {
  return SetExternalAllocator(std::move(allocator), ExecuteGraphType::kMain);
}
LoweringGlobalData &LoweringGlobalData::SetExternalAllocator(bg::ValueHolderPtr &&allocator,
                                                             const ExecuteGraphType graph_type) {
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
bg::ValueHolderPtr LoweringGlobalData::GetOrCreateAllocator(const AllocatorDesc desc) {
  const auto key = desc.GetKey();
  const auto from_init = CurrentOnInitGraph();

  bg::ValueHolderPtr allocator_holder;
  if (from_init) {
    const auto init_key = key + "-Init";
    allocator_holder = GetUniqueValueHolder(init_key);
  } else {
    allocator_holder = GetUniqueValueHolder(key);
  }

  if (allocator_holder != nullptr) {
    return allocator_holder;
  }

  bg::ValueHolderPtr init_selected_allocator = nullptr;
  auto init_out = bg::FrameSelector::OnInitRoot([&desc, &init_selected_allocator,
                                                 this]() -> std::vector<bg::ValueHolderPtr> {
    auto placement_holder = bg::ValueHolder::CreateConst(&desc.placement, sizeof(desc.placement));
    auto memory_type_holder = bg::ValueHolder::CreateConst(&desc.usage, sizeof(desc.usage));
    auto created_allocator =
        bg::ValueHolder::CreateSingleDataOutput("CreateAllocator", {placement_holder, memory_type_holder});
    if (external_allocators_.holders[static_cast<size_t>(ExecuteGraphType::kInit)] != nullptr) {
      init_selected_allocator = bg::ValueHolder::CreateSingleDataOutput(
          "SelectAllocator",
          {placement_holder, memory_type_holder,
           external_allocators_.holders[static_cast<size_t>(ExecuteGraphType::kInit)],
           created_allocator, GetStream()});
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
           init_out[0], GetStream()});
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

bg::ValueHolderPtr LoweringGlobalData::GetOrCreateUniqueValueHolder(
    const std::string &name, const std::function<bg::ValueHolderPtr()> &builder) {
  return GetOrCreateUniqueValueHolder(name, [&builder]() -> std::vector<bg::ValueHolderPtr> { return {builder()}; })[0];
}

std::vector<bg::ValueHolderPtr> LoweringGlobalData::GetOrCreateUniqueValueHolder(
    const std::string &name, const std::function<std::vector<bg::ValueHolderPtr>()> &builder) {
  const decltype(unique_name_to_value_holders_)::const_iterator &iter = unique_name_to_value_holders_.find(name);
  if (iter == unique_name_to_value_holders_.cend()) {
    auto holder = builder();
    return unique_name_to_value_holders_.emplace(name, holder).first->second;
  }
  return iter->second;
}
void LoweringGlobalData::SetUniqueValueHolder(const string &name, const bg::ValueHolderPtr &holder) {
  unique_name_to_value_holders_.emplace(name, std::vector<bg::ValueHolderPtr>{holder});
}
bg::ValueHolderPtr LoweringGlobalData::GetUniqueValueHolder(const string &name) const {
  const auto &iter = unique_name_to_value_holders_.find(name);
  if (iter == unique_name_to_value_holders_.cend()) {
    return nullptr;
  }
  return iter->second[0];
}

void LoweringGlobalData::SetValueHolders(const string &name, const bg::ValueHolderPtr &holder) {
  unique_name_to_value_holders_[name].emplace_back(holder);
}

size_t LoweringGlobalData::GetValueHoldersSize(const string &name) {
  const auto &iter = unique_name_to_value_holders_.find(name);
  if (iter == unique_name_to_value_holders_.cend()) {
    return 0U;
  }
  return iter->second.size();
}

void LoweringGlobalData::SetModelWeightSize(const size_t require_weight_size) {
  model_weight_size_ = require_weight_size;
}
size_t LoweringGlobalData::GetModelWeightSize() const {
  return model_weight_size_;
}
}  // namespace gert