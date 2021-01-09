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

#include "transformer/inc/padding_dimension.h"
#include "transformer/inc/axis_name_util.h"
#include "transformer/inc/axis_util.h"
#include "transformer/inc/transfer_shape_according_to_format.h"
namespace common {
namespace transformer {
using namespace ge;
using namespace std;
#define DIMENSION_BITMAP(shift_num) (0x8 >> shift_num)


bool PadDimensionTo4UsingDefaultRule(const string& op_type, const ge::Format& format, const uint32_t& tensor_index,
                                     vector<int64_t>& dims, uint32_t old_dims_size) {
  if (dims.size() < DIM_DEFAULT_SIZE) {
    /* Default rules: */
    /* Insert 1 at the beginning and fill the tail with ones if there is
    * any position. If the original shape is 1d, we assume it is Axis C
    * For example:
    * {a} -> {1, a, 1, 1} (NCHW)   or {1, 1, 1, a} (NHWC);
    * {a, b} -> {1, a, b, 1};
    * {a, b, c} -> {1, a, b, c}
    * And after this operation, we will not change the shape by original
    * format */
    dims.insert(dims.begin(), SHAPE_DIM_DEFAULT_VALUE);
    uint32_t fill_in_times = DIM_DEFAULT_SIZE - dims.size();
    for (uint32_t i = 0; i < fill_in_times; i++) {
      dims.push_back(SHAPE_DIM_DEFAULT_VALUE);
    }
    if (old_dims_size != 1) {
      return true;
    }
  }
  
  /* For reshape type NC, we will do the following reshape-transpose like
   * operation. For example:
   * reshape_type is NC, original shape is {a,b}, original format is NCHW,
   * the final shape will be padded as {a, b, 1, 1}; But if the original format
   * is NHWC, the final shape will be {a, 1, 1, b}, the memory is the same,
   * because the padding value is 1. */
  std::vector<int64_t> new_shape_dims;
  if (dims.size() < DIM_DEFAULT_SIZE) {
    GELOGE(GRAPH_FAILED, "The size of dim can not be less than 4.");
    return false;
  }
  ge::Format old_format = ge::FORMAT_NCHW;
  ge::DataType dtype = ge::DT_FLOAT;
  ShapeAndFormat input_and_output_info = {
      dims, new_shape_dims, old_format, format, dtype, (int64_t)EN_IMPL_HW_TBE};
  ShapeTransferAccordingToFormat transfer;
  (void)transfer.GetShapeAccordingToFormat(input_and_output_info);
  /* if format is NHWC, we need to do shape transpose */
  dims = new_shape_dims;
  return false;
}

bool PadDimensionTo4(const std::string &op_type, const ge::Format &original_format,
                     const ge::Format &final_format, const uint32_t &tensor_index, const std::string &reshape_type_str,
                     std::vector<int64_t> &dims) {
  uint32_t reshape_type = AxisNameUtil::ReshapeTypeToUint(original_format, reshape_type_str);
  GELOGD("Reshape type of tensor %u of op %s is %u", tensor_index, op_type.c_str(), reshape_type);
  uint32_t old_dims_size = dims.size();
  vector<int64_t> new_dims;
  /* Shape of Nz format will not be padding to four */
  bool no_need_reshape_flag =
      (old_dims_size >= DIM_DEFAULT_SIZE || reshape_type == RESHAPE_FORBIDDEN ||
       final_format == ge::FORMAT_FRACTAL_NZ || (original_format == ge::FORMAT_ND && final_format == ge::FORMAT_FRACTAL_Z));
  if (no_need_reshape_flag) {
    return true;
  }

  if (reshape_type < RESHAPE_DEFAULT) {
    uint32_t index_in_old_dim_vec = 0;
    for (uint32_t i = 0; i < DIM_DEFAULT_SIZE; i++) {
      /* from 1,2,3D to 4D NCHW */
      if ((DIMENSION_BITMAP(i) & reshape_type) != 0 && index_in_old_dim_vec < old_dims_size) {
        new_dims.push_back(dims[index_in_old_dim_vec++]);
      } else {
        new_dims.push_back(1);
      }
    }
    dims.swap(new_dims);
  } else if (reshape_type == RESHAPE_CN || original_format == ge::FORMAT_HWCN) {
    /* Pad {a,b} as CN to NCHW {b, a, 1, 1} */
    if (dims.size() < 2) {
      GELOGD("oldDimsSize %zu is less than 2. Reshape type is %u", dims.size(), reshape_type);
      return true;
    }
    new_dims.push_back(1);
    new_dims.push_back(1);
    new_dims.push_back(dims[0]);
    new_dims.push_back(dims[1]);
    dims.swap(new_dims);
    /* In this case the final format must be HWCN,
     * we just return true */
    return true;
  } else {
    if (PadDimensionTo4UsingDefaultRule(op_type, original_format, tensor_index, dims, old_dims_size)) {
      return true;
    }
  }
  return true;
}
} // namespace transformer
} // namespace common
