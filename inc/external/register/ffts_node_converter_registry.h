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

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_FFTS_NODE_CONVERTER_REGISTRY_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_FFTS_NODE_CONVERTER_REGISTRY_H_
#include <string>
#include <functional>
#include <vector>
#include "node_converter_registry.h"
#include "graph/node.h"
#include "exe_graph/lowering/value_holder.h"
#include "exe_graph/lowering/lowering_global_data.h"

namespace gert {
using FFTSPreThreadFunc = std::function<ge::graphStatus(const ge::ComputeGraphPtr sub_graph,
    const std::vector<bg::ValueHolderPtr> &input_shapes, std::vector<bg::ValueHolderPtr> &output)>;
using FFTSThreadFunc = std::function<ge::graphStatus(const ge::NodePtr &node,
    const std::vector<bg::ValueHolderPtr> &input_shapes,
    const std::vector<bg::ValueHolderPtr> &output_shapes, const bg::ValueHolderPtr thread_dim,
    std::vector<bg::ValueHolderPtr> &output)>;

struct FFTSLowerInput {
  std::vector<bg::ValueHolderPtr> input_shapes;
  std::vector<bg::ValueHolderPtr> input_addrs;
  std::vector<uint32_t> mem_pool_types;
  LoweringGlobalData *global_data;
  bg::ValueHolderPtr task_info;
  bg::ValueHolderPtr thread_dim;
  bg::ValueHolderPtr window_size;
  bg::ValueHolderPtr args_para;
  bg::ValueHolderPtr ffts_mem_allocator;
  FFTSThreadFunc ffts_thread_fun;
};
class FFTSNodeConverterRegistry {
 public:
  using NodeConverter = LowerResult (*)(const ge::NodePtr &node, const FFTSLowerInput &lower_input);
  struct ConverterRegisterData {
    NodeConverter converter;
    int32_t require_placement;
  };
  static FFTSNodeConverterRegistry &GetInstance();
  NodeConverter FindNodeConverter(const std::string &func_name);
  const ConverterRegisterData *FindRegisterData(const std::string &func_name) const;
  void RegisterNodeConverter(const std::string &func_name, NodeConverter func);
  void Register(const std::string &func_name, const ConverterRegisterData &data);

 private:
  std::unordered_map<std::string, ConverterRegisterData> names_to_register_data_;
};

class FFTSNodeConverterRegister {
 public:
  FFTSNodeConverterRegister(const char *lower_func_name, FFTSNodeConverterRegistry::NodeConverter func) noexcept;
  FFTSNodeConverterRegister(const char *lower_func_name, int32_t require_placement,
                            FFTSNodeConverterRegistry::NodeConverter func) noexcept;
};
}  // namespace gert

#ifdef __GNUC__
#define ATTRIBUTE_USED __attribute__((used))
#else
#define ATTRIBUTE_USED
#endif

#define GERT_REGISTER_FFTS_NODE_CONVERTER_COUNTER2(type, placement, func, counter)                  \
  static const gert::FFTSNodeConverterRegister g_register_node_converter_##counter ATTRIBUTE_USED =  \
      gert::FFTSNodeConverterRegister(type, placement, func)
#define GERT_REGISTER_FFTS_NODE_CONVERTER_COUNTER(type, placement, func, counter)                    \
  GERT_REGISTER_FFTS_NODE_CONVERTER_COUNTER2(type, placement, func, counter)
#define FFTS_REGISTER_NODE_CONVERTER_PLACEMENT(type, placement, func)                                \
  GERT_REGISTER_FFTS_NODE_CONVERTER_COUNTER(type, placement, func, __COUNTER__)
#define FFTS_REGISTER_NODE_CONVERTER(type, func) FFTS_REGISTER_NODE_CONVERTER_PLACEMENT(type, -1, func)

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_FFTS_NODE_CONVERTER_REGISTRY_H_
