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
#include "exe_graph/runtime/shape_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/math_util.h"
#include "inc/common/checker.h"

namespace gert {
const Shape g_vec_1_shape = {1};

ge::graphStatus CalcAlignedSizeByShape(const Shape &shape, ge::DataType data_type, uint64_t &ret_tensor_size) {
  constexpr uint64_t kAlignBytes = 32U;
  auto shape_size = shape.GetShapeSize();
  int64_t cal_size = 0;
  if (data_type == ge::DT_STRING) {
    uint32_t type_size = 0U;
    GE_ASSERT_TRUE(ge::TypeUtils::GetDataTypeLength(data_type, type_size));
    if (ge::MulOverflow(shape_size, static_cast<int64_t>(type_size), cal_size)) {
      GELOGE(ge::GRAPH_FAILED, "[Calc][TensorSizeByShape] shape_size[%ld] multiplied by type_size[%u] overflowed!",
             shape_size, type_size);
      return ge::GRAPH_FAILED;
    }
  } else {
    cal_size = ge::GetSizeInBytes(shape_size, data_type);
  }
  if (cal_size < 0) {
    GELOGE(ge::GRAPH_FAILED, "[Calc][TensorSizeByShape] shape_size[%" PRId64 "] data_type[%s] failed", shape_size,
           ge::TypeUtils::DataTypeToSerialString(data_type).c_str());
    return ge::GRAPH_FAILED;
  }

  // 不可能溢出，因为ret最大值也只有int64的最大值
  ret_tensor_size = ge::RoundUp(static_cast<uint64_t>(cal_size), kAlignBytes) + kAlignBytes;
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert