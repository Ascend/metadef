/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef METADEF_INC_GRAPH_PARALLELISM_TENSOR_PARALLEL_ATTRS_H_
#define METADEF_INC_GRAPH_PARALLELISM_TENSOR_PARALLEL_ATTRS_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ge/ge_api_types.h"

namespace ge {
namespace tp {
// tensor deployment attrs
struct DimSlice {
  int64_t begin;
  int64_t end;
};

struct DeviceIndex {
  std::string engine_type;
  std::vector<int32_t> indices;
  std::string DebugString() const;
};

bool operator==(const DeviceIndex &lhs, const DeviceIndex &rhs);
bool operator!=(const DeviceIndex &lhs, const DeviceIndex &rhs);
bool operator<(const DeviceIndex &lhs, const DeviceIndex &rhs);

struct TensorSliceDeployment {
  std::vector<std::vector<DimSlice>> axis_slices;
  std::vector<std::vector<DeviceIndex>> device_indices_each_slice;
  std::string reduce_type;
};

struct TensorDeployment {
  TensorSliceDeployment shard_deployment;
};

struct NodeDeployment {
  std::vector<DeviceIndex> devices;
};

// P2P communications
struct CommPair {
  DeviceIndex src_device_index;
  DeviceIndex dst_device_index;
};

struct SendRecvReshardTask {
  std::vector<CommPair> comm_pairs;
};

// group communications
struct CommGroup {
  std::vector<DeviceIndex> device_indices;
};

struct AllToAllReshardTask {
  std::vector<CommGroup> comm_groups;
};

struct AllGatherReshardTask {
  std::vector<CommGroup> comm_groups;
};

struct AllReduceReshardTask {
  std::string reduction;
  std::vector<CommGroup> comm_groups;
};

struct ReduceScatterReshardTask {
  std::string reduction;
  std::vector<CommGroup> comm_groups;
};

struct BroadcastReshardTask {
  std::vector<DeviceIndex> root_device_indices;  // size == num_groups
  std::vector<CommGroup> comm_groups;
};

// local reshardings
struct SliceReshardTask {
  std::vector<int64_t> offsets;
  std::vector<int64_t> sizes;
};

struct SplitReshardTask {
  std::vector<int64_t> size_splits;
  int32_t split_axis = 0;
};

struct ConcatReshardTask {
  int32_t concat_dim = 0;
};

struct TransposeReshardTask {
  std::vector<int32_t> perm;
};

struct ModifyValueReshardTask {
  std::string op_type;  // mul, div
  std::vector<int64_t> value;
};

struct CommTask {
  std::string task_type;
  std::shared_ptr<SendRecvReshardTask> send_recv_reshard_task;
  std::shared_ptr<AllGatherReshardTask> all_gather_reshard_task;
  std::shared_ptr<AllToAllReshardTask> all_to_all_reshard_task;
  std::shared_ptr<AllReduceReshardTask> all_reduce_reshard_task;
  std::shared_ptr<ReduceScatterReshardTask> reduce_scatter_reshard_task;
  std::shared_ptr<BroadcastReshardTask> broadcast_reshard_task;
  std::shared_ptr<SplitReshardTask> split_reshard_task;
  std::shared_ptr<ConcatReshardTask> concat_reshard_task;
  std::shared_ptr<SliceReshardTask> slice_reshard_task;
  std::shared_ptr<TransposeReshardTask> transpose_reshard_task;
  std::shared_ptr<ModifyValueReshardTask> modify_value_reshard_task;
};

struct CommStepInput {
  int32_t step_id = -1;
  int32_t output_index = -1;
};

bool operator==(const CommStepInput &lhs, const CommStepInput &rhs);
bool operator<(const CommStepInput &lhs, const CommStepInput &rhs);

struct CommStep {
  int32_t id;
  std::vector<CommStepInput> inputs;
  CommTask comm_task;
};

struct PeerInput {
  int32_t step_id = -1;
  std::string node_name;
  uint32_t input_index;
};

// reshard ops for one output tensor
struct OutputReshardRes {
  std::vector<CommStep> comm_steps;
  std::vector<PeerInput> peer_inputs;
  std::vector<DeviceIndex> device_indices;
};

struct ReshardAttr {
  std::vector<std::vector<OutputReshardRes>> reshard_infos;  // indexed by output index
};

class TensorParallelAttrs {
 public:
  static Status FromJson(const std::string &json_str, DeviceIndex &device_index);
  static Status FromJson(const std::string &json_str, NodeDeployment &node_deployment);
  static Status FromJson(const std::string &json_str, TensorDeployment &tensor_deployment);
  static Status FromJson(const std::string &json_str, CommTask &comm_task);
  static Status FromJson(const std::string &json_str, CommStep &comm_step);
  static Status FromJson(const std::string &json_str, OutputReshardRes &tensor_shard_info);
  static Status FromJson(const std::string &json_str, ReshardAttr &outputs_reshard_info);

  static std::string ToJson(const NodeDeployment &node_deployment);
  static std::string ToJson(const DeviceIndex &device_index);
};
}  // namespace tp
}  // namespace ge

#endif  // METADEF_INC_GRAPH_PARALLELISM_TENSOR_PARALLEL_ATTRS_H_
