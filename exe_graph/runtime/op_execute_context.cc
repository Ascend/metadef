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

#include "exe_graph/runtime/op_execute_context.h"
#include "exe_graph/runtime/gert_mem_allocator.h"

namespace gert {
  void *OpExecuteContext::MallocWorkspace(const size_t size) {
    auto memory_vec =
        GetOutputPointer<std::vector<ge::MemBlock *>>(
            static_cast<size_t>(OpExecuteOutputIndex::kBlockMemory));
    if (memory_vec == nullptr) {
      return nullptr;
    }
    const size_t input_num = GetComputeNodeInputNum();
    const size_t output_num = GetComputeNodeOutputNum();
    auto allocator_info =
        GetInputValue<memory::GertMemAllocator *>(input_num + output_num +
        static_cast<size_t>(OpExecuteInputExtendIndex::kAllocate));
    if ((allocator_info == nullptr) || (allocator_info->allocator == nullptr)) {
      return nullptr;
    }
    auto mem_block = allocator_info->allocator->Malloc(size);
    if (mem_block == nullptr) {
      return nullptr;
    }
    (void)memory_vec->emplace_back(mem_block);
    return mem_block->GetAddr();
  }

  void OpExecuteContext::FreeWorkspace() {
    auto memory_vec =
        GetOutputPointer<std::vector<ge::MemBlock *>>(
            static_cast<size_t>(OpExecuteOutputIndex::kBlockMemory));
    if (memory_vec == nullptr) {
      return;
    }
    for (size_t i = 0UL; i < memory_vec->size(); i++) {
      if (memory_vec->at(i) != nullptr) {
        memory_vec->at(i)->Free();
      }
    }
    memory_vec->clear();
  }
}