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

#ifndef INC_EXTERNAL_REGISTER_OP_IMPL_REGISTRY_API_H_
#define INC_EXTERNAL_REGISTER_OP_IMPL_REGISTRY_API_H_

#include <stdlib.h>
#include "op_impl_kernel_registry.h"

typedef struct {
  const char *op_type;
  gert::OpImplKernelRegistry::OpImplFunctions funcs;
} TypesToImpl;

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((visibility("default"))) size_t GetRegisteredOpNum(void);
__attribute__((visibility("default"))) int32_t GetOpImplFunctions(TypesToImpl *impl, size_t impl_num);

#ifdef __cplusplus
}
#endif

#endif  // INC_EXTERNAL_REGISTER_OP_IMPL_REGISTRY_API_H_
