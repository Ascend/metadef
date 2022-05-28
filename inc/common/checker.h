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

#ifndef METADEF_CXX_INC_COMMON_CHECKER_H_
#define METADEF_CXX_INC_COMMON_CHECKER_H_
#include <securec.h>
#include "graph/ge_error_codes.h"
#include "framework/common/debug/ge_log.h"
#include "hyper_status.h"

struct ErrorResult {
  operator bool() const {
    return false;
  }
  operator ge::graphStatus() const {
    return ge::PARAM_INVALID;
  }
  template<typename T>
  operator std::unique_ptr<T>() const {
    return nullptr;
  }
  template<typename T>
  operator std::shared_ptr<T>() const {
    return nullptr;
  }
  template<typename T>
  operator T *() const {
    return nullptr;
  }
};

#define GE_ASSERT_NOTNULL(val)                                                                                         \
  do {                                                                                                                 \
    if ((val) == nullptr) {                                                                                            \
      REPORT_INNER_ERROR("E19999", "Check error, get NULL");                                                           \
      GELOGE(ge::FAILED, "Check error, get NULL");                                                                     \
      return ::ErrorResult();                                                                                          \
    }                                                                                                                  \
  } while (0)

#define GE_ASSERT_SUCCESS(expr)                                                                                        \
  do {                                                                                                                 \
    auto tmp_expr_ret = (expr);                                                                                        \
    if (tmp_expr_ret != ge::GRAPH_SUCCESS) {                                                                           \
      REPORT_INNER_ERROR("E19999", "Expect success, but get %d", tmp_expr_ret);                                        \
      GELOGE(ge::FAILED, "Expect success, but get %d", tmp_expr_ret);                                                  \
      return ::ErrorResult();                                                                                          \
    }                                                                                                                  \
  } while (0)

#define GE_ASSERT_HYPER_SUCCESS(expr)                                                                                  \
  do {                                                                                                                 \
    const auto &tmp_expr_ret = (expr);                                                                                 \
    if (!tmp_expr_ret.IsSuccess()) {                                                                                   \
      REPORT_INNER_ERROR("E19999", "Expect success, but get error message %s", tmp_expr_ret.GetErrorMessage());        \
      GELOGE(ge::FAILED, "Expect success, but get error message %s", tmp_expr_ret.GetErrorMessage());                  \
      return ::ErrorResult();                                                                                          \
    }                                                                                                                  \
  } while (0)

#define GE_ASSERT_TRUE(val)                                                                                            \
  do {                                                                                                                 \
    if (!(val)) {                                                                                                      \
      REPORT_INNER_ERROR("E19999", "Check error, get FALSE");                                                          \
      GELOGE(ge::FAILED, "Check error, get FALSE");                                                                    \
      return ::ErrorResult();                                                                                          \
    }                                                                                                                  \
  } while (0)

#define GE_ASSERT_EOK(expr)                                                                                            \
  do {                                                                                                                 \
    auto tmp_expr_ret = (expr);                                                                                        \
    if (tmp_expr_ret != EOK) {                                                                                         \
      REPORT_INNER_ERROR("E19999", "Expect EOK, but get %d", tmp_expr_ret);                                            \
      GELOGE(ge::FAILED, "Expect EOK, but get %d", tmp_expr_ret);                                                      \
      return ::ErrorResult();                                                                                          \
    }                                                                                                                  \
  } while (0)

#define GE_ASSERT_RT_OK(expr)                                                                                          \
  do {                                                                                                                 \
    auto tmp_expr_ret = (expr);                                                                                        \
    if (tmp_expr_ret != 0) {                                                                                           \
      REPORT_INNER_ERROR("E19999", "Expect RT_ERROR_NONE, but get %d", tmp_expr_ret);                                  \
      GELOGE(ge::FAILED, "Expect RT_ERROR_NONE, but get %d", tmp_expr_ret);                                            \
      return ::ErrorResult();                                                                                          \
    }                                                                                                                  \
  } while (0)
#endif  //METADEF_CXX_INC_COMMON_CHECKER_H_
