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
#include <vector>

#include <gtest/gtest.h>
#define private public
#include "ge_tensor.h"
#include "ge_ir.pb.h"

namespace ge {
class TensorUT : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(TensorUT, CreateShareTensorDataFromGeTensor) {

}

TEST_F(TensorUT, CreateShareTensorDataFromGeTensorOperatorEqual) {
  GeTensor t1;

  std::vector<uint8_t> vec;
  for (uint8_t i = 0; i < 100; ++i) {
    vec.push_back(i * 2);
  }
  t1.SetData(vec);
  GeTensor t2 = t1;

  t1.MutableTensorDesc().SetFormat(FORMAT_NCHW);
  t2.MutableTensorDesc().SetFormat(FORMAT_NHWC);
  ASSERT_EQ(t1.GetTensorDesc().GetFormat(), FORMAT_NCHW);
  ASSERT_EQ(t2.GetTensorDesc().GetFormat(), FORMAT_NHWC);

  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);
  ASSERT_EQ(t1.GetData().GetData(), t2.GetData().GetData());
}

TEST_F(TensorUT, Construct1) {
  GeTensor t1;
  ASSERT_EQ(t1.__desc_.tensor_descriptor_.GetProtoMsg(), t1.GetData().tensor_descriptor_.GetProtoMsg());

  GeTensorDesc td;
  ASSERT_NE(td.tensor_descriptor_.GetProtoOwner(), nullptr);
  ASSERT_NE(td.tensor_descriptor_.GetProtoMsg(), nullptr);

  GeIrProtoHelper<ge::proto::TensorDef> helper;
  helper.InitDefault();
  helper.GetProtoMsg()->mutable_data()->resize(200);
  GeTensor t2(helper.GetProtoOwner(), helper.GetProtoMsg());
  ASSERT_NE(t2.tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_NE(t2.tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_EQ(t2.__desc_.tensor_descriptor_.GetProtoOwner(), t2.tensor_def_.GetProtoOwner());
  ASSERT_EQ(t2.tensor_data_.tensor_descriptor_.GetProtoOwner(), t2.tensor_def_.GetProtoOwner());
  ASSERT_EQ(t2.tensor_data_.tensor_descriptor_.GetProtoMsg(), t2.__desc_.tensor_descriptor_.GetProtoMsg());
  ASSERT_EQ(reinterpret_cast<const char *>(t2.tensor_data_.GetData()), t2.tensor_def_.GetProtoMsg()->data().data());
}
TEST_F(TensorUT, Construct2) {
  GeTensorDesc desc;
  GeTensor t1(desc);
  ASSERT_NE(t1.__desc_.tensor_descriptor_.GetProtoMsg(), desc.tensor_descriptor_.GetProtoMsg());
}
TEST_F(TensorUT, CopyConstruct1_NullTensorDef) {
  GeTensor t1;

  std::vector<uint8_t> vec;
  for (uint8_t i = 0; i < 100; ++i) {
    vec.push_back(i * 2);
  }
  t1.SetData(vec);
  GeTensor t2(t1);

  // The copy construct share tensor_data_, do not share tensor_desc
  ASSERT_EQ(t1.tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_EQ(t1.tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_NE(t1.__desc_.tensor_descriptor_.GetProtoMsg(), t2.__desc_.tensor_descriptor_.GetProtoMsg());
  ASSERT_NE(t1.__desc_.tensor_descriptor_.GetProtoOwner(), t2.__desc_.tensor_descriptor_.GetProtoOwner());
  ASSERT_EQ(t1.tensor_data_.tensor_descriptor_.GetProtoMsg(), t1.__desc_.tensor_descriptor_.GetProtoMsg());
  ASSERT_EQ(t1.tensor_data_.tensor_descriptor_.GetProtoOwner(), t1.__desc_.tensor_descriptor_.GetProtoOwner());
  ASSERT_EQ(t2.tensor_data_.tensor_descriptor_.GetProtoMsg(), t2.__desc_.tensor_descriptor_.GetProtoMsg());
  ASSERT_EQ(t2.tensor_data_.tensor_descriptor_.GetProtoOwner(), t2.__desc_.tensor_descriptor_.GetProtoOwner());
  ASSERT_EQ(t1.tensor_data_.GetData(), t2.tensor_data_.GetData());

  t1.MutableTensorDesc().SetFormat(FORMAT_NCHW);
  t2.MutableTensorDesc().SetFormat(FORMAT_NHWC);
  ASSERT_EQ(t1.GetTensorDesc().GetFormat(), FORMAT_NCHW);
  ASSERT_EQ(t2.GetTensorDesc().GetFormat(), FORMAT_NHWC);

  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);
  ASSERT_EQ(t1.GetData().GetData(), t2.GetData().GetData());
}

TEST_F(TensorUT, CopyConstruct2_WithTensorDef) {
  GeIrProtoHelper<ge::proto::TensorDef> helper;
  helper.InitDefault();
  helper.GetProtoMsg()->mutable_data()->resize(100);
  GeTensor t1(helper.GetProtoOwner(), helper.GetProtoMsg());

  std::vector<uint8_t> vec;
  for (uint8_t i = 0; i < 100; ++i) {
    vec.push_back(i * 2);
  }
  t1.SetData(vec);
  GeTensor t2(t1);

  // The copy construct share tensor_data_ and tensor_desc
  ASSERT_NE(t1.tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_NE(t1.tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_EQ(t1.__desc_.tensor_descriptor_.GetProtoMsg(), t2.__desc_.tensor_descriptor_.GetProtoMsg());
  ASSERT_EQ(t1.__desc_.tensor_descriptor_.GetProtoOwner(), t2.__desc_.tensor_descriptor_.GetProtoOwner());
  ASSERT_EQ(t1.tensor_data_.tensor_descriptor_.GetProtoMsg(), t1.__desc_.tensor_descriptor_.GetProtoMsg());
  ASSERT_EQ(t1.tensor_data_.tensor_descriptor_.GetProtoOwner(), t1.__desc_.tensor_descriptor_.GetProtoOwner());
  ASSERT_EQ(t2.tensor_data_.tensor_descriptor_.GetProtoMsg(), t2.__desc_.tensor_descriptor_.GetProtoMsg());
  ASSERT_EQ(t2.tensor_data_.tensor_descriptor_.GetProtoOwner(), t2.__desc_.tensor_descriptor_.GetProtoOwner());
  ASSERT_EQ(t1.tensor_data_.GetData(), t2.tensor_data_.GetData());

  t1.MutableTensorDesc().SetFormat(FORMAT_NCHW);
  ASSERT_EQ(t1.GetTensorDesc().GetFormat(), FORMAT_NCHW);
  ASSERT_EQ(t2.GetTensorDesc().GetFormat(), FORMAT_NCHW);
  t2.MutableTensorDesc().SetFormat(FORMAT_NHWC);
  ASSERT_EQ(t1.GetTensorDesc().GetFormat(), FORMAT_NHWC);
  ASSERT_EQ(t2.GetTensorDesc().GetFormat(), FORMAT_NHWC);

  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);
  ASSERT_EQ(t1.GetData().GetData(), t2.GetData().GetData());
}
}  // namespace ge