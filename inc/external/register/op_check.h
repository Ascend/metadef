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

#ifndef INC_REGISTER_TIK2_OP_CHECK_H_
#define INC_REGISTER_TIK2_OP_CHECK_H_

#include <map>

#include "graph/ascend_string.h"
#include "graph/operator.h"

namespace optiling {
const ge::AscendString FUNC_CHECK_SUPPORTED = "check_supported";
const ge::AscendString FUNC_OP_SELECT_FORMAT = "op_select_format";
const ge::AscendString FUNC_GET_OP_SUPPORT_INFO = "get_op_support_info";
const ge::AscendString FUNC_GET_SPECIFIC_INFO = "get_op_specific_info";

typedef int (*OP_CHECK_FUNC)(const ge::Operator &op, ge::AscendString &result);

typedef int (*PARAM_GENERALIZE_FUNC)(const ge::Operator &op, const ge::AscendString &generalize_config,
                                     ge::AscendString &generalized_op_params);

class OpCheckFuncRegistry {
public:
  static void RegisterOpCapability(const ge::AscendString &check_type, const ge::AscendString &op_type, OP_CHECK_FUNC func);

  static OP_CHECK_FUNC GetOpCapability(const ge::AscendString &check_type, const ge::AscendString &op_type);

  static PARAM_GENERALIZE_FUNC GetParamGeneralize(const ge::AscendString &op_type);

  static void RegisterParamGeneralize(const ge::AscendString &op_type, PARAM_GENERALIZE_FUNC func);

private:
  static std::map<ge::AscendString, std::map<ge::AscendString, OP_CHECK_FUNC>> check_op_capability_instance_;
  static std::map<ge::AscendString, PARAM_GENERALIZE_FUNC> param_generalize_instance_;
};

class OpCheckFuncHelper {
public:
  OpCheckFuncHelper(const ge::AscendString &check_type, const ge::AscendString &op_type, OP_CHECK_FUNC func);

  OpCheckFuncHelper(const ge::AscendString &op_type, PARAM_GENERALIZE_FUNC func);
};

#define REG_CHECK_SUPPORT(op_type, func)                                                                               \
  static OpCheckFuncHelper op_check_registry_##op_type##_check_supported(FUNC_CHECK_SUPPORTED, #op_type, func)
#define REG_OP_SELECT_FORMAT(op_type, func)                                                                            \
  static OpCheckFuncHelper op_check_registry_##op_type##_op_select_format(FUNC_OP_SELECT_FORMAT, #op_type, func)
#define REG_OP_SUPPORT_INFO(op_type, func)                                                                             \
  static OpCheckFuncHelper op_check_registry_##op_type##_get_op_support_info(FUNC_GET_OP_SUPPORT_INFO, #op_type, func)
#define REG_OP_SPEC_INFO(op_type, func)                                                                                \
  static OpCheckFuncHelper op_check_registry_##op_type##_get_specific_info(FUNC_GET_SPECIFIC_INFO, #op_type, func)

#define REG_OP_PARAM_GENERALIZE(op_type, generalize_func)                                                              \
  static OpCheckFuncHelper op_check_generalize_registry_##op_type(#op_type, generalize_func)
} // end of namespace optiling
#endif  // INC_REGISTER_TIK2_OP_CHECK_H_
