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

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_NODE_CONVERTER_REGISTRY_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_NODE_CONVERTER_REGISTRY_H_
#include <string>
#include <functional>
#include <vector>

#include "graph/node.h"
#include "exe_graph/lowering/value_holder.h"
#include "exe_graph/lowering/lowering_global_data.h"

namespace gert {
struct LowerInput {
  std::vector<bg::ValueHolderPtr> input_shapes;
  std::vector<bg::ValueHolderPtr> input_addrs;
  LoweringGlobalData *global_data;
};
struct LowerResult {
  HyperStatus result;
  std::vector<bg::ValueHolderPtr> order_holders;
  std::vector<bg::ValueHolderPtr> out_shapes;
  std::vector<bg::ValueHolderPtr> out_addrs;
};
class NodeConverterRegistry {
 public:
  using NodeConverter = LowerResult(*)(const ge::NodePtr &node, const LowerInput &lower_input);
  static NodeConverterRegistry &GetInstance();
  NodeConverter FindNodeConverter(const std::string &func_name);
  void RegisterNodeConverter(const std::string &func_name, NodeConverter func);

 private:
  std::unordered_map<std::string, NodeConverter> names_to_func_;
};

class NodeConverterRegister {
 public:
  NodeConverterRegister(const char *lower_func_name, NodeConverterRegistry::NodeConverter func) noexcept;
};
}

#define GERT_REGISTER_NODE_CONVERTER_COUNTER2(type, func, counter) \
    static const gert::NodeConverterRegister g_register_node_converter_##counter __attribute__((used)) = \
      gert::NodeConverterRegister(type, func)
#define GERT_REGISTER_NODE_CONVERTER_COUNTER(type, func, counter) GERT_REGISTER_NODE_CONVERTER_COUNTER2(type, func, counter)
#define REGISTER_NODE_CONVERTER(type, func) GERT_REGISTER_NODE_CONVERTER_COUNTER(type, func, __COUNTER__)

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_NODE_CONVERTER_REGISTRY_H_
