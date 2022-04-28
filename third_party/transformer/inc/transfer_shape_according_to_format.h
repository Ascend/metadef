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

#ifndef COMMON_UTILS_TRANSFORMER_INC_TRANSFER_SHAPE_ACCORDING_TO_FORMAT_H_
#define COMMON_UTILS_TRANSFORMER_INC_TRANSFER_SHAPE_ACCORDING_TO_FORMAT_H_

#include <memory.h>

#include "axis_util.h"
#include "graph/types.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"

namespace transformer {
struct CalcShapeExtraAttr {
  int64_t hidden_size;
  int64_t input_size;
  int64_t state_size;
};

struct ShapeAndFormatInfo {
  ge::GeShape &oldShape;
  const ge::Format &oldFormat;
  const ge::Format &newFormat;
  const ge::DataType &currentDataType;
  CalcShapeExtraAttr extra_attr;
  ShapeAndFormatInfo(ge::GeShape &old_shape, const ge::Format &old_format, const ge::Format &new_format,
                     const ge::DataType &data_type)
                     : oldShape(old_shape), oldFormat(old_format), newFormat(new_format), currentDataType(data_type),
                       extra_attr({1, 1, -1}) {}
};

using ShapeAndFormat = struct ShapeAndFormatInfo;

class ShapeTransferAccordingToFormat {
 public:
  ShapeTransferAccordingToFormat();

  ~ShapeTransferAccordingToFormat() {};

  ShapeTransferAccordingToFormat(const ShapeTransferAccordingToFormat&) = delete;

  ShapeTransferAccordingToFormat &operator=(const ShapeTransferAccordingToFormat&) = delete;

#ifdef ONLY_COMPILE_OPEN_SRC
  bool GetShapeAccordingToFormat(ShapeAndFormat &shapeAndFormatInfo, int64_t* c = nullptr);

  bool GetShapeAccordingToFormat(const ge::OpDescPtr &op_desc, ShapeAndFormat &shapeAndFormatInfo,
                                 int64_t* c = nullptr);
#else
  bool GetShapeAccordingToFormat(ShapeAndFormat &shapeAndFormatInfo);

  bool GetShapeAccordingToFormat(const ge::OpDescPtr &op_desc, ShapeAndFormat &shapeAndFormatInfo);
#endif

  static bool TransferShape(const ge::Format &origin_format, const ge::Format &format, const ge::DataType &data_type,
                            gert::Shape &shape, const ge::OpDescPtr op_desc = nullptr);

private:
  static bool TransferShapeByFormat(const ge::Format &primary_format, const AxisValue &axis_value,
                                    gert::Shape &shape);
  static bool IsNeedTransferShape(const ge::Format &origin_format, const ge::Format &format, const gert::Shape &shape);
  static bool CheckInputParam(const ge::Format &origin_format, const ge::Format &primary_format,
                              const ge::DataType &data_type);
  static void FillupRnnAxisValue(const ge::OpDescPtr &op_desc, const ge::Format &format, AxisValue &axis_value);
  static bool IsNeedAxisValue(const ge::Format &format, const size_t &origin_dim_size);
  static int64_t GetC0Value(const ge::DataType &data_type, const ge::Format &format);

  /* ----------Below is the function of getting new shape---------------------- */
  static bool GetNCHWShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetNHWCShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetHWCNShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetCHWNShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetNC1HWC0ShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetFzShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetNDC1HWC0ShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetC1HWNCoC0ShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetNzShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetFz3DShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetFz3DTransposeShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetFzLstmShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetFzC04ShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetFzGShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetFznRNNShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);

  static bool GetNDRNNShapeByAxisValue(gert::Shape &shape, const AxisValue &axis_value);
};
} // namespace transformer

#endif  // COMMON_UTILS_TRANSFORMER_INC_TRANSFER_SHAPE_ACCORDING_TO_FORMAT_H_
