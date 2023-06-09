/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "register/tuning_bank_key_registry.h"
#include "framework/common/debug/ge_log.h"

namespace tuningtiling {
std::map<ge::AscendString, OpBankKeyConstructor> &OpBankKeyClassFactory::RegisterInfo() {
  static std::map<ge::AscendString, OpBankKeyConstructor> instance;
  return instance;
}

void OpBankKeyClassFactory::RegisterOpBankKey(const ge::AscendString &optype, OpBankKeyConstructor const constructor) {
  auto &instance = OpBankKeyClassFactory::RegisterInfo();
  instance[optype] = constructor;
}

std::shared_ptr<OpBankKeyDef> OpBankKeyClassFactory::CreateBankKeyInstance(const ge::AscendString &optype) {
  const auto &instance = OpBankKeyClassFactory::RegisterInfo();
  const auto it = instance.find(optype);
  if (it == instance.cend()) {
    GELOGW("CreateBankKeyInstance: can not find optype: %s", optype.GetString());
    return nullptr;
  }
  const OpBankKeyConstructor constructor = it->second;
  if (constructor == nullptr) {
    GELOGW("CreateBankKeyInstance: constructor is nullptr");
    return nullptr;
  }
  return (*constructor)();
}

OpBankKeyFuncRegistry::OpBankKeyFuncRegistry(const ge::AscendString &optype, OpBankKeyFun bank_key_fun) {
  auto &op_func_map = RegisteredOpFuncInfo();
  const auto iter = op_func_map.find(optype);
  if (iter == op_func_map.cend()) {
    (void) op_func_map.emplace(optype, bank_key_fun);
  } else {
    iter->second = bank_key_fun;
  }
  GELOGI("Register op bank key function for optype: %s", optype.GetString());
}

std::unordered_map<ge::AscendString, OpBankKeyFun> &OpBankKeyFuncRegistry::RegisteredOpFuncInfo() {
  static std::unordered_map<ge::AscendString, OpBankKeyFun> op_func_map;
  return op_func_map;
}
}  // namespace tuningtiling
