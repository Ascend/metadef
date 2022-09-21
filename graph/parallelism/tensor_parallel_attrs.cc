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

#include "parallelism/tensor_parallel_attrs.h"
#include "framework/common/util.h"
#include "graph/debug/ge_util.h"
#include "nlohmann/json.hpp"
#include "parallelism/comm_task_builder.h"

#define USED_BY_JSON __attribute__((unused))

namespace ge {
namespace tp {
namespace {
constexpr size_t kValidDimSliceItemNum = 2U;
constexpr size_t kIndexStepId = 0U;
constexpr size_t kIndexOutputIndex = 1U;

using Json = nlohmann::json;
Status StringToJson(const std::string &json_str, Json &json) {
  std::stringstream ss;
  ss << json_str;
  try {
    ss >> json;
  } catch (const nlohmann::json::exception &e) {
    GELOGE(PARAM_INVALID, "Failed to init json object, err = %s, json_str = %s", e.what(), json_str.c_str());
    return PARAM_INVALID;
  }
  return SUCCESS;
}

template<typename T>
Status ParseFromJson(const std::string &type, const std::string &json_str, T &value) {
  Json json;
  GE_CHK_STATUS_RET_NOLOG(StringToJson(json_str, json));
  try {
    value = json.get<T>();
  } catch (const nlohmann::json::exception &e) {
    GELOGE(PARAM_INVALID,
           "Failed to parse json object, type = %s, err = %s, json_str = %s",
           type.c_str(),
           e.what(),
           json_str.c_str());
    return PARAM_INVALID;
  }
  return SUCCESS;
}

template<typename T>
std::shared_ptr<T> CreateReshardTaskInfo(const Json &j) {
  return ComGraphMakeShared<T>(j.get<T>());
}
}  // namespace

void CommTaskBuilder::BuildCommTask(const Json &j, CommTask &comm_task) {
  auto task_type = j.at("task_type").get<std::string>();
  const decltype(builders_)::const_iterator it = builders_.find(task_type);
  if (it == builders_.cend()) {
    GELOGE(PARAM_INVALID, "unsupported op type %s", comm_task.task_type.c_str());
    return;
  }
  it->second(j, comm_task);  // exception caught by caller
  comm_task.task_type = std::move(task_type);
}

Status CommTaskBuilder::ConvertToJson(const CommTask &comm_task, nlohmann::json &j) {
  const decltype(json_converters_)::const_iterator it = json_converters_.find(comm_task.task_type);
  GE_CHK_BOOL_RET_STATUS(it != json_converters_.cend(),
                         PARAM_INVALID,
                         "unsupported op type %s",
                         comm_task.task_type.c_str());
  return it->second(comm_task, j);  // exception caught by caller
}

std::string DeviceIndex::DebugString() const {
  return engine_type + ToString(indices);
}

USED_BY_JSON void to_json(Json &j, const DimSlice &dim_slice) {
  j = std::vector<int64_t>{dim_slice.begin, dim_slice.end};
}

USED_BY_JSON void from_json(const Json &j, DimSlice &dim_slice) {
  const auto &range = j.get<std::vector<int64_t>>();
  if (range.size() == kValidDimSliceItemNum) {
    dim_slice.begin = range.front();
    dim_slice.end = range.back();
  } else {
    dim_slice.begin = -1;
    dim_slice.end = -1;
    GELOGE(PARAM_INVALID, "invalid DimSlice: %s", j.dump().c_str());
  }
}

USED_BY_JSON void to_json(Json &j, const DeviceIndex &device_index) {
  j = Json();
  j["engine_type"] = device_index.engine_type;
  j["index"] = device_index.indices;
}

USED_BY_JSON void from_json(const Json &j, DeviceIndex &device_index) {
  device_index.engine_type = j.at("engine_type").get<std::string>();
  device_index.indices = j.at("index").get<std::vector<int32_t>>();
}

USED_BY_JSON void to_json(Json &j, const TensorSliceDeployment &tensor_slice_deployment) {
  j = Json();
  j["device_indices_each_slice"] = tensor_slice_deployment.device_indices_each_slice;
  j["axis_slices"] = tensor_slice_deployment.axis_slices;
}

USED_BY_JSON void from_json(const Json &j, TensorSliceDeployment &tensor_slice_deployment) {
  tensor_slice_deployment.device_indices_each_slice =
      j.at("device_indices_each_slice").get<std::vector<std::vector<DeviceIndex>>>();
  tensor_slice_deployment.axis_slices = j.at("axis_slices").get<std::vector<std::vector<DimSlice>>>();
}

USED_BY_JSON void to_json(Json &j, const TensorDeployment &tensor_deployment) {
  j = Json();
  j["shard_deployment"] = tensor_deployment.shard_deployment;
}

USED_BY_JSON void from_json(const Json &j, TensorDeployment &tensor_deployment) {
  tensor_deployment.shard_deployment = j.at("shard_deployment").get<TensorSliceDeployment>();
}

USED_BY_JSON void to_json(Json &j, const NodeDeployment &node_deployment) {
  j = Json();
  j["devices"] = node_deployment.devices;
}

USED_BY_JSON void from_json(const Json &j, NodeDeployment &node_deployment) {
  node_deployment.devices = j.at("devices").get<std::vector<DeviceIndex>>();
}

USED_BY_JSON void to_json(Json &j, const CommPair &comm_pair) {
  j = Json();
  j["src_device_index"] = comm_pair.src_device_index;
  j["dst_device_index"] = comm_pair.dst_device_index;
}

USED_BY_JSON void from_json(const Json &j, CommPair &comm_pair) {
  comm_pair.src_device_index = j.at("src_device_index").get<DeviceIndex>();
  comm_pair.dst_device_index = j.at("dst_device_index").get<DeviceIndex>();
}

USED_BY_JSON void to_json(Json &j, const CommGroup &comm_group) {
  j = comm_group.device_indices;
}

USED_BY_JSON void from_json(const Json &j, CommGroup &comm_group) {
  comm_group.device_indices = j.get<std::vector<DeviceIndex>>();
}

USED_BY_JSON void to_json(Json &j, const SendRecvReshardTask &task_info) {
  j = Json();
  j["task_type"] = "SendReceive";
  j["comm_pairs"] = task_info.comm_pairs;
}

USED_BY_JSON void from_json(const Json &j, SendRecvReshardTask &task_info) {
  task_info.comm_pairs = j.at("comm_pairs").get<std::vector<CommPair>>();
}

USED_BY_JSON void to_json(Json &j, const AllGatherReshardTask &task_info) {
  j = Json();
  j["task_type"] = "HcomAllGather";
  j["comm_groups"] = task_info.comm_groups;
}

USED_BY_JSON void from_json(const Json &j, AllGatherReshardTask &all_gather_task_info) {
  all_gather_task_info.comm_groups = j.at("comm_groups").get<std::vector<CommGroup>>();
}

USED_BY_JSON void to_json(Json &j, const AllToAllReshardTask &task_info) {
  j = Json();
  j["task_type"] = "HcomAllToAll";
  j["comm_groups"] = task_info.comm_groups;
}

USED_BY_JSON void from_json(const Json &j, AllToAllReshardTask &all_to_all_task_info) {
  all_to_all_task_info.comm_groups = j.at("comm_groups").get<std::vector<CommGroup>>();
}

USED_BY_JSON void to_json(Json &j, const AllReduceReshardTask &task_info) {
  j = Json();
  j["task_type"] = "HcomAllReduce";
  j["comm_groups"] = task_info.comm_groups;
  j["reduction"] = task_info.reduction;
}

USED_BY_JSON void from_json(const Json &j, AllReduceReshardTask &all_reduce_task_info) {
  all_reduce_task_info.reduction = j.at("reduction").get<std::string>();
  all_reduce_task_info.comm_groups = j.at("comm_groups").get<std::vector<CommGroup>>();
}

USED_BY_JSON void to_json(Json &j, const ReduceScatterReshardTask &task_info) {
  j = Json();
  j["task_type"] = "HcomReduceScatter";
  j["comm_groups"] = task_info.comm_groups;
  j["reduction"] = task_info.reduction;
}

USED_BY_JSON void from_json(const Json &j, ReduceScatterReshardTask &reduce_scatter_task_info) {
  reduce_scatter_task_info.reduction = j.at("reduction").get<std::string>();
  reduce_scatter_task_info.comm_groups = j.at("comm_groups").get<std::vector<CommGroup>>();
}

USED_BY_JSON void to_json(Json &j, const BroadcastReshardTask &task_info) {
  j = Json();
  j["task_type"] = "HcomBroadcast";
  j["comm_groups"] = task_info.comm_groups;
  j["roots"] = task_info.root_device_indices;
}

USED_BY_JSON void from_json(const Json &j, BroadcastReshardTask &broadcast_task_info) {
  broadcast_task_info.root_device_indices = j.at("roots").get<std::vector<DeviceIndex>>();
  broadcast_task_info.comm_groups = j.at("comm_groups").get<std::vector<CommGroup>>();
}

USED_BY_JSON void to_json(Json &j, const SliceReshardTask &task_info) {
  j = Json();
  j["task_type"] = "Slice";
  j["offsets"] = task_info.offsets;
  j["size"] = task_info.sizes;
}

USED_BY_JSON void from_json(const Json &j, SliceReshardTask &task_info) {
  task_info.offsets = j.at("offsets").get<std::vector<int64_t>>();
  task_info.sizes = j.at("size").get<std::vector<int64_t>>();
}

USED_BY_JSON void to_json(Json &j, const ConcatReshardTask &task_info) {
  j = Json();
  j["task_type"] = "Concat";
  j["concat_dim"] = task_info.concat_dim;
}

USED_BY_JSON void from_json(const Json &j, ConcatReshardTask &task_info) {
  task_info.concat_dim = j.at("concat_dim").get<int32_t>();
}

USED_BY_JSON void to_json(Json &j, const SplitReshardTask &task_info) {
  j = Json();
  j["task_type"] = "SplitV";
  j["size_splits"] = task_info.size_splits;
  j["split_dim"] = task_info.split_axis;
}

USED_BY_JSON void from_json(const Json &j, SplitReshardTask &task_info) {
  task_info.size_splits = j.at("size_splits").get<std::vector<int64_t>>();
  task_info.split_axis = j.at("split_dim").get<int32_t>();
}

USED_BY_JSON void to_json(Json &j, const TransposeReshardTask &task_info) {
  j = Json();
  j["task_type"] = "Transpose";
  j["perm"] = task_info.perm;
}

USED_BY_JSON void from_json(const Json &j, TransposeReshardTask &task_info) {
  task_info.perm = j.at("perm").get<std::vector<int32_t>>();
}

USED_BY_JSON void to_json(Json &j, const ModifyValueReshardTask &task_info) {
  j = Json();
  j["task_type"] = "ModifyValue";
  j["op_type"] = task_info.op_type;
  j["value"] = task_info.value;
}

USED_BY_JSON void from_json(const Json &j, ModifyValueReshardTask &task_info) {
  task_info.op_type = j.at("op_type").get<std::string>();
  task_info.value = j.at("value").get<std::vector<int64_t>>();
}

USED_BY_JSON void to_json(Json &j, const CommTask &comm_task) {
  GE_CHK_STATUS(CommTaskBuilder::GetInstance().ConvertToJson(comm_task, j));
}

USED_BY_JSON void from_json(const Json &j, CommTask &comm_task) {
  CommTaskBuilder::GetInstance().BuildCommTask(j, comm_task);
}

USED_BY_JSON void to_json(Json &j, const CommStepInput &step_input) {
  j = std::vector<int32_t>{step_input.step_id, step_input.output_index};
}

USED_BY_JSON void from_json(const Json &j, CommStepInput &step_input) {
  const auto step_id_and_out_index = j.get<std::vector<int32_t>>();
  size_t num_items = step_id_and_out_index.size();
  if (num_items > kIndexStepId) {
    step_input.step_id = step_id_and_out_index[kIndexStepId];
  }
  if (num_items > kIndexOutputIndex) {
    step_input.output_index = step_id_and_out_index[kIndexOutputIndex];
  }
}

USED_BY_JSON void to_json(Json &j, const CommStep &comm_step) {
  j = Json();
  j["id"] = comm_step.id;
  if (!comm_step.inputs.empty()) {
    j["input_ids"] = comm_step.inputs;
  }
  j["comm_task"] = comm_step.comm_task;
}

USED_BY_JSON void from_json(const Json &j, CommStep &comm_step) {
  comm_step.id = j.at("id").get<int32_t>();
  if (j.contains("input_ids")) {
    comm_step.inputs = j.at("input_ids").get<std::vector<CommStepInput>>();
  }
  comm_step.comm_task = j.at("comm_task").get<CommTask>();
}

USED_BY_JSON void to_json(Json &j, const PeerInput &peer_input) {
  j = Json();
  j["step_id"] = peer_input.step_id;
  j["node_name"] = peer_input.node_name;
  j["input_index"] = peer_input.input_index;
}

USED_BY_JSON void from_json(const Json &j, PeerInput &peer_input) {
  peer_input.step_id = j.at("step_id").get<int32_t>();
  peer_input.node_name = j.at("node_name").get<std::string>();
  peer_input.input_index = j.at("input_index").get<uint32_t>();
}

USED_BY_JSON void to_json(Json &j, const OutputReshardRes &reshard_res) {
  j = Json();
  j["comm_steps"] = reshard_res.comm_steps;
  j["outputs"] = reshard_res.peer_inputs;
  j["device_list"] = reshard_res.device_indices;
}

USED_BY_JSON void from_json(const Json &j, OutputReshardRes &reshard_res) {
  reshard_res.comm_steps = j.at("comm_steps").get<std::vector<CommStep>>();
  reshard_res.peer_inputs = j.at("outputs").get<std::vector<PeerInput>>();
  reshard_res.device_indices = j.at("device_list").get<std::vector<DeviceIndex>>();
}

USED_BY_JSON void to_json(Json &j, const ReshardAttr &reshard_attr) {
  j = reshard_attr.reshard_infos;
}

USED_BY_JSON void from_json(const Json &j, ReshardAttr &reshard_attr) {
  reshard_attr.reshard_infos = j.get<std::vector<std::vector<OutputReshardRes>>>();
}

Status TensorParallelAttrs::FromJson(const std::string &json_str, DeviceIndex &device_index) {
  return ParseFromJson("DeviceIndex", json_str, device_index);
}

Status TensorParallelAttrs::FromJson(const std::string &json_str,
                                     ReshardAttr &reshard_attr) {
  return ParseFromJson("ReshardRes", json_str, reshard_attr);
}

Status TensorParallelAttrs::FromJson(const std::string &json_str,
                                     TensorDeployment &tensor_deployment) {
  return ParseFromJson("TensorDeployment", json_str, tensor_deployment);
}

Status TensorParallelAttrs::FromJson(const std::string &json_str, CommTask &comm_task) {
  return ParseFromJson("CommTask", json_str, comm_task);
}

Status TensorParallelAttrs::FromJson(const std::string &json_str, CommStep &comm_step) {
  return ParseFromJson("CommStep", json_str, comm_step);
}

Status TensorParallelAttrs::FromJson(const std::string &json_str,
                                     OutputReshardRes &output_reshard_res) {
  return ParseFromJson("TensorReshardInfo", json_str, output_reshard_res);
}

Status TensorParallelAttrs::FromJson(const std::string &json_str, NodeDeployment &node_deployment) {
  return ParseFromJson("NodeDeployment", json_str, node_deployment);
}

std::string TensorParallelAttrs::ToJson(const DeviceIndex &device_index) {
  Json j = device_index;
  return j.dump();
}

std::string TensorParallelAttrs::ToJson(const NodeDeployment &node_deployment) {
  Json j = node_deployment;
  return j.dump();
}

std::string TensorParallelAttrs::ToJson(const TensorDeployment &tensor_deployment) {
  Json j = tensor_deployment;
  return j.dump();
}

std::string TensorParallelAttrs::ToJson(const ReshardAttr &reshard_attr) {
  Json j = reshard_attr;
  return j.dump();
}

CommTaskBuilder::CommTaskBuilder() {
  InitCommTaskBuilders();
  InitJsonConverters();
}

void CommTaskBuilder::InitCommTaskBuilders() {
  builders_["Slice"] = [](const Json &j, CommTask &comm_task) {
    comm_task.slice_reshard_task = CreateReshardTaskInfo<SliceReshardTask>(j);
  };
  builders_["SplitV"] = [](const Json &j, CommTask &comm_task) {
    comm_task.split_reshard_task = CreateReshardTaskInfo<SplitReshardTask>(j);
  };
  builders_["Concat"] = [](const Json &j, CommTask &comm_task) {
    comm_task.concat_reshard_task = CreateReshardTaskInfo<ConcatReshardTask>(j);
  };
  builders_["Transpose"] = [](const Json &j, CommTask &comm_task) {
    comm_task.transpose_reshard_task = CreateReshardTaskInfo<TransposeReshardTask>(j);
  };
  builders_["HcomAllGather"] = [](const Json &j, CommTask &comm_task) {
    comm_task.all_gather_reshard_task = CreateReshardTaskInfo<AllGatherReshardTask>(j);
  };
  builders_["HcomAllReduce"] = [](const Json &j, CommTask &comm_task) {
    comm_task.all_reduce_reshard_task = CreateReshardTaskInfo<AllReduceReshardTask>(j);
  };
  builders_["HcomReduceScatter"] = [](const Json &j, CommTask &comm_task) {
    comm_task.reduce_scatter_reshard_task = CreateReshardTaskInfo<ReduceScatterReshardTask>(j);
  };
  builders_["HcomBroadcast"] = [](const Json &j, CommTask &comm_task) {
    comm_task.broadcast_reshard_task = CreateReshardTaskInfo<BroadcastReshardTask>(j);
  };
  builders_["HcomAllToAll"] = [](const Json &j, CommTask &comm_task) {
    comm_task.all_to_all_reshard_task = CreateReshardTaskInfo<AllToAllReshardTask>(j);
  };
  builders_["SendReceive"] = [](const Json &j, CommTask &comm_task) {
    comm_task.send_recv_reshard_task = CreateReshardTaskInfo<SendRecvReshardTask>(j);
  };
  builders_["ModifyValue"] = [](const Json &j, CommTask &comm_task) {
    comm_task.modify_value_reshard_task = CreateReshardTaskInfo<ModifyValueReshardTask>(j);
  };
}

template<typename T>
Status CommTaskBuilder::ConvertToJson(const T *reshard_task, nlohmann::json &j) {
  GE_CHECK_NOTNULL(reshard_task);
  j = *reshard_task;
  return SUCCESS;
}

void CommTaskBuilder::InitJsonConverters() {
  json_converters_["Slice"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.slice_reshard_task.get(), j);
  };
  json_converters_["SplitV"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.split_reshard_task.get(), j);
  };
  json_converters_["Concat"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.concat_reshard_task.get(), j);
  };
  json_converters_["Transpose"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.transpose_reshard_task.get(), j);
  };
  json_converters_["HcomAllGather"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.all_gather_reshard_task.get(), j);
  };
  json_converters_["HcomAllReduce"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.all_reduce_reshard_task.get(), j);
  };
  json_converters_["HcomReduceScatter"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.reduce_scatter_reshard_task.get(), j);
  };
  json_converters_["HcomBroadcast"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.broadcast_reshard_task.get(), j);
  };
  json_converters_["HcomAllToAll"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.all_to_all_reshard_task.get(), j);
  };
  json_converters_["SendReceive"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.send_recv_reshard_task.get(), j);
  };
  json_converters_["ModifyValue"] = [](const CommTask &comm_task, nlohmann::json &j) {
    return ConvertToJson(comm_task.modify_value_reshard_task.get(), j);
  };
}

bool operator==(const DeviceIndex &lhs, const DeviceIndex &rhs) {
  return lhs.engine_type == rhs.engine_type &&
      lhs.indices == rhs.indices;
}

bool operator!=(const DeviceIndex &lhs, const DeviceIndex &rhs) {
  return !(rhs == lhs);
}

bool operator<(const DeviceIndex &lhs, const DeviceIndex &rhs) {
  if (lhs.engine_type < rhs.engine_type) {
    return true;
  }
  if (rhs.engine_type < lhs.engine_type) {
    return false;
  }
  return lhs.indices < rhs.indices;
}

bool operator==(const CommStepInput &lhs, const CommStepInput &rhs) {
  return (lhs.step_id == rhs.step_id) && (lhs.output_index == rhs.output_index);
}

bool operator<(const CommStepInput &lhs, const CommStepInput &rhs) {
  if (lhs.step_id < rhs.step_id) {
    return true;
  }
  if (rhs.step_id < lhs.step_id) {
    return false;
  }
  return lhs.output_index < rhs.output_index;
}
}  // namespace tp
}  // namespace ge