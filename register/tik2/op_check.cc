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

#include "register/op_check.h"
#include "register/tilingdata_base.h"

namespace optiling {
std::map<ge::AscendString, std::map<ge::AscendString, OP_CHECK_FUNC>> OpCheckFuncRegistry::check_op_capability_instance_;
std::map<ge::AscendString, PARAM_GENERALIZE_FUNC> OpCheckFuncRegistry::param_generalize_instance_;
std::map<ge::AscendString, TilingDataConstructor> CTilingDataClassFactory::instance_;

void OpCheckFuncRegistry::RegisterOpCapability(const ge::AscendString &check_type,
                                               const ge::AscendString &op_type, OP_CHECK_FUNC func) {
  check_op_capability_instance_[check_type][op_type] = func;
}

OP_CHECK_FUNC OpCheckFuncRegistry::GetOpCapability(const ge::AscendString &check_type,
                                                   const ge::AscendString &op_type) {
  const auto &check_map_it = check_op_capability_instance_.find(check_type);
  if (check_map_it == check_op_capability_instance_.end()) {
    return nullptr;
  }
  const auto &func_it = check_map_it->second.find(op_type);
  if (func_it == check_map_it->second.end()) {
    return nullptr;
  }
  return func_it->second;
}

PARAM_GENERALIZE_FUNC OpCheckFuncRegistry::GetParamGeneralize(const ge::AscendString &op_type) {
  const auto &func_it = param_generalize_instance_.find(op_type);
  if (func_it == param_generalize_instance_.end()) {
    return nullptr;
  }
  return func_it->second;
}

void OpCheckFuncRegistry::RegisterParamGeneralize(const ge::AscendString &op_type, PARAM_GENERALIZE_FUNC func) {
  param_generalize_instance_[op_type] = func;
}

OpCheckFuncHelper::OpCheckFuncHelper(const ge::AscendString &check_type, const ge::AscendString &op_type,
                                     OP_CHECK_FUNC func) {
  OpCheckFuncRegistry::RegisterOpCapability(check_type, op_type, func);
}

OpCheckFuncHelper::OpCheckFuncHelper(const ge::AscendString &op_type, PARAM_GENERALIZE_FUNC func) {
  OpCheckFuncRegistry::RegisterParamGeneralize(op_type, func);
}
}  // end of namespace optiling
