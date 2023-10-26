/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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
#include <memory>
#include <string>

#define private public
#define protected public
#include "ge_ir.pb.h"
#include "graph/args_format_desc.h"
#include "graph/ge_attr_value.h"
#include "graph/ge_tensor.h"
#include "graph/ge_tensor_impl.h"
#include "graph/tensor.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#undef private
#undef protected
#include "external/graph/operator_factory.h"
#include "external/graph/operator_reg.h"
#include "external/register/op_impl_registry.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/op_desc_utils_ex.h"

using namespace std;
using namespace ge;
namespace ge {
class UtestArgsFormatDesc : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestArgsFormatDesc, serialize_simple_args) {
  auto op_desc = std::make_shared<OpDesc>("tmp_op", "Mul");
  ArgsFormatDesc desc;
  desc.Append(AddrType::FFTS_ADDR);
  desc.Append(AddrType::OVERFLOW_ADDR);
  desc.Append(AddrType::TILING);
  desc.Append(AddrType::TILING_FFTS, 0);
  desc.Append(AddrType::TILING_FFTS, 1);
  std::string res = desc.ToString();
  std::string expect_res = "{ffts_addr}{overflow_addr}{t}{t_ffts.non_tail}{t_ffts.tail}";
  EXPECT_EQ(expect_res, res);
  size_t args_size{0UL};
  EXPECT_EQ(desc.GetArgsSize(op_desc, args_size), ge::GRAPH_SUCCESS);
  EXPECT_EQ(args_size, 40UL);
  std::vector<ArgDesc> descs;
  EXPECT_EQ(ArgsFormatDesc::Parse(op_desc, expect_res, descs), SUCCESS);
}

REG_OP(SimpleOP)
    .INPUT(a, TensorType(DT_FLOAT32))
    .INPUT(b, TensorType(DT_FLOAT32))
    .INPUT(c, TensorType(DT_FLOAT32))
    .OUTPUT(x, TensorType(DT_FLOAT32))
    .OUTPUT(y, TensorType(DT_FLOAT32))
    .OUTPUT(z, TensorType(DT_FLOAT32))
    .OP_END_FACTORY_REG(SimpleOP);

TEST_F(UtestArgsFormatDesc, common_args) {
  auto op = OperatorFactory::CreateOperator("test1", "SimpleOP");
  auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
  ASSERT_NE(op_desc, nullptr);
  GeShape shape({1, 40, 1024, 256});
  GeTensorDesc desc(shape);
  op_desc->AddInputDesc(desc);
  op_desc->AddInputDesc(desc);
  op_desc->AddInputDesc(desc);

  op_desc->AddOutputDesc(desc);
  op_desc->AddOutputDesc(desc);
  op_desc->AddOutputDesc(desc);

  ArgsFormatDesc args_des;
  args_des.Append(AddrType::INPUT, -1);
  args_des.Append(AddrType::OUTPUT, -1);
  args_des.Append(AddrType::WORKSPACE, -1);
  std::string res = args_des.ToString();
  std::string expect_res = "{i*}{o*}{ws*}";
  EXPECT_EQ(expect_res, res);

  size_t args_size{0UL};
  EXPECT_EQ(args_des.GetArgsSize(op_desc, args_size), ge::GRAPH_SUCCESS);
  EXPECT_EQ(args_size, 176UL);
  std::vector<ArgDesc> descs;
  EXPECT_EQ(ArgsFormatDesc::Parse(op_desc, expect_res, descs), SUCCESS);
  EXPECT_EQ(descs.size(), 7UL);
  EXPECT_EQ(descs[2].addr_type, AddrType::INPUT);
  EXPECT_EQ(descs[2].ir_idx, 2);
}

REG_OP(DynOP)
    .INPUT(a, TensorType(DT_FLOAT32))
    .DYNAMIC_INPUT(b, TensorType(DT_FLOAT32))
    .INPUT(c, TensorType(DT_FLOAT32))
    .OUTPUT(x, TensorType(DT_FLOAT32))
    .DYNAMIC_OUTPUT(y, TensorType(DT_FLOAT32))
    .OUTPUT(z, TensorType(DT_FLOAT32))
    .OP_END_FACTORY_REG(DynOP);

TEST_F(UtestArgsFormatDesc, common_args_dynamic_folded) {
  auto op = OperatorFactory::CreateOperator("test1", "DynOP");
  auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
  ASSERT_NE(op_desc, nullptr);
  GeShape shape({1, 40, 1024, 256});
  GeTensorDesc desc(shape);
  op_desc->UpdateInputDesc(0, desc);
  op_desc->AddDynamicInputDescByIndex("b", 1, 1);
  op_desc->UpdateInputDesc(1, desc);
  op_desc->UpdateInputDesc(2, desc);
  op_desc->UpdateOutputDesc(0, desc);
  op_desc->AddDynamicOutputDesc("y", 1, true);
  op_desc->UpdateOutputDesc("y0", desc);
  op_desc->UpdateOutputDesc(0, desc);

  ArgsFormatDesc args_des;
  args_des.Append(AddrType::INPUT, -1);
  args_des.Append(AddrType::OUTPUT, -1);
  args_des.Append(AddrType::WORKSPACE, 0);
  args_des.Append(AddrType::WORKSPACE, 1);
  std::string res = args_des.ToString();
  std::string expect_res = "{i*}{o*}{ws0*}{ws1*}";
  EXPECT_EQ(expect_res, res);

  size_t args_size{0UL};
  EXPECT_EQ(args_des.GetArgsSize(op_desc, args_size), ge::GRAPH_SUCCESS);
  EXPECT_EQ(args_size, 80UL);
  std::vector<ArgDesc> descs;
  EXPECT_EQ(ArgsFormatDesc::Parse(op_desc, expect_res, descs), SUCCESS);

  std::string expanded_res = "{i0}{i1}{i2}{o0}{o1}{o2}{ws0}{ws1}";
  std::vector<ArgDesc> expanded_descs;
  EXPECT_EQ(ArgsFormatDesc::Parse(op_desc, expanded_res, expanded_descs), SUCCESS);
  EXPECT_EQ(descs.size(), expanded_descs.size());
  for (auto i = 0UL; i < descs.size(); ++i) {
    EXPECT_EQ(descs[i].addr_type, expanded_descs[i].addr_type);
    EXPECT_EQ(descs[i].ir_idx, expanded_descs[i].ir_idx);
    EXPECT_EQ(descs[i].folded, expanded_descs[i].folded);
  }
}

TEST_F(UtestArgsFormatDesc, common_args_size_equal) {
  auto op = OperatorFactory::CreateOperator("test1", "DynOP");
  auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
  ASSERT_NE(op_desc, nullptr);
  GeShape shape({1, 40, 1024, 256});
  GeTensorDesc desc(shape);
  op_desc->UpdateInputDesc(0, desc);
  op_desc->AddDynamicInputDescByIndex("b", 1, 1);
  op_desc->UpdateInputDesc(1, desc);
  op_desc->UpdateInputDesc(2, desc);
  op_desc->UpdateOutputDesc(0, desc);
  op_desc->AddDynamicOutputDesc("y", 1, true);
  op_desc->UpdateOutputDesc("y0", desc);
  op_desc->UpdateOutputDesc(0, desc);

  ArgsFormatDesc args_des;
  args_des.Append(AddrType::INPUT, -1);
  args_des.Append(AddrType::OUTPUT, -1);
  args_des.Append(AddrType::WORKSPACE, 0);
  args_des.Append(AddrType::WORKSPACE, 1);
  size_t args_size{0UL};
  EXPECT_EQ(args_des.GetArgsSize(op_desc, args_size), ge::GRAPH_SUCCESS);
  ArgsFormatDesc args_des1;
  args_des1.Append(AddrType::INPUT, 0);
  args_des1.Append(AddrType::INPUT, 1, true);
  args_des1.Append(AddrType::INPUT, 2);
  args_des1.Append(AddrType::OUTPUT, 0);
  args_des1.Append(AddrType::OUTPUT, 1, true);
  args_des1.Append(AddrType::OUTPUT, 2);
  args_des1.Append(AddrType::WORKSPACE, 0);
  args_des1.Append(AddrType::WORKSPACE, 1);
  size_t args_size1{0UL};
  EXPECT_EQ(args_des1.GetArgsSize(op_desc, args_size1), ge::GRAPH_SUCCESS);
  EXPECT_EQ(args_size, args_size1);
}

REG_OP(IFA)
    .INPUT(query, TensorType(DT_FLOAT32))
    .DYNAMIC_INPUT(key, TensorType(DT_FLOAT32))
    .DYNAMIC_INPUT(value, TensorType(DT_FLOAT32))
    .OPTIONAL_INPUT(padding_mask, TensorType(DT_FLOAT32))
    .OPTIONAL_INPUT(atten_mask, TensorType(DT_FLOAT32))
    .OPTIONAL_INPUT(actual_seq_lengths, TensorType(DT_FLOAT32))
    .DYNAMIC_OUTPUT(attention_out, TensorType(DT_FLOAT32))
    .OP_END_FACTORY_REG(IFA);

TEST_F(UtestArgsFormatDesc, serialize_dynamic_args) {
  auto op = OperatorFactory::CreateOperator("test1", "IFA");
  auto op_desc = OpDescUtils::GetOpDescFromOperator(op);

  ASSERT_NE(op_desc, nullptr);
  GeShape shape({1, 40, 1024, 256});
  GeTensorDesc desc(shape);
  GeShape scalar_shape;
  GeTensorDesc scalar_desc(scalar_shape);
  op_desc->UpdateInputDesc(0, desc);
  op_desc->AddDynamicInputDescByIndex("key", 2, 1);
  op_desc->UpdateInputDesc(1, desc);
  op_desc->UpdateInputDesc(2, desc);
  op_desc->AddDynamicInputDescByIndex("value", 2, 3);
  op_desc->UpdateInputDesc(3, desc);
  op_desc->UpdateInputDesc(4, scalar_desc);
  op_desc->UpdateInputDesc("atten_mask", desc);

  op_desc->AddDynamicOutputDesc("attention_out", 2, true);
  op_desc->UpdateOutputDesc("attention_out0", desc);
  op_desc->UpdateOutputDesc("attention_out1", scalar_desc);

  ArgsFormatDesc args_des;
  args_des.Append(AddrType::FFTS_ADDR);
  args_des.Append(AddrType::INPUT, 0);
  args_des.Append(AddrType::INPUT_DESC, 1, true);
  args_des.Append(AddrType::INPUT_DESC, 2, true);
  args_des.Append(AddrType::INPUT, 4);
  args_des.Append(AddrType::OUTPUT_DESC, 0, true);
  args_des.Append(AddrType::WORKSPACE, 0);
  args_des.Append(AddrType::TILING_FFTS, 1);
  std::string res = args_des.ToString();
  std::string expect_res = "{ffts_addr}{i0*}{i_desc1}{i_desc2}{i4*}{o_desc0}{ws0*}{t_ffts.tail}";
  EXPECT_EQ(expect_res, res);

  size_t args_size{0UL};
  EXPECT_EQ(args_des.GetArgsSize(op_desc, args_size), ge::GRAPH_SUCCESS);
  EXPECT_EQ(args_size, 328UL);
  std::vector<ArgDesc> descs;
  EXPECT_EQ(ArgsFormatDesc::Parse(op_desc, expect_res, descs), SUCCESS);
  EXPECT_EQ(descs.size(), 8UL);
  EXPECT_EQ(descs[2].addr_type, AddrType::INPUT_DESC);
}

}  // namespace ge
