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
#include <bitset>
#define private public
#include "graph/ge_tensor.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "axis_util.h"
#include "expand_dimension.h"
#include "transfer_shape_according_to_format.h"
#include "transfer_range_according_to_format.h"

namespace transformer {
class TransformerExpandDimsUT : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}

  void RunExpandDimsCase(const ge::Format &origin_format, const ge::Format &format,
                         const string &reshape_type, const int64_t &expect_reshape_type,
                         const vector<int64_t> &dims, const vector<int64_t> &expect_dims) {
    std::cout << "RunExpandDimsCase: origin_format=" << origin_format << ", format=" << format
              << ", reahpe type=" << reshape_type << ", dim size=" << dims.size() << std::endl;
    string op_type = "Relu";
    uint32_t tensor_index = 0;
    ge::GeShape shape(dims);
    bool ret = ExpandDimension(op_type, origin_format, format, tensor_index, reshape_type, shape);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(shape.GetDims(), expect_dims);

    ge::GeShape new_shape(dims);
    int64_t int_reshape_type = ExpandDimension::GenerateReshapeType(origin_format, format, new_shape.GetDimNum(),
                                                                    reshape_type);
    if (int_reshape_type != 0) {
      size_t full_size = static_cast<size_t>(int_reshape_type >> 56);
      size_t expect_full_size = 0;
      ExpandDimension::GetFormatFullSize(origin_format, expect_full_size);
      EXPECT_EQ(full_size, expect_full_size);
    }
    EXPECT_EQ(int_reshape_type & 0xff, expect_reshape_type);

    ExpandDimension::ExpandDims(int_reshape_type, new_shape);
    EXPECT_EQ(new_shape.GetDims(), expect_dims);

    ge::GeShape shape_1(dims);
    ge::GeShape shape_2(dims);
    ExpandDimension::ExpandDims(int_reshape_type, shape_1, shape_2);
    EXPECT_EQ(shape_2.GetDims(), expect_dims);
  }

  void RunNewExpandDimsCase(const ge::Format &origin_format, const ge::Format &format,
                            const string &reshape_type, const int64_t &expect_reshape_type,
                            const vector<int64_t> &dims, const vector<int64_t> &expect_dims) {
    std::cout << "origin_format=" << origin_format << ", format=" << format
              << ", reahpe type=" << reshape_type << ", dim size=" << dims.size() << std::endl;
    ge::GeShape new_shape(dims);
    int64_t int_reshape_type = ExpandDimension::GenerateReshapeType(origin_format, format, new_shape.GetDimNum(),
                                                                    reshape_type);
    if (int_reshape_type != 0) {
      size_t full_size = static_cast<size_t>(int_reshape_type >> 56);
      size_t expect_full_size = 0;
      ExpandDimension::GetFormatFullSize(origin_format, expect_full_size);
      EXPECT_EQ(full_size, expect_full_size);
    }
    EXPECT_EQ(int_reshape_type & 0xff, expect_reshape_type);
    ExpandDimension::ExpandDims(int_reshape_type, new_shape);
    EXPECT_EQ(new_shape.GetDims(), expect_dims);
  }
};

TEST_F(TransformerExpandDimsUT, all_expand_dims_cases_1) {
  int64_t max_reshape_type = 0xff;
  vector<size_t> full_size_vec = {4, 5};
  vector<vector<int64_t>> dim_vecs = {{}, {5}, {5, 6}, {5, 6, 7}, {5, 6, 7, 8}, {5, 6, 7, 8, 9}};
  for (const size_t &full_size : full_size_vec) {
    for (const vector<int64_t> &dims : dim_vecs) {
      for (int64_t i = 0; i <= max_reshape_type; i++) {
        ge::GeShape shape(dims);
        int64_t reshape_type = i | (full_size << 56);
        std::cout << "reshape_type = " << std::bitset<8>(reshape_type) << ", shape = " << shape.ToString();
        ExpandDimension::ExpandDims(reshape_type, shape);
        std::cout << ", after expand dims shape = " << shape.ToString() << std::endl;
      }
    }
  }
}

TEST_F(TransformerExpandDimsUT, all_expand_dims_cases_2) {
  int64_t max_reshape_type = 0xff;
  vector<size_t> full_size_vec = {4, 5};
  vector<vector<int64_t>> dim_vecs = {{}, {5}, {5, 6}, {5, 6, 7}, {5, 6, 7, 8}, {5, 6, 7, 8, 9}};
  for (const size_t &full_size : full_size_vec) {
    for (const vector<int64_t> &dims : dim_vecs) {
      for (int64_t i = 0; i <= max_reshape_type; i++) {
        ge::GeShape shape_1(dims);
        ge::GeShape shape_2(dims);
        int64_t reshape_type = i | (full_size << 56);
        std::cout << "reshape_type = " << std::bitset<8>(reshape_type) << ", shape = " << shape_1.ToString();
        ExpandDimension::ExpandDims(reshape_type, shape_1, shape_2);
        std::cout << ", after expand dims shape = " << shape_2.ToString() << std::endl;
      }
    }
  }
}

TEST_F(TransformerExpandDimsUT, not_expand_cases) {
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "FORBIDDEN", 0, {8, 9}, {8, 9});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_FRACTAL_NZ, "HW", 0, {8, 9}, {8, 9});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "", 0, {6, 7, 8, 9}, {6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "", 0, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NC1HWC0, "", 0, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NC1HWC0, "", 0, {4, 5, 6, 7, 8, 9}, {4, 5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_ND, ge::FORMAT_FRACTAL_Z, "CN", 0, {8, 9}, {8, 9});

  RunNewExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_ND, "HW", 0, {8, 9}, {8, 9});
  RunNewExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_ND_RNN_BIAS, "HW", 0, {8, 9}, {8, 9});
  RunNewExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_FRACTAL_ZN_RNN, "HW", 0, {8, 9}, {8, 9});
}

TEST_F(TransformerExpandDimsUT, default_reshape_type_cases) {
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "WN", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "CN", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "NH", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "NC", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NC1HWC0, "CN", 0x1f, {}, {1, 1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NC1HWC0, "WN", 0x1f, {}, {1, 1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NC1HWC0, "ND", 0x1f, {}, {1, 1, 1, 1, 1});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "", 0x0e, {5}, {1, 1, 1, 5});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "", 0x0d, {5}, {1, 1, 5, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NC1HWC0, "", 0x1e, {5}, {1, 1, 1, 1, 5});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NC1HWC0, "", 0x17, {5}, {1, 5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NC1HWC0, "", 0x1d, {5}, {1, 1, 1, 5, 1});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "WN", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "CWN", 0x0e, {5}, {1, 1, 1, 5});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "NH", 0x0d, {5}, {1, 1, 5, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "NCHW", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NC1HWC0, "CN", 0x1e, {5}, {1, 1, 1, 1, 5});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NC1HWC0, "WNCD", 0x17, {5}, {1, 5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NC1HWC0, "ND", 0x1d, {5}, {1, 1, 1, 5, 1});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "WN", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "CN", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "NH", 0x0c, {5, 6}, {1, 1, 5, 6});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "NC", 0x0c, {5, 6}, {1, 1, 5, 6});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NC1HWC0, "CN", 0x1c, {5, 6}, {1, 1, 1, 5, 6});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NC1HWC0, "WN", 0x1c, {5, 6}, {1, 1, 1, 5, 6});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NC1HWC0, "ND", 0x1c, {5, 6}, {1, 1, 1, 5, 6});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "WHN", 0x08, {5, 6, 7}, {1, 5, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "CWN", 0x08, {5, 6, 7}, {1, 5, 6, 7});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "NHW", 0x08, {5, 6, 7}, {1, 5, 6, 7});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CNW", 0x08, {5, 6, 7}, {1, 5, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NC1HWC0, "CND", 0x18, {5, 6, 7}, {1, 1, 5, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NC1HWC0, "WDN", 0x18, {5, 6, 7}, {1, 1, 5, 6, 7});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NC1HWC0, "WCND", 0x18, {5, 6, 7}, {1, 1, 5, 6, 7});

  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NC1HWC0, "CNWD", 0x10, {5, 6, 7, 8}, {1, 5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NC1HWC0, "NDHWC", 0x10, {5, 6, 7, 8}, {1, 5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NC1HWC0, "NCHW", 0x10, {5, 6, 7, 8}, {1, 5, 6, 7, 8});
}

TEST_F(TransformerExpandDimsUT, nchw_reshape_type) {
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "N", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "HW", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "HCW", 0x0f, {}, {1, 1, 1, 1});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "N", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "C", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "H", 0x0d, {5}, {1, 1, 5, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "W", 0x0e, {5}, {1, 1, 1, 5});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCHW", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "CHW", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "HW", 0x0d, {5}, {1, 1, 5, 1});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "N", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "C", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "H", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "W", 0x00, {5, 6}, {5, 6});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NC", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCH", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCHW", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCW", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NH", 0x05, {5, 6}, {5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NHW", 0x05, {5, 6}, {5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NW", 0x06, {5, 6}, {5, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "CH", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "CHW", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "CW", 0x0a, {5, 6}, {1, 5, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "HW", 0x0c, {5, 6}, {1, 1, 5, 6});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCH", 0x01, {5, 6, 7}, {5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCHW", 0x01, {5, 6, 7}, {5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCW", 0x02, {5, 6, 7}, {5, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NHW", 0x04, {5, 6, 7}, {5, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "CHW", 0x08, {5, 6, 7}, {1, 5, 6, 7});

  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCHW", 0x00, {5, 6, 7, 8}, {5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NC", 0x00, {5, 6, 7, 8}, {5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "NCHW", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, "HW", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
}

TEST_F(TransformerExpandDimsUT, nhwc_reshape_type) {
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "N", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NH", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "HWC", 0x0f, {}, {1, 1, 1, 1});

  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "N", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "H", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "W", 0x0d, {5}, {1, 1, 5, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "C", 0x0e, {5}, {1, 1, 1, 5});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHWC", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "HWC", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "WC", 0x0d, {5}, {1, 1, 5, 1});

  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "N", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "H", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "W", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "C", 0x00, {5, 6}, {5, 6});

  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NH", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHW", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHWC", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHC", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NW", 0x05, {5, 6}, {5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NWC", 0x05, {5, 6}, {5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NC", 0x06, {5, 6}, {5, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "HW", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "HWC", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "HC", 0x0a, {5, 6}, {1, 5, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "WC", 0x0c, {5, 6}, {1, 1, 5, 6});

  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHW", 0x01, {5, 6, 7}, {5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHWC", 0x01, {5, 6, 7}, {5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHC", 0x02, {5, 6, 7}, {5, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NWC", 0x04, {5, 6, 7}, {5, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "HWC", 0x08, {5, 6, 7}, {1, 5, 6, 7});

  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHWC", 0x00, {5, 6, 7, 8}, {5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NC", 0x00, {5, 6, 7, 8}, {5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "NHWC", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, "HW", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
}

TEST_F(TransformerExpandDimsUT, hwcn_reshape_type) {
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "N", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HW", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWN", 0x0f, {}, {1, 1, 1, 1});

  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "H", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "W", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "C", 0x0d, {5}, {1, 1, 5, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "N", 0x0e, {5}, {1, 1, 1, 5});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWCN", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "WCN", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "CN", 0x0d, {5}, {1, 1, 5, 1});

  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "H", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "W", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "C", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "N", 0x00, {5, 6}, {5, 6});

  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HW", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWC", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWCN", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWN", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HC", 0x05, {5, 6}, {5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HCN", 0x05, {5, 6}, {5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HN", 0x06, {5, 6}, {5, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "WC", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "WCN", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "WN", 0x0a, {5, 6}, {1, 5, 1, 6});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "CN", 0x0c, {5, 6}, {1, 1, 5, 6});

  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWC", 0x01, {5, 6, 7}, {5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWCN", 0x01, {5, 6, 7}, {5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWN", 0x02, {5, 6, 7}, {5, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HCN", 0x04, {5, 6, 7}, {5, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "WCN", 0x08, {5, 6, 7}, {1, 5, 6, 7});

  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWCN", 0x00, {5, 6, 7, 8}, {5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HW", 0x00, {5, 6, 7, 8}, {5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "HWCN", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, "CN", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
}

TEST_F(TransformerExpandDimsUT, chwn_reshape_type) {
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "C", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "HW", 0x0f, {}, {1, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "HWN", 0x0f, {}, {1, 1, 1, 1});

  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "C", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "H", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "W", 0x0d, {5}, {1, 1, 5, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "N", 0x0e, {5}, {1, 1, 1, 5});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHWN", 0x07, {5}, {5, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "HWN", 0x0b, {5}, {1, 5, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "WN", 0x0d, {5}, {1, 1, 5, 1});

  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "C", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "H", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "W", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "N", 0x00, {5, 6}, {5, 6});

  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CH", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHW", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHWN", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHN", 0x03, {5, 6}, {5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CW", 0x05, {5, 6}, {5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CWN", 0x05, {5, 6}, {5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CN", 0x06, {5, 6}, {5, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "HW", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "HWN", 0x09, {5, 6}, {1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "HN", 0x0a, {5, 6}, {1, 5, 1, 6});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "WN", 0x0c, {5, 6}, {1, 1, 5, 6});

  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHW", 0x01, {5, 6, 7}, {5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHWN", 0x01, {5, 6, 7}, {5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHN", 0x02, {5, 6, 7}, {5, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CWN", 0x04, {5, 6, 7}, {5, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "HWN", 0x08, {5, 6, 7}, {1, 5, 6, 7});

  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHWN", 0x00, {5, 6, 7, 8}, {5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "HW", 0x00, {5, 6, 7, 8}, {5, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CHWN", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, "CN", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
}

TEST_F(TransformerExpandDimsUT, ndhwc_reshape_type) {
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "C", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "HW", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NHW", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDWC", 0x1f, {}, {1, 1, 1, 1 ,1});

  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "N", 0x0f, {5}, {5, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "D", 0x17, {5}, {1, 5, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "H", 0x1b, {5}, {1, 1, 5, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "W", 0x1d, {5}, {1, 1, 1, 5 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "C", 0x1e, {5}, {1, 1, 1, 1 ,5});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHWC", 0x0f, {5}, {5, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "DHWC", 0x17, {5}, {1, 5, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "HWC", 0x1b, {5}, {1, 1, 5, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "WC", 0x1d, {5}, {1, 1, 1, 5 ,1});

  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "N", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "D", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "H", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "W", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "C", 0x00, {5, 6}, {5, 6});

  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "ND", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDH", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDW", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDC", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHW", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHC", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHWC", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NH", 0x0b, {5, 6}, {5, 1, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NW", 0x0d, {5, 6}, {5, 1, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NC", 0x0e, {5, 6}, {5, 1, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "DH", 0x13, {5, 6}, {1, 5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "DW", 0x15, {5, 6}, {1, 5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "DC", 0x16, {5, 6}, {1, 5, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "HW", 0x19, {5, 6}, {1, 1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "HC", 0x1a, {5, 6}, {1, 1, 5, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "WC", 0x1c, {5, 6}, {1, 1, 1, 5, 6});

  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDH", 0x03, {5, 6, 7}, {5, 6, 7, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDW", 0x05, {5, 6, 7}, {5, 6, 1, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDC", 0x06, {5, 6, 7}, {5, 6, 1, 1, 7});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NHW", 0x09, {5, 6, 7}, {5, 1, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NHC", 0x0a, {5, 6, 7}, {5, 1, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NWC", 0x0c, {5, 6, 7}, {5, 1, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "DHW", 0x11, {5, 6, 7}, {1, 5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "DHC", 0x12, {5, 6, 7}, {1, 5, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "DWC", 0x14, {5, 6, 7}, {1, 5, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "HWC", 0x18, {5, 6, 7}, {1, 1, 5, 6, 7});

  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHW", 0x01, {5, 6, 7, 8}, {5, 6, 7, 8, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHWC", 0x01, {5, 6, 7, 8}, {5, 6, 7, 8, 1});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHC", 0x02, {5, 6, 7, 8}, {5, 6, 7, 1, 8});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDWC", 0x04, {5, 6, 7, 8}, {5, 6, 1, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NHWC", 0x08, {5, 6, 7, 8}, {5, 1, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "DHWC", 0x10, {5, 6, 7, 8}, {1, 5, 6, 7, 8});

  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NHWC", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHWC", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDWC", 0x00, {5, 6, 7, 8, 9, 7}, {5, 6, 7, 8, 9, 7});
  RunExpandDimsCase(ge::FORMAT_NDHWC, ge::FORMAT_NDC1HWC0, "NDHWC", 0x00, {5, 6, 7, 8, 9, 7}, {5, 6, 7, 8, 9, 7});
}

TEST_F(TransformerExpandDimsUT, ncdhw_reshape_type) {
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "C", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "HW", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NHW", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NDHW", 0x1f, {}, {1, 1, 1, 1 ,1});

  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "N", 0x0f, {5}, {5, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "C", 0x17, {5}, {1, 5, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "D", 0x1b, {5}, {1, 1, 5, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "H", 0x1d, {5}, {1, 1, 1, 5 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "W", 0x1e, {5}, {1, 1, 1, 1 ,5});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDHW", 0x0f, {5}, {5, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CDHW", 0x17, {5}, {1, 5, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "DHW", 0x1b, {5}, {1, 1, 5, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "HW", 0x1d, {5}, {1, 1, 1, 5 ,1});

  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "N", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "C", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "D", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "H", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "W", 0x00, {5, 6}, {5, 6});

  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NC", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCD", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCH", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCW", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDH", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDW", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCHW", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDHW", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "ND", 0x0b, {5, 6}, {5, 1, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NH", 0x0d, {5, 6}, {5, 1, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NW", 0x0e, {5, 6}, {5, 1, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CD", 0x13, {5, 6}, {1, 5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CH", 0x15, {5, 6}, {1, 5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CW", 0x16, {5, 6}, {1, 5, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "DH", 0x19, {5, 6}, {1, 1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "DW", 0x1a, {5, 6}, {1, 1, 5, 1, 6});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "HW", 0x1c, {5, 6}, {1, 1, 1, 5, 6});

  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCD", 0x03, {5, 6, 7}, {5, 6, 7, 1, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCH", 0x05, {5, 6, 7}, {5, 6, 1, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCW", 0x06, {5, 6, 7}, {5, 6, 1, 1, 7});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NDH", 0x09, {5, 6, 7}, {5, 1, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NDW", 0x0a, {5, 6, 7}, {5, 1, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NHW", 0x0c, {5, 6, 7}, {5, 1, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CDH", 0x11, {5, 6, 7}, {1, 5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CDW", 0x12, {5, 6, 7}, {1, 5, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CHW", 0x14, {5, 6, 7}, {1, 5, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "DHW", 0x18, {5, 6, 7}, {1, 1, 5, 6, 7});

  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDH", 0x01, {5, 6, 7, 8}, {5, 6, 7, 8, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDHW", 0x01, {5, 6, 7, 8}, {5, 6, 7, 8, 1});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDW", 0x02, {5, 6, 7, 8}, {5, 6, 7, 1, 8});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCHW", 0x04, {5, 6, 7, 8}, {5, 6, 1, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NDHW", 0x08, {5, 6, 7, 8}, {5, 1, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CDHW", 0x10, {5, 6, 7, 8}, {1, 5, 6, 7, 8});

  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCHW", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDHW", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "CDHW", 0x00, {5, 6, 7, 8, 9, 7}, {5, 6, 7, 8, 9, 7});
  RunExpandDimsCase(ge::FORMAT_NCDHW, ge::FORMAT_NDC1HWC0, "NCDHW", 0x00, {5, 6, 7, 8, 9, 7}, {5, 6, 7, 8, 9, 7});
}

TEST_F(TransformerExpandDimsUT, dhwcn_reshape_type) {
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "C", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HW", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HWC", 0x1f, {}, {1, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWN", 0x1f, {}, {1, 1, 1, 1 ,1});

  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "D", 0x0f, {5}, {5, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "H", 0x17, {5}, {1, 5, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "W", 0x1b, {5}, {1, 1, 5, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "C", 0x1d, {5}, {1, 1, 1, 5 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "N", 0x1e, {5}, {1, 1, 1, 1 ,5});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWCN", 0x0f, {5}, {5, 1, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HWCN", 0x17, {5}, {1, 5, 1, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "WCN", 0x1b, {5}, {1, 1, 5, 1 ,1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "CN", 0x1d, {5}, {1, 1, 1, 5 ,1});

  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "D", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "H", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "W", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "C", 0x00, {5, 6}, {5, 6});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "N", 0x00, {5, 6}, {5, 6});

  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DH", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHW", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHC", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHN", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWC", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWN", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHCN", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWCN", 0x07, {5, 6}, {5, 6, 1, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DW", 0x0b, {5, 6}, {5, 1, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DC", 0x0d, {5, 6}, {5, 1, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DN", 0x0e, {5, 6}, {5, 1, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HW", 0x13, {5, 6}, {1, 5, 6, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HC", 0x15, {5, 6}, {1, 5, 1, 6, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HN", 0x16, {5, 6}, {1, 5, 1, 1, 6});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "WC", 0x19, {5, 6}, {1, 1, 5, 6, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "WN", 0x1a, {5, 6}, {1, 1, 5, 1, 6});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "CN", 0x1c, {5, 6}, {1, 1, 1, 5, 6});

  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHW", 0x03, {5, 6, 7}, {5, 6, 7, 1, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHC", 0x05, {5, 6, 7}, {5, 6, 1, 7, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHN", 0x06, {5, 6, 7}, {5, 6, 1, 1, 7});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DWC", 0x09, {5, 6, 7}, {5, 1, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DWN", 0x0a, {5, 6, 7}, {5, 1, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DCN", 0x0c, {5, 6, 7}, {5, 1, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HWC", 0x11, {5, 6, 7}, {1, 5, 6, 7, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HWN", 0x12, {5, 6, 7}, {1, 5, 6, 1, 7});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HCN", 0x14, {5, 6, 7}, {1, 5, 1, 6, 7});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "WCN", 0x18, {5, 6, 7}, {1, 1, 5, 6, 7});

  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWC", 0x01, {5, 6, 7, 8}, {5, 6, 7, 8, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWCN", 0x01, {5, 6, 7, 8}, {5, 6, 7, 8, 1});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWN", 0x02, {5, 6, 7, 8}, {5, 6, 7, 1, 8});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHCN", 0x04, {5, 6, 7, 8}, {5, 6, 1, 7, 8});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DWCN", 0x08, {5, 6, 7, 8}, {5, 1, 6, 7, 8});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HWCN", 0x10, {5, 6, 7, 8}, {1, 5, 6, 7, 8});

  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWC", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWCN", 0x00, {5, 6, 7, 8, 9}, {5, 6, 7, 8, 9});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "HWCN", 0x00, {5, 6, 7, 8, 9, 7}, {5, 6, 7, 8, 9, 7});
  RunExpandDimsCase(ge::FORMAT_DHWCN, ge::FORMAT_NDC1HWC0, "DHWCN", 0x00, {5, 6, 7, 8, 9, 7}, {5, 6, 7, 8, 9, 7});
}

}  // namespace ge