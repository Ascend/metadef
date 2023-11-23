/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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

#ifndef METADEF_CXX_GERT_TENSOR_DATA_H
#define METADEF_CXX_GERT_TENSOR_DATA_H
#include <utility>
#include "exe_graph/runtime/tensor_data.h"
#include "gert_mem_block.h"
namespace gert {
/**
 * 框架、引擎在执行时内部使用的TensorData，在TensorData的基础上，本类提供了引用计数和多流能力，但是这部分能力并不对外提供。
 * 因此在air仓外，仅基础的方法可使用，引用计数和多流能力(Share、Wander等方法)实现在air仓，仅链接air仓的引擎可调用
 */
class GertTensorData {
 public:
  GertTensorData() : tensor_data_{nullptr, nullptr, 0U, kTensorPlacementEnd}, mem_block_{nullptr}, stream_id_{-1} {}
  GertTensorData(GertMemBlock *block, size_t size, TensorPlacement placement, int64_t stream_id);

  GertTensorData(const GertTensorData &) = delete;
  GertTensorData &operator=(const GertTensorData &other) = delete;

  GertTensorData(GertTensorData &&other) noexcept
      : tensor_data_(std::move(other.tensor_data_)), mem_block_(other.mem_block_), stream_id_(other.stream_id_) {
    other.Clear();
  }
  GertTensorData &operator=(GertTensorData &&other) noexcept {
    if (this != &other) {
      if (NeedFree()) {
        Free();
      }
      tensor_data_ = std::move(other.tensor_data_);
      mem_block_ = other.mem_block_;
      stream_id_ = other.stream_id_;
      other.Clear();
    }
    return *this;
  }
  ~GertTensorData() noexcept {
    if (NeedFree()) {
      Free();
    }
  }

  size_t GetSize() const {
    return tensor_data_.GetSize();
  }
  void SetSize(const size_t size) {
    tensor_data_.SetSize(size);
  }

  TensorPlacement GetPlacement() const {
    return tensor_data_.GetPlacement();
  }
  void SetPlacement(const TensorPlacement placement) {
    tensor_data_.SetPlacement(placement);
  }

  TensorAddress GetAddr() const {
    return tensor_data_.GetAddr();
  }

  const GertMemBlock *GetGertMemBlock() const {
    return mem_block_;
  }

  void Free() {
    mem_block_->Free(stream_id_);
    Clear();
  }
  int64_t GetStreamId() const {
    return stream_id_;
  }

  const TensorData &GetTensorData() const {
    return tensor_data_;
  }
  TensorData &MutableTensorData() {
    return tensor_data_;
  }

  GertMemBlock *Release() {
    auto block = mem_block_;
    Clear();
    return block;
  }

  bool IsSharedWith(const GertTensorData &other) const;
  ge::graphStatus ShareFrom(const GertTensorData &other);
  ge::graphStatus WanderFrom(const GertTensorData &other, int64_t dst_stream_id);

 private:
  void CopyFromWithoutStream(const GertTensorData &other);
  bool NeedFree() const {
    return mem_block_ != nullptr;
  }
  void Clear() {
    tensor_data_.SetAddr(nullptr, nullptr);
    mem_block_ = nullptr;
    stream_id_ = -1;
  }

 private:
  TensorData tensor_data_;
  GertMemBlock *mem_block_;
  int64_t stream_id_;
};
}  // namespace gert
#endif // METADEF_CXX_GERT_TENSOR_DATA_H
