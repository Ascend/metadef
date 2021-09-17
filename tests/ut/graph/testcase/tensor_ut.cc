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
#include "graph/ge_tensor.h"
#include "ge_ir.pb.h"
#include "graph/ge_tensor_impl.h"
#include "graph/utils/tensor_adapter.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"

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
  ASSERT_EQ(t1.impl_->desc_.impl_, t1.GetData().impl_->tensor_descriptor_);

  GeTensorDesc td;
  ASSERT_NE(td.impl_->tensor_descriptor_.GetProtoOwner(), nullptr);
  ASSERT_NE(td.impl_->tensor_descriptor_.GetProtoMsg(), nullptr);

  GeIrProtoHelper<ge::proto::TensorDef> helper;
  helper.InitDefault();
  helper.GetProtoMsg()->mutable_data()->resize(200);
  GeTensor t2(helper.GetProtoOwner(), helper.GetProtoMsg());
  ASSERT_NE(t2.impl_->tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_NE(t2.impl_->tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_EQ(t2.impl_->tensor_data_.impl_->tensor_descriptor_, t2.impl_->desc_.impl_);
  ASSERT_EQ(reinterpret_cast<const char *>(t2.impl_->tensor_data_.GetData()),
            t2.impl_->tensor_def_.GetProtoMsg()->data().data());
}
TEST_F(TensorUT, Construct2_CopyDesc) {
  GeTensorDesc desc;
  GeTensor t1(desc);
  ASSERT_NE(t1.impl_->desc_.impl_->tensor_descriptor_.GetProtoMsg(), desc.impl_->tensor_descriptor_.GetProtoMsg());
}
TEST_F(TensorUT, Construct3_ExceptionalScenes) {
  GeIrProtoHelper<ge::proto::TensorDef> helper;
  helper.InitDefault();
  GeTensor t1(nullptr, helper.GetProtoMsg());
  GeTensor t2(helper.GetProtoOwner(), nullptr);
  GeTensor t3(nullptr, nullptr);

  ASSERT_EQ(t1.impl_->tensor_def_.GetProtoMsg(), helper.GetProtoMsg());
  ASSERT_EQ(t1.impl_->tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_EQ(t1.impl_->tensor_data_.impl_->tensor_descriptor_, t1.impl_->desc_.impl_);

  ASSERT_EQ(t2.impl_->tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_EQ(t2.impl_->tensor_def_.GetProtoOwner(), helper.GetProtoOwner());
  ASSERT_EQ(t2.impl_->tensor_data_.impl_->tensor_descriptor_, t2.impl_->desc_.impl_);

  ASSERT_EQ(t3.impl_->tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_EQ(t3.impl_->tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_EQ(t3.impl_->tensor_data_.impl_->tensor_descriptor_, t3.impl_->desc_.impl_);
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
  ASSERT_EQ(t1.impl_->tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_EQ(t1.impl_->tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_NE(t1.impl_->desc_.impl_->tensor_descriptor_.GetProtoMsg(),
            t2.impl_->desc_.impl_->tensor_descriptor_.GetProtoMsg());
  ASSERT_NE(t1.impl_->desc_.impl_->tensor_descriptor_.GetProtoOwner(),
            t2.impl_->desc_.impl_->tensor_descriptor_.GetProtoOwner());
  ASSERT_EQ(t1.impl_->tensor_data_.impl_->tensor_descriptor_, t1.impl_->desc_.impl_);
  ASSERT_EQ(t2.impl_->tensor_data_.impl_->tensor_descriptor_, t2.impl_->desc_.impl_);
  ASSERT_EQ(t1.impl_->tensor_data_.GetData(), t2.impl_->tensor_data_.GetData());

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

  // Copy construct should share tensordata only
  ASSERT_NE(t1.impl_->tensor_def_.GetProtoOwner(), nullptr);
  ASSERT_NE(t1.impl_->tensor_def_.GetProtoMsg(), nullptr);
  ASSERT_EQ(t1.impl_->tensor_data_.impl_->tensor_descriptor_, t1.impl_->desc_.impl_);
  ASSERT_EQ(t2.impl_->tensor_data_.impl_->tensor_descriptor_, t2.impl_->desc_.impl_);
  ASSERT_EQ(t1.impl_->tensor_data_.GetData(), t2.impl_->tensor_data_.GetData());

  t1.MutableTensorDesc().SetFormat(FORMAT_NCHW);
  ASSERT_EQ(t1.GetTensorDesc().GetFormat(), FORMAT_NCHW);
  t2.MutableTensorDesc().SetFormat(FORMAT_NHWC);
  ASSERT_EQ(t1.GetTensorDesc().GetFormat(), FORMAT_NCHW);
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

TEST_F(TensorUT, SetDataDelete_success) {
  auto deleter = [](uint8_t *ptr) {
    delete[] ptr;
    ptr = nullptr;
  };
  uint8_t *data_ptr = new uint8_t[10];
  GeTensor ge_tensor;
  ge_tensor.SetData(data_ptr, 10, deleter);
  auto length = ge_tensor.GetData().GetSize();
  ASSERT_EQ(length, 10);
}

TEST_F(TensorUT, TransTensorDescWithoutOriginShape2GeTensorDesc) {
  TensorDesc desc(Shape({1, 2, 3, 4}), FORMAT_NCHW);
  GeTensorDesc ge_desc = TensorAdapter::TensorDesc2GeTensorDesc(desc);
  ASSERT_EQ(desc.GetFormat(), ge_desc.GetFormat());
  ASSERT_EQ(desc.GetShape().GetDims().size(), ge_desc.GetShape().GetDims().size());
  for (size_t i = 0; i < desc.GetShape().GetDims().size(); i++) {
    ASSERT_EQ(desc.GetShape().GetDim(i), ge_desc.GetShape().GetDim(i));
  }
  bool origin_format_is_set = false;
  EXPECT_FALSE(AttrUtils::GetBool(ge_desc, ATTR_NAME_ORIGIN_FORMAT_IS_SET, origin_format_is_set));
}

TEST_F(TensorUT, TransTensorDescWithOriginShape2GeTensorDesc) {
  TensorDesc desc(Shape({1, 2, 3, 4}), FORMAT_NCHW);
  desc.SetOriginFormat(FORMAT_NHWC);
  desc.SetOriginShape(Shape({1, 3, 4, 2}));
  GeTensorDesc ge_desc = TensorAdapter::TensorDesc2GeTensorDesc(desc);

  ASSERT_EQ(desc.GetFormat(), ge_desc.GetFormat());
  ASSERT_EQ(desc.GetShape().GetDims().size(), ge_desc.GetShape().GetDims().size());
  for (size_t i = 0; i < desc.GetShape().GetDims().size(); i++) {
    ASSERT_EQ(desc.GetShape().GetDim(i), ge_desc.GetShape().GetDim(i));
  }

  ASSERT_EQ(desc.GetOriginFormat(), ge_desc.GetOriginFormat());
  ASSERT_EQ(desc.GetOriginShape().GetDims().size(), ge_desc.GetOriginShape().GetDims().size());
  for (size_t i = 0; i < desc.GetOriginShape().GetDims().size(); i++) {
    ASSERT_EQ(desc.GetOriginShape().GetDim(i), ge_desc.GetOriginShape().GetDim(i));
  }
  bool origin_format_is_set = false;
  EXPECT_TRUE(AttrUtils::GetBool(ge_desc, ATTR_NAME_ORIGIN_FORMAT_IS_SET, origin_format_is_set));
  EXPECT_TRUE(origin_format_is_set);
}

TEST_F(TensorUT, NormalizeGeTensorWithOriginShape) {
  TensorDesc desc(Shape({1, 2, 3, 4}), FORMAT_NCHW);
  desc.SetOriginFormat(FORMAT_NHWC);
  desc.SetOriginShape(Shape({1, 3, 4, 2}));
  Tensor tensor(desc);
  auto ge_tensor = TensorAdapter::AsGeTensor(tensor);
  auto &ge_desc = ge_tensor.MutableTensorDesc();

  bool origin_format_is_set = false;
  EXPECT_TRUE(AttrUtils::GetBool(ge_desc, ATTR_NAME_ORIGIN_FORMAT_IS_SET, origin_format_is_set));
  EXPECT_TRUE(origin_format_is_set);

  auto normalized_ge_tensor = TensorAdapter::NormalizeGeTensor(ge_tensor);
  auto &normalized_ge_desc = normalized_ge_tensor.MutableTensorDesc();

  EXPECT_TRUE(AttrUtils::GetBool(normalized_ge_desc, ATTR_NAME_ORIGIN_FORMAT_IS_SET, origin_format_is_set));
  EXPECT_FALSE(origin_format_is_set);

  auto storage_format = static_cast<int64_t>(FORMAT_MAX);
  EXPECT_TRUE(AttrUtils::GetInt(normalized_ge_desc, ATTR_NAME_STORAGE_FORMAT, storage_format));
  EXPECT_EQ(storage_format, static_cast<int64_t>(ge_desc.GetFormat()));

  std::vector<int64_t> storage_dims;
  EXPECT_TRUE(AttrUtils::GetListInt(normalized_ge_desc, ATTR_NAME_STORAGE_SHAPE, storage_dims));
  EXPECT_EQ(storage_dims.size(), ge_desc.GetShape().GetDims().size());
  for (size_t i = 0; i < storage_dims.size(); i++) {
    ASSERT_EQ(ge_desc.GetShape().GetDim(i), storage_dims[i]);
  }

  EXPECT_EQ(ge_desc.GetOriginFormat(), normalized_ge_desc.GetFormat());
  ASSERT_EQ(ge_desc.GetOriginShape().GetDims().size(), normalized_ge_desc.GetShape().GetDims().size());
  for (size_t i = 0; i < ge_desc.GetOriginShape().GetDims().size(); i++) {
    ASSERT_EQ(ge_desc.GetOriginShape().GetDim(i), normalized_ge_desc.GetShape().GetDim(i));
  }
}

TEST_F(TensorUT, GeShapeSetDimNum) {
  ge::GeShape shape;
  EXPECT_EQ(shape.GetDimNum(), 0);
  shape.SetDimNum(2);  // Normal dim nums
  EXPECT_EQ(shape.GetDimNum(), 2);
  EXPECT_EQ(shape.GetDim(0), ge::UNKNOWN_DIM);
  EXPECT_EQ(shape.GetDim(1), ge::UNKNOWN_DIM);
  shape.SetDimNum(0);  // Scalar dim nums
  EXPECT_EQ(shape.GetDimNum(), 0);
  shape.SetDimNum(20);  // Big dim nums
  EXPECT_EQ(shape.GetDimNum(), 20);
  for (int i = 0; i < 20; i++) {
    EXPECT_EQ(shape.GetDim(i), ge::UNKNOWN_DIM);
  }
}

TEST_F(TensorUT, GeShapeIsUnknownDimNum) {
  ge::GeShape shape;
  EXPECT_FALSE(shape.IsUnknownDimNum());
  shape.SetDimNum(2);
  EXPECT_FALSE(shape.IsUnknownDimNum());
  shape.SetIsUnknownDimNum();
  EXPECT_TRUE(shape.IsUnknownDimNum());
  shape.SetDimNum(2);
  EXPECT_FALSE(shape.IsUnknownDimNum());
}

TEST_F(TensorUT, GeShapeAppendDim) {
  ge::GeShape shape;
  EXPECT_EQ(shape.GetDimNum(), 0);
  shape.AppendDim(1);
  EXPECT_EQ(shape.GetDimNum(), 1);
  EXPECT_EQ(shape.GetDim(0), 1);
  shape.AppendDim(2);
  EXPECT_EQ(shape.GetDimNum(), 2);
  EXPECT_EQ(shape.GetDim(0), 1);
  EXPECT_EQ(shape.GetDim(1), 2);
  shape.SetIsUnknownDimNum();
  EXPECT_TRUE(shape.IsUnknownDimNum());
  shape.AppendDim(1);
  EXPECT_FALSE(shape.IsUnknownDimNum());
}

TEST_F(TensorUT, GeTensorDescGetShape) {
  ge::GeTensorDesc desc(ge::GeShape(std::vector<int64_t>({1, 2})));
  auto &shape = desc.GetShape();
  EXPECT_EQ(shape.GetDim(0), 1);
  EXPECT_EQ(shape.GetDim(1), 2);
  const_cast<ge::GeShape *>(&shape)->SetDim(0, 10);
  const_cast<ge::GeShape *>(&shape)->SetDim(1, 20);
  auto &shape2 = desc.GetShape();
  EXPECT_EQ(shape2.GetDim(0), 10);
  EXPECT_EQ(shape2.GetDim(1), 20);
}

TEST_F(TensorUT, GeTensorSerializeUtils_GeShape) {
  GeShape shape({1, 2, 3, 4});
  proto::ShapeDef shape_proto;
  GeTensorSerializeUtils::GeShapeAsProto(shape, &shape_proto);
  GeShape shape_from_proto;
  GeTensorSerializeUtils::AssembleGeShapeFromProto(&shape_proto, shape_from_proto);
  EXPECT_EQ(shape, shape_from_proto);
}

TEST_F(TensorUT, GeTensorSerializeUtils_GeTensorDesc) {
  GeShape shape({1, 2, 3, 4});
  GeTensorDesc desc(shape, FORMAT_NC1HWC0, DT_FLOAT16);
  desc.SetOriginDataType(DT_INT32);
  desc.SetOriginFormat(FORMAT_NHWC1C0);
  desc.SetOriginShape(GeShape({4, 3, 2, 1}));
  proto::TensorDescriptor desc_proto;
  GeTensorSerializeUtils::GeTensorDescAsProto(desc, &desc_proto);
  GeTensorDesc desc_from_proto;
  GeTensorSerializeUtils::AssembleGeTensorDescFromProto(&desc_proto, desc_from_proto);
  bool res = false;
  EXPECT_TRUE(AttrUtils::GetBool(desc_from_proto, "origin_shape_initialized", res));
  EXPECT_TRUE(res);
  EXPECT_EQ(desc, desc_from_proto);
}

TEST_F(TensorUT, GeTensorSerializeUtils_GeTensor) {
  GeShape shape({1, 2, 3, 4});
  GeTensorDesc desc(shape, FORMAT_NC1HWC0, DT_FLOAT16);
  desc.SetOriginDataType(DT_INT32);
  desc.SetOriginFormat(FORMAT_NHWC1C0);
  desc.SetOriginShape(GeShape({4, 3, 2, 1}));
  GeTensor tensor(desc);
  proto::TensorDef tensor_proto;
  GeTensorSerializeUtils::GeTensorAsProto(tensor, &tensor_proto);
  GeTensor tensor_from_proto;
  GeTensorSerializeUtils::AssembleGeTensorFromProto(&tensor_proto, tensor_from_proto);
  EXPECT_EQ(tensor.GetTensorDesc(), desc);
  EXPECT_EQ(tensor.GetTensorDesc(), tensor_from_proto.GetTensorDesc());
}

TEST_F(TensorUT, GeShape_ModifyDimNum) {
  GeShape shape({1, 2, 3, 4});
  EXPECT_EQ(shape.GetShapeSize(), 24);
  EXPECT_EQ(shape.GetDimNum(), 4);
  shape.SetDimNum(2);
  EXPECT_EQ(shape.GetDimNum(), 2);
  EXPECT_FALSE(shape.IsUnknownDimNum());
  shape.SetIsUnknownDimNum();
  EXPECT_TRUE(shape.IsUnknownDimNum());
  EXPECT_EQ(shape.GetShapeSize(), -1);
  shape.SetDimNum(2);
  EXPECT_EQ(shape.GetDimNum(), 2);
  EXPECT_FALSE(shape.IsUnknownDimNum());
  shape.SetDim(0, 2);
  shape.SetDim(1, 2);
  EXPECT_EQ(shape.GetShapeSize(), 4);
  shape.SetDim(0, INT64_MAX);
  shape.SetDim(1, 2);
  EXPECT_EQ(shape.GetShapeSize(), -1);
}

TEST_F(TensorUT, GeTensorDesc_Update) {
  GeShape shape({1, 2, 3, 4});
  GeTensorDesc desc(shape, FORMAT_NC1HWC0, DT_FLOAT16);
  EXPECT_EQ(desc.GetShape(), shape);
  EXPECT_EQ(desc.GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(desc.GetDataType(), DT_FLOAT16);
  GeShape shape2({4, 3, 2, 1});
  desc.Update(shape2, FORMAT_NHWC, DT_INT32);
  EXPECT_EQ(desc.GetShape(), shape2);
  EXPECT_EQ(desc.GetFormat(), FORMAT_NHWC);
  EXPECT_EQ(desc.GetDataType(), DT_INT32);
}

TEST_F(TensorUT, AttrUtils_SetGeTensorDesc) {
  GeShape shape({1, 2, 3, 4});
  GeTensorDesc desc(shape, FORMAT_NC1HWC0, DT_FLOAT16);
  GeTensorDesc obj;
  ge::AttrUtils::SetTensorDesc(obj, "attr_tensor", desc);
  GeTensorDesc desc_from_attr;
  ge::AttrUtils::GetTensorDesc(obj, "attr_tensor", desc_from_attr);
  EXPECT_EQ(desc, desc_from_attr);
}

TEST_F(TensorUT, AttrUtils_SetListGeTensorDesc) {
  GeShape shape({1, 2, 3, 4});
  std::vector<GeTensorDesc> descs;
  descs.emplace_back(GeTensorDesc(GeShape({1, 2, 3, 4}), FORMAT_NC1HWC0, DT_FLOAT16));
  descs.emplace_back(GeTensorDesc(GeShape({4, 3, 2, 1}), FORMAT_NCHW, DT_INT32));
  GeTensorDesc obj;
  ge::AttrUtils::SetListTensorDesc(obj, "attr_tensors", descs);
  std::vector<GeTensorDesc> descs_from_attr;
  ge::AttrUtils::GetListTensorDesc(obj, "attr_tensors", descs_from_attr);
  EXPECT_EQ(descs.size(), descs_from_attr.size());
  for (size_t i = 0; i < descs.size(); i++) {
    EXPECT_EQ(descs[i], descs_from_attr[i]);
  }
}

}  // namespace ge