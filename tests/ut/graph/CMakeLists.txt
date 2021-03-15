# Copyright 2019-2020 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

project(ut_graph)

set(CMAKE_CXX_STANDARD 11)

set(PROTO_LIST
    "${METADEF_DIR}/proto/om.proto"
    "${METADEF_DIR}/proto/ge_ir.proto"
    "${METADEF_DIR}/proto/insert_op.proto"
    "${METADEF_DIR}/proto/task.proto"
    "${METADEF_DIR}/proto/dump_task.proto"
    "${METADEF_DIR}/proto/fwk_adapter.proto"
    "${METADEF_DIR}/proto/op_mapping_info.proto"
    "${METADEF_DIR}/proto/proto_inner/ge_onnx.proto"
)

protobuf_generate(ge PROTO_SRCS PROTO_HDRS ${PROTO_LIST})

# include directories
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../inc)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../inc/graph)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../inc/external)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../inc/external/graph)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../graph)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../third_party)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../third_party/graphengine/inc)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../third_party/graphengine/inc/external)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../third_party/graphengine/inc/external/ge)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../third_party/fwkacllib/inc)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../third_party/transformer/inc)
include_directories(${CMAKE_CURRENT_LIST_DIR} ../../../)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/proto/ge)
include_directories(${CMAKE_BINARY_DIR}/proto/ge/proto)

set(UT_FILES
    "testcase/graph_unittest.cc"
    "testcase/types_unittest.cc"
    "testcase/type_utils_unittest.cc"
)

set(SRC_FILES
    "../../../graph/types.cc"
    "../../../graph/anchor.cc"
    "../../../graph/ge_attr_value.cc"
    "../../../graph/attr_value.cc"
    "../../../graph/buffer.cc"
    "../../../graph/aligned_ptr.cc"
    "../../../graph/compute_graph.cc"
    "../../../graph/ascend_string.cc"
    "../../../graph/gnode.cc"
    "../../../graph/graph.cc"
    "../../../graph/inference_context.cc"
    "../../../graph/shape_refiner.cc"
    "../../../graph/format_refiner.cc"
    "../../../graph/ref_relation.cc"
    "../../../graph/model.cc"
    "../../../graph/model_serialize.cc"
    "../../../graph/node.cc"
    "../../../graph/op_desc.cc"
    "../../../graph/operator.cc"
    "../../../graph/operator_factory.cc"
    "../../../graph/operator_factory_impl.cc"
    "../../../graph/ge_attr_define.cc"
    "../../../graph/ge_tensor.cc"
    "../../../graph/tensor.cc"
    "../../../graph/runtime_inference_context.cc"
    "../../../graph/debug/graph_debug.cc"
    "../../../graph/detail/attributes_holder.cc"
    "../../../graph/opsproto/opsproto_manager.cc"
    "../../../graph/option/ge_context.cc"
    "../../../graph/option/ge_local_context.cc"
    "../../../graph/utils/anchor_utils.cc"
    "../../../graph/utils/tuning_utils.cc"
    "../../../graph/utils/graph_utils.cc"
    "../../../graph/utils/ge_ir_utils.cc"
    "../../../graph/utils/node_utils.cc"
    "../../../graph/utils/op_desc_utils.cc"
    "../../../graph/utils/type_utils.cc"
    "../../../graph/utils/tensor_utils.cc"
    "../../../graph/utils/transformer_utils.cc"
    "../../../ops/op_imp.cpp"
    "${METADEF_DIR}/third_party/transformer/src/axis_util.cc"
    "${METADEF_DIR}/third_party/transformer/src/transfer_shape_according_to_format.cc"
)

add_executable(ut_graph ${UT_FILES} ${SRC_FILES} ${PROTO_SRCS} ${PROTO_HDRS})

target_compile_options(ut_graph PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
)

target_compile_definitions(ut_graph PRIVATE
    google=ascend_private
)

target_link_libraries(ut_graph 
    $<BUILD_INTERFACE:intf_pub>
    gtest
    gtest_main
    slog_stub
    ascend_protobuf
    c_sec
    error_manager_stub
    mmpa_stub
    -lrt
    -ldl
    -lgcov
)
