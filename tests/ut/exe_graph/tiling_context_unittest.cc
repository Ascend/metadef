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
#include "exe_graph/runtime/tiling_context.h"
#include <gtest/gtest.h>
#include "faker/kernel_run_context_facker.h"

namespace gert {
class TilingContextUT : public testing::Test {};
namespace {
struct TestTilingData {
  int64_t a;
};
}

TEST_F(TilingContextUT, GetTypedTilingDataOk) {
  gert::StorageShape in_shape = {{1, 16, 256}, {1, 16, 256}};
  gert::StorageShape out_shape = {{1, 16, 256}, {1, 16, 1, 16, 16}};

  // tiling data
  auto param = gert::TilingData::CreateCap(2048);
  auto holder = gert::TilingContextFaker()
                    .NodeIoNum(1, 1)
                    .IrInputNum(1)
                    .NodeInputTd(0, ge::DT_FLOAT16, ge::FORMAT_ND, ge::FORMAT_ND)
                    .NodeOutputTd(0, ge::DT_FLOAT16, ge::FORMAT_ND, ge::FORMAT_FRACTAL_NZ)
                    .InputShapes({&in_shape})
                    .OutputShapes({&out_shape})
                    .TilingData(param.get())
                    .Build();

  auto context = holder.GetContext<TilingContext>();
  auto tiling_data = context->GetTilingData<TestTilingData>();
  ASSERT_NE(tiling_data, nullptr);
  tiling_data->a = 10;
  auto root_tiling_data = reinterpret_cast<TilingData *>(param.get());

  EXPECT_EQ(root_tiling_data->GetDataSize(), sizeof(TestTilingData));
  EXPECT_EQ(root_tiling_data->GetData(), tiling_data);
}

TEST_F(TilingContextUT, GetAppendTilingDataOk) {
  gert::StorageShape in_shape = {{1, 16, 256}, {1, 16, 256}};
  gert::StorageShape out_shape = {{1, 16, 256}, {1, 16, 1, 16, 16}};

  // tiling data
  auto param = gert::TilingData::CreateCap(2048);
  auto holder = gert::TilingContextFaker()
                    .NodeIoNum(1, 1)
                    .IrInputNum(1)
                    .NodeInputTd(0, ge::DT_FLOAT16, ge::FORMAT_ND, ge::FORMAT_ND)
                    .NodeOutputTd(0, ge::DT_FLOAT16, ge::FORMAT_ND, ge::FORMAT_FRACTAL_NZ)
                    .InputShapes({&in_shape})
                    .OutputShapes({&out_shape})
                    .TilingData(param.get())
                    .Build();

  auto context = holder.GetContext<TilingContext>();

  // 算子tiling中可以用如下操作append
  auto tiling_data = context->GetRawTilingData();
  ASSERT_NE(tiling_data, nullptr);

  tiling_data->Append(static_cast<int64_t>(10));
  tiling_data->Append(static_cast<int64_t>(20));
  tiling_data->Append(static_cast<int64_t>(30));
  tiling_data->Append(static_cast<int32_t>(40));
  tiling_data->Append(static_cast<int16_t>(50));
  tiling_data->Append(static_cast<int8_t>(60));

  EXPECT_EQ(context->GetRawTilingData()->GetDataSize(), 31);  // 3 * 8 + 4 + 2 + 1
}

}  // namespace gert