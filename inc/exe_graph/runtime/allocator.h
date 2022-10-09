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

#ifndef METADEF_INC_EXE_GRAPH_RUNTIME_ALLOCATOR_H_
#define METADEF_INC_EXE_GRAPH_RUNTIME_ALLOCATOR_H_
#include "tensor.h"
namespace gert {
enum class AllocatorUsage {
  kAllocNodeOutput,
  kAllocNodeWorkspace,
  kAllocNodeShapeBuffer,
  kEnd
};
struct AllocatorDesc {
  TensorPlacement placement;
  AllocatorUsage usage;
  bool operator<(const AllocatorDesc &other) const {
    return std::tie(placement, usage) < std::tie(other.placement, other.usage);
  }
};
}
#endif  // METADEF_INC_EXE_GRAPH_RUNTIME_ALLOCATOR_H_
