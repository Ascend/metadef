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

project(ut_register)

set(CMAKE_CXX_STANDARD 11)

# include directories
include_directories(${CMAKE_CURRENT_LIST_DIR})

set(UT_FILES
    "testcase/register_unittest.cc"
)

set(SRC_FILES

)

add_executable(ut_register ${UT_FILES} ${SRC_FILES})

target_compile_options(ut_register PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
)

target_compile_definitions(ut_register PRIVATE
    google=ascend_private
)

target_link_libraries(ut_register
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
