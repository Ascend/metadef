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
#include "external/graph/types.h"

namespace ge {
const char *GetFormatName(Format format) {
  static const char *names[FORMAT_END] = {
      "NCHW",
      "NHWC",
      "ND",
      "NC1HWC0",
      "FRACTAL_Z",
      "NC1C0HWPAD", // 5
      "NHWC1C0",
      "FSR_NCHW",
      "FRACTAL_DECONV",
      "C1HWNC0",
      "FRACTAL_DECONV_TRANSPOSE",  // 10
      "FRACTAL_DECONV_SP_STRIDE_TRANS",
      "NC1HWC0_C04",
      "FRACTAL_Z_C04",
      "CHWN",
      "DECONV_SP_STRIDE8_TRANS", // 15
      "HWCN",
      "NC1KHKWHWC0",
      "BN_WEIGHT",
      "FILTER_HWCK",
      "LOOKUP_LOOKUPS", // 20
      "LOOKUP_KEYS",
      "LOOKUP_VALUE",
      "LOOKUP_OUTPUT",
      "LOOKUP_HITS",
      "C1HWNCoC0", // 25
      "MD",
      "NDHWC",
      "UNKNOWN", // FORMAT_FRACTAL_ZZ
      "FRACTAL_NZ",
      "NCDHW", // 30
      "DHWCN",
      "NDC1HWC0",
      "FRACTAL_Z_3D",
      "CN",
      "NC", // 35
      "DHWNC",
      "FRACTAL_Z_3D_TRANSPOSE",
      "FRACTAL_ZN_LSTM",
      "FRACTAL_Z_G",
      "UNKNOWN", // 40, FORMAT_RESERVED
      "UNKNOWN", // FORMAT_ALL
      "UNKNOWN", // FORMAT_NULL
  };
  if (format >= FORMAT_END) {
    return "UNKNOWN";
  }
  return names[format];
}
}