/**
 * Copyright 2019-2022 Huawei Technologies Co., Ltd
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

#ifndef INC_REGISTER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_DESC_H_
#define INC_REGISTER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_DESC_H_
#include <stdint.h>
namespace fe {
using PassAttr = uint64_t;
const PassAttr FORBIDDEN_CLOSE = 0x01U;  // forbidden close, can not be closed by fusion switch
const PassAttr SINGLE_SCENE_OPEN = 0x04U;  // open for single op scene, can be close by fusion switch
}  // namespace fe
#endif  // INC_REGISTER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_DESC_H_