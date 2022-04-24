/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#ifndef INC_2BD8E92ACD8442A693A5A727250EC78E_H
#define INC_2BD8E92ACD8442A693A5A727250EC78E_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FreeCallback)(void *);

typedef struct {
  void *data;
  FreeCallback deleter;
} AsyncAnyValue;

typedef struct {
  size_t input_size;
  size_t output_size;
  const void *compute_node_info;
  const void *kernel_extend_info;
  AsyncAnyValue **output_start;  // todo delete this
  AsyncAnyValue *values[1];
} KernelRunContext;

#ifdef __cplusplus
}
#endif

#endif
