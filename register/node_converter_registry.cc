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

#include "register/node_converter_registry.h"
#include "common/hyper_status.h"

namespace gert {
NodeConverterRegistry &NodeConverterRegistry::GetInstance() {
  static NodeConverterRegistry registry;
  return registry;
}

NodeConverterRegistry::NodeConverter NodeConverterRegistry::FindNodeConverter(const string &func_name) {
  auto iter = names_to_func_.find(func_name);
  if (iter == names_to_func_.end()) {
    return nullptr;
  }
  return iter->second;
}
void NodeConverterRegistry::RegisterNodeConverter(const std::string &func_name, NodeConverter func) {
  names_to_func_[func_name] = func;
}
NodeConverterRegister::NodeConverterRegister(const char *lower_func_name,
                                             NodeConverterRegistry::NodeConverter func) noexcept {
  NodeConverterRegistry::GetInstance().RegisterNodeConverter(lower_func_name, func);
}

LowerResult CreateErrorLowerResult(const char *error_msg, ...) {
  va_list arg;
  va_start(arg, error_msg);
  auto msg = std::unique_ptr<char[]>(CreateMessage(error_msg, arg));
  va_end(arg);
  return {HyperStatus::ErrorStatus(std::move(msg)), {}, {}, {}};
}
}  // namespace gert