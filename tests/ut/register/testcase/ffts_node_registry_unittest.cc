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

#include "register/ffts_node_calculater_registry.h"
#include "register/ffts_node_converter_registry.h"
#include <gtest/gtest.h>

class FFTSNodeRegistryUnittest : public testing::Test {};

namespace TestFFTSNodeRegistry {
gert::LowerResult TestFftsLowFunc(const ge::NodePtr &node, const gert::FFTSLowerInput &lower_input) {
  return {};
}

ge::graphStatus TestFftsCalcFunc(const ge::NodePtr &node, const gert::LoweringGlobalData *global_data,
    size_t &total_size, size_t &pre_data_size, std::unique_ptr<uint8_t[]> &pre_data_ptr) {
  return ge::GRAPH_SUCCESS;
}

TEST_F(FFTSNodeRegistryUnittest, ConverterRegisterSuccess_Test) {
  EXPECT_EQ(gert::FFTSNodeConverterRegistry::GetInstance().FindNodeConverter("RegisterSuccess1"), nullptr);
  FFTS_REGISTER_NODE_CONVERTER("RegisterSuccess1", TestFftsLowFunc);
  EXPECT_EQ(gert::FFTSNodeConverterRegistry::GetInstance().FindNodeConverter("RegisterSuccess1"), TestFftsLowFunc);
}

TEST_F(FFTSNodeRegistryUnittest, CalculaterRegisterSuccess_Test) {
  EXPECT_EQ(gert::FFTSNodeCalculaterRegistry::GetInstance().FindNodeCalculater("RegisterSuccess2"), nullptr);
  FFTS_REGISTER_NODE_CALCULATER("RegisterSuccess2", TestFftsCalcFunc);
  EXPECT_EQ(gert::FFTSNodeCalculaterRegistry::GetInstance().FindNodeCalculater("RegisterSuccess2"), TestFftsCalcFunc);
}
}
