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

#define protected public
#define private public
#include "graph/op_desc.h"
#include "graph/op_desc_impl.h"
#include "graph/ge_tensor.h"
#include "graph/utils/ge_ir_utils.h"
#undef private
#undef protected
#include "graph/utils/transformer_utils.h"

namespace ge {
class UtestOpDesc : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestOpDesc, TestCommonVerifyOnDummyShape) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({-3}));
  tensor_desc->SetFormat(FORMAT_NCHW);
  tensor_desc->SetDataType(DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>("test", "Identity");
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddOutputDesc(tensor_desc->Clone());

  EXPECT_EQ(GRAPH_SUCCESS, op_desc->CommonVerify());
}

TEST_F(UtestOpDesc, TestOpDescGetSetTensorDesc) {
  GeTensorDesc desc(GeShape(), FORMAT_NCHW, DT_INT32);
  OpDesc op_desc("foo", "Foo");
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddInputDesc("x", desc));
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddOutputDesc("y", desc));

  EXPECT_EQ(op_desc.GetInputDesc("x"), desc);
  EXPECT_EQ(op_desc.GetOutputDesc("y"), desc);
}

TEST_F(UtestOpDesc, TestNodeShapeTransUtils) {

  NodeShapeTransUtils transformer1(nullptr);
  EXPECT_NE(transformer1.Init(), true);

  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1, 1, 16, 16}));
  tensor_desc->SetFormat(FORMAT_FRACTAL_NZ);
  tensor_desc->SetDataType(DT_FLOAT);
  tensor_desc->SetOriginFormat(FORMAT_ND);

  auto op_desc = std::make_shared<OpDesc>("test", "Identity");
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddOutputDesc(tensor_desc->Clone());
  NodeShapeTransUtils transformer2(op_desc);
  EXPECT_EQ(transformer2.Init(), true);
  EXPECT_EQ(transformer2.CatchFormatAndShape(), true);
  EXPECT_EQ(transformer2.UpdateFormatAndShape(), true);


  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddOutputDesc(tensor_desc->Clone());

  NodeShapeTransUtils transformer3(op_desc);
  EXPECT_EQ(transformer3.Init(), true);
  EXPECT_EQ(transformer3.CatchFormatAndShape(), true);
  EXPECT_EQ(transformer3.UpdateFormatAndShape(), true);


  EXPECT_EQ(GRAPH_SUCCESS, op_desc->CommonVerify());
}

TEST_F(UtestOpDesc, IndexOutOfRange) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1}));
  tensor_desc->SetFormat(FORMAT_NCHW);
  tensor_desc->SetDataType(DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>("test", "Identity");
  op_desc->AddInputDesc(tensor_desc->Clone());

  EXPECT_NE(nullptr, op_desc->MutableInputDesc(0));
  EXPECT_EQ(nullptr, op_desc->MutableInputDesc(1));
  EXPECT_EQ(nullptr, op_desc->MutableInputDesc(999));
}

TEST_F(UtestOpDesc, SerializeMetadata) {
  OpDescImpl impl;
  impl.meta_data_.inputs_.emplace_back("input");
  impl.meta_data_.input_names_.emplace_back("names");
  impl.meta_data_.src_names_.push_back("src");
  impl.meta_data_.dst_names_.push_back("dst");
  impl.meta_data_.dst_indexes_.push_back(2);
  impl.meta_data_.src_indexes_.push_back(2);
  impl.meta_data_.input_offsets_.push_back(987654321);
  impl.meta_data_.output_offsets_.push_back(987654321);
  impl.meta_data_.workspaces.push_back(222);
  impl.meta_data_.workspace_bytes_list_.push_back(111);
  impl.meta_data_.is_input_consts_.push_back(false);

  proto::OpDef def;
  impl.SerializeMetaDataToOpDef(&def);
  EXPECT_EQ(def.input(0), "input");
  EXPECT_EQ(def.input_name(0), "names");
  EXPECT_EQ(def.src_name(0), "src");
  EXPECT_EQ(def.dst_name(0), "dst");
  EXPECT_EQ(def.dst_index(0), 2);
  EXPECT_EQ(def.src_index(0), 2);
  EXPECT_EQ(def.input_i(0), 987654321);
  EXPECT_EQ(def.output_i(0), 987654321);
  EXPECT_EQ(def.workspace(0), 222);
  EXPECT_EQ(def.workspace_bytes(0), 111);
  EXPECT_EQ(def.is_input_const(0), false);
}

TEST_F(UtestOpDesc, DeSerializeMetadata) {
  proto::OpDef def;
  def.add_input("input");
  def.add_input_name("names");
  def.add_src_name("src");
  def.add_dst_name("dst");
  def.add_dst_index(2);
  def.add_src_index(2);
  def.add_input_i(987654321);
  def.add_output_i(987654321);
  def.add_workspace(222);
  def.add_workspace_bytes(222);
  def.add_is_input_const(false);
  OpDescImpl impl;
  impl.DeSerializeOpDefToMetaData(def);
  EXPECT_EQ(impl.meta_data_.inputs_.size(), 1);
  EXPECT_EQ(impl.meta_data_.inputs_[0], "input");
  EXPECT_EQ(impl.meta_data_.input_names_.size(), 1);
  EXPECT_EQ(impl.meta_data_.input_names_[0], "names");
  EXPECT_EQ(impl.meta_data_.src_names_.size(), 1);
  EXPECT_EQ(impl.meta_data_.src_names_[0], "src");
  EXPECT_EQ(impl.meta_data_.dst_names_.size(), 1);
  EXPECT_EQ(impl.meta_data_.dst_names_[0], "dst");
  EXPECT_EQ(impl.meta_data_.dst_indexes_.size(), 1);
  EXPECT_EQ(impl.meta_data_.dst_indexes_[0], 2);
  EXPECT_EQ(impl.meta_data_.src_indexes_.size(), 1);
  EXPECT_EQ(impl.meta_data_.src_indexes_[0], 2);
  EXPECT_EQ(impl.meta_data_.input_offsets_.size(), 1);
  EXPECT_EQ(impl.meta_data_.input_offsets_[0], 987654321);
  EXPECT_EQ(impl.meta_data_.output_offsets_.size(), 1);
  EXPECT_EQ(impl.meta_data_.output_offsets_[0], 987654321);
  EXPECT_EQ(impl.meta_data_.workspaces.size(), 1);
  EXPECT_EQ(impl.meta_data_.workspaces[0], 222);
  EXPECT_EQ(impl.meta_data_.workspace_bytes_list_.size(), 1);
  EXPECT_EQ(impl.meta_data_.workspace_bytes_list_[0], 222);
  EXPECT_EQ(impl.meta_data_.is_input_consts_.size(), 1);
  EXPECT_EQ(impl.meta_data_.is_input_consts_[0], false);

  OpDescImpl impl1;
  impl1.DeSerializeOpDefToMetaData(def);
  EXPECT_TRUE(impl1.OpDescAttrsAreEqual(impl));
}

TEST_F(UtestOpDesc, AddDescForward) {
  GeTensorDesc desc(GeShape(), FORMAT_NCHW, DT_INT32);
  OpDesc op_desc("foo", "Foo");
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddOutputDesc("x", desc));
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddOutputDesc("y", desc));
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddOutputDesc("z", desc));
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddOutputDescForward("t", 2));

  EXPECT_EQ(5, op_desc.GetOutputsSize());

  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddInputDesc("x", desc));
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddInputDesc("y", desc));
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddInputDesc("z", desc));
  EXPECT_EQ(GRAPH_SUCCESS, op_desc.AddInputDescForward("t", 2));

  EXPECT_EQ(5, op_desc.GetInputsSize());
}

}
