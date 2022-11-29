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

#include "graph/flow_graph/data_flow_attr_define.h"

namespace ge {
namespace dflow {
// Public attribute
const char *const ATTR_NAME_DATA_FLOW_PROCESS_POINTS = "_dflow_process_points";
const char *const ATTR_NAME_IS_DATA_FLOW_GRAPH = "_dflow_is_data_flow_graph";
const char *const ATTR_NAME_DATA_FLOW_INPUT = "input";
const char *const ATTR_NAME_DATA_FLOW_OUTPUT = "output";

// For count batch
const char *const ATTR_NAME_COUNT_BATCH_BATCH_SIZE = "_auto_batch_batch_size";
const char *const ATTR_NAME_COUNT_BATCH_SLIDE_STRIDE = "_auto_batch_slide_stride";
const char *const ATTR_NAME_COUNT_BATCH_TIMEOUT = "_auto_batch_timeout";
const char *const ATTR_NAME_COUNT_BATCH_BATCH_DIM = "_auto_batch_dim";
const char *const ATTR_NAME_COUNT_BATCH_FLAG = "_auto_batch_flag";
const char *const ATTR_NAME_COUNT_BATCH_PADDING = "_auto_batch_padding";
const char *const ATTR_NAME_COUNT_BATCH_DROP_REMAINDER = "_auto_batch_drop_remainder";

// For time batch
const char *const ATTR_NAME_TIME_BATCH_TIME_WINDOW = "_time_batch_window";
const char *const ATTR_NAME_TIME_BATCH_TIME_INTERVAL = "_time_batch_time_interval";
const char *const ATTR_NAME_TIME_BATCH_TIMEOUT = "_time_batch_timeout";
const char *const ATTR_NAME_TIME_BATCH_BATCH_DIM = "_time_batch_dim";
const char *const ATTR_NAME_TIME_BATCH_FLAG = "_time_batch_flag";
const char *const ATTR_NAME_TIME_BATCH_PADDING = "_time_batch_padding";
const char *const ATTR_NAME_TIME_BATCH_DROP_REMAINDER = "_time_batch_drop_remainder";
}  // namespace dflow
}  // namespace ge