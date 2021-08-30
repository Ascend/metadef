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

#ifndef COMMON_UTILS_TRANSFORMER_INC_EXPAND_DIMENSION_H_
#define COMMON_UTILS_TRANSFORMER_INC_EXPAND_DIMENSION_H_

#include <memory.h>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include "graph/types.h"
#include "axis_util.h"

namespace transformer {

static const int32_t CHWN_DIM_C = 0;
static const int32_t CHWN_DIM_H = 1;
static const int32_t CHWN_DIM_W = 2;
static const int32_t CHWN_DIM_N = 3;

const size_t DIMENSION_NUM_FOUR = 4;
const size_t DIMENSION_NUM_FIVE = 5;
const size_t DIMENSION_NUM_TWO = 2;
const std::string RESHAPE_TYPE_FORBIDDEN = "FORBIDDEN";

static const std::map<ge::Format, size_t> FULL_SIZE_OF_FORMAT {
    {ge::FORMAT_NCHW, DIMENSION_NUM_FOUR},
    {ge::FORMAT_NHWC, DIMENSION_NUM_FOUR},
    {ge::FORMAT_HWCN, DIMENSION_NUM_FOUR},
    {ge::FORMAT_CHWN, DIMENSION_NUM_FOUR},
    {ge::FORMAT_NDHWC, DIMENSION_NUM_FIVE},
    {ge::FORMAT_NCDHW, DIMENSION_NUM_FIVE},
    {ge::FORMAT_DHWCN, DIMENSION_NUM_FIVE},
    {ge::FORMAT_ND, DIMENSION_NUM_FOUR}
};

static const std::map<size_t, std::map<ge::Format, std::string>> DEFAULT_RESHAPE_TYPE {
    {0, {{ge::FORMAT_NCHW, ""}, {ge::FORMAT_NHWC, ""}, {ge::FORMAT_HWCN, ""}, {ge::FORMAT_CHWN, ""},
         {ge::FORMAT_NDHWC, ""}, {ge::FORMAT_NCDHW, ""}, {ge::FORMAT_DHWCN, ""}}},

    {1, {{ge::FORMAT_NCHW, "C"}, {ge::FORMAT_NHWC, "C"}, {ge::FORMAT_HWCN, "C"}, {ge::FORMAT_CHWN, "C"},
         {ge::FORMAT_NDHWC, "C"}, {ge::FORMAT_NCDHW, "C"}, {ge::FORMAT_DHWCN, "C"}}},

    {2, {{ge::FORMAT_NCHW, "CH"}, {ge::FORMAT_NHWC, "HW"}, {ge::FORMAT_HWCN, "CN"}, {ge::FORMAT_CHWN, "WN"},
         {ge::FORMAT_NDHWC, "WC"}, {ge::FORMAT_NCDHW, "HW"}, {ge::FORMAT_DHWCN, "CN"}}},

    {3, {{ge::FORMAT_NCHW, "CHW"}, {ge::FORMAT_NHWC, "HWC"}, {ge::FORMAT_HWCN, "WCN"}, {ge::FORMAT_CHWN, "HWN"},
         {ge::FORMAT_NDHWC, "HWC"}, {ge::FORMAT_NCDHW, "DHW"}, {ge::FORMAT_DHWCN, "WCN"}}},

    {4, {{ge::FORMAT_NDHWC, "DHWC"}, {ge::FORMAT_NCDHW, "CDHW"}, {ge::FORMAT_DHWCN, "HWCN"}}}
};

static const std::map<ge::Format, std::map<std::string, int32_t>> AXIS_INDEX_OF_FORMAT {
    {ge::FORMAT_NCHW, {{"N", AXIS_NCHW_DIM_N}, {"C", AXIS_NCHW_DIM_C}, {"H", AXIS_NCHW_DIM_H}, {"W", AXIS_NCHW_DIM_W}}},
    {ge::FORMAT_HWCN, {{"N", AXIS_HWCN_DIM_N}, {"C", AXIS_HWCN_DIM_C}, {"H", AXIS_HWCN_DIM_H}, {"W", AXIS_HWCN_DIM_W}}},
    {ge::FORMAT_NHWC, {{"N", AXIS_NHWC_DIM_N}, {"C", AXIS_NHWC_DIM_C}, {"H", AXIS_NHWC_DIM_H}, {"W", AXIS_NHWC_DIM_W}}},
    {ge::FORMAT_CHWN, {{"N", CHWN_DIM_N}, {"C", CHWN_DIM_C}, {"H", CHWN_DIM_H}, {"W", CHWN_DIM_W}}},
    {ge::FORMAT_NDHWC,
     {{"N", NDHWC_DIM_N}, {"C", NDHWC_DIM_C}, {"H", NDHWC_DIM_H}, {"W", NDHWC_DIM_W}, {"D", NDHWC_DIM_D}}},
    {ge::FORMAT_NCDHW,
     {{"N", NCDHW_DIM_N}, {"C", NCDHW_DIM_C}, {"H", NCDHW_DIM_H}, {"W", NCDHW_DIM_W}, {"D", NCDHW_DIM_D}}},
    {ge::FORMAT_DHWCN,
     {{"N", DHWCN_DIM_N}, {"C", DHWCN_DIM_C}, {"H", DHWCN_DIM_H}, {"W", DHWCN_DIM_W}, {"D", DHWCN_DIM_D}}},
    {ge::FORMAT_DHWNC,
     {{"N", DHWNC_DIM_N}, {"C", DHWNC_DIM_C}, {"H", DHWNC_DIM_H}, {"W", DHWNC_DIM_W}, {"D", DHWNC_DIM_D}}}
};

static const std::map<ge::Format, std::unordered_set<std::string>> ALL_VALID_RESHAPE_TYPE {
        {ge::FORMAT_NCHW, {
                              "N", "C", "H", "W",
                              "NC", "NH", "NW", "CH", "CW", "HW",
                              "NCH", "NCW", "NHW", "CHW"
                          }},
        {ge::FORMAT_NHWC, {
                              "N", "H", "W", "C",
                              "NH", "NW", "NC", "HW", "HC", "WC",
                              "NHW", "NHC", "NWC", "HWC"
                          }},
        {ge::FORMAT_HWCN, {
                              "H", "W", "C", "N",
                              "HW", "HC", "HN", "WC", "WN", "CN",
                              "HWC", "HWN", "HCN", "WCN"
                           }},
        {ge::FORMAT_CHWN, {
                              "C", "H", "W", "N",
                              "CH", "CW", "CN", "HW", "HN", "WN",
                              "CHW", "CHN", "CWN", "HWN"
                           }},
        {ge::FORMAT_NDHWC, {
                              "N", "D", "H", "W", "C",
                              "ND", "NH", "NW", "NC", "DH", "DW", "DC", "HW", "HC", "WC",
                              "NDH", "NDW", "NDC", "NHW", "NHC", "NWC", "DHW", "DHC", "DWC", "HWC",
                              "NDHW", "NDHC", "NDWC", "NHWC", "DHWC"
                           }},
        {ge::FORMAT_NCDHW, {
                               "N", "C", "D", "H", "W",
                               "NC", "ND", "NH", "NW", "CD", "CH", "CW", "DH", "DW", "HW",
                               "NCD", "NCH", "NCW", "NDH", "NDW", "NHW", "CDH", "CDW", "CHW", "DHW",
                               "NCDH", "NCDW", "NCHW", "NDHW", "CDHW"
                          }},
        {ge::FORMAT_DHWCN, {
                               "D", "H", "W", "C", "N",
                               "DH", "DW", "DC", "DN", "HW", "HC", "HN", "WC", "WN", "CN",
                               "DHW", "DHC", "DHN", "DWC", "DWN", "DCN", "HWC", "HWN", "HCN", "WCN",
                               "DHWC", "DHWN", "DHCN", "DWCN", "HWCN"
                         }}
};
/* Pad dimension according to reshape type */
bool ExpandDimension(const std::string &op_type, const ge::Format &original_format, const ge::Format &final_format,
                     const uint32_t &tensor_index, const std::string &reshape_type, std::vector<int64_t> &dims);

bool ExpandRangeDimension(const std::string &op_type, const ge::Format &original_format, const ge::Format &final_format,
                          const uint32_t &tensor_index, const std::string &reshape_type, std::vector<int64_t> &dims);

} // namespace transformer

#endif //COMMON_UTILS_TRANSFORMER_INC_EXPAND_DIMENSION_H_
