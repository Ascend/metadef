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
#ifndef METADEF_CXX_INC_EXE_GRAPH_TENSOR_DATA_H_
#define METADEF_CXX_INC_EXE_GRAPH_TENSOR_DATA_H_

#include "graph/ge_error_codes.h"

namespace gert {
using TensorAddress = void *;
using ConstTensorAddress = void *const;
using ConstTensorAddressPtr = const void *;

enum TensorOperateType {
  kGetTensorAddress,  ///< 获取Tensor的地址
  kFreeTensor,        ///< 释放Tensor
  kPlusShareCount,    ///< 共享Tensor
  kTensorOperateType
};

/**
 * Tensor的管理函数
 */
using TensorAddrManager = ge::graphStatus (*)(TensorAddress addr, TensorOperateType operate_type, void **out);

class TensorData {
 public:
  /**
   * 构造一个TensorData
   * @param addr tensor的地址
   * @param manager tensor data的管理函数，若manager为空，则认为addr就是tensor的数据地址，且此数据不需要被释放
   */
  explicit TensorData(TensorAddress addr = nullptr, TensorAddrManager manager = nullptr)
      : addr_(addr), manager_(manager) {}
  TensorData(const TensorData &) = delete;
  TensorData(TensorData &&other) noexcept : addr_(other.addr_), manager_(other.manager_) {
    other.addr_ = nullptr;
    other.manager_ = nullptr;
  }
  // todo delete after air adopted
  TensorData &operator=(const TensorData &other) {
    Free();
    addr_ = other.addr_;
    manager_ = other.manager_;
    return *this;
  }
  TensorData &operator=(TensorData &&other) noexcept {
    Free();
    addr_ = other.addr_;
    manager_ = other.manager_;
    other.addr_ = nullptr;
    other.manager_ = nullptr;
    return *this;
  }
  ~TensorData() {
    Free();
  }

  /**
   * 获取tensor地址
   * @return tensor地址
   */
  TensorAddress GetAddr() const {
    if (manager_ == nullptr || addr_ == nullptr) {
      return addr_;
    }
    TensorAddress addr;
    if (manager_(addr_, kGetTensorAddress, &addr) != ge::GRAPH_SUCCESS) {
      return nullptr;
    } else {
      return addr;
    }
  }
  /**
   * 释放tensor
   * @return 成功时返回ge::GRAPH_SUCCESS
   */
  ge::graphStatus Free() {
    if (manager_ == nullptr) {
      return ge::GRAPH_SUCCESS;
    }
    auto ret = manager_(addr_, kFreeTensor, nullptr);
    if (ret == ge::GRAPH_SUCCESS) {
      addr_ = nullptr;
      manager_ = nullptr;
    }
    return ret;
  }
  /**
   * 设置tensor地址
   * @param addr tensor地址
   * @param manager tensor的管理函数
   */
  ge::graphStatus SetAddr(ConstTensorAddressPtr addr, TensorAddrManager manager) {
    auto ret = Free();
    if (ret != ge::GRAPH_SUCCESS) {
      return ret;
    }
    addr_ = const_cast<TensorAddress>(addr);
    manager_ = manager;
    return ge::GRAPH_SUCCESS;
  }

  ge::graphStatus ShareFrom(const TensorData &other) {
    auto ret = Free();
    if (ret != ge::GRAPH_SUCCESS) {
      return ret;
    }

    addr_ = other.addr_;
    manager_ = other.manager_;

    if (manager_ != nullptr) {
      return manager_(addr_, kPlusShareCount, nullptr);
    } else {
      return ge::GRAPH_SUCCESS;
    }
  }

 private:
  TensorAddress addr_;
  TensorAddrManager manager_;
};
}  // namespace gert

#endif  // METADEF_CXX_INC_EXE_GRAPH_TENSOR_DATA_H_
