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

#ifndef TENSOR_ASSIGN_H_
#define TENSOR_ASSIGN_H_

#include <vector>
#include "graph/ge_tensor.h"
#include "graph/def_types.h"
#include "external/register/register_error_codes.h"
#include "proto/tensorflow/tensor.pb.h"

namespace domi {
using GeTensorPtr = std::shared_ptr<ge::GeTensor>;
using Status = uint32_t;

class TensorAssign {
 public:
  static Status SetGeTensor(const domi::tensorflow::TensorProto &tensor, GeTensorPtr &weight);

  static Status SetGeTensorDataType(const int64_t data_type, GeTensorPtr &weight);

  static ge::DataType ConvertTensorflowDataType(const uint32_t tf_data_type);

 private:
  static bool CheckBoolVal(const tensorflow::DataType data_type);

  static bool CheckHalfVal(const tensorflow::DataType data_type);

  static bool CheckFloatVal(const tensorflow::DataType data_type);

  static bool CheckDoubleVal(const tensorflow::DataType data_type);

  static bool CheckComplex64Val(const tensorflow::DataType data_type);

  static bool CheckComplex128Val(const tensorflow::DataType data_type);

  static bool CheckStringVal(const tensorflow::DataType data_type);

  static bool CheckByte(const tensorflow::DataType data_type);

  static bool CheckDoubleByte(const tensorflow::DataType data_type);

  static bool CheckSignedFourByte(const tensorflow::DataType data_type);

  static bool CheckUnsignedFourByte(const tensorflow::DataType data_type);

  static bool CheckSignedEightByte(const tensorflow::DataType data_type);

  static bool CheckUnsignedEightByte(const tensorflow::DataType data_type);

  static Status GetDoubleByteVal(const int32_t val_size,
                                 const google::protobuf::RepeatedField<google::protobuf::int32> &val_vector,
                                 const int32_t count, GeTensorPtr &weight);
  static Status GetByteVal(const int32_t val_size,
                           const google::protobuf::RepeatedField<google::protobuf::int32> &val_vector,
                           const int32_t count, GeTensorPtr &weight);

  static Status GetStringVal(const int32_t val_size, const google::protobuf::RepeatedPtrField<std::string> &val_vector,
                             const int32_t count, GeTensorPtr &weight);

  static void SetGeTensorWeightData(const domi::tensorflow::TensorProto &tensor, const int32_t val_size,
                                    const int32_t count, GeTensorPtr &weight);

  static void SetWeightData(const tensorflow::DataType data_type, const int32_t count,
                            const std::string &tensor_content, GeTensorPtr &weight);

  template <typename T>
  static Status GetVal(const int32_t val_size, const google::protobuf::RepeatedField<T> &val_vector,
                       const int32_t count, GeTensorPtr &weight) {
    const std::unique_ptr<T[]> addr(new (std::nothrow) T[count]());
    GE_CHECK_NOTNULL(addr);
    const bool zerosLike = ((count != val_size) && (val_size == 1));
    if (!zerosLike) {
      const int32_t minCount = (count > val_size) ? val_size : count;
      const size_t minSize = static_cast<size_t>(minCount);
      for (size_t i = 0UL; i < minSize; i++) {
        addr[i] = val_vector.Get(static_cast<int32_t>(i));
      }
      for (size_t i = minSize; i < static_cast<size_t>(count); i++) {
        addr[i] = val_vector.Get(minCount - 1);
      }
    } else {
      for (size_t i = 0UL; i < static_cast<size_t>(count); i++) {
        addr[i] = val_vector.Get(0);
      }
    }
    (void)weight->SetData(ge::PtrToPtr<T, uint8_t>(addr.get()), static_cast<size_t>(count) * sizeof(T));
    return SUCCESS;
  }
};
}  // namespace domi
#endif  // TENSOR_ASSIGN_H_
