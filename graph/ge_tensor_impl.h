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

#ifndef GRAPH_GE_TENSOR_IMPL_H_
#define GRAPH_GE_TENSOR_IMPL_H_


#include <deque>
#include <string>
#include <vector>
#include "graph/ge_tensor.h"

namespace ge {
class GeTensorDescImpl {
 public:
  GeTensorDescImpl() = default;
  GeTensorDescImpl(const GeShape &shape, Format format, DataType dt);
  GeTensorDescImpl(const ProtoMsgOwner &proto_owner, proto::TensorDescriptor *proto_msg);
  ~GeTensorDescImpl() = default;

  GeShape &ShapeReference() const;
  GeShape &OriginShapeReference() const;

  bool GeTensorDescAttrsAreEqual(const GeTensorDescImpl &other) const;
  bool operator==(const GeTensorDescImpl &other) const;

  ProtoAttrMap &MutableAttrMap();
  ConstProtoAttrMap &GetAttrMap() const;
  void SetShape(const GeShape &shape);

  void SetDataType(DataType dataType);
  DataType GetDataType() const;
  void SetFormat(Format format);
  Format GetFormat() const;
  void SetOriginFormat(Format format);
  Format GetOriginFormat() const;
  void SetOriginDataType(DataType dataType);
  DataType GetOriginDataType() const;
  void SetName(const std::string &name);
  const std::string GetName() const;

 private:
  friend class GeTensorImpl;
  friend class TensorUtils;
  friend class GeAttrValueImp;
  friend class ModelSerializeImp;
  friend class GeTensorSerializeUtils;
  friend class OnnxUtils;

  class ExtMeta {
   public:
    bool operator==(const ExtMeta& other) const {
      return (name == other.name && device_type == other.device_type && size == other.size &&
        weight_size == other.weight_size && data_offset == other.data_offset &&
        real_dim_cnt == other.real_dim_cnt && input_tensor == other.input_tensor &&
        reuse_input == other.reuse_input && reuse_input_index == other.reuse_input_index &&
        output_tensor == other.output_tensor && cmps_size == other.cmps_size && cmps_tab == other.cmps_tab &&
        cmps_tab_offset == other.cmps_tab_offset);
    }
    // for name
    std::string GetName() const {
      return name;
    }

    void SetName(const std::string &v) {
      name = v;
    }

    // for device_type
    DeviceType GetDeviceType() const {
      return device_type;
    }

    std::string GetDeviceTypeStr();

    void SetDeviceType(DeviceType v) {
      device_type = v;
    }

    // for size
    int64_t GetSize() const {
      return size;
    }

    void SetSize(int64_t v) {
      size = v;
    }

    // for weight_size
    int64_t GetWeightSize() const {
      return weight_size;
    }

    void SetWeightSize(int64_t v) {
      weight_size = v;
    }

    // for data_offset
    int64_t GetDataOffset() const {
      return data_offset;
    }

    void SetDataOffset(int64_t v) {
      data_offset = v;
    }

    // for real_dim_cnt
    uint32_t GetRealDimCnt() const {
      return real_dim_cnt;
    }

    void SetRealDimCnt(uint32_t v) {
      real_dim_cnt = v;
    }

    // for input_tensor
    bool GetInputTensor() const {
      return input_tensor;
    }

    void SetInputTensor(bool v) {
      input_tensor = v;
    }

    // for reuse_input
    bool GetReuseInput() const {
      return reuse_input;
    }

    void SetReuseInput(bool v) {
      reuse_input = v;
    }

    // for reuse_input_index
    uint32_t GetReuseInputIndex() const {
      return reuse_input_index;
    }

    void SetReuseInputIndex(uint32_t v) {
      reuse_input_index = v;
    }

    // for output_tensor
    bool GetOutputTensor() const {
      return output_tensor;
    }

    void SetOutputTensor(bool v) {
      output_tensor = v;
    }

    // for cmps_size
    int64_t GetCmpsSize() const {
      return cmps_size;
    }

    void SetCmpsSize(int64_t v) {
      cmps_size = v;
    }

    // for cmps_tab
    std::string GetCmpsTab() const {
      return cmps_tab;
    }

    void SetCmpsTab(const std::string &v) {
      cmps_tab = v;
    }

    // for cmps_tab_offset
    int64_t GetCmpsTabOffset() const {
      return cmps_tab_offset;
    }

    void SetCmpsTabOffset(int64_t v) {
      cmps_tab_offset = v;
    }

   private:
    int64_t size{0};
    int64_t data_offset{0};
    int64_t cmps_tab_offset{0};
    int64_t cmps_size{0};
    int64_t weight_size{0};

    uint32_t real_dim_cnt{0U};
    uint32_t reuse_input_index{0U};

    DeviceType device_type{NPU};
    bool input_tensor{false};
    bool reuse_input{false};
    bool output_tensor{false};

    std::string cmps_tab;
    std::string name;
  };

  mutable GeShape shape_;
  Format format_{FORMAT_ND};
  DataType dtype_{DT_FLOAT};

  mutable GeShape origin_shape_;
  Format origin_format_{FORMAT_ND};
  DataType origin_dtype_{DT_UNDEFINED};

  ExtMeta ext_meta_;
  AttrStore attrs_;
};

class TensorDataImpl {
 public:
  TensorDataImpl() = default;

  TensorDataImpl(const TensorDataImpl &other);

  ~TensorDataImpl() = default;

  TensorDataImpl &operator=(const TensorDataImpl &other);

  graphStatus SetData(const uint8_t *data, size_t size);
  graphStatus SetData(uint8_t *data, size_t size, const AlignedPtr::Deleter &delete_fuc);
  void SetData(std::shared_ptr<AlignedPtr> aligned_ptr, size_t size);

  const uint8_t *MallocAlignedPtr(size_t size);

  size_t GetSize() const;
  const uint8_t *GetData() const;
  uint8_t *GetData();

  void clear();

  uint8_t operator[](size_t index) const;

  const std::shared_ptr<AlignedPtr> &GetAlignedPtr() { return aligned_ptr_; }

 private:
  friend class GeTensorImpl;
  friend class TensorUtils;
  friend class GeAttrValueImp;
  friend class ModelSerializeImp;
  friend class GeTensorSerializeUtils;
  // TensorDatat shared with a GeTensorDesc by holding the impl of GeTensorDesc
  std::shared_ptr<GeTensorDescImpl> tensor_descriptor_;
  std::shared_ptr<AlignedPtr> aligned_ptr_ = nullptr;
  size_t length_ = 0;
  // functions data() & mutable_data() return address of invalid_data_ when length_ is 0
  // defined for coding convenience
  static uint32_t invalid_data_;
};

class GeTensorImpl {
 public:
  GeTensorImpl();
  explicit GeTensorImpl(const GeTensorDesc &tensor_desc);
  GeTensorImpl(const GeTensorDesc &tensor_desc, const std::vector<uint8_t> &data);
  GeTensorImpl(const GeTensorDesc &tensor_desc, const uint8_t *data, size_t size);
  GeTensorImpl(GeTensorDesc &&tensor_desc, std::vector<uint8_t> &&data);
  GeTensorImpl(const GeTensorDesc &tensor_desc, const Buffer &data);
  GeTensorImpl(const GeTensorDesc &tensor_desc, std::shared_ptr<AlignedPtr> aligned_ptr, size_t size);
  GeTensorImpl(const GeTensorDesc &tensor_desc, size_t size);
  GeTensorImpl(const ProtoMsgOwner &proto_owner, proto::TensorDef *proto_msg);
  GeTensorImpl(const GeTensorImpl &other);

  ~GeTensorImpl() = default;

  GeTensorImpl &operator=(const GeTensorImpl &other);

  GeTensorDesc &DescReference() const;
  void BuildAlignerPtrWithProtoData();
  graphStatus SetData(std::vector<uint8_t> &&data);
  graphStatus SetData(const std::vector<uint8_t> &data);
  graphStatus SetData(const uint8_t *data, size_t size);
  graphStatus SetData(const Buffer &data);
  graphStatus SetData(const TensorData &data);
  graphStatus SetData(uint8_t *data, size_t size, const AlignedPtr::Deleter &delete_fuc);
  void ClearData();
  void Clone(GeTensorImpl &tensor) const;

  std::shared_ptr<AlignedPtr> GetAlignedPtr();
  const TensorData &GetData() const { return tensor_data_; }
  TensorData &MutableData() { return tensor_data_; }
  // zero copy SetData
  void SetData(std::shared_ptr<AlignedPtr> aligned_ptr, size_t size) {
    tensor_data_.SetData(std::move(aligned_ptr), size);
  }

 private:
  friend class TensorUtils;
  friend class GeAttrValueImp;
  friend class ModelSerializeImp;
  friend class GeTensorSerializeUtils;
  GeIrProtoHelper<proto::TensorDef> tensor_def_;
  // Reference from tensor_data_, do not direct use
  mutable GeTensorDesc desc_;
  TensorData tensor_data_;
};
}  // namespace ge
#endif  // GRAPH_GE_TENSOR_IMPL_H_
