/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#ifndef COMMON_GRAPH_DEBUG_GE_UTIL_H_
#define COMMON_GRAPH_DEBUG_GE_UTIL_H_

#include "framework/common/util.h"
#include "graph/debug/ge_log.h"
#include "graph/ge_error_codes.h"

template <typename T, typename... Args>
static inline std::shared_ptr<T> ComGraphMakeShared(Args &&... args) {
  using T_nc = typename std::remove_const<T>::type;
  std::shared_ptr<T> ret(new (std::nothrow) T_nc(std::forward<Args>(args)...));
  return ret;
}

#endif  // COMMON_GRAPH_DEBUG_GE_UTIL_H_
