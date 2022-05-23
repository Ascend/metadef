/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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
namespace gert {
const bg::ValueHolderPtr &LoweringGlobalData::GetStream() const {
  return stream_;
}
LoweringGlobalData &LoweringGlobalData::SetStream(bg::ValueHolderPtr &&stream) {
  stream_ = std::move(stream);
  return *this;
}
const LoweringGlobalData::NodeCompileResult *LoweringGlobalData::FindCompiledResult(const ge::NodePtr &node) const {
  auto iter = node_ids_to_compile_result_holders_.find(node->GetOpDesc()->GetId());
  if (iter == node_ids_to_compile_result_holders_.end()) {
    return nullptr;
  }
  return &iter->second;
}
LoweringGlobalData &LoweringGlobalData::AddCompiledResult(const ge::NodePtr &node,
                                                          LoweringGlobalData::NodeCompileResult compile_result) {
  node_ids_to_compile_result_holders_[node->GetOpDesc()->GetId()] = std::move(compile_result);
  return *this;
}
bg::ValueHolderPtr LoweringGlobalData::GetAllocator(AllocatorDesc desc) const {
  auto iter = placements_to_allocator_.find(desc);
  if (iter == placements_to_allocator_.end()) {
    return nullptr;
  }
  return iter->second;
}
LoweringGlobalData &LoweringGlobalData::SetAllocator(AllocatorDesc desc, bg::ValueHolderPtr allocator) {
  placements_to_allocator_[desc] = std::move(allocator);
  return *this;
}
bg::ValueHolderPtr LoweringGlobalData::GetOrCreateAllocator(AllocatorDesc desc) {
  auto iter = placements_to_allocator_.find(desc);
  if (iter == placements_to_allocator_.end()) {
    auto memory_type_holder = bg::ValueHolder::CreateConst(&desc, sizeof(desc));
    auto allocator = bg::ValueHolder::CreateSingleDataOutput("CreateAllocator", {memory_type_holder});
    GE_ASSERT_NOTNULL(allocator);
    SetAllocator(desc, allocator);
    return allocator;
  }
  return iter->second;
}
}  // namespace gert