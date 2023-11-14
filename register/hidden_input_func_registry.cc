/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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

#include "register/hidden_input_func_registry.h"
#include <iostream>
#include "common/ge_common/debug/ge_log.h"
namespace ge {
GetHiddenAddr HiddenInputFuncRegistry::FindHiddenInputFunc(const HiddenInputType input_type) {
  const auto &iter = type_to_funcs_.find(input_type);
  if (iter != type_to_funcs_.end()) {
    return iter->second;
  }
  GELOGW("Hidden input func not found, type:[%d].", static_cast<int32_t>(input_type));
  return nullptr;
}

void HiddenInputFuncRegistry::Register(const HiddenInputType input_type, const GetHiddenAddr func) {
  type_to_funcs_[input_type] = func;
}
HiddenInputFuncRegistry &HiddenInputFuncRegistry::GetInstance() {
  static HiddenInputFuncRegistry registry;
  return registry;
}

HiddenInputFuncRegister::HiddenInputFuncRegister(const HiddenInputType input_type, const GetHiddenAddr func) {
  HiddenInputFuncRegistry::GetInstance().Register(input_type, func);
}
}  // namespace ge