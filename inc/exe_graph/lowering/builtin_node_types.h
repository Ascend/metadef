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

#ifndef METADEF_CXX_INC_EXE_GRAPH_LOWERING_BUILTIN_NODE_TYPES_H_
#define METADEF_CXX_INC_EXE_GRAPH_LOWERING_BUILTIN_NODE_TYPES_H_
#include "graph/types.h"
namespace gert {
// 子图的输出，InnerNetOutput有多个输入，没有输出，每个InnerNetOutput的输入，对应相同Index的parent node的输出
constexpr ge::char_t const *kInnerNetOutput = "InnerNetOutput";

// 子图的输入，InnerData没有输入，有一个个输出，代表其对应的parent node的输入。
// InnerData具有一个key为index的属性，该属性类型是int32，代表该InnerData对应的parent node输入的index
constexpr ge::char_t const *kInnerData = "InnerData";

// 图的输出，NetOutput出现在Main图上，表达执行完成后，图的输出。NetOutput有多个输入，没有输出，每个输入对应相同Index的图输出
constexpr char const *kNetOutput = "NetOutput";

// 图的输入，Data没有输入，有一个个输出，代表图的输入。
// Data具有一个key为index的属性，该属性类型是int32，代表图的输入的Index
constexpr ge::char_t const *kData = "Data";

// 常量节点，该节点没有输入，有一个输出，代表常量的值
// 常量节点有一个属性"value"代表该常量节点的值，value是一段二进制，常量节点本身不关注其内容的格式
constexpr ge::char_t const *kConst = "Const";


inline bool IsTypeData(const ge::char_t *const node_type) {
  return strcmp(kData, node_type) == 0;
}
inline bool IsTypeInnerData(const ge::char_t *const node_type) {
  return strcmp(kInnerData, node_type) == 0;
}
inline bool IsTypeInnerNetOutput(const ge::char_t *const node_type) {
  return strcmp(kInnerNetOutput, node_type) == 0;
}
inline bool IsTypeNetOutput(const ge::char_t *const node_type) {
  return strcmp(kNetOutput, node_type) == 0;
}
inline bool IsTypeConst(const ge::char_t *const node_type) {
  return strcmp(kConst, node_type) == 0;
}
}  // namespace gert
#endif  // METADEF_CXX_INC_EXE_GRAPH_LOWERING_BUILTIN_NODE_TYPES_H_