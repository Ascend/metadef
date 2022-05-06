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
#include "exe_graph/runtime/tensor.h"
#include <gtest/gtest.h>
namespace gert {
class TensorUT : public testing::Test {};
TEST_F(TensorUT, ConstructOk) {
  Tensor tensor{{{8, 3, 224, 224}, {16, 3, 224, 224}},       // shape
                {ge::FORMAT_ND, ge::FORMAT_FRACTAL_NZ, {}},  // format
                kOnDeviceHbm,                                // placement
                ge::DT_FLOAT16,                              //dt
                nullptr};
  const Tensor &t = tensor;

  EXPECT_EQ(t.GetOriginShape(), Shape({8, 3, 224, 224}));
  EXPECT_EQ(t.GetStorageShape(), Shape({16, 3, 224, 224}));

  EXPECT_EQ(t.GetOriginFormat(), ge::FORMAT_ND);
  EXPECT_EQ(t.GetStorageFormat(), ge::FORMAT_FRACTAL_NZ);
  EXPECT_EQ(t.GetExpandDimsType(), ExpandDimsType{});

  EXPECT_EQ(t.GetPlacement(), kOnDeviceHbm);
  EXPECT_EQ(t.GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(t.GetAddr(), nullptr);
  EXPECT_EQ(t.GetData<int64_t>(), nullptr);
}

TEST_F(TensorUT, GetDataAddrFollowingOk) {
  Tensor tensor{{{8, 3, 224, 224}, {16, 3, 224, 224}},       // shape
                {ge::FORMAT_ND, ge::FORMAT_FRACTAL_NZ, {}},  // format
                kFollowing,                                // placement
                ge::DT_FLOAT16,                              //dt
                nullptr};
  const Tensor &t = tensor;

  EXPECT_EQ(t.GetOriginShape(), Shape({8, 3, 224, 224}));
  EXPECT_EQ(t.GetStorageShape(), Shape({16, 3, 224, 224}));

  EXPECT_EQ(t.GetOriginFormat(), ge::FORMAT_ND);
  EXPECT_EQ(t.GetStorageFormat(), ge::FORMAT_FRACTAL_NZ);
  EXPECT_EQ(t.GetExpandDimsType(), ExpandDimsType{});

  EXPECT_EQ(t.GetPlacement(), kFollowing);
  EXPECT_EQ(t.GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(t.GetAddr(), &tensor + 1);
  EXPECT_EQ(t.GetData<int64_t>(), reinterpret_cast<const int64_t *>(&tensor + 1));
}

TEST_F(TensorUT, GetCopiedDataAddrFollowingOk) {
  Tensor tensor{{{8, 3, 224, 224}, {16, 3, 224, 224}},       // shape
                {ge::FORMAT_ND, ge::FORMAT_FRACTAL_NZ, {}},  // format
                kFollowing,                                // placement
                ge::DT_FLOAT16,                              //dt
                nullptr};
  Tensor t;
  memcpy(&t, &tensor, sizeof(tensor));

  EXPECT_EQ(t.GetOriginShape(), Shape({8, 3, 224, 224}));
  EXPECT_EQ(t.GetStorageShape(), Shape({16, 3, 224, 224}));

  EXPECT_EQ(t.GetOriginFormat(), ge::FORMAT_ND);
  EXPECT_EQ(t.GetStorageFormat(), ge::FORMAT_FRACTAL_NZ);
  EXPECT_EQ(t.GetExpandDimsType(), ExpandDimsType{});

  EXPECT_EQ(t.GetPlacement(), kFollowing);
  EXPECT_EQ(t.GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(t.GetAddr(), &t + 1);
  EXPECT_EQ(t.GetData<int64_t>(), reinterpret_cast<int64_t *>(&t + 1));
}

TEST_F(TensorUT, SetGetShapeOk) {
  Tensor t;
  const Tensor &ct = t;
  t.MutableOriginShape() = Shape{8,3,224,224};
  t.MutableStorageShape() = Shape{8,1,224,224,16};
  EXPECT_EQ(t.GetOriginShape(), Shape({8,3,224,224}));
  EXPECT_EQ(t.GetStorageShape(), Shape({8,1,224,224,16}));
  EXPECT_EQ(ct.GetOriginShape(), Shape({8,3,224,224}));
  EXPECT_EQ(ct.GetStorageShape(), Shape({8,1,224,224,16}));
}

TEST_F(TensorUT, SetGetFormatOk) {
  Tensor tensor;
  const Tensor &t = tensor;
  tensor.SetOriginFormat(ge::FORMAT_NHWC);
  tensor.SetStorageFormat(ge::FORMAT_NC1HWC0);

  EXPECT_EQ(t.GetOriginFormat(), ge::FORMAT_NHWC);
  EXPECT_EQ(t.GetStorageFormat(), ge::FORMAT_NC1HWC0);

  EXPECT_EQ(t.GetFormat().GetOriginFormat(), ge::FORMAT_NHWC);
  EXPECT_EQ(t.GetFormat().GetStorageFormat(), ge::FORMAT_NC1HWC0);
}

TEST_F(TensorUT, SetGetPlacementOk) {
  Tensor t;
  const Tensor &ct = t;
  t.SetPlacement(kOnHost);
  EXPECT_EQ(t.GetPlacement(), kOnHost);
  EXPECT_EQ(ct.GetPlacement(), kOnHost);
}

TEST_F(TensorUT, SetGetDataTypeOk) {
  Tensor t;
  const Tensor &ct = t;
  t.SetDataType(ge::DT_FLOAT16);
  EXPECT_EQ(t.GetDataType(), ge::DT_FLOAT16);
  EXPECT_EQ(ct.GetDataType(), ge::DT_FLOAT16);
}

TEST_F(TensorUT, SetGetAddrOk) {
  Tensor t;
  const Tensor &ct = t;
  void *a = &t;

  TensorData td(a, nullptr);
  t.SetData(td);
  EXPECT_EQ(t.GetAddr(), a);
  EXPECT_EQ(ct.GetAddr(), a);

  EXPECT_EQ(t.GetData<Tensor>(), &t);
  EXPECT_EQ(ct.GetData<Tensor>(), &t);
}

ge::graphStatus StubAddManger(TensorAddress addr, TensorOperateType operate_type, void **out){
  if(operate_type == kGetTensorAddress){
    *out = reinterpret_cast<void*>(8);
  }
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus StubAddMangerFailed(TensorAddress addr, TensorOperateType operate_type, void **out){
  return ge::GRAPH_FAILED;
}

TEST_F(TensorUT, TensorDataWithMangerSuccess) {
  auto addr = reinterpret_cast<void*>(0x16);
  TensorData data(addr, StubAddManger);
  EXPECT_EQ(reinterpret_cast<uint64_t>(data.GetAddr()), 8);
  EXPECT_EQ(data.Free(), ge::GRAPH_SUCCESS);
  EXPECT_EQ(data.GetAddr(), nullptr);
  data.SetAddr(addr, nullptr);
  EXPECT_EQ(reinterpret_cast<uint64_t>(data.GetAddr()), 0x16);
  data.SetAddr(addr, StubAddMangerFailed);
  EXPECT_EQ(data.GetAddr(), nullptr);
}




}  // namespace gert