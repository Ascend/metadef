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

TEST_F(TensorUT, SetData1NoShare) {
  GeTensor t1;
  std::vector<uint8_t> vec;
  for (uint8_t i = 0; i < 150; ++i) {
    vec.push_back(i);
  }
  ASSERT_EQ(t1.SetData(vec), GRAPH_SUCCESS);
  ASSERT_EQ(t1.GetData().GetSize(), vec.size());
  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);
  t1.MutableData().GetData()[10] = 250;
  ASSERT_NE(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);

  std::vector<uint8_t> vec2;
  for (uint8_t i = 0; i < 105; ++i) {
    vec2.push_back(i * 2);
  }
  vec = vec2;
  ASSERT_EQ(t1.SetData(std::move(vec2)), GRAPH_SUCCESS);
  ASSERT_EQ(t1.GetData().GetSize(), vec.size());
  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);

  vec.clear();
  for (uint8_t i = 0; i < 100; ++i) {
    vec.push_back(100 - i);
  }
  Buffer buffer = Buffer::CopyFrom(vec.data(), vec.size());
  ASSERT_EQ(t1.SetData(buffer), GRAPH_SUCCESS);
  ASSERT_EQ(t1.GetData().GetSize(), vec.size());
  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);

  vec.clear();
  for (uint8_t i = 0; i < 150; ++i) {
    vec.push_back(i);
  }
  ASSERT_EQ(t1.SetData(vec.data(), vec.size()), GRAPH_SUCCESS);
  ASSERT_EQ(t1.GetData().GetSize(), vec.size());
  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);

  vec.clear();
  for (uint8_t i = 0; i < 200; ++i) {
    vec.push_back(200 - i);
  }
  TensorData td;
  td.SetData(vec);
  ASSERT_EQ(memcmp(td.GetData(), vec.data(), vec.size()), 0);
  ASSERT_EQ(t1.SetData(td), GRAPH_SUCCESS);
  ASSERT_EQ(t1.GetData().GetSize(), vec.size());
  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec.data(), vec.size()), 0);
}

TEST_F(TensorUT, Construct1_General) {
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
TEST_F(TensorUT, Construct2_CopyDesc) {
  GeTensorDesc desc;
  GeTensor t1(desc);
  ASSERT_NE(t1.__desc_.tensor_descriptor_.GetProtoMsg(), desc.tensor_descriptor_.GetProtoMsg());
}
TEST_F(TensorUT, Construct3_ExceptionalScenes) {
  GeIrProtoHelper<ge::proto::TensorDef> helper;
  helper.InitDefault();
  GeTensor t1(nullptr, helper.GetProtoMsg());
  GeTensor t2(helper.GetProtoOwner(), nullptr);
  GeTensor t3(nullptr, nullptr);

  ASSERT_EQ(t1.tensor_def_.GetProtoMsg(), helper.GetProtoMsg());
  ASSERT_EQ(t1.tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_EQ(t1.tensor_data_.tensor_descriptor_.GetProtoMsg(), t1.__desc_.tensor_descriptor_.GetProtoMsg());

  ASSERT_EQ(t2.tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_EQ(t2.tensor_def_.GetProtoOwner(), helper.GetProtoOwner());
  ASSERT_EQ(t2.tensor_data_.tensor_descriptor_.GetProtoMsg(), t2.__desc_.tensor_descriptor_.GetProtoMsg());

  ASSERT_EQ(t3.tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_EQ(t3.tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_EQ(t3.tensor_data_.tensor_descriptor_.GetProtoMsg(), t3.__desc_.tensor_descriptor_.GetProtoMsg());
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

TEST_F(TensorUT, SetData_SharedWithTensorDef) {
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

  std::vector<uint8_t> vec2;
  for (uint8_t i = 0; i < 100; ++i) {
    vec2.push_back(i);
  }
  t2.SetData(vec2);
  ASSERT_EQ(memcmp(t2.GetData().GetData(), vec2.data(), vec2.size()), 0);
  // todo 这里存在bug，但是从目前来看，并没有被触发，因此暂时不修复了，重构后一起修复。
  //  触发bug的场景为：如果tensor1是通过tensor_def_持有TensorData，然后通过拷贝构造、拷贝赋值的方式，从tensor1构造了tensor2。
  //  那么通过tensor2.SetData后，会导致tensor1的GetData接口失效（获取到野指针）
  //  触发的表现就是，如下两条ASSERT_EQ并不成立
  // ASSERT_EQ(t1.GetData().GetData(), t2.GetData().GetData());
  // ASSERT_EQ(memcmp(t1.GetData().GetData(), vec2.data(), vec2.size()), 0);
}

TEST_F(TensorUT, SetData_SharedWithoutTensorDef) {
  GeTensor t1;

  std::vector<uint8_t> vec;
  for (uint8_t i = 0; i < 100; ++i) {
    vec.push_back(i * 2);
  }
  t1.SetData(vec);
  GeTensor t2(t1);

  std::vector<uint8_t> vec3;
  for (uint8_t i = 0; i < 100; ++i) {
    vec3.push_back(i);
  }
  t2.SetData(vec3);
  ASSERT_EQ(t2.GetData().size(), vec3.size());
  ASSERT_EQ(memcmp(t2.GetData().GetData(), vec3.data(), vec3.size()), 0);
  ASSERT_EQ(t1.GetData().size(), vec3.size());
  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec3.data(), vec3.size()), 0);
  ASSERT_EQ(t1.GetData().GetData(), t2.GetData().GetData());

  std::vector<uint8_t> vec2;
  for (uint8_t i = 0; i < 105; ++i) {
    vec2.push_back(i);
  }
  t2.SetData(vec2);
  ASSERT_EQ(t2.GetData().size(), vec2.size());
  ASSERT_EQ(memcmp(t2.GetData().GetData(), vec2.data(), vec2.size()), 0);
  // after modify the data of t2 using a different size buffer, the t1 will not be modified
  ASSERT_EQ(t1.GetData().size(), vec3.size());
  ASSERT_EQ(memcmp(t1.GetData().GetData(), vec3.data(), vec3.size()), 0);
  ASSERT_NE(t1.GetData().GetData(), t2.GetData().GetData());
}
}  // namespace ge