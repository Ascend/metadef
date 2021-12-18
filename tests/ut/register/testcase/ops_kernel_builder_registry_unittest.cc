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

#include <gtest/gtest.h>

#define protected public
#define private public

#include "register/ops_kernel_builder_registry.h"
#include "graph/debug/ge_log.h"

#define protected public
#define private public

namespace ge {
class UtestOpsKernelBuilderRegistry : public testing::Test { 
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestOpsKernelBuilderRegistry, GetAllKernelBuildersTest) {
    ge::OpsKernelBuilderRegistry ops_registry;
    OpsKernelBuilderPtr opsptr = std::shared_ptr<OpsKernelBuilder>();
    ops_registry.kernel_builders_.insert(pair<std::string, OpsKernelBuilderPtr>("ops1", opsptr));
    std::map<std::string, OpsKernelBuilderPtr> ops_map;
    ops_map = ops_registry.GetAll();
    EXPECT_EQ(ops_map.size(), 1);
    EXPECT_EQ(ops_map["ops1"], opsptr);
}

} // namespace ge
