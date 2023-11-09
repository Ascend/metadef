/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "register/op_ext_gentask_registry.h"

namespace fe {
OpExtGenTaskRegistry &OpExtGenTaskRegistry::GetInstance() {
  static OpExtGenTaskRegistry registry;
  return registry;
}

OpExtGenTaskFunc OpExtGenTaskRegistry::FindRegisterFunc(const std::string &op_type) const {
  auto iter = names_to_register_func_.find(op_type);
  if (iter == names_to_register_func_.end()) {
    return nullptr;
  }
  return iter->second;
}
void OpExtGenTaskRegistry::Register(const std::string &op_type, const OpExtGenTaskFunc func) {
  names_to_register_func_[op_type] = func;
}
OpExtGenTaskRegister::OpExtGenTaskRegister(const char *op_type, OpExtGenTaskFunc func) noexcept {
  OpExtGenTaskRegistry::GetInstance().Register(op_type, func);
}
}  // namespace fe