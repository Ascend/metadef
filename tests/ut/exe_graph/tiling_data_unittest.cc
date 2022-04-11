/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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
#include "exe_graph/runtime/tiling_data.h"
#include <gtest/gtest.h>
namespace gert {
class TilingDataUT : public testing::Test {};
namespace {
struct TestData {
  int64_t a;
  int32_t b;
  int16_t c;
  int16_t d;
};
}
TEST_F(TilingDataUT, AppendSameTypesOk) {
  auto data = TilingData::CreateCap(2048);
  auto tiling_data = reinterpret_cast<TilingData *>(data.get());
  ASSERT_NE(tiling_data, nullptr);
  std::vector<int64_t> expect_vec;
  for (int64_t i = 0; i < 10; ++i) {
    tiling_data->Append(i);
    expect_vec.push_back(i);
  }
  ASSERT_EQ(tiling_data->GetDataSize(), 80);
  EXPECT_EQ(memcmp(tiling_data->GetData(), expect_vec.data(), tiling_data->GetDataSize()), 0);
}
TEST_F(TilingDataUT, AppendStructOk) {
  auto data = TilingData::CreateCap(2048);
  auto tiling_data = reinterpret_cast<TilingData *>(data.get());
  ASSERT_NE(tiling_data, nullptr);
  TestData td {
      .a = 1024,
      .b = 512,
      .c = 256,
      .d = 128
  };
  tiling_data->Append(td);
  ASSERT_EQ(tiling_data->GetDataSize(), sizeof(td));
  EXPECT_EQ(memcmp(tiling_data->GetData(), &td, tiling_data->GetDataSize()), 0);
}

TEST_F(TilingDataUT, AppendDifferentTypes) {
  auto data = TilingData::CreateCap(2048);
  auto tiling_data = reinterpret_cast<TilingData *>(data.get());
  ASSERT_NE(tiling_data, nullptr);
  std::vector<int64_t> expect_vec1;
  for (int64_t i = 0; i < 10; ++i) {
    tiling_data->Append(i);
    expect_vec1.push_back(i);
  }

  std::vector<int32_t> expect_vec2;
  for (int32_t i = 0; i < 3; ++i) {
    tiling_data->Append(i);
    expect_vec2.push_back(i);
  }

  TestData td {
      .a = 1024,
      .b = 512,
      .c = 256,
      .d = 128
  };
  tiling_data->Append(td);

  ASSERT_EQ(tiling_data->GetDataSize(), 80 + 12 + sizeof(TestData));
  EXPECT_EQ(memcmp(tiling_data->GetData(), expect_vec1.data(), 80), 0);
  EXPECT_EQ(memcmp(reinterpret_cast<uint8_t*>(tiling_data->GetData()) + 80, expect_vec2.data(), 12), 0);
  EXPECT_EQ(memcmp(reinterpret_cast<uint8_t*>(tiling_data->GetData()) + 92, &td, sizeof(td)), 0);
}
}  // namespace gert