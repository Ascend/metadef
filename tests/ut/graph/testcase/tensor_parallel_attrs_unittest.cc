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

#include <gtest/gtest.h>
#include "nlohmann/json.hpp"

#define private public
#include "graph/parallelism/tensor_parallel_attrs.h"
#include "ge/ge_api_error_codes.h"
#undef private
#include "common/ge_inner_error_codes.h"

using namespace testing;
namespace ge {
namespace tp {
using Json = nlohmann::json;

class TensorParallelAttrsTest : public testing::Test {
 protected:
  static void TestToAndFromJson(const CommTask &comm_task, CommTask &out_comm_task) {
    ReshardAttr reshard_attr;
    OutputReshardRes output_reshard_res;
    CommStep comm_step;
    comm_step.id = 0;
    comm_step.comm_task = comm_task;
    output_reshard_res.comm_steps.emplace_back(comm_step);
    reshard_attr.reshard_infos.emplace_back(std::vector<OutputReshardRes>{output_reshard_res});
    const auto &json_str = TensorParallelAttrs::ToJson(reshard_attr);
    ASSERT_TRUE(!json_str.empty());
    std::cout << json_str << std::endl;
    ReshardAttr reshard_attr_from_json;
    ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, reshard_attr_from_json), SUCCESS);
    out_comm_task = reshard_attr_from_json.reshard_infos[0][0].comm_steps[0].comm_task;
  }
};

TEST_F(TensorParallelAttrsTest, ParseFailed_InvalidJsonStr) {
  std::string json_str = "invalid";
  DeviceIndex device_index;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, device_index), PARAM_INVALID);
}

TEST_F(TensorParallelAttrsTest, ParseFailed_FieldMismatches) {
  std::string json_str = R"(
      {"engine_type": "NPU", "index": 0}
)";
  DeviceIndex device_index;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, device_index), PARAM_INVALID);
}

TEST_F(TensorParallelAttrsTest, DeviceIndex_ToAndFromJsonStr) {
  DeviceIndex device_index;
  device_index.indices = {0, 1, 2};
  device_index.engine_type = "MyEngine";
  std::string json_str = TensorParallelAttrs::ToJson(device_index);
  DeviceIndex another_device_index;
  TensorParallelAttrs::FromJson(json_str, another_device_index);
  EXPECT_EQ(device_index, another_device_index);
}

TEST_F(TensorParallelAttrsTest, DeviceIndex_operators) {
  std::map<DeviceIndex, int32_t> device_index_to_value;
  DeviceIndex device_index;
  device_index.indices = {0, 1, 2};
  device_index.engine_type = "MyEngine";

  DeviceIndex another_device_index;
  device_index.indices = {0, 1, 2};
  device_index.engine_type = "CPU";
  ASSERT_TRUE(device_index_to_value.emplace(device_index, 0).second);
  ASSERT_FALSE(device_index_to_value.emplace(device_index, 0).second);
  ASSERT_FALSE(device_index.DebugString().empty());
  ASSERT_EQ(device_index_to_value.count(another_device_index), 0U);
  ASSERT_NE(device_index, another_device_index);
  ASSERT_EQ(device_index, device_index);
}

TEST_F(TensorParallelAttrsTest, NodeDeployment_ToAndFromJsonStr) {
  DeviceIndex device_index_0;
  device_index_0.indices = {0, 0, 1};
  device_index_0.engine_type = "CPU";

  DeviceIndex device_index_1;
  device_index_1.indices = {0, 0, 2};
  device_index_1.engine_type = "NPU";

  NodeDeployment node_deployment;
  node_deployment.devices = {device_index_0, device_index_1};
  std::string json_str = TensorParallelAttrs::ToJson(node_deployment);
  NodeDeployment another_node_deployment;
  TensorParallelAttrs::FromJson(json_str, another_node_deployment);
  EXPECT_EQ(node_deployment.devices, another_node_deployment.devices);
}

TEST_F(TensorParallelAttrsTest, ParseSendRecvTaskInfo) {
  const std::string &json_str =
      R"(
{
  "task_type": "SendReceive",
  "comm_pairs": [
    {
      "src_device_index": {"engine_type": "NPU", "index": [0, 0, 1]},
      "dst_device_index": {"engine_type": "NPU", "index": [0, 0, 2]}
    }
  ]
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);
  ASSERT_TRUE(comm_task.send_recv_reshard_task != nullptr);
  ASSERT_EQ(comm_task.send_recv_reshard_task->comm_pairs.size(), 1U);
  EXPECT_EQ(comm_task.send_recv_reshard_task->comm_pairs[0].src_device_index.indices, (std::vector<int32_t>{0, 0, 1}));
  EXPECT_EQ(comm_task.send_recv_reshard_task->comm_pairs[0].dst_device_index.indices, (std::vector<int32_t>{0, 0, 2}));

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
}

TEST_F(TensorParallelAttrsTest, ParseAllGatherCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "HcomAllGather",
  "comm_groups": [
    [
      {"engine_type": "NPU", "index": [0, 0, 0]},
      {"engine_type": "NPU", "index": [0, 0, 1]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 2]},
      {"engine_type": "NPU", "index": [0, 0, 3]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 4]},
      {"engine_type": "NPU", "index": [0, 0, 5]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 6]},
      {"engine_type": "NPU", "index": [0, 0, 7]}
    ]
  ],
  "axis": 0
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(out_comm_task.all_gather_reshard_task != nullptr);
  EXPECT_EQ(out_comm_task.all_gather_reshard_task->comm_groups.size(), 4);
}

TEST_F(TensorParallelAttrsTest, ParseAllReduceCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "HcomAllReduce",
  "reduction": "sum",
  "comm_groups": [
    [
      {"engine_type": "NPU", "index": [0, 0, 0]},
      {"engine_type": "NPU", "index": [0, 0, 1]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 2]},
      {"engine_type": "NPU", "index": [0, 0, 3]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 4]},
      {"engine_type": "NPU", "index": [0, 0, 5]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 6]},
      {"engine_type": "NPU", "index": [0, 0, 7]}
    ]
  ]
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);
  ASSERT_TRUE(comm_task.all_reduce_reshard_task != nullptr);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  EXPECT_EQ(out_comm_task.all_reduce_reshard_task->reduction, "sum");
  EXPECT_EQ(out_comm_task.all_reduce_reshard_task->comm_groups.size(), 4);
}

TEST_F(TensorParallelAttrsTest, ParseAllReduceMeanCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "HcomAllReduceMean",
  "comm_groups": [
    [
      {"engine_type": "NPU", "index": [0, 0, 0]},
      {"engine_type": "NPU", "index": [0, 0, 1]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 2]},
      {"engine_type": "NPU", "index": [0, 0, 3]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 4]},
      {"engine_type": "NPU", "index": [0, 0, 5]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 6]},
      {"engine_type": "NPU", "index": [0, 0, 7]}
    ]
  ],
  "axis": 0,
  "value": 2
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);
  ASSERT_TRUE(comm_task.all_reduce_mean_reshard_task != nullptr);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  EXPECT_EQ(out_comm_task.all_reduce_mean_reshard_task->comm_groups.size(), 4);
}

TEST_F(TensorParallelAttrsTest, ParseReduceScatterCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "HcomReduceScatter",
  "reduction": "sum",
  "comm_groups": [
    [
      {"engine_type": "NPU", "index": [0, 0, 0]},
      {"engine_type": "NPU", "index": [0, 0, 1]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 2]},
      {"engine_type": "NPU", "index": [0, 0, 3]}
    ]
  ]
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);
  ASSERT_TRUE(comm_task.reduce_scatter_reshard_task != nullptr);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  EXPECT_EQ(out_comm_task.reduce_scatter_reshard_task->reduction, "sum");
  EXPECT_EQ(out_comm_task.reduce_scatter_reshard_task->comm_groups.size(), 2);
}

TEST_F(TensorParallelAttrsTest, ParseAllToAllCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "HcomAllToAll",
  "comm_groups": [
    [
      {"engine_type": "NPU", "index": [0, 0, 0]},
      {"engine_type": "NPU", "index": [0, 0, 1]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 2]},
      {"engine_type": "NPU", "index": [0, 0, 3]}
    ]
  ]
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(out_comm_task.all_to_all_reshard_task != nullptr);
  EXPECT_EQ(out_comm_task.all_to_all_reshard_task->comm_groups.size(), 2);
}

TEST_F(TensorParallelAttrsTest, ParseSliceCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "Slice",
  "offsets": [2, 4],
  "size": [4, 8]
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(out_comm_task.slice_reshard_task != nullptr);
  ASSERT_EQ(out_comm_task.slice_reshard_task->offsets, (std::vector<int64_t>{2, 4}));
  ASSERT_EQ(out_comm_task.slice_reshard_task->sizes, (std::vector<int64_t>{4, 8}));
}

TEST_F(TensorParallelAttrsTest, ParseSliceByAxisCommTask) {
  CommTask comm_task;
  comm_task.task_type = "SliceByAxis";
//  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);
  comm_task.slice_by_axis_reshard_task = std::make_shared<SliceByAxisReshardTask>();
  auto &axis_to_slice_deployments = comm_task.slice_by_axis_reshard_task->axis_to_slice_deployments;
  std::vector<DeviceIndex> dim_0_slice_0_deployments{DeviceIndex{"NPU", {0, 0, 0}}, DeviceIndex{"NPU", {0, 0, 1}}};
  std::vector<DeviceIndex> dim_0_slice_1_deployments{DeviceIndex{"NPU", {0, 0, 2}}, DeviceIndex{"NPU", {0, 0, 3}}};
  std::vector<DeviceIndex> dim_1_slice_0_deployments{DeviceIndex{"NPU", {0, 1, 0}}, DeviceIndex{"NPU", {0, 1, 1}}};
  std::vector<DeviceIndex> dim_1_slice_1_deployments{DeviceIndex{"NPU", {0, 1, 2}}, DeviceIndex{"NPU", {0, 1, 3}}};

  axis_to_slice_deployments[0].emplace_back(dim_0_slice_0_deployments);
  axis_to_slice_deployments[0].emplace_back(dim_0_slice_1_deployments);
  axis_to_slice_deployments[1].emplace_back(dim_1_slice_0_deployments);
  axis_to_slice_deployments[2].emplace_back(dim_1_slice_1_deployments);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(out_comm_task.slice_by_axis_reshard_task != nullptr);
}

TEST_F(TensorParallelAttrsTest, ParseSplitCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "Split",
  "split_dim": 1,
  "num_split": 2
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(out_comm_task.split_reshard_task != nullptr);
  ASSERT_EQ(out_comm_task.split_reshard_task->split_dim, 1);
  ASSERT_EQ(out_comm_task.split_reshard_task->num_split, 2);
}

TEST_F(TensorParallelAttrsTest, ParseConcatCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "Concat",
  "concat_dim": 1
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(out_comm_task.concat_reshard_task != nullptr);
  ASSERT_EQ(out_comm_task.concat_reshard_task->concat_dim, 1);
}

TEST_F(TensorParallelAttrsTest, ParseTransposeTaskInfo) {
  const std::string &json_str =
      R"(
{
  "task_type": "Transpose",
  "perm": [1, 0, 2, 3]
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(out_comm_task.transpose_reshard_task != nullptr);
  ASSERT_EQ(out_comm_task.transpose_reshard_task->perm, (std::vector<int32_t>{1, 0, 2, 3}));
}

TEST_F(TensorParallelAttrsTest, ParseModifyValueCommTask) {
  const std::string &json_str = R"(
{
  "task_type": "ModifyValue",
  "op_type": "Mul",
  "value": [1, 2]
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(out_comm_task.modify_value_reshard_task != nullptr);
  ASSERT_EQ(out_comm_task.modify_value_reshard_task->op_type, "Mul");
  ASSERT_EQ(out_comm_task.modify_value_reshard_task->value, (std::vector<int64_t>{1, 2}));
}

TEST_F(TensorParallelAttrsTest, ParseBroadcastCommTask) {
  const std::string &json_str =
      R"(
{
  "task_type": "HcomBroadcast",
  "roots": [
    {"engine_type": "NPU", "index": [0, 0, 0]},
    {"engine_type": "NPU", "index": [0, 0, 2]},
    {"engine_type": "NPU", "index": [0, 0, 4]},
    {"engine_type": "NPU", "index": [0, 0, 6]}
  ],
  "comm_groups": [
    [
      {"engine_type": "NPU", "index": [0, 0, 0]},
      {"engine_type": "NPU", "index": [0, 0, 1]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 2]},
      {"engine_type": "NPU", "index": [0, 0, 3]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 4]},
      {"engine_type": "NPU", "index": [0, 0, 5]}
    ],
    [
      {"engine_type": "NPU", "index": [0, 0, 6]},
      {"engine_type": "NPU", "index": [0, 0, 7]}
    ]
  ]
}
)";
  CommTask comm_task;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_task), SUCCESS);

  CommTask out_comm_task;
  TestToAndFromJson(comm_task, out_comm_task);
  ASSERT_TRUE(comm_task.broadcast_reshard_task != nullptr);
  std::vector<DeviceIndex> root_device_index;
  root_device_index.emplace_back(DeviceIndex{"NPU", {0, 0, 0}});
  root_device_index.emplace_back(DeviceIndex{"NPU", {0, 0, 2}});
  root_device_index.emplace_back(DeviceIndex{"NPU", {0, 0, 4}});
  root_device_index.emplace_back(DeviceIndex{"NPU", {0, 0, 6}});
  EXPECT_EQ(out_comm_task.broadcast_reshard_task->root_device_indices, root_device_index);
  EXPECT_EQ(out_comm_task.broadcast_reshard_task->comm_groups.size(), 4);
}

TEST_F(TensorParallelAttrsTest, ParseParseCommStep) {
  const std::string &json_str =
      R"(
{
  "id": 2,
  "input_ids": [[0, 0], [1, 0]],
  "comm_task": {
    "task_type": "Split",
    "num_split": 2,
    "split_dim": 1
  }
}
        )";
  CommStep comm_step;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, comm_step), SUCCESS);
  EXPECT_TRUE(comm_step.comm_task.split_reshard_task != nullptr);
  EXPECT_EQ(comm_step.id, 2);
}

TEST_F(TensorParallelAttrsTest, ParseTensorReshardInfo) {
  const std::string &json_str =
      R"(
{
  "output_index": 1,
  "device_list": [
    {"engine_type": "NPU", "index": [0, 0, 0]},
    {"engine_type": "NPU", "index": [0, 0, 2]},
    {"engine_type": "NPU", "index": [0, 0, 3]},
    {"engine_type": "NPU", "index": [0, 0, 1]}
  ],
  "comm_steps": [
    {
      "id": 1,
      "input_ids": [],
      "comm_task": {
        "task_type": "SplitVD",
        "size_splits": [2, 4],
        "split_dim": 1
      }
    },
    {
      "id": 2,
      "input_ids": [[1, 0]],
      "comm_task": {
        "task_type": "SplitVD",
        "size_splits": [2, 4],
        "split_dim": 1
      }
    }
  ],
  "peer_inputs": [
    {"step_id": 1, "node_name": "dst_node", "input_index": 0},
    {"step_id": 1, "node_name": "dst_node_1", "input_index": 1}
  ]
}
)";
  OutputReshardRes tensor_reshard_info;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, tensor_reshard_info), SUCCESS);
  EXPECT_EQ(tensor_reshard_info.comm_steps.size(), 2);
  ASSERT_EQ(tensor_reshard_info.peer_inputs.size(), 2);
  EXPECT_EQ(tensor_reshard_info.peer_inputs[0].step_id, 1);
  EXPECT_EQ(tensor_reshard_info.peer_inputs[0].node_name, "dst_node");
  EXPECT_EQ(tensor_reshard_info.peer_inputs[0].input_index, 0);
  EXPECT_EQ(tensor_reshard_info.peer_inputs[1].node_name, "dst_node_1");
  EXPECT_EQ(tensor_reshard_info.peer_inputs[1].step_id, 1);
  EXPECT_EQ(tensor_reshard_info.peer_inputs[1].input_index, 1);
}

TEST_F(TensorParallelAttrsTest, ReshardAttrToAndFromJson) {
  const std::string &json_str =
      R"(
[
  [
    {
      "device_list": [
        {"engine_type": "NPU", "index": [0, 0, 0]},
        {"engine_type": "NPU", "index": [0, 0, 2]},
        {"engine_type": "NPU", "index": [0, 0, 3]},
        {"engine_type": "NPU", "index": [0, 0, 1]}
      ],
      "comm_steps": [
        {
          "id": 1,
          "input_ids": [],
          "comm_task": {
            "task_type": "Split",
            "num_split": 3,
            "split_dim": 1
          }
        },
        {
          "id": 2,
          "input_ids": [[1, 0]],
          "comm_task": {
            "task_type": "Split",
            "num_split": 4,
            "split_dim": 1
          }
        }
      ],
      "peer_inputs": [
        {"step_id": 1, "node_name": "dst_node", "input_index": 0},
        {"step_id": 1, "node_name": "dst_node_1", "input_index": 1}
      ]
    }
  ]
]
)";
  ReshardAttr reshard_attr;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, reshard_attr), SUCCESS);
  ASSERT_EQ(reshard_attr.reshard_infos.size(), 1);
  auto str = TensorParallelAttrs::ToJson(reshard_attr);
  ASSERT_EQ(TensorParallelAttrs::FromJson(str, reshard_attr), SUCCESS);
}

TEST_F(TensorParallelAttrsTest, TensorDeploymentToAndFromJson) {
  const std::string &json_str =
      R"(
{
  "shard_deployment": {
    "device_indices_each_slice": [
      [{"engine_type": "NPU", "index": [0, 0, 0]}],
      [{"engine_type": "NPU", "index": [0, 0, 1]}],
      [{"engine_type": "NPU", "index": [0, 0, 2]}],
      [{"engine_type": "NPU", "index": [0, 0, 3]}]
    ],
    "axis_slices": [
      [[0, 2], [2, 4]],
      [[0, 4], [4, 8]]
    ]
  }
}
)";
  TensorDeployment tensor_deployment;
  ASSERT_EQ(TensorParallelAttrs::FromJson(json_str, tensor_deployment), SUCCESS);
  const auto str = TensorParallelAttrs::ToJson(tensor_deployment);

  TensorDeployment tensor_deployment_from_json;
  ASSERT_EQ(TensorParallelAttrs::FromJson(str, tensor_deployment_from_json), SUCCESS);
  const auto &tensor_slice_deployment = tensor_deployment_from_json.shard_deployment;
  EXPECT_EQ(tensor_slice_deployment.device_indices_each_slice.size(), 4);
  EXPECT_EQ(tensor_slice_deployment.axis_slices.size(), 2);
}
}  // namespace tp
}  // namespace ge