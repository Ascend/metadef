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
#include "inc/register/register_format_transfer.h"

namespace ge {
  class UtestRegisterFormatTransfer : public testing::Test {
  protected:
    void SetUp() {}
    void TearDown() {}
  };

TEST_F(UtestRegisterFormatTransfer, FormatTransferExists_failed) {
  uint16_t data_5d[1 * 1 * 1 * 1 * 16] = {
    13425, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  formats::TransArgs args{
    reinterpret_cast<uint8_t *>(data_5d), FORMAT_NC1HWC0, FORMAT_NCHW,
      {1, 1, 1, 1, 16}, {1, 1, 1, 1}, DT_FLOAT16};
  EXPECT_EQ(ge::formats::FormatTransferExists(args), false);
}
}