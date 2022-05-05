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

#include "transfer_shape_according_to_format.h"
#include <algorithm>
#include <set>
#include <vector>
#include "axis_constants.h"
#include "graph/utils/attr_utils.h"

namespace transformer {
namespace {
  const int64_t SHAPE_NUMBER_16 = 16;
  const int64_t SHAPE_NUMBER_32 = 32;
  const int64_t SHAPE_NUMBER_64 = 64;
  const int64_t SHAPE_NUMBER_128 = 128;
  const int64_t SHAPE_NUMBER_256 = 256;
  const int64_t SHAPE_DIM_VALUE_C04 = 4;
  const int64_t NI = 16;
  const int64_t GROUPS_DEFAULT_VALUE = 1;
  const int64_t UNKNOWN_SHAPE_VALUE = -1;
  const int64_t RNN_STATE_SIZE_DEFAULT_VALUE = -1;
  const int64_t LSTM_NI = 4;
  const int64_t X0 = 16;

  const size_t MINUS_VALUE_ONE = 1;
  const size_t MINUS_VALUE_TWO = 2;
  const size_t SIZE_OF_CN = 2;
  const size_t MINIMUM_NZ_SHAPE_DIM_NUM = 2;
  const size_t MINIMUM_ND_TO_RNN_SHAPE_NUM = 2;

  const std::string kAttrHiddenSize = "hidden_size";
  const std::string kAttrInputSize = "input_size";
  const std::string kAttrStateSize = "state_size";

  const std::set<ge::Format> kOriginFormatVec = {
          ge::FORMAT_NCHW,  ge::FORMAT_NHWC,  ge::FORMAT_HWCN,
          ge::FORMAT_CHWN,  ge::FORMAT_NDHWC, ge::FORMAT_NCDHW,
          ge::FORMAT_DHWCN, ge::FORMAT_DHWNC, ge::FORMAT_ND
  };

  const std::vector<int64_t> kDataTypeAndC0Vec = {
          SHAPE_NUMBER_16,  // DT_FLOAT = 0,
          SHAPE_NUMBER_16,  // DT_FLOAT16 = 1,
          SHAPE_NUMBER_32,  // DT_INT8 = 2,
          SHAPE_NUMBER_16,  // DT_INT32 = 3,
          SHAPE_NUMBER_32,  // DT_UINT8 = 4,
          SHAPE_NUMBER_16,  // None = 5
          SHAPE_NUMBER_16,  // DT_INT16 = 6,
          SHAPE_NUMBER_16,  // DT_UINT16 = 7,
          SHAPE_NUMBER_16,  // DT_UINT32 = 8,
          SHAPE_NUMBER_16,  // DT_INT64 = 9,
          SHAPE_NUMBER_16,  // DT_UINT64 = 10,
          SHAPE_NUMBER_16,  // DT_DOUBLE = 11,
          SHAPE_NUMBER_16,  // DT_BOOL = 12,
          SHAPE_NUMBER_16,  // DT_DUAL = 13,
          SHAPE_NUMBER_16,  // DT_DUAL_SUB_INT8 = 14,
          SHAPE_NUMBER_16,  // DT_DUAL_SUB_UINT8 = 15,
          SHAPE_NUMBER_16,  // DT_COMPLEX64 = 16,
          SHAPE_NUMBER_16,  // DT_COMPLEX128 = 17,
          SHAPE_NUMBER_16,  // DT_QINT8 = 18,
          SHAPE_NUMBER_16,  // DT_QINT16 = 19,
          SHAPE_NUMBER_16,  // DT_QINT32 = 20,
          SHAPE_NUMBER_16,  // DT_QUINT8 = 21,
          SHAPE_NUMBER_16,  // DT_QUINT16 = 22,
          SHAPE_NUMBER_16,  // DT_RESOURCE = 23,
          SHAPE_NUMBER_16,  // DT_STRING_REF = 24,
          SHAPE_NUMBER_16,  // DT_DUAL = 25,
          SHAPE_NUMBER_16,  // DT_VARIANT = 26,
          SHAPE_NUMBER_16,  // DT_BF16 = 27,
          SHAPE_NUMBER_16,  // DT_UNDEFINED,
          SHAPE_NUMBER_64,  // DT_INT4 = 29,
          SHAPE_NUMBER_256, // DT_UINT1 = 30
          SHAPE_NUMBER_128, // DT_INT2 = 31
          SHAPE_NUMBER_128  // DT_UINT2 = 32
  };

  inline int64_t GetGreatestCommonDivisor(int64_t x, int64_t y) {
    if (y == 0) {
      return x;
    }
    int64_t z = y;
    while (x % y != 0) {
      z = x % y;
      x = y;
      y = z;
    }
    return z;
  }

  inline int64_t GetLeastCommonMultiple(int64_t x, int64_t y) {
    if (x == 0 || y == 0) {
      return 0;
    }
    return (x * y) / GetGreatestCommonDivisor(x, y);
  }

  inline int64_t GetAsisEnlargeValue(int64_t cin, int64_t cout, int64_t c0, int64_t group) {
    if (cin == 0 || cout == 0) {
      return 0;
    }

    return std::min(GetLeastCommonMultiple(c0 / GetGreatestCommonDivisor(cin, c0),
        NI / GetGreatestCommonDivisor(cout, NI)), group);
  }

  void GeShapeToRtShape(const ge::GeShape &ge_shape, gert::Shape &rt_shape) {
    rt_shape.SetDimNum(0);
    for (size_t i = 0; i < ge_shape.GetDimNum(); ++i) {
      rt_shape.AppendDim(ge_shape.GetDim(i));
    }
  }

  void RtShapeToGeShape(const gert::Shape &rt_shape, ge::GeShape &ge_shape) {
    ge_shape.SetDimNum(0);
    for (size_t i = 0; i < rt_shape.GetDimNum(); ++i) {
      ge_shape.AppendDim(rt_shape.GetDim(i));
    }
  }
}

ShapeTransferAccordingToFormat::ShapeTransferAccordingToFormat() {}

bool ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(const ge::OpDescPtr &op_desc,
                                                               ShapeAndFormat &shapeAndFormatInfo) {
  gert::Shape shape;
  GeShapeToRtShape(shapeAndFormatInfo.oldShape, shape);
  bool ret = TransferShape(shapeAndFormatInfo.oldFormat, shapeAndFormatInfo.newFormat,
                           shapeAndFormatInfo.currentDataType, shape, op_desc);
  RtShapeToGeShape(shape, shapeAndFormatInfo.oldShape);
  return ret;
}

bool ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(ShapeAndFormat &shapeAndFormatInfo) {
  gert::Shape shape;
  GeShapeToRtShape(shapeAndFormatInfo.oldShape, shape);
  bool ret = TransferShape(shapeAndFormatInfo.oldFormat, shapeAndFormatInfo.newFormat,
                           shapeAndFormatInfo.currentDataType, shape);
  RtShapeToGeShape(shape, shapeAndFormatInfo.oldShape);
  return ret;
}

bool ShapeTransferAccordingToFormat::TransferShape(const ge::Format &origin_format, const ge::Format &format,
                                                   const ge::DataType &data_type, gert::Shape &shape,
                                                   const ge::OpDescPtr op_desc) {
  GELOGD("Original format is %u, new format %u", origin_format, format);
  if (!IsNeedTransferShape(origin_format, format, shape)) {
    return true;
  }

  ge::Format primary_format = static_cast<ge::Format>(GetPrimaryFormat(format));
  if (!CheckInputParam(origin_format, primary_format, data_type)) {
    return false;
  }

  AxisValue axis_value;
  axis_value.fill(1);
  int64_t group = static_cast<int64_t>(ge::GetSubFormat(format));
  if (group > GROUPS_DEFAULT_VALUE) {
    axis_value[AXIS_G] = group;
  }

  axis_value[AXIS_C0] = GetC0Value(data_type, primary_format);

  FillupRnnAxisValue(op_desc, primary_format, axis_value);

  if (!IsNeedAxisValue(primary_format, shape.GetDimNum())) {
    return TransferShapeByFormat(primary_format, axis_value, shape);
  }

  if (!AxisUtil::GetAxisValueByOriginFormat(origin_format, shape, axis_value)) {
    return true;
  }

  return TransferShapeByFormat(primary_format, axis_value, shape);
}

bool ShapeTransferAccordingToFormat::IsNeedTransferShape(const ge::Format &origin_format, const ge::Format &format,
                                                         const gert::Shape &shape) {
  if (origin_format == ge::FORMAT_ND && kOriginFormatVec.count(format) > 0) {
    GELOGD("Do not need to do shape transformation from ND to original format.");
    return false;
  }

  if (shape.IsScalar()) {
    GELOGD("Do not need to do shape transformation if the shape is scalar.");
    return false;
  }
  return true;
}

void ShapeTransferAccordingToFormat::FillupRnnAxisValue(const ge::OpDescPtr &op_desc, const ge::Format &format,
                                                        AxisValue &axis_value) {
  if (format != ge::FORMAT_FRACTAL_ZN_RNN && format != ge::FORMAT_ND_RNN_BIAS) {
    return;
  }
  int64_t input_size = 1;
  int64_t hidden_size = -1;
  int64_t state_size = -1;
  if (op_desc != nullptr) {
    (void)ge::AttrUtils::GetInt(op_desc, kAttrHiddenSize, hidden_size);
    (void)ge::AttrUtils::GetInt(op_desc, kAttrInputSize, input_size);
    (void)ge::AttrUtils::GetInt(op_desc, kAttrStateSize, state_size);
  }

  axis_value[AXIS_INPUT_SIZE] = input_size;
  axis_value[AXIS_HIDEEN_SIZE] = hidden_size;
  axis_value[AXIS_STATE_SIZE] = state_size;
}

bool ShapeTransferAccordingToFormat::IsNeedAxisValue(const ge::Format &format, const size_t &origin_dim_size) {
  if (format == ge::FORMAT_FRACTAL_NZ || format == ge::FORMAT_FRACTAL_ZN_RNN || format == ge::FORMAT_ND_RNN_BIAS) {
    return false;
  }
  if (format == ge::FORMAT_FRACTAL_Z && origin_dim_size == SIZE_OF_CN) {
    return false;
  }
  return true;
}

bool ShapeTransferAccordingToFormat::TransferShapeByFormat(const ge::Format &primary_format,
                                                           const AxisValue &axis_value,
                                                           gert::Shape &shape) {
  switch (primary_format) {
    case ge::FORMAT_NCHW:
      return GetNCHWShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_NHWC:
      return GetNHWCShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_HWCN:
      return GetHWCNShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_CHWN:
      return GetCHWNShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_NC1HWC0:
    case ge::FORMAT_NC1HWC0_C04:
      return GetNC1HWC0ShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_NDC1HWC0:
      return GetNDC1HWC0ShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_C1HWNCoC0:
      return GetC1HWNCoC0ShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_FRACTAL_Z:
      return GetFzShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_FRACTAL_Z_C04:
      return GetFzC04ShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_FRACTAL_Z_G:
      return GetFzGShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_FRACTAL_Z_3D:
      return GetFz3DShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_FRACTAL_Z_3D_TRANSPOSE:
      return GetFz3DTransposeShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_FRACTAL_NZ:
      return GetNzShapeByAxisValue(axis_value, shape); // need c0
    case ge::FORMAT_FRACTAL_ZN_LSTM:
      return GetFzLstmShapeByAxisValue(axis_value, shape);
    case ge::FORMAT_FRACTAL_ZN_RNN:
      return GetFznRNNShapeByAxisValue(axis_value, shape); // need c0, input, hidden, state
    case ge::FORMAT_ND_RNN_BIAS:
      return GetNDRNNShapeByAxisValue(axis_value, shape); // need c0, input, hidden, state
    default:
      GELOGD("Can not get new shape by new format %d.", primary_format);
      return true;
  }
}

bool ShapeTransferAccordingToFormat::CheckInputParam(const ge::Format &origin_format,
                                                     const ge::Format &primary_format,
                                                     const ge::DataType &data_type) {
  bool invalid_format = (origin_format == ge::FORMAT_RESERVED || origin_format >= ge::FORMAT_END) ||
                        (primary_format == ge::FORMAT_RESERVED || primary_format >= ge::FORMAT_END);
  if (invalid_format) {
    GELOGE(ge::GRAPH_FAILED, "Old format %u or new format %u is invalid!", origin_format, primary_format);
    return false;
  }

  if (data_type == ge::DT_UNDEFINED || data_type >= ge::DT_MAX) {
    GELOGE(ge::GRAPH_FAILED, "currentDataType %u is invalid!", origin_format);
    return false;
  }

  return true;
}

int64_t ShapeTransferAccordingToFormat::GetC0Value(const ge::DataType &data_type, const ge::Format &format) {
  // The value of C0 should be 4 while format is 5HD-4 or FRAZ-4
  if (format == ge::FORMAT_NC1HWC0_C04) {
    return SHAPE_DIM_VALUE_C04;
  }

  if (static_cast<size_t>(data_type) < kDataTypeAndC0Vec.size()) {
    return kDataTypeAndC0Vec[static_cast<size_t>(data_type)];
  }
  return SHAPE_NUMBER_16;
}

bool ShapeTransferAccordingToFormat::GetNDC1HWC0ShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  shape.SetDimNum(DIM_SIZE_SIX);
  shape.SetDim(0, axis_value[AXIS_N]);
  shape.SetDim(1, axis_value[AXIS_D]);
  shape.SetDim(2, axis_value[AXIS_C1]);
  shape.SetDim(3, axis_value[AXIS_H]);
  shape.SetDim(4, axis_value[AXIS_W]);
  shape.SetDim(5, axis_value[AXIS_C0]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetNCHWShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, axis_value[AXIS_N]);
  shape.SetDim(1, axis_value[AXIS_C]);
  shape.SetDim(2, axis_value[AXIS_H]);
  shape.SetDim(3, axis_value[AXIS_W]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetNHWCShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, axis_value[AXIS_N]);
  shape.SetDim(1, axis_value[AXIS_H]);
  shape.SetDim(2, axis_value[AXIS_W]);
  shape.SetDim(3, axis_value[AXIS_C]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetNC1HWC0ShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  shape.SetDimNum(DIM_SIZE_FIVE);
  shape.SetDim(0, axis_value[AXIS_N]);
  shape.SetDim(1, axis_value[AXIS_C1]);
  shape.SetDim(2, axis_value[AXIS_H]);
  shape.SetDim(3, axis_value[AXIS_W]);
  shape.SetDim(4, axis_value[AXIS_C0]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetFzShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  size_t size_of_original_vec = shape.GetDimNum();
  if (size_of_original_vec == SIZE_OF_CN) {
    /* size_of_original_vec - 1 mean the last value of original vec
     * size_of_original_vec - 2 mean the second last value of original vec */
    shape.SetDim((size_of_original_vec - MINUS_VALUE_ONE),
        DivisionCeiling(shape.GetDim(size_of_original_vec - MINUS_VALUE_ONE), SHAPE_NUMBER_16));
    shape.SetDim((size_of_original_vec - MINUS_VALUE_TWO),
        DivisionCeiling(shape.GetDim(size_of_original_vec - MINUS_VALUE_TWO), axis_value[AXIS_C0]));
    shape.AppendDim(SHAPE_NUMBER_16);
    shape.AppendDim(axis_value[AXIS_C0]);
    return true;
  }
  return GetFz3DShapeByAxisValue(axis_value, shape);
}

bool ShapeTransferAccordingToFormat::GetHWCNShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, axis_value[AXIS_H]);
  shape.SetDim(1, axis_value[AXIS_W]);
  shape.SetDim(2, axis_value[AXIS_C]);
  shape.SetDim(3, axis_value[AXIS_N]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetC1HWNCoC0ShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  shape.SetDimNum(DIM_SIZE_SIX);
  shape.SetDim(0, axis_value[AXIS_C1]);
  shape.SetDim(1, axis_value[AXIS_H]);
  shape.SetDim(2, axis_value[AXIS_W]);
  shape.SetDim(3, axis_value[AXIS_N]);
  shape.SetDim(4, axis_value[AXIS_Co]);
  shape.SetDim(5, axis_value[AXIS_C0]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetNzShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  CHECK(shape.IsScalar(), GELOGD("Origin shape is empty!"), return true);
  size_t size_of_original_vec = shape.GetDimNum();
  if (size_of_original_vec < MINIMUM_NZ_SHAPE_DIM_NUM) {
    GELOGD("nd_value's dim num is less than 2!");
    return true;
  }
  /* size_of_original_vec - 1 mean the last value of original vec
   * size_of_original_vec - 2 mean the second last value of original vec */
  int64_t dim_back_two = shape.GetDim(size_of_original_vec - MINUS_VALUE_TWO);
  int64_t dim_back_one = shape.GetDim(size_of_original_vec - MINUS_VALUE_ONE);
  shape.SetDim((size_of_original_vec - MINUS_VALUE_ONE), DivisionCeiling(dim_back_two, SHAPE_NUMBER_16));

  shape.SetDim((size_of_original_vec - MINUS_VALUE_TWO), DivisionCeiling(dim_back_one, axis_value[AXIS_C0]));
  shape.AppendDim(SHAPE_NUMBER_16);
  shape.AppendDim(axis_value[AXIS_C0]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetFznRNNShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  size_t origin_shape_size = shape.GetDimNum();
  CHECK(origin_shape_size < MINIMUM_ND_TO_RNN_SHAPE_NUM, GELOGW("ndValue's dim num is less than 2!"), return true);

  /* check nd shape value */
  int64_t k_num;
  int64_t n_num;
  int64_t k_value = shape.GetDim(origin_shape_size - MINUS_VALUE_TWO);
  int64_t hidden_or_state_size = axis_value[AXIS_HIDEEN_SIZE];
  if (axis_value[AXIS_STATE_SIZE] != RNN_STATE_SIZE_DEFAULT_VALUE) {
    hidden_or_state_size = axis_value[AXIS_STATE_SIZE];
  }
  if (k_value == hidden_or_state_size + axis_value[AXIS_INPUT_SIZE]) {
    k_num = 2; // use input size and hidden size
  } else if (k_value == hidden_or_state_size || k_value == axis_value[AXIS_INPUT_SIZE]) {
    k_num = 1; // only use hidden size or input size
  } else {
    return true;
  }
  INT64_ZEROCHECK(axis_value[AXIS_HIDEEN_SIZE]);
  int64_t n_value = shape.GetDim(origin_shape_size - MINUS_VALUE_ONE);
  n_num = n_value / axis_value[AXIS_HIDEEN_SIZE];
  if (k_num == 1) {
    shape.SetDim(origin_shape_size - MINUS_VALUE_TWO, DivisionCeiling(k_value, SHAPE_NUMBER_16));
  } else {
    shape.SetDim(origin_shape_size - MINUS_VALUE_TWO,
                 DivisionCeiling(axis_value[AXIS_INPUT_SIZE], SHAPE_NUMBER_16) +
                 DivisionCeiling(hidden_or_state_size, SHAPE_NUMBER_16));
  }
  MUL_OVERFLOW(n_num, DivisionCeiling(axis_value[AXIS_HIDEEN_SIZE], axis_value[AXIS_C0]), n_num);
  shape.SetDim(origin_shape_size - MINUS_VALUE_ONE, n_num);
  shape.AppendDim(SHAPE_NUMBER_16);
  shape.AppendDim(axis_value[AXIS_C0]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetNDRNNShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  CHECK(axis_value[AXIS_HIDEEN_SIZE] == 0, GELOGD("hidden_size is zero"), return true);
  size_t size_of_original_vec = shape.GetDimNum();
  /* check nd shape value */
  int64_t n_num = shape.GetDim(size_of_original_vec - MINUS_VALUE_ONE) / axis_value[AXIS_HIDEEN_SIZE];
  MUL_OVERFLOW(n_num, DivisionCeiling(axis_value[AXIS_HIDEEN_SIZE], axis_value[AXIS_C0]), n_num);
  MUL_OVERFLOW(n_num, axis_value[AXIS_C0], n_num);
  shape.SetDim(size_of_original_vec - MINUS_VALUE_ONE, n_num);
  return true;
}

bool ShapeTransferAccordingToFormat::GetCHWNShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, axis_value[AXIS_C]);
  shape.SetDim(1, axis_value[AXIS_H]);
  shape.SetDim(2, axis_value[AXIS_W]);
  shape.SetDim(3, axis_value[AXIS_N]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetFz3DShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  bool has_unknown_shape = axis_value[AXIS_D] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_H] == UNKNOWN_SHAPE_VALUE ||
                           axis_value[AXIS_W] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_C] == UNKNOWN_SHAPE_VALUE ||
                           axis_value[AXIS_N] == UNKNOWN_SHAPE_VALUE;
  int64_t gdhwc1 = UNKNOWN_SHAPE_VALUE;
  int64_t axis_g_val = GROUPS_DEFAULT_VALUE;
  int64_t axis_n_val = axis_value[AXIS_N];
  int64_t axis_c_val = axis_value[AXIS_C];
  int64_t axis_c1_val = axis_value[AXIS_C1];
  if (!has_unknown_shape) {
    if (axis_value[AXIS_G] > GROUPS_DEFAULT_VALUE && axis_n_val >= axis_value[AXIS_G]) {
      axis_n_val = axis_n_val / axis_value[AXIS_G];
      int64_t enlarge_value = GetAsisEnlargeValue(axis_c_val, axis_n_val, axis_value[AXIS_C0], axis_value[AXIS_G]);
      axis_g_val = DivisionCeiling(axis_value[AXIS_G], enlarge_value);
      MUL_OVERFLOW(axis_c_val, enlarge_value, axis_c_val);
      MUL_OVERFLOW(axis_n_val, enlarge_value, axis_n_val);
      axis_c1_val = DivisionCeiling(axis_c_val, axis_value[AXIS_C0]);
    }
    MUL_OVERFLOW(axis_g_val, axis_c1_val, gdhwc1);
    MUL_OVERFLOW(gdhwc1, axis_value[AXIS_D], gdhwc1);
    MUL_OVERFLOW(gdhwc1, axis_value[AXIS_H], gdhwc1);
    MUL_OVERFLOW(gdhwc1, axis_value[AXIS_W], gdhwc1);
  }
  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, gdhwc1);
  shape.SetDim(1, DivisionCeiling(axis_n_val, NI));
  shape.SetDim(2, NI);
  shape.SetDim(3, axis_value[AXIS_C0]);
  return true;
}

bool ShapeTransferAccordingToFormat::GetFz3DTransposeShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  int64_t dhwn1 = UNKNOWN_SHAPE_VALUE;
  if (axis_value[AXIS_N] != UNKNOWN_SHAPE_VALUE && axis_value[AXIS_H] != UNKNOWN_SHAPE_VALUE &&
      axis_value[AXIS_W] != UNKNOWN_SHAPE_VALUE && axis_value[AXIS_D] != UNKNOWN_SHAPE_VALUE) {
    dhwn1 = DivisionCeiling(axis_value[AXIS_N], NI);
    MUL_OVERFLOW(dhwn1, axis_value[AXIS_D], dhwn1);
    MUL_OVERFLOW(dhwn1, axis_value[AXIS_H], dhwn1);
    MUL_OVERFLOW(dhwn1, axis_value[AXIS_W], dhwn1);
  }

  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, dhwn1);
  if (axis_value[AXIS_C] == UNKNOWN_SHAPE_VALUE) {
    shape.SetDim(1, UNKNOWN_SHAPE_VALUE);
  } else {
    shape.SetDim(1, axis_value[AXIS_C1]);
  }
  shape.SetDim(2, NI);
  shape.SetDim(3, axis_value[AXIS_C0]);

  return true;
}

bool ShapeTransferAccordingToFormat::GetFzLstmShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  int64_t h = axis_value[AXIS_N] / LSTM_NI;
  int64_t i = axis_value[AXIS_C] - h;
  int64_t first_element_of_fz_lstm = DivisionCeiling(i, NI) + DivisionCeiling(h, NI);
  int64_t second_element_of_fz_lstm = DivisionCeiling(h, NI);
  MUL_OVERFLOW(second_element_of_fz_lstm, LSTM_NI, second_element_of_fz_lstm);
  if (axis_value[AXIS_N] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_C] == UNKNOWN_SHAPE_VALUE) {
    first_element_of_fz_lstm = UNKNOWN_SHAPE_VALUE;
    second_element_of_fz_lstm = UNKNOWN_SHAPE_VALUE;
  }
  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, first_element_of_fz_lstm);
  shape.SetDim(1, second_element_of_fz_lstm);
  shape.SetDim(2, NI);
  shape.SetDim(3, NI);
  return true;
}

bool ShapeTransferAccordingToFormat::GetFzC04ShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  int64_t x = SHAPE_DIM_VALUE_C04;
  MUL_OVERFLOW(x, axis_value[AXIS_H], x);
  MUL_OVERFLOW(x, axis_value[AXIS_W], x);
  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, DivisionCeiling(x, X0));
  shape.SetDim(1, DivisionCeiling(axis_value[AXIS_N], NI));
  shape.SetDim(2, NI);
  shape.SetDim(3, X0);
  return true;
}

bool ShapeTransferAccordingToFormat::GetFzGShapeByAxisValue(const AxisValue &axis_value, gert::Shape &shape) {
  int64_t axis_c_val = axis_value[AXIS_C];
  MUL_OVERFLOW(axis_c_val, axis_value[AXIS_G], axis_c_val);
  int64_t hwc1 = DivisionCeiling(axis_c_val, axis_value[AXIS_C0]);
  MUL_OVERFLOW(hwc1, axis_value[AXIS_H], hwc1);
  MUL_OVERFLOW(hwc1, axis_value[AXIS_W], hwc1);
  shape.SetDimNum(DIM_DEFAULT_SIZE);
  shape.SetDim(0, hwc1);
  shape.SetDim(1, DivisionCeiling(axis_value[AXIS_N], NI));
  shape.SetDim(2, NI);
  shape.SetDim(3, axis_value[AXIS_C0]);
  return true;
}
} // namespace transformer
