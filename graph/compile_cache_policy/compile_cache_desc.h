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

#ifndef GRAPH_COMPILE_CACHE_POLICY_COMPILE_CACHE_DESC_H
#define GRAPH_COMPILE_CACHE_POLICY_COMPILE_CACHE_DESC_H

#include <string>
#include <vector>
#include <unordered_map>

#include <securec.h>

#include "graph/small_vector.h"
#include "graph/ascend_limits.h"
#include "graph/types.h"
#include "graph/debug/ge_log.h"
#include "graph/debug/ge_util.h"
#include "graph/def_types.h"
#include "hash_utils.h"

namespace ge {
using ShapeType = std::vector<int64_t>;
using ShapeRangeType = std::vector<std::pair<int64_t, int64_t>>;

class BinaryHolder {
 public:
  BinaryHolder() = default;

  BinaryHolder(const BinaryHolder &other) {
    if ((other.GetDataPtr() != nullptr) && (other.GetDataLen() != 0UL)) {
      data_len_ = other.GetDataLen();
      holder_ = ComGraphMakeUnique<uint8_t[]>(data_len_);
      const auto mem_ret = memcpy_s(holder_.get(), data_len_,
                                    ge::PtrToPtr<const uint8_t, const void>(other.GetDataPtr()), data_len_);
      if (mem_ret != EOK) {
        GELOGE(ge::GRAPH_FAILED, "[BinaryHolder] Memcpy Falied.");
      }
    }
  }
  BinaryHolder &operator=(const BinaryHolder &other) {
    if ((other.GetDataPtr() != nullptr) && (other.GetDataLen() != 0UL)) {
      data_len_ = other.GetDataLen();
      holder_ = ComGraphMakeUnique<uint8_t[]>(data_len_);
      const auto mem_ret = memcpy_s(holder_.get(), data_len_,
                                    ge::PtrToPtr<const uint8_t, const void>(other.GetDataPtr()), data_len_);
      if (mem_ret != EOK) {
        GELOGE(ge::GRAPH_FAILED, "[BinaryHolder] Memcpy Falied.");
      }
    }
    return *this;
  }

  ~BinaryHolder() = default;

  void SharedFrom(uint8_t *const data, const size_t data_len) {
    data_ptr_ = data;
    data_len_ = data_len;
  }

  const uint8_t *GetDataPtr() const noexcept {
    if (holder_.get() != nullptr) {
      return holder_.get();
    }
    return data_ptr_;
  }

  const size_t &GetDataLen() const noexcept {
    return data_len_;
  }

  bool operator!=(const BinaryHolder &second) const {
    if (this->GetDataLen() != second.GetDataLen()) {
      return false;
    }
    const auto this_data = this->GetDataPtr();
    const auto second_data = second.GetDataPtr();
    if (((this_data == nullptr) && (second_data != nullptr)) ||
        ((this_data != nullptr) && (second_data == nullptr))) {
      return true;
    }
    if ((this_data == nullptr) && (second_data == nullptr)) {
      return false;
    }
    if (memcmp(this_data, second_data, this->GetDataLen()) != 0) {
      return true;
    }
    return false;
  }
 private:
  std::unique_ptr<uint8_t[]> holder_ = nullptr;
  uint8_t *data_ptr_ = nullptr;
  size_t data_len_ = 0UL;
};

class CompileCacheDesc {
  friend class CompileCacheHasher;
 public:
  struct TensorInfoArgs {
    friend class CompileCacheDesc;
   private:
    SmallVector<ShapeType, kDefaultMaxInputNum> shapes;
    SmallVector<ShapeType, kDefaultMaxInputNum> origin_shapes;
    SmallVector<ShapeRangeType, kDefaultMaxInputNum> shape_ranges;
    SmallVector<Format, kDefaultMaxInputNum> formats;
    SmallVector<Format, kDefaultMaxInputNum> origin_formats;
    SmallVector<DataType, kDefaultMaxInputNum> data_types;
    SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc;
  };

  CompileCacheDesc() = delete;

  CompileCacheDesc(const int64_t unique_id, const TensorInfoArgs &tensor_info_args) : unique_id_(unique_id),
                   shapes_(tensor_info_args.shapes),
                   origin_shapes_(tensor_info_args.origin_shapes),
                   shape_ranges_(tensor_info_args.shape_ranges),
                   formats_(tensor_info_args.formats),
                   origin_formats_(tensor_info_args.origin_formats),
                   data_types_(tensor_info_args.data_types),
                   other_desc_(tensor_info_args.other_desc) {}

  ~CompileCacheDesc() = default;
  static bool IsSameCompileDesc(const CompileCacheDesc &first, const CompileCacheDesc &second) {
    if ((first.unique_id_ != second.unique_id_) ||
        (first.origin_shapes_ != second.origin_shapes_) ||
        (first.formats_ != second.formats_) ||
        (first.origin_formats_ != second.origin_formats_) ||
        (first.data_types_ != second.data_types_) ||
        (first.other_desc_.size() != second.other_desc_.size())) {
      return false;
    }

    for (size_t idx = 0UL; idx < first.other_desc_.size(); idx++) {
      if (first.other_desc_[idx] != second.other_desc_[idx]) {
        return false;
      }
    }
    return true;
  }

 private:
  int64_t unique_id_ = 0L;
  SmallVector<ShapeType, kDefaultMaxInputNum> shapes_;
  SmallVector<ShapeType, kDefaultMaxInputNum> origin_shapes_;
  SmallVector<ShapeRangeType, kDefaultMaxInputNum> shape_ranges_;
  SmallVector<Format, kDefaultMaxInputNum> formats_;
  SmallVector<Format, kDefaultMaxInputNum> origin_formats_;
  SmallVector<DataType, kDefaultMaxInputNum> data_types_;
  SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc_;
};
}  // namespace ge
namespace std {
template<>
struct hash<ge::BinaryHolder> {
  size_t operator()(const ge::BinaryHolder &value) const {
    GE_CHECK_NOTNULL(value.GetDataPtr());
    size_t seed = ge::HashUtils::MultiHash();
    const uint64_t u8_data = ge::PtrToValue(ge::PtrToPtr<const uint8_t, const void>(value.GetDataPtr()));
    for (size_t idx = 0UL; idx < value.GetDataLen(); idx++) {
      seed = ge::HashUtils::HashCombine(seed, *(ge::PtrToPtr<void, uint8_t>(ge::ValueToPtr(u8_data + idx))));
    }
    return seed;
  }
};
}  // namespace std
#endif
