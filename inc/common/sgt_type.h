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

#ifndef FUSION_ENGINE_INC_COMMON_SGT_TYPES_H_
#define FUSION_ENGINE_INC_COMMON_SGT_TYPES_H_

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "graph/anchor.h"
#include "graph/types.h"
#include "graph/utils/op_desc_utils.h"


namespace fe {
const std::string SGT_JSON_INFO = "_sgt_json_info";
const std::string SGT_STRUCT_INFO = "_sgt_struct_info";
const uint32_t kMaxPersistNum = 8;
const int64_t kBytesInMB = 1048576;
const char SGT_TILING_NUM = 2;
struct OpCut {
    int16_t split_cut_idx = -1;
    int16_t reduce_cut_idx = -1;
    int64_t cut_id = -1;
};

struct DimRange {
  int64_t lower;
  int64_t higher;
  bool operator==(const DimRange& dim_range) const {
    return (this->higher == dim_range.higher) && (this->lower == dim_range.lower);
  }
};

struct ThreadSliceMap {
  uint32_t thread_scope_id;
  bool is_first_node_in_topo_order;
  uint32_t thread_mode;
  uint32_t node_num_in_thread_scope;
  bool is_input_node_of_thread_scope;
  bool is_output_node_of_thread_scope;
  std::vector<std::vector<std::vector<int64_t>>> ori_input_tensor_shape;
  std::vector<std::vector<std::vector<int64_t>>> ori_output_tensor_shape;
  std::string original_node;
  uint32_t slice_instance_num;
  uint32_t parallel_window_size;
  uint32_t thread_id;
  std::vector<std::vector<std::pair<std::string, uint32_t>>> dependencies;
  std::vector<uint32_t> core_num;
  std::vector<OpCut> cut_type;
  std::vector<uint32_t> input_tensor_indexes;
  std::vector<uint32_t> output_tensor_indexes;
  std::vector<std::vector<std::vector<DimRange>>> input_tensor_slice;
  std::vector<std::vector<std::vector<DimRange>>> output_tensor_slice;
  std::vector<std::vector<std::vector<DimRange>>> ori_input_tensor_slice;
  std::vector<std::vector<std::vector<DimRange>>> ori_output_tensor_slice;
  ThreadSliceMap() {
    is_first_node_in_topo_order = false;
    is_input_node_of_thread_scope = false;
    is_output_node_of_thread_scope = false;
    thread_mode = 0;
    thread_id = 0;
  }
};

enum class CACHE_OPERATION {
  PREFETCH = 0,
  INVALIDATE = 1,
  WRITE_BACK = 2,
  CACHE_OPERATION_BOTTOM = 3
};

void inline SetBitOne(const uint32_t pos, uint32_t &bm) {
  bm |= (0x1 << pos);
}

void inline SetBitOne(const uint32_t pos, uint64_t &bm) {
  bm |= (0x1 << pos);
}

const size_t kMaxCacheOperationSize = 64;

struct TickCacheMap {
  std::vector<int32_t> src_out_of_graph_input_index;
  std::map<int32_t, uint8_t> input_cache_table;
  std::map<int32_t, uint8_t> output_cache_table;
};

using ThreadSliceMapPtr = std::shared_ptr<ThreadSliceMap>;

void to_json(nlohmann::json& json_value, const DimRange &struct_value);
void from_json(const nlohmann::json &json_value, DimRange &struct_value);
void to_json(nlohmann::json &json_value, const ThreadSliceMap &struct_value);
void from_json(const nlohmann::json &json_value, ThreadSliceMap &struct_value);
void to_json(nlohmann::json &j, const OpCut &d);
void from_json(const nlohmann::json &j, OpCut &d);

}
#endif  // FUSION_ENGINE_INC_COMMON_SGT_TYPES_H_
