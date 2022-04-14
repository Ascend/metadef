/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef COMMON_UTILS_TRANSFORMER_INC_AXIS_UTIL_H_
#define COMMON_UTILS_TRANSFORMER_INC_AXIS_UTIL_H_

#include <memory.h>
#include <vector>
#include <array>

#include "external/graph/ge_error_codes.h"
#include "external/graph/types.h"
#include "graph/ge_tensor.h"

namespace transformer {
inline bool CheckInt64MulOverflow(int64_t m, int64_t n) {
  if (m > 0) {
    if (n > 0) {
      if (m > ((int64_t)INT64_MAX / n)) {
        return false;
      }
    } else {
      if (n < ((int64_t)INT64_MIN / m)) {
        return false;
      }
    }
  } else {
    if (n > 0) {
      if (m < ((int64_t)INT64_MIN / n)) {
        return false;
      }
    } else {
      if ((m != 0) && (n < ((int64_t)INT64_MAX / m))) {
        return false;
      }
    }
  }
  return true;
}

#define INT64_MULCHECK(a, b)                                                                      \
  if (CheckInt64MulOverflow((a), (b)) != true) {                                                  \
    return false;                                                                                 \
  }

#define CHECK_NOTNULL(val)                                       \
  do {                                                           \
    if ((val) == nullptr) {                                      \
      GELOGE(GRAPH_FAILED, "[ERROR]Parameter[%s] must not be null.", #val); \
      return false;                                              \
    }                                                            \
  } while (0)

#define CHECK(cond, log_func, return_expr) \
  do {                                     \
    if (cond) {                            \
      log_func;                            \
      return_expr;                         \
    }                                      \
  } while (0)

#define INT64_ZEROCHECK(a)                                                                            \
  if (a == 0) {                                                                                       \
    return false;                                                                                     \
  }

enum AxisValueType {
  AXIS_N = 0,
  AXIS_C = 1,
  AXIS_H = 2,
  AXIS_W = 3,
  AXIS_C1 = 4,
  AXIS_C0 = 5,
  AXIS_Co = 6,
  AXIS_D = 7,
  AXIS_G = 8,
  AXIS_INPUT_SIZE = 9,
  AXIS_HIDEEN_SIZE = 10,
  AXIS_STATE_SIZE = 11,
  AXIS_BOTTOM = 12
};

using AxisValue = std::array<int64_t, static_cast<size_t>(AXIS_BOTTOM)>;

inline int64_t DivisionCeiling(int64_t dividend, int64_t divisor) {
  if (divisor == 0) {
    return 0;
  } else if (dividend <= 0) {
    return dividend;
  } else {
    return (dividend + divisor - 1) / divisor;
  }
}

class AxisUtil {
 public:
  AxisUtil() {};
  ~AxisUtil() {};
  static bool GetAxisValueByOriginFormat(const ge::Format& format, const ge::GeShape &shape, AxisValue &axis_value);

private:
  static bool GetAxisValueByNCHW(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByNHWC(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByHWCN(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByND(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByNDHWC(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByNCDHW(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByDHWCN(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByDHWNC(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByNC1HWC0(const ge::GeShape &shape, AxisValue &axis_value);

  static bool GetAxisValueByC1HWNCoC0(const ge::GeShape &shape, AxisValue &axis_value);
};
} // namespace transformer

#endif // COMMON_UTILS_TRANSFORMER_INC_AXIS_UTIL_H_
