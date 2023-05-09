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

#include <unordered_set>
#include "graph/utils/op_type_utils.h"
#include "graph/debug/ge_op_types.h"

namespace ge {
namespace {
const std::unordered_set<std::string> kDataOpSet = {DATA, REFDATA, AIPPDATA, ANN_DATA, QUEUE_DATA};
const std::unordered_set<std::string> kVariableOpSet = {VARIABLE, VARIABLEV2, VARHANDLEOP};
const std::unordered_set<std::string> kAssignOpSet = {
    ASSIGNADD, ASSIGN, ASSIGNSUB, ASSIGNADDVARIABLEOP, ASSIGNSUBVARIABLEOP, ASSIGNVARIABLEOP};
const std::unordered_set<std::string> kIdentityOpSet = {IDENTITY, READVARIABLEOP};
}  // namespace

bool OpTypeUtils::IsDataNode(const std::string &type) {
  return (kDataOpSet.count(type) > 0);
}

bool OpTypeUtils::IsVariableNode(const std::string &type) {
  return (kVariableOpSet.count(type) > 0);
}

bool OpTypeUtils::IsVarLikeNode(const std::string &type) {
  return IsVariableNode(type) || (type == REFDATA);
}

bool OpTypeUtils::IsAssignLikeNode(const std::string &type) {
  return kAssignOpSet.count(type) > 0U;
}

bool OpTypeUtils::IsIdentityLikeNode(const std::string &type) {
  return kIdentityOpSet.count(type) > 0U;
}
}  // namespace ge
