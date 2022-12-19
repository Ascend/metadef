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

#ifndef INC_GRAPH_UTILS_TENSORBOARD_GRAPH_DUMP_UTILS_H_
#define INC_GRAPH_UTILS_TENSORBOARD_GRAPH_DUMP_UTILS_H_

#include <string>

#include "graph/compute_graph.h"

#define GE_DUMP_TENSORBOARD_GRAPH(compute_graph, name)                                                             \
  do {                                                                                                             \
    (void)ge::TensorBoardGraphDumpUtils::DumpGraph(*(compute_graph), (name));                                      \
  } while (false)

namespace ge {
class TensorBoardGraphDumpUtils {
 public:
  /**
   * 图dump接口，用于把`graph`对象按照tensorboard格式序列化到文件，默认落盘到当前路径
   * @param compute_graph
   * @param suffix 用于拼接文件名
   */
  static graphStatus DumpGraph(const ge::ComputeGraph &compute_graph, const std::string &suffix);
};
}  // namespace ge
#endif  // INC_GRAPH_UTILS_TENSORBOARD_GRAPH_DUMP_UTILS_H_
