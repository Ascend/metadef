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

#ifndef AIR_CXX_TESTS_UT_GE_RUNTIME_V2_FAKER_ALLOCATOR_FACKER_H_
#define AIR_CXX_TESTS_UT_GE_RUNTIME_V2_FAKER_ALLOCATOR_FACKER_H_
#include "ge/ge_allocator.h"
#include "exe_graph/runtime/gert_mem_block.h"
#include "exe_graph/runtime/gert_mem_allocator.h"
namespace gert {
class GertMemBlockFaker : public GertMemBlock {
  public:
  explicit GertMemBlockFaker(void *addr) : addr_(addr) {}
  ~GertMemBlockFaker() override = default;
  void Free(int64_t stream_id) override {}
  void *GetAddr() override { return addr_; }
  private:
  void *addr_;
};
class AllocatorFaker : public GertAllocator {
  GertMemBlock *Malloc(size_t size) override;
  void Free(GertMemBlock *block) override;
  GertTensorData MallocTensorData(size_t size) override {
    return GertTensorData();
  }
  TensorData MallocTensorDataFromL1(size_t size) override {
    return TensorData();
  }
  ge::graphStatus ShareFromTensorData(const TensorData &td, GertTensorData &gtd) override {
    return 0;
  }
  ge::graphStatus SetL1Allocator(ge::Allocator *allocator) override {
    return ge::GRAPH_SUCCESS;
  }
  ge::graphStatus FreeAt(int64_t stream_id, GertMemBlock *block) override {
    return ge::GRAPH_SUCCESS;
  }
  int64_t GetStreamNum() override {
    return 0;
  }
};
}
#endif