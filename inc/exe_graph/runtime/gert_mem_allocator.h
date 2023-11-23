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

#ifndef METADEF_CXX_GERT_MEM_ALLOCATOR_H
#define METADEF_CXX_GERT_MEM_ALLOCATOR_H
#include "gert_mem_block.h"
#include "gert_tensor_data.h"
#include "ge/ge_allocator.h"
namespace gert {
namespace memory {
struct GertMemAllocator {
  ge::Allocator *allocator;  // 暂时不能删，air仓适配之后再删除
  TensorPlacement placement;
};
}  // namespace memory
class GertAllocator {
 public:
  GertAllocator() : GertAllocator(-1, kTensorPlacementEnd) {}
  GertAllocator(int64_t stream_id, TensorPlacement placement) : stream_id_(stream_id), placement_(placement) {}
  GertAllocator(const GertAllocator &) = delete;
  GertAllocator(GertAllocator &&) = delete;
  GertAllocator &operator=(const GertAllocator &) = delete;
  GertAllocator &operator=(GertAllocator &&) = delete;

  virtual ~GertAllocator() = default;
  virtual GertMemBlock *Malloc(size_t size) = 0;
  virtual void Free(GertMemBlock *block) = 0;
  virtual GertTensorData MallocTensorData(size_t size) = 0;
  virtual GertTensorData MoveInFromTensorData(TensorData &&tensor_data) = 0;
  virtual TensorData MoveOutToTensorData(GertTensorData &&gert_tensor_data) = 0;
  virtual ge::graphStatus SetL1Allocator(ge::Allocator *allocator) = 0;
  TensorPlacement GetPlacement() const {
    return placement_;
  }
  void SetPlacement(TensorPlacement placement) {
    placement_ = placement;
  }
  void SetStreamId(int64_t stream_id) {
    stream_id_ = stream_id;
  }
  int64_t GetStreamId() const {
    return stream_id_;
  }
 private:
  int64_t stream_id_;
  TensorPlacement placement_;
};
}  // namespace gert
#endif  // METADEF_CXX_GERT_MEM_ALLOCATOR_H
