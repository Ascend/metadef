/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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
#ifndef COMMON_UTILS_TRANSFER_AXIS_NAME_UTIL_H_
#define COMMON_UTILS_TRANSFER_AXIS_NAME_UTIL_H_
#include <functional>
#include <vector>
#include <memory.h>
#include "external/graph/types.h"
#include "framework/common/debug/ge_log.h"

namespace common {
namespace transformer {
enum ReshapeType {
  RESHAPE_N = 0x8,        // 1000, {9} -> {9,1,1,1}
  RESHAPE_C = 0x4,        // 0100,        {1,9,1,1}
  RESHAPE_H = 0x2,        // 0010,
  RESHAPE_W = 0x1,        // 0001,
  RESHAPE_NC = 0xC,       // 1100,
  RESHAPE_NH = 0xA,       // 1010,
  RESHAPE_NW = 0x9,       // 1001,
  RESHAPE_CH = 0x6,       // 0110,
  RESHAPE_CW = 0x5,       // 0101, {2,3}-> {1,2,1,3}
  RESHAPE_HW = 0x3,       // 0011,
  RESHAPE_NCH = 0xE,      // 1110,
  RESHAPE_NCW = 0xD,      // 1101,
  RESHAPE_NHW = 0xB,      // 1011,
  RESHAPE_CHW = 0x7,      // 0111
  RESHAPE_DEFAULT = 0xF,  // use default reules
  RESHAPE_CN = 0x10,
  RESHAPE_FORBIDDEN = 0xFFFFFFFF  // Forbid Reshaping
};
using ReshapeTypeEnumUint32 = uint32_t;

enum ReshapeTypeNHWC {
  RESHAPE_NHWC_N = 0x8,                // 1000, {9} -> {9,1,1,1}
  RESHAPE_NHWC_H = 0x4,                // 0100,        {1,9,1,1}
  RESHAPE_NHWC_W = 0x2,                // 0010,
  RESHAPE_NHWC_C = 0x1,                // 0001,
  RESHAPE_NHWC_NH = 0xC,               // 1100,
  RESHAPE_NHWC_NW = 0xA,               // 1010,
  RESHAPE_NHWC_NC = 0x9,               // 1001,
  RESHAPE_NHWC_HW = 0x6,               // 0110,
  RESHAPE_NHWC_HC = 0x5,               // 0101, {2,3}-> {1,2,1,3}
  RESHAPE_NHWC_WC = 0x3,               // 0011,
  RESHAPE_NHWC_NHW = 0xE,              // 1110,
  RESHAPE_NHWC_NHC = 0xD,              // 1101,
  RESHAPE_NHWC_NWC = 0xB,              // 1011,
  RESHAPE_NHWC_HWC = 0x7,              // 0111
  RESHAPE_NHWC_DEFAULT = 0xF,          // use default reules
  RESHAPE_NHWC_FORBIDDEN = 0xFFFFFFFF  // Forbid Reshaping
};

static const std::map<std::string, ReshapeTypeEnumUint32> RESHAPE_TYPE_MAP{
  {"N", RESHAPE_N}, {"C", RESHAPE_C}, {"H", RESHAPE_H}, {"W", RESHAPE_W}, {"NC", RESHAPE_NC},
  {"NH", RESHAPE_NH}, {"NW", RESHAPE_NW}, {"CH", RESHAPE_CH}, {"CW", RESHAPE_CW}, {"HW", RESHAPE_HW},
  {"NCH", RESHAPE_NCH}, {"NCW", RESHAPE_NCW}, {"NHW", RESHAPE_NHW}, {"CHW", RESHAPE_CHW}, {"CN", RESHAPE_CN},
  {"X", RESHAPE_FORBIDDEN}, {"FORBIDDEN", RESHAPE_FORBIDDEN}};

static const std::map<std::string, ReshapeTypeEnumUint32> NHWC_RESHAPE_TYPE_MAP{
    {"N", RESHAPE_NHWC_N},       {"H", RESHAPE_NHWC_H},
    {"W", RESHAPE_NHWC_W},       {"C", RESHAPE_NHWC_C},
    {"NH", RESHAPE_NHWC_NH},     {"NW", RESHAPE_NHWC_NW},
    {"NC", RESHAPE_NHWC_NC},     {"HW", RESHAPE_NHWC_HW},
    {"HC", RESHAPE_NHWC_HC},     {"WC", RESHAPE_NHWC_WC},
    {"NHW", RESHAPE_NHWC_NHW},   {"NHC", RESHAPE_NHWC_NHC},
    {"NWC", RESHAPE_NHWC_NWC},   {"HWC", RESHAPE_NHWC_HWC},
    {"X", RESHAPE_NHWC_DEFAULT}, {"FORBIDDEN", RESHAPE_NHWC_FORBIDDEN}};

static const std::map<ge::Format, std::map<std::string, ReshapeTypeEnumUint32>> ALL_RESHAPE_TYPE_MAP{
    {ge::FORMAT_NCHW, RESHAPE_TYPE_MAP},
    {ge::FORMAT_NHWC, NHWC_RESHAPE_TYPE_MAP},
    {ge::FORMAT_HWCN, RESHAPE_TYPE_MAP},
    {ge::FORMAT_CHWN, RESHAPE_TYPE_MAP},
};

/* Axis value is arranged as {N,C,H,W,C1,C0,...} */
/* The first parameter is old shape's dimension,
 * second is c0 and third is axis value. */
using GetAxisNameByAxisValueInfo = std::function<bool(std::vector<int64_t> &, std::vector<std::string> &)>;

using GetAxisNameByAxisValueInfoPtr = std::shared_ptr<GetAxisNameByAxisValueInfo>;

class AxisNameUtil {
 public:
  static uint32_t ReshapeTypeToUint(const ge::Format &format, const std::string &reshape_type);

};
} // namespace transformer
} // namespace common
#endif //COMMON_UTILS_TRANSFER_AXIS_NAME_UTIL_H_
