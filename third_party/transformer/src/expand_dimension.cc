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

#include "expand_dimension.h"
#include <unordered_set>
#include "framework/common/debug/ge_log.h"

namespace transformer {

bool GetDefaultReshapeType(const ge::Format &original_format, size_t old_dims_size, std::string &reshape_type) {
  auto rsp_tp_all_format = DEFAULT_RESHAPE_TYPE.find(old_dims_size);
  if (rsp_tp_all_format == DEFAULT_RESHAPE_TYPE.end()) {
    GELOGW("dim size %zu is invalid.", old_dims_size);
    return false;
  }

  auto iter_rsp_tp = rsp_tp_all_format->second.find(original_format);
  if (iter_rsp_tp == rsp_tp_all_format->second.end()) {
    GELOGW("Cannot find default reshape type for %u.", original_format);
    return false;
  }

  reshape_type = iter_rsp_tp->second;
  return true;
}

bool IsExpandNecessary(std::vector<int64_t> &dims, const ge::Format &original_format, const ge::Format &final_format,
                       const std::string &reshape_type, size_t &full_size) {
  /* 1. Check whether the old dim size is full. Full size is not necessary for expand. */
  size_t old_dims_size = dims.size();
  auto iter_full_size = FULL_SIZE_OF_FORMAT.find(original_format);
  if (iter_full_size == FULL_SIZE_OF_FORMAT.end()) {
    GELOGW("Original Format %u is invalid.", original_format);
    return false;
  } else {
    if (old_dims_size >= iter_full_size->second) {
      return false;
    }
  }
  /* 2. Check whether the final format does not need expanding demension. */
  bool no_need_reshape_flag = reshape_type == RESHAPE_TYPE_FORBIDDEN || final_format == ge::FORMAT_FRACTAL_NZ ||
                              (original_format == ge::FORMAT_ND && final_format == ge::FORMAT_FRACTAL_Z);
  if (no_need_reshape_flag) {
    return false;
  }
  full_size = iter_full_size->second;
  return true;
}

void ExpandByReshapeType(std::vector<int64_t> &dims, const std::string &op_type, const ge::Format &original_format,
                         size_t full_size, const uint32_t &tensor_index, const std::string &reshape_type) {
  GELOGD("Expand tensor %u of %s by reshape type %s.", tensor_index, op_type.c_str(), reshape_type.c_str());
  auto old_dims_size = dims.size();
  if (reshape_type == "CN") {
    /* If the reshape type is CN, we will consider the original format is HWCN. */
    std::vector<int64_t> new_dims;
    if (old_dims_size < DIMENSION_NUM_TWO) {
      GELOGW("old dims size %zu is less than 2. Reshape type is %s.", dims.size(), reshape_type.c_str());
      return;
    }
    new_dims.push_back(1);
    new_dims.push_back(1);
    new_dims.push_back(dims[0]);
    new_dims.push_back(dims[1]);
    dims.swap(new_dims);
    /* In this case the final format must be HWCN, we just return true */
    return;
  } else {
    /* Build a array with all 1 of full size. Then we will substitute some of the 1 with the original axis value. */
    std::vector<int64_t> new_dims;
    for (size_t i = 0; i < full_size; i++) {
      new_dims.emplace_back(1);
    }

    auto iter_axis_name_index = AXIS_INDEX_OF_FORMAT.find(original_format);
    if (iter_axis_name_index == AXIS_INDEX_OF_FORMAT.end()) {
      GELOGW("Cannot find axis index name map value of original format %u of tensor %u of %s.",
             original_format, tensor_index, op_type.c_str());
      return;
    }
    for (size_t i = 0; i < old_dims_size; i++) {
      /* The length of reshape type is larger than the dims. */
      std::string axis_str(1, reshape_type.at(i));
      auto iter_axis_index = iter_axis_name_index->second.find(axis_str);
      if (iter_axis_index == iter_axis_name_index->second.end()) {
        GELOGW("Invalid reshape type %s for tensor %u of %s.", reshape_type.c_str(), tensor_index, op_type.c_str());
        return;
      }
      int32_t index = iter_axis_index->second;
      if (index < 0 || index >= (int32_t)full_size) {
        GELOGW("Index of %s is %d which is larger than the full size %zu.", axis_str.c_str(), index, full_size);
        return;
      }
      new_dims[index] = dims[i];
    }
    dims.swap(new_dims);
  }
}

bool ExpandDimension(const std::string &op_type, const ge::Format &original_format, const ge::Format &final_format,
                     const uint32_t &tensor_index, const std::string &reshape_type, std::vector<int64_t> &dims) {
  /* 1. Check expanding necessary. */
  size_t full_size = 0;
  if (!IsExpandNecessary(dims, original_format, final_format, reshape_type, full_size)) {
    return true;
  }

  /* 2. Check whether the reshape type is consistent with the original format.
   * If not consistent, just return and report a warning. */
  std::string valid_reshape_type = reshape_type;
  size_t old_dims_size = dims.size();
  auto iter_format = ALL_VALID_RESHAPE_TYPE.find(original_format);
  if (iter_format != ALL_VALID_RESHAPE_TYPE.end()) {
    auto iter_reshape_type = iter_format->second.find(reshape_type);
    if (iter_reshape_type == iter_format->second.end()) {
      if (!GetDefaultReshapeType(original_format, old_dims_size, valid_reshape_type)) {
        return true;
      }
      GELOGI("Get default reshape type %s for op %s tensor %u original format %u is invalid.",
             valid_reshape_type.c_str(), op_type.c_str(), tensor_index, original_format);
    }
  }

  /* 3. Check whether the dimension of original shape is less than or equal to
   * the length of reshape type. If the dimension of original shape if larger,
   * we cannot find suitable posotion for all axis in original shape and we just return. */
  if (old_dims_size > valid_reshape_type.length()) {
    GELOGW("Dimension %zu of tensor %u of %s is larger than the length of reshape type which is %zu.",
           old_dims_size, tensor_index, op_type.c_str(), valid_reshape_type.length());
    return true;
  }

  /* 4. Expand dimension. */
  ExpandByReshapeType(dims, op_type, original_format, full_size, tensor_index, valid_reshape_type);
  return true;
}

bool ExpandRangeDimension(const std::string &op_type, const ge::Format &original_format,
    const ge::Format &final_format, const uint32_t &tensor_index, const std::string &reshape_type,
    std::vector<std::pair<int64_t, int64_t>> &ranges) {
  std::vector<int64_t> range_upper;
  std::vector<int64_t> range_low;
  for (auto &i : ranges) {
    range_low.emplace_back(i.first);
    range_upper.emplace_back(i.second);
  }
  bool res = ExpandDimension(op_type, original_format, final_format, tensor_index, reshape_type, range_low) &&
      ExpandDimension(op_type, original_format, final_format, tensor_index, reshape_type, range_upper);
  if (!res || (range_low.size() != range_upper.size())) {
    return false;
  }
  ranges.clear();
  for (size_t idx = 0; idx < range_low.size(); ++idx) {
    ranges.emplace_back(std::pair<int64_t, int64_t>(range_low[idx], range_upper[idx]));
  }
  return res;
}
} // namespace transformer
