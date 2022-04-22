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
#ifndef INC_84597B1676BF44EC9AD9CC3343DED27D_H
#define INC_84597B1676BF44EC9AD9CC3343DED27D_H

#include "graph/ge_error_codes.h"

namespace gert {
using TensorAddress = void *;
using ConstTensorAddress = void *const;

enum TensorOperateType {
  kGetTensorAddress,
  kFreeTensor,
  kTensorOperateType
};

using TensorAddrManager = ge::graphStatus (*)(TensorAddress addr, TensorOperateType operate_type, void **out);

struct TensorData {
  TensorData(TensorAddress addr = nullptr, TensorAddrManager manager = nullptr): addr_(addr), manager_(manager){
  }
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
  ge::graphStatus Free() {
    if (manager_ == nullptr) {
      return ge::GRAPH_SUCCESS;
    }
    auto ret = manager_(addr_, kFreeTensor, nullptr);
    if (ret == ge::GRAPH_SUCCESS) {
      addr_ = nullptr;
    }
    return ret;
  }

  void SetAddr(TensorAddress addr, TensorAddrManager manager) {
    Free();
    addr_ = addr;
    manager_ = manager;
  }
 private:
  TensorAddress addr_;
  TensorAddrManager manager_;
};
}  // namespace gert

#endif
