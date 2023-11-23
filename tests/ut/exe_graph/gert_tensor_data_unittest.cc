/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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
#include "inc/exe_graph/runtime/gert_tensor_data.h"

namespace gert {
class GertTensorDataUT : public testing::Test {};

TEST_F(GertTensorDataUT, Init_success) {
  GertTensorData gert_tensor_data = GertTensorData();
  ASSERT_EQ(gert_tensor_data.GetSize(), 0U);
  ASSERT_EQ(gert_tensor_data.GetPlacement(), kTensorPlacementEnd);
  ASSERT_EQ(gert_tensor_data.GetStreamId(), -1);
}

TEST_F(GertTensorDataUT, SetSize_SetPlacement_SetAddr_success) {
  GertTensorData gert_tensor_data = GertTensorData();
  ASSERT_EQ(gert_tensor_data.GetSize(), 0U);
  ASSERT_EQ(gert_tensor_data.GetPlacement(), kTensorPlacementEnd);
  ASSERT_EQ(gert_tensor_data.GetStreamId(), -1);

  gert_tensor_data.SetSize(100U);
  ASSERT_EQ(gert_tensor_data.GetSize(), 100U);

  gert_tensor_data.SetPlacement(kOnDeviceHbm);
  ASSERT_EQ(gert_tensor_data.GetPlacement(), kOnDeviceHbm);

  gert_tensor_data.MutableTensorData().SetAddr((void *)1, nullptr);
  ASSERT_EQ(gert_tensor_data.GetAddr(), (void *)1);
}
}  // namespace gert