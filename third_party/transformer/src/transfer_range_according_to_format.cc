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

#include "axis_util.h"
#include "transfer_range_according_to_format.h"
#include "expand_dimension.h"
#include "graph/types.h"
#include "debug/ge_log.h"
#include "debug/ge_util.h"
#include "framework/common/debug/ge_log.h"

namespace transformer {

bool RangeTransferAccordingToFormat::GetRangeAccordingToFormat(RangeAndFormat &range_and_format_info) {
  /* The default new range is old range */
  std::vector<int64_t> range_upper_old;
  std::vector<int64_t> range_low_old;
  std::vector<int64_t> range_upper_new;
  std::vector<int64_t> range_low_new;
  for (auto &i : range_and_format_info.old_range) {
    range_low_old.emplace_back(i.first);
    range_upper_old.emplace_back(i.second);
  }

  transformer::ShapeAndFormat shape_and_format_info_low {range_low_old, range_low_new, range_and_format_info.old_format,
      range_and_format_info.new_format, range_and_format_info.current_data_type, range_and_format_info.op_impl_type};
  transformer::ShapeAndFormat shape_and_format_info_upper {range_upper_old, range_upper_new, range_and_format_info.old_format,
      range_and_format_info.new_format, range_and_format_info.current_data_type, range_and_format_info.op_impl_type};  
  ShapeTransferAccordingToFormat shape_transfer;
  bool res = (shape_transfer.GetShapeAccordingToFormat(shape_and_format_info_low) &&
      shape_transfer.GetShapeAccordingToFormat(shape_and_format_info_upper));
  if (!res || (range_low_new.size() != range_upper_new.size())) {
    return false;
  }
  range_and_format_info.new_range.clear();
  for (size_t i = 0; i < range_and_format_info.new_range.size(); ++i) {
    range_and_format_info.new_range.emplace_back(range_low_new[i], range_upper_new[i]);
  }
  return res;
}
};  // namespace fe