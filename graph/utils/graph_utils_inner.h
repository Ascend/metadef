/*
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

#ifndef COMMON_GRAPH_UTILS_GRAPH_UTILS_INNER_H_
#define COMMON_GRAPH_UTILS_GRAPH_UTILS_INNER_H_
#include <string>

namespace ge {
const int32_t kBaseOfIntegerValue = 10;
const char_t *const kDumpGeGraph = "DUMP_GE_GRAPH";
const int32_t kDumpGraphIndexWidth = 8;
const size_t kMaxFileNameLen = 255U;
const char_t *const kNpuCollectPath = "NPU_COLLECT_PATH";
const char_t *const kDumpGraphPath = "DUMP_GRAPH_PATH";
const char_t *const kDumpGraphLevel = "DUMP_GRAPH_LEVEL";

void GetDumpGraphPrefix(std::stringstream &stream_file_name);
}  // namespace ge
#endif  // COMMON_GRAPH_UTILS_GRAPH_UTILS_INNER_H_