/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#include "graph/aligned_ptr.h"
#include "utils/mem_utils.h"
#include "graph/debug/ge_log.h"

namespace ge {
AlignedPtr::AlignedPtr(size_t buffer_size, size_t alignment) {
  size_t alloc_size = buffer_size;
  if (alignment > 0) {
    alloc_size = buffer_size + alignment - 1;
  }
  if ((buffer_size == 0) || (alloc_size < buffer_size)) {
    GELOGW("Allocate empty buffer or overflow, size=%zu, alloc_size=%zu", buffer_size, alloc_size);
    return;
  }

  base_ = std::unique_ptr<uint8_t[], deleter>(new (std::nothrow) uint8_t[alloc_size],
                                              [](uint8_t *ptr) {
                                                delete[] ptr;
                                                ptr = nullptr;
                                              });
  if (base_ == nullptr) {
    GELOGW("Allocate buffer failed, size=%zu", alloc_size);
    return;
  }

  if (alignment == 0) {
    aligned_addr_ = base_.get();
  } else {
    size_t offset = alignment - 1;
    aligned_addr_ =
        reinterpret_cast<uint8_t *>((static_cast<size_t>(reinterpret_cast<uintptr_t>(base_.get())) + offset) & ~offset);
  }
}

std::shared_ptr<AlignedPtr> AlignedPtr::BuildFromAllocFunc(const allocator &alloc_func, const deleter &delete_func) {
  if ((alloc_func == nullptr) || (delete_func == nullptr)) {
      GELOGE(FAILED, "alloc_func/delete_func is null");
      return nullptr;
  }
  auto aligned_ptr = MakeShared<AlignedPtr>();
  if (aligned_ptr == nullptr) {
    GELOGE(INTERNAL_ERROR, "make shared for AlignedPtr failed");
    return nullptr;
  }
  aligned_ptr->base_.reset();
  alloc_func(aligned_ptr->base_);
  aligned_ptr->base_.get_deleter() = delete_func;
  if (aligned_ptr->base_ == nullptr) {
    GELOGE(FAILED, "allocate for AlignedPtr failed");
    return nullptr;
  }
  aligned_ptr->aligned_addr_ = aligned_ptr->base_.get();
  return aligned_ptr;
}
}  // namespace ge
