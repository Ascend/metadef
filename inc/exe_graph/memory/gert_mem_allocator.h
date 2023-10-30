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

#ifndef METADEF_MEMORY_GERT_MEM_ALLOCATOR_H
#define METADEF_MEMORY_GERT_MEM_ALLOCATOR_H

#include "exe_graph/runtime/tensor_data.h"
#include "ge/ge_allocator.h"

namespace gert {
namespace memory {
struct GertMemAllocator {
  ge::Allocator *allocator;
  TensorPlacement placement;
};
}
}  // namespace gert
#endif  // METADEF_MEMORY_GERT_MEM_ALLOCATOR_H