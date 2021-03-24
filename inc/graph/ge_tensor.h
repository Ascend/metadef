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

#ifndef INC_GRAPH_GE_TENSOR_H_
#define INC_GRAPH_GE_TENSOR_H_

#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "detail/attributes_holder.h"
#include "graph/buffer.h"
#include "graph/aligned_ptr.h"
#include "graph/ge_error_codes.h"
#include "graph/types.h"

namespace ge {
class GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY GeShape {
 public:
  GeShape();
  ~GeShape() = default;
  explicit GeShape(std::vector<int64_t> s);

  size_t GetDimNum() const;
  // If the idx is invalid, return 0
  int64_t GetDim(size_t idx) const;
  graphStatus SetDim(size_t idx, int64_t value);
  std::vector<int64_t> GetDims() const;

  int64_t GetShapeSize() const;
  std::string ToString() const;

  ///
  /// @brief Check is unknown shape
  /// @return bool
  ///
  bool IsUnknownShape() const;

  ///
  /// @brief Check is a scalar
  /// @return bool
  ///
  bool IsScalar() const;

  GeShape(const GeShape &other);
  GeShape(GeShape &&other);
  GeShape &operator=(const GeShape &other);
  GeShape &operator=(GeShape &&other);

 private:
  GeIrProtoHelper<proto::ShapeDef> shape_def_;
  friend class GeTensorDesc;
  // Create from proto obj
  GeShape(const ProtoMsgOwner &protoOnwer, proto::ShapeDef *protoMsg);

  void RefTo(const GeShape &shape) { shape_def_ = shape.shape_def_; }
};

class GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY GeTensorDesc : public AttrHolder {
  friend class TensorUtils;
  friend class GeAttrValue;
  friend class ModelSerialize;

 public:
  GeTensorDesc();
  explicit GeTensorDesc(GeShape shape, Format format = FORMAT_ND, DataType dt = DT_FLOAT);
  GeTensorDesc(const GeTensorDesc &desc);
  GeTensorDesc(GeTensorDesc &&desc);

  ~GeTensorDesc() = default;
  bool operator==(const GeTensorDesc &r_ge_tensor_desc) const;

  void Update(GeShape shape, Format format = FORMAT_ND, DataType dt = DT_FLOAT);

  GeShape GetShape() const;
  GeShape &MutableShape();
  void SetShape(GeShape shape);

  // set shape with -2, it stand for unknown shape
  void SetUnknownDimNumShape();
  // for unknown shape
  graphStatus SetShapeRange(const std::vector<std::pair<int64_t, int64_t>> &range);
  graphStatus SetOriginShapeRange(const std::vector<std::pair<int64_t, int64_t>> &range);
  graphStatus GetShapeRange(std::vector<std::pair<int64_t, int64_t>> &range) const;
  graphStatus GetOriginShapeRange(std::vector<std::pair<int64_t, int64_t>> &range) const;

  GeShape GetOriginShape() const;
  void SetOriginShape(const GeShape &originShape);

  Format GetFormat() const;
  void SetFormat(Format format);

  Format GetOriginFormat() const;
  void SetOriginFormat(Format originFormat);

  void SetName(const std::string &name);
  const std::string GetName() const;

  DataType GetDataType() const;
  void SetDataType(DataType dt);

  DataType GetOriginDataType() const;
  void SetOriginDataType(DataType originDataType);

  std::vector<uint32_t> GetRefPortIndex() const;
  void SetRefPortByIndex(const std::vector<uint32_t> &index);

  GeTensorDesc Clone() const;
  GeTensorDesc &operator=(const GeTensorDesc &desc);
  GeTensorDesc &operator=(GeTensorDesc &&desc);

  graphStatus IsValid() const;

 protected:
  ProtoAttrMapHelper MutableAttrMap() override;
  ConstProtoAttrMapHelper GetAttrMap() const override;

 private:
  bool GeTensorDescAttrsAreEqual(const GeTensorDesc &r_ge_tensor_desc) const;
  using AttrHolder::DelAttr;
  using AttrHolder::GetAllAttrs;
  using AttrHolder::GetAttr;
  using AttrHolder::HasAttr;
  using AttrHolder::SetAttr;

  void Init();

  // Create from proto obj
  GeTensorDesc(const ProtoMsgOwner &protoOnwer, proto::TensorDescriptor *protoMsg);
  friend class GeTensor;
  friend class GeAttrValueImp;
  friend class ModelSerializeImp;
  friend class OnnxUtils;

  GeIrProtoHelper<proto::TensorDescriptor> tensor_descriptor_;
  // Reference from tensorDescriptor_, do not direct use
  mutable GeShape __shape_;

  void RefTo(const GeTensorDesc &tensorDesc) { tensor_descriptor_ = tensorDesc.tensor_descriptor_; }
  GeShape &ShapeReference() const;
};

class GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY TensorData {
 public:
  TensorData() = default;
  TensorData(const TensorData &other);
  ~TensorData() = default;

  TensorData &operator=(const TensorData &other);

  graphStatus SetData(std::vector<uint8_t> &&data);
  graphStatus SetData(const std::vector<uint8_t> &data);
  graphStatus SetData(const Buffer &data);
  graphStatus SetData(const TensorData &data);
  graphStatus SetData(const uint8_t *data, size_t size);
  // zero copy SetData
  void SetData(std::shared_ptr<AlignedPtr> aligned_ptr, size_t size);

  const uint8_t *MallocAlignedPtr(size_t size);

  inline const std::uint8_t *data() const { return GetData(); }
  inline std::uint8_t *data() { return GetData(); }
  inline std::size_t size() const { return GetSize(); }
  inline void clear() {
    aligned_ptr_.reset();
    length_ = 0;
  }
  uint8_t operator[](size_t index) const {
    if (aligned_ptr_ != nullptr && index < length_) {
      return *(aligned_ptr_->MutableGet() + index);
    }
    return 0xff;
  }

  std::size_t GetSize() const;
  const std::uint8_t *GetData() const;
  std::uint8_t *GetData();

  const std::shared_ptr<AlignedPtr> &GetAlignedPtr() { return aligned_ptr_; }

 private:
  friend class GeTensor;
  friend class GeAttrValueImp;
  friend class ModelSerializeImp;
  GeIrProtoHelper<proto::TensorDescriptor> tensor_descriptor_;
  std::shared_ptr<AlignedPtr> aligned_ptr_ = nullptr;
  size_t length_ = 0;
  // functions data() & mutable_data() return address of invalid_data_ when length_ is 0
  // defined for coding convenience
  static uint32_t invalid_data_;
};

class GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY GeTensor {
 public:
  GeTensor();
  explicit GeTensor(const GeTensorDesc &tensorDesc);
  explicit GeTensor(const GeTensorDesc &tensorDesc, const std::vector<uint8_t> &data);
  explicit GeTensor(const GeTensorDesc &tensorDesc, const Buffer &data);
  explicit GeTensor(const GeTensorDesc &tensorDesc, const uint8_t *data, size_t size);
  explicit GeTensor(GeTensorDesc &&tensorDesc, std::vector<uint8_t> &&data);
  // zero copy constructor
  explicit GeTensor(const GeTensorDesc &tensorDesc, std::shared_ptr<AlignedPtr> aligned_ptr, size_t size);
  explicit GeTensor(const GeTensorDesc &tensorDesc, size_t size);
  ~GeTensor() = default;

  GeTensorDesc GetTensorDesc() const;
  GeTensorDesc &MutableTensorDesc();
  void SetTensorDesc(const GeTensorDesc &tensorDesc);

  std::shared_ptr<AlignedPtr> GetAlignedPtr() { return tensor_data_.aligned_ptr_; }

  const TensorData &GetData() const { return tensor_data_; }
  TensorData &MutableData() { return tensor_data_; }

  graphStatus SetData(std::vector<uint8_t> &&data);
  graphStatus SetData(const std::vector<uint8_t> &data);
  graphStatus SetData(const Buffer &data);
  graphStatus SetData(const uint8_t *data, size_t size);
  graphStatus SetData(const TensorData &data);

  // zero copy SetData
  void SetData(std::shared_ptr<AlignedPtr> aligned_ptr, size_t size) {
    tensor_data_.SetData(std::move(aligned_ptr), size);
  }

  void ClearData();

  GeTensor Clone() const;

  // Share value
  GeTensor(const GeTensor &other);
  // Share value
  GeTensor &operator=(const GeTensor &other);

 private:
  friend class GeAttrValueImp;
  friend class ModelSerializeImp;
  friend class OnnxUtils;
  friend class TensorData;
  // Create from proto obj
  GeTensor(const ProtoMsgOwner &protoOnwer, proto::TensorDef *protoMsg);
  void BuildAlignerPtrWithProtoData();
  GeIrProtoHelper<proto::TensorDef> tensor_def_;
  // Reference from tensor_data_, do not direct use
  mutable GeTensorDesc __desc_;
  GeTensorDesc &DescReference() const;
  TensorData tensor_data_;
};
}  // namespace ge
#endif  // INC_GRAPH_GE_TENSOR_H_
