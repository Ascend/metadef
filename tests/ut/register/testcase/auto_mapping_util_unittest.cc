/**
 * Copyright 2021-2021 Huawei Technologies Co., Ltd
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
#include <iostream>
#include "graph_builder_utils.h"
#include "external/register/register.h"
#include <google/protobuf/message.h>
#include "graph/debug/ge_util.h"
#include "graph/debug/ge_op_types.h"
#include "framework/common/debug/ge_log.h"
#include "graph/debug/ge_log.h"
#include "graph/debug/ge_util.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/type_utils.h"
#include "proto/tensorflow/node_def.pb.h"
#include "register/op_registry.h"
#include "graph/graph.h"
#include "graph/utils/attr_utils.h"
#define private public
#define protected public
#include "register/auto_mapping_util.h"
#include "external/register/scope/scope_fusion_pass_register.h"
#include "register/scope/scope_graph_impl.h"
#undef private
#undef protected


using namespace ge;
using namespace domi;
class AutoMappingUtils : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

void CreateTFGraphDef(domi::tensorflow::GraphDef &graph_def) {
  // 1. add node
  auto placeholder0 = graph_def.add_node();
  auto placeholder1 = graph_def.add_node();
  auto add0 = graph_def.add_node();
  auto add1 = graph_def.add_node();
  auto mul0 = graph_def.add_node();
  auto mul1 = graph_def.add_node();
  auto add2 = graph_def.add_node();
  auto retval0 = graph_def.add_node();
  auto retval1 = graph_def.add_node();

  // 2. set info
  placeholder0->set_name("placeholder0");
  placeholder0->set_op("PlaceHolder");
  placeholder1->set_name("placeholder1");
  placeholder1->set_op("PlaceHolder");

  add0->set_name("add0");
  add0->set_op("Add");
  add1->set_name("add1");
  add1->set_op("Add");
  add2->set_name("add2");
  add2->set_op("Add");

  mul0->set_name("mul0");
  mul0->set_op("Mul");
  mul1->set_name("mul1");
  mul1->set_op("Mul");

  retval0->set_name("retval0");
  retval0->set_op("_RetVal");
  retval1->set_name("retval1");
  retval1->set_op("_RetVal");

  // 3. add edges
  add0->add_input("placeholder0");
  add0->add_input("placeholder1");

  mul0->add_input("placeholder0");
  mul0->add_input("placeholder1");

  mul1->add_input("placeholder0");
  mul1->add_input("add0");
  mul1->add_input("^mul0");

  add1->add_input("mul0");
  add1->add_input("placeholder1");

  add2->add_input("mul1");
  add2->add_input("mul0");

  retval0->add_input("add2:0");
  retval1->add_input("add1:0");
}

TEST_F(AutoMappingUtils, FindAttrValueFalse) {
    domi::tensorflow::GraphDef graph_def;
    domi::tensorflow::AttrValue attr_num;
    CreateTFGraphDef(graph_def);
    int node_size = graph_def.node_size();
    bool ret;

    domi::tensorflow::NodeDef *node0 = nullptr;
    ret = ge::AutoMappingUtil::FindAttrValue(node0, string(""), attr_num);
    EXPECT_FALSE(ret);

    domi::tensorflow::NodeDef node1;
    ret = ge::AutoMappingUtil::FindAttrValue(&node1, string(""), attr_num);
    EXPECT_FALSE(ret);

    const domi::tensorflow::NodeDef *node2 = graph_def.mutable_node(0);
    ret = ge::AutoMappingUtil::FindAttrValue(node2, node2->name(), attr_num);
    EXPECT_FALSE(ret);
}

TEST_F(AutoMappingUtils, ConvertShape) {
  domi::tensorflow::TensorShapeProto shape;
  vector<int64_t> shape_dims;

  shape.set_unknown_rank(true);
  ge::AutoMappingUtil::ConvertShape(shape, shape_dims);
  EXPECT_EQ(shape_dims, ge::UNKNOWN_SHAPE);

  shape.set_unknown_rank(false);
  shape.add_dim();
  ge::AutoMappingUtil::ConvertShape(shape, shape_dims);
  EXPECT_NE(shape_dims, ge::UNKNOWN_SHAPE);
}

TEST_F(AutoMappingUtils, ConvertTensor) {
  ge::graphStatus ret;
  domi::tensorflow::TensorProto tensor;
  ge::GeTensorPtr weight;

  tensor.set_dtype(domi::tensorflow::DataType_INT_MAX_SENTINEL_DO_NOT_USE_);
  ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
  EXPECT_EQ(ret, GRAPH_FAILED);

  tensor.set_dtype(domi::tensorflow::DT_UINT16_REF);
  ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
  EXPECT_EQ(ret, GRAPH_FAILED);
}

TEST_F(AutoMappingUtils, ConvertTensorList) {
  domi::tensorflow::AttrValue_ListValue list;
  std::vector<ge::GeTensorPtr> vec;

  list.add_tensor();
  ge::AutoMappingUtil::ConvertTensorList(list, vec);
  EXPECT_EQ(vec.empty(), true);
}

TEST_F(AutoMappingUtils, ConvertFunc) {
  domi::tensorflow::NameAttrList tf_func;
  ge::NamedAttrs ge_func;

  tf_func.set_name("test_fun");
  ge::AutoMappingUtil::ConvertFunc(tf_func, ge_func);
  ge::AutoMappingUtil::ConvertFunc(tf_func, ge_func, 66);
}

TEST_F(AutoMappingUtils, ConvertDataTypeList) {
  domi::tensorflow::AttrValue_ListValue list;
  std::vector<ge::DataType> vec;

  list.add_type(domi::tensorflow::DT_INT16);
  ge::AutoMappingUtil::ConvertDataTypeList(list, vec);
  EXPECT_EQ(vec.empty(), false);
}

TEST_F(AutoMappingUtils, ConvertShapeList) {
  domi::tensorflow::AttrValue_ListValue list;
  std::vector<vector<int64_t>> vec;

  list.add_shape();
  ge::AutoMappingUtil::ConvertShapeList(list, vec);
  EXPECT_EQ(vec.empty(), false);
}

TEST_F(AutoMappingUtils, ConvertFuncList) {
  domi::tensorflow::AttrValue_ListValue list;
  std::vector<ge::NamedAttrs> vec;

  list.add_func();
  ge::AutoMappingUtil::ConvertFuncList(list, vec, 66);
  EXPECT_EQ(vec.empty(), true);

  ge::AutoMappingUtil::ConvertFuncList(list, vec);
  EXPECT_EQ(vec.empty(), false);
}

// for tensor_assign.cpp
class ConvertTensorUtest : public testing::Test {
public:
    domi::tensorflow::TensorProto tensor;
    ge::graphStatus ret;
    ge::GeTensorPtr weight;

 protected:
    void SetUp() {
        tensor.set_tensor_content("tensor_context_for_test");
    }
    void TearDown() {}
};

TEST_F(ConvertTensorUtest, ConvertTensorNoType) {
    tensor.set_dtype(domi::tensorflow::DataType_INT_MAX_SENTINEL_DO_NOT_USE_);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, GRAPH_FAILED);
    tensor.clear_dtype();
}
TEST_F(ConvertTensorUtest, ConvertTensorFloat) {
    tensor.add_float_val(3.14);
    tensor.set_dtype(domi::tensorflow::DT_FLOAT);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_float_val();
}
TEST_F(ConvertTensorUtest, ConvertTensorDouble) {
    tensor.add_double_val(3.14);
    tensor.set_dtype(domi::tensorflow::DT_DOUBLE);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_double_val();
}
TEST_F(ConvertTensorUtest, ConvertTensorSComplex) {
    tensor.add_scomplex_val(3.14);
    tensor.set_dtype(domi::tensorflow::DT_COMPLEX64);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_scomplex_val();
}
TEST_F(ConvertTensorUtest, ConvertTensorDComplex) {
    tensor.add_dcomplex_val(3.14);
    tensor.set_dtype(domi::tensorflow::DT_COMPLEX128);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_dcomplex_val();
}
TEST_F(ConvertTensorUtest, ConvertTensorInt) {
    tensor.add_int_val(666);
    tensor.set_dtype(domi::tensorflow::DT_INT32);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_int_val();

    tensor.add_int64_val(666);
    tensor.set_dtype(domi::tensorflow::DT_INT64);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_int64_val();

    tensor.add_int_val(666);
    tensor.add_int_val(111);
    tensor.set_dtype(domi::tensorflow::DT_INT16);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_int_val();
    
    tensor.add_int_val(66);
    tensor.set_dtype(domi::tensorflow::DT_UINT8);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_int_val();

}
TEST_F(ConvertTensorUtest, ConvertTensorUnsignedInt) {
    tensor.add_uint32_val(666);
    tensor.set_dtype(domi::tensorflow::DT_UINT32);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_int_val();

    tensor.add_uint64_val(666);
    tensor.set_dtype(domi::tensorflow::DT_UINT64);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_int64_val();
}
TEST_F(ConvertTensorUtest, ConvertTensorBool) {
    tensor.add_bool_val(true);
    tensor.set_dtype(domi::tensorflow::DT_BOOL);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_bool_val();
}
TEST_F(ConvertTensorUtest, ConvertTensorString) {
    domi::tensorflow::TensorShapeProto *tensor_shape = new domi::tensorflow::TensorShapeProto();
    tensor.set_allocated_tensor_shape(tensor_shape);
    tensor.add_string_val("1");
    tensor.set_dtype(domi::tensorflow::DT_STRING);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);

    tensor.add_string_val("str_test2");
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_string_val();
}
TEST_F(ConvertTensorUtest, ConvertTensorHalf) {
    tensor.add_half_val(666);
    tensor.set_dtype(domi::tensorflow::DT_HALF);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
    tensor.clear_half_val();
}
TEST_F(ConvertTensorUtest, ConvertTensorHalfVariant) {
  tensor.add_variant_val();
  tensor.set_dtype(domi::tensorflow::DT_VARIANT);
  ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
  EXPECT_EQ(ret, domi::SUCCESS);
  tensor.clear_variant_val();
}

TEST_F(ConvertTensorUtest, SetWeightFloat) {
    tensor.set_dtype(domi::tensorflow::DT_FLOAT);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightDouble) {
    tensor.set_dtype(domi::tensorflow::DT_DOUBLE);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightSComplex) {
    tensor.set_dtype(domi::tensorflow::DT_COMPLEX64);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightDComplex) {
    tensor.set_dtype(domi::tensorflow::DT_COMPLEX128);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightInt) {
    tensor.set_dtype(domi::tensorflow::DT_INT16);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);

    tensor.set_dtype(domi::tensorflow::DT_INT32);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);

    tensor.set_dtype(domi::tensorflow::DT_INT64);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightUnsignedInt) {
    tensor.set_dtype(domi::tensorflow::DT_UINT8);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);

    tensor.set_dtype(domi::tensorflow::DT_UINT32);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);

    tensor.set_dtype(domi::tensorflow::DT_UINT64);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightBool) {
    tensor.set_dtype(domi::tensorflow::DT_BOOL);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightString) {
    tensor.set_dtype(domi::tensorflow::DT_STRING);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightHalf) {
    tensor.set_dtype(domi::tensorflow::DT_HALF);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}
TEST_F(ConvertTensorUtest, SetWeightVarient) {
    tensor.set_dtype(domi::tensorflow::DT_VARIANT);
    ret = ge::AutoMappingUtil::ConvertTensor(tensor, weight);
    EXPECT_EQ(ret, domi::SUCCESS);
}

TEST_F(AutoMappingUtils, CopyAttrValueInputTest) {
    ut::GraphBuilder *builder = new ut::GraphBuilder("graph");
    NodePtr node_src = builder->AddNode("ParseSingleNode", "ParseSingleType", 3, 0, FORMAT_ALL);

    ge::Operator op_src = OpDescUtils::CreateOperatorFromNode(node_src);
    ge::Operator op_dst = ge::Operator("ParseSingleExample");
    std::shared_ptr<ge::OpDesc> op_desc_dst = ge::OpDescUtils::GetOpDescFromOperator(op_dst);
    std::vector<domi::DynamicInputOutputInfo> value;
    EXPECT_EQ(op_dst.GetInputsSize(), 0);

    AttrUtils::SetStr(node_src->GetOpDesc(), "attrStr", "str_shapes");
    op_desc_dst->AddRegisterInputName("port_dense_shapes");
    domi::DynamicInputOutputInfo input1(kInput, "port_dense_shapes", 17, "dense_shapes", 12);
    value.push_back(input1);

    AttrUtils::SetInt(node_src->GetOpDesc(), "num_sparse", 0);
    op_desc_dst->AddRegisterInputName("port_num_sparse");
    domi::DynamicInputOutputInfo input2(kInput, "port_num_sparse", 15, "num_sparse", 10);
    value.push_back(input2);

    AttrUtils::SetFloat(node_src->GetOpDesc(), "attrFloat", 3.14);
    op_desc_dst->AddRegisterInputName("port_attrFloat");
    domi::DynamicInputOutputInfo input3(kInput, "port_attrFloat", 14, "attrFloat", 9);
    value.push_back(input3);

    vector<DataType> InListDataType = {DT_STRING, DT_INT32, DT_FLOAT};
    AttrUtils::SetListDataType(node_src->GetOpDesc(), "attrInListDataType", InListDataType);
    op_desc_dst->AddRegisterInputName("port_attrInListDataType");
    domi::DynamicInputOutputInfo input4(kInput, "port_attrInListDataType", 23, "attrInListDataType", 18);
    value.push_back(input4);

    auto ret = domi::AutoMappingByOpFnDynamic(op_src, op_dst, value);
    EXPECT_EQ(ret, domi::SUCCESS);
    EXPECT_EQ(op_dst.GetInputsSize(), 3);
}

TEST_F(AutoMappingUtils, CopyAttrValueInputListTest) {
    ut::GraphBuilder *builder = new ut::GraphBuilder("graph");
    NodePtr node_src = builder->AddNode("ParseSingleNode", "ParseSingleType", 6, 0, FORMAT_ALL);

    ge::Operator op_src = OpDescUtils::CreateOperatorFromNode(node_src);
    ge::Operator op_dst = ge::Operator("ParseSingleExample");
    std::shared_ptr<ge::OpDesc> op_desc_dst = ge::OpDescUtils::GetOpDescFromOperator(op_dst);
    std::vector<domi::DynamicInputOutputInfo> value;
    EXPECT_EQ(op_dst.GetInputsSize(), 0);

    vector<std::string> attrStrList = {"image/class/lable","image/encode", "image/format"};
    AttrUtils::SetListStr(node_src->GetOpDesc(), "dense_keys", attrStrList);
    op_desc_dst->AddRegisterInputName("port_dense_keys");
    domi::DynamicInputOutputInfo input1(kInput, "port_dense_keys", 15, "dense_keys", 10);
    value.push_back(input1);

    vector<int64_t> attrIntList = {0, 1, 2, 3};
    AttrUtils::SetListInt(node_src->GetOpDesc(), "attrIntList", attrIntList);
    op_desc_dst->AddRegisterInputName("port_attrIntList");
    domi::DynamicInputOutputInfo input2(kInput, "port_attrIntList", 16, "attrIntList", 11);
    value.push_back(input2);

    vector<float> attrFloatList = {0.0, 0.1, 0.2, 0.3};
    AttrUtils::SetListFloat(node_src->GetOpDesc(), "attrFloatList", attrFloatList);
    op_desc_dst->AddRegisterInputName("port_attrFloatList");
    domi::DynamicInputOutputInfo input3(kInput, "port_attrFloatList", 18, "attrFloatList", 13);
    value.push_back(input3);

    vector<bool> attrBoolList = {true, false, false, true};
    AttrUtils::SetListBool(node_src->GetOpDesc(), "attrBoolList", attrBoolList);
    op_desc_dst->AddRegisterInputName("port_attrBoolList");
    domi::DynamicInputOutputInfo input4(kInput, "port_attrBoolList", 17, "attrBoolList", 12);
    value.push_back(input4);

    NamedAttrs name1; NamedAttrs name2;
    vector<NamedAttrs> attrNamedAttrsList = {name1, name2};
    AttrUtils::SetListNamedAttrs(node_src->GetOpDesc(), "attrNamedAttrsList", attrNamedAttrsList);
    op_desc_dst->AddRegisterInputName("port_attrNamedAttrsList");
    domi::DynamicInputOutputInfo input5(kInput, "port_attrNamedAttrsList", 23, "attrNamedAttrsList", 18);
    value.push_back(input5);

    vector<vector<int64_t>> attrIntListList = {attrIntList, attrIntList};
    AttrUtils::SetListListInt(node_src->GetOpDesc(), "attrIntListList", attrIntListList);
    op_desc_dst->AddRegisterInputName("port_attrIntListList");
    domi::DynamicInputOutputInfo input6(kInput, "port_attrIntListList", 20, "attrIntListList", 15);
    value.push_back(input6);

    vector<DataType> InListDataType = {DT_STRING, DT_INT32, DT_FLOAT, DT_BOOL, DT_UNDEFINED, DT_UNDEFINED};
    AttrUtils::SetListDataType(node_src->GetOpDesc(), "attrInListDataType", InListDataType);
    op_desc_dst->AddRegisterInputName("port_attrInListDataType");
    domi::DynamicInputOutputInfo input7(kInput, "port_attrInListDataType", 23, "attrInListDataType", 18);
    value.push_back(input7);

    auto ret = domi::AutoMappingByOpFnDynamic(op_src, op_dst, value);
    EXPECT_EQ(ret, domi::SUCCESS);
    EXPECT_EQ(op_dst.GetInputsSize(), 6);
}

TEST_F(AutoMappingUtils, CopyAttrValueOutputTest) {
    ut::GraphBuilder *builder = new ut::GraphBuilder("graph");
    NodePtr node_src = builder->AddNode("ParseSingleNode", "ParseSingleType", 0, 4, FORMAT_ALL);

    ge::Operator op_src = OpDescUtils::CreateOperatorFromNode(node_src);
    ge::Operator op_dst = ge::Operator("ParseSingleExample");
    std::shared_ptr<ge::OpDesc> op_desc_dst = ge::OpDescUtils::GetOpDescFromOperator(op_dst);
    std::vector<domi::DynamicInputOutputInfo> value;
    EXPECT_EQ(op_dst.GetInputsSize(), 0);

    AttrUtils::SetBool(node_src->GetOpDesc(), "attrBool", true);
    op_desc_dst->AddRegisterOutputName("port_attrBool");
    domi::DynamicInputOutputInfo output1(kOutput, "port_attrBool", 13, "attrBool", 8);
    value.push_back(output1);

    NamedAttrs NamedAttr; NamedAttr.SetName("NamedAttr");
    AttrUtils::SetNamedAttrs(node_src->GetOpDesc(), "attrName", NamedAttr);
    op_desc_dst->AddRegisterOutputName("port_attrName");
    domi::DynamicInputOutputInfo output2(kOutput, "port_attrName", 13, "attrName", 8);
    value.push_back(output2);

    AttrUtils::SetDataType(node_src->GetOpDesc(), "attrDataType", DT_INT16);
    op_desc_dst->AddRegisterOutputName("port_attrDataType");
    domi::DynamicInputOutputInfo output3(domi::kOutput, "port_attrDataType", 17, "attrDataType", 12);
    value.push_back(output3);

    ComputeGraphPtr graph = builder->GetGraph();
    AttrUtils::SetGraph(node_src->GetOpDesc(), "attrGraph", graph);
    op_desc_dst->AddRegisterOutputName("port_attrGraph");
    domi::DynamicInputOutputInfo output4(kOutput, "port_attrGraph", 14, "attrGraph", 9);
    value.push_back(output4);

    vector<DataType> OutListDataType = {DT_BOOL, DT_STRING, DT_INT16, DT_RESOURCE};
    AttrUtils::SetListDataType(node_src->GetOpDesc(), "attrOutListDataType", OutListDataType);
    op_desc_dst->AddRegisterOutputName("port_attrOutListDataType");
    domi::DynamicInputOutputInfo output5(kOutput, "port_attrOutListDataType", 24, "attrOutListDataType", 19);
    value.push_back(output5);

    auto ret = domi::AutoMappingByOpFnDynamic(op_src, op_dst, value);
    EXPECT_EQ(ret, domi::SUCCESS);
    EXPECT_EQ(op_dst.GetOutputsSize(), 4);
}
