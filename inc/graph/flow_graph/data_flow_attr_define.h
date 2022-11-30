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

#ifndef INC_GRAPH_FLOW_GRAPH_DATA_FLOW_ATTR_DEFINE_H_
#define INC_GRAPH_FLOW_GRAPH_DATA_FLOW_ATTR_DEFINE_H_

#include "graph/debug/ge_attr_define.h"

namespace ge {
namespace dflow {
// Public attribute
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_DATA_FLOW_PROCESS_POINTS;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_IS_DATA_FLOW_GRAPH;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_DATA_FLOW_INPUT;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_DATA_FLOW_OUTPUT;

// For count batch
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_COUNT_BATCH_BATCH_SIZE;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_COUNT_BATCH_SLIDE_STRIDE;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_COUNT_BATCH_TIMEOUT;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_COUNT_BATCH_BATCH_DIM;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_COUNT_BATCH_FLAG;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_COUNT_BATCH_PADDING;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_COUNT_BATCH_DROP_REMAINDER;

// For time batch
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_TIME_BATCH_TIME_WINDOW;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_TIME_BATCH_TIME_INTERVAL;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_TIME_BATCH_TIMEOUT;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_TIME_BATCH_BATCH_DIM;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_TIME_BATCH_FLAG;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_TIME_BATCH_PADDING;
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY extern const char *const ATTR_NAME_TIME_BATCH_DROP_REMAINDER;
}  // namespace dflow
}  // namespace ge
#endif  // INC_GRAPH_FLOW_GRAPH_DATA_FLOW_ATTR_DEFINE_H_
