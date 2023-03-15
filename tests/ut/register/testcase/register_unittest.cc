/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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
#define private public
#define protected public
#include "external/register/scope/scope_fusion_pass_register.h"
#include "register/scope/scope_graph_impl.h"
#include "external/graph/operator.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/node_impl.h"
#include "graph/op_desc_impl.h"
#include "graph_builder_utils.h"
#include "graph/debug/ge_op_types.h"
#include "graph/compute_graph_impl.h"
#include "external/register/register.h"
#undef private
#undef protected

#include <gtest/gtest.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdlib.h>

#include "framework/common/debug/ge_log.h"
#include "register/op_registry.h"
#include "op_tiling/op_tiling_utils.h"
#include "register/op_tiling_registry.h"
#include "op_tiling/op_tiling_utils.h"
#include "op_tiling/op_tiling_constants.h"
#include "register/op_compile_info_base.h"
#include "op_tiling.h"
#include "register/op_check.h"
#include "register/tilingdata_base.h"

#include "graph/graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/attr_value.h"

#include "graph/debug/ge_util.h"
#include "graph/debug/ge_log.h"
#include "graph/debug/ge_attr_define.h"

#include "proto/tensorflow/attr_value.pb.h"
#include "proto/tensorflow/node_def.pb.h"
#include "exe_graph/runtime/kernel_run_context_builder.h"
#include "external/register/op_impl_registry.h"
#include "exe_graph/runtime/continuous_vector.h"
#include "common/util/tiling_utils.h"

using namespace domi;
using namespace ge;
using namespace optiling;

class CompileInfoJson : public CompileInfoBase {
 public:
  CompileInfoJson(const std::string &json) : json_str_(json) {}
  ~CompileInfoJson() {}

 private:
  std::string json_str_;
};

namespace {
struct StubCompileInfo : public CompileInfoBase {
  int64_t stub_ = 2;
};

void *CreateCompileInfo() {
  return new StubCompileInfo();
}

void DeleteCompileInfo(void *compile_info) {
  delete reinterpret_cast<StubCompileInfo *>(compile_info);
}

UINT32 OpTilingStubNew(gert::TilingContext *kernel_context) {
  auto tensor_without_data = kernel_context->GetInputTensor(1);
  EXPECT_EQ(tensor_without_data->GetAddr(), nullptr);
  EXPECT_EQ(tensor_without_data->GetStorageShape(), gert::Shape({5, 5, 5, 5}));
  EXPECT_EQ(tensor_without_data->GetOriginShape(), gert::Shape({5, 5, 5, 5}));
  auto tensor = kernel_context->GetInputTensor(0);
  EXPECT_EQ(tensor->GetShape().GetStorageShape().GetDimNum(), 4);
  gert::Shape expect_shape({4, 4, 4, 4});
  EXPECT_EQ(tensor->GetShape().GetStorageShape(), expect_shape);
  EXPECT_EQ(tensor->GetDataType(), DT_INT8);
  EXPECT_EQ((tensor->GetData<int8_t>())[3], 4);
  EXPECT_EQ((tensor->GetData<int8_t>())[2], 3);
  EXPECT_EQ((tensor->GetData<int8_t>())[1], 2);
  EXPECT_EQ((tensor->GetData<int8_t>())[0], 1);
  EXPECT_EQ(tensor->GetFormat().GetStorageFormat(), FORMAT_ND);
  gert::Shape expect_shape2({9, 9, 9, 9});
  EXPECT_TRUE(kernel_context->GetOutputShape(0)->GetStorageShape() == expect_shape2);
  auto shape = kernel_context->GetInputShape(1);
  EXPECT_TRUE(*shape == gert::StorageShape({5, 5, 5, 5}, {5, 5, 5, 5}));
  auto ci = kernel_context->GetCompileInfo();
  EXPECT_EQ(reinterpret_cast<const StubCompileInfo *>(ci)->stub_, 1);

  EXPECT_EQ(kernel_context->GetAttrs()->GetAttrNum(), 4);
  std::vector<int64_t> expect_attr = {1, 2, 3, 4};
  for (size_t i = 0UL; i < 4UL; ++i) {
    EXPECT_EQ(reinterpret_cast<const int64_t *>(
                  kernel_context->GetAttrs()->GetAttrPointer<gert::ContinuousVector>(0)->GetData())[i],
              expect_attr[i]);
  }
  EXPECT_EQ(*kernel_context->GetAttrs()->GetAttrPointer<int8_t>(1), 99);
  kernel_context->SetBlockDim(2);
  kernel_context->SetNeedAtomic(true);
  kernel_context->SetTilingKey(78);
  *kernel_context->GetWorkspaceSizes(1) = 12;
  kernel_context->GetRawTilingData()->Append<uint8_t>(6);
  kernel_context->GetRawTilingData()->Append<uint8_t>(7);
  kernel_context->GetRawTilingData()->Append<uint8_t>(8);
  kernel_context->GetRawTilingData()->Append<uint8_t>(9);
  kernel_context->GetRawTilingData()->Append<uint8_t>(10);
  return ge::GRAPH_SUCCESS;
}

UINT32 OpTilingParseStubNew(gert::KernelContext *kernel_context) {
  auto ci = kernel_context->GetOutputPointer<StubCompileInfo>(0);
  ci->stub_ = 1;
  return ge::GRAPH_SUCCESS;
}

UINT32 OpTilingStubV5(gert::TilingContext *kernel_context) {
  auto tensor = kernel_context->GetInputTensor(0);
  std::vector<float> real_data = {1.1, 2.1, 3.1, 4.1};
  for (size_t i = 0UL; i < 4UL; ++i) {
    EXPECT_EQ((tensor->GetData<uint16_t>())[i], optiling::FloatToUint16(real_data[i]));
  }
  return ge::GRAPH_SUCCESS;
}

UINT32 DefaultOptilingStub(gert::TilingContext *kernel_context) {
  return ge::GRAPH_SUCCESS;
}

UINT32 OpTilingParseStubV5(gert::KernelContext *kernel_context) {
  auto av = kernel_context->GetOutput(0);
  av->Set(CreateCompileInfo(), DeleteCompileInfo);
  return ge::GRAPH_SUCCESS;
}

bool op_tiling_stub_failed(const Operator &op, const utils::OpCompileInfo &compile_info, utils::OpRunInfo &run_info) {
  EXPECT_EQ(true, false);
  return true;
}
}  // namespace

class UtestRegister : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

extern "C" int TbeOpTilingPyInterfaceEx2(const char *optype, const char *compile_info, const char *inputs,
                                         const char *outputs, char *run_info_json, size_t run_info_len,
                                         const char *compile_info_hash, uint64_t *elapse);

extern "C" int TbeOpTilingPyInterface(const char *optype, const char *compile_info, const char *compile_info_hash,
                                      const char *inputs, const char *outputs, const char *attrs, char *run_info_json,
                                      size_t run_info_len, uint64_t *elapse);

bool op_tiling_stub_v2(const Operator &op, const utils::OpCompileInfo &compile_info, utils::OpRunInfo &run_info) {
  return true;
}

bool op_tiling_stub_v3(const Operator &op, const void *value, OpRunInfoV2 &run_info) {
  return true;
}

void *op_parse_stub_v3(const Operator &op, const ge::AscendString &compile_info_json) {
  //  static void *p = new int(3);
  static int x = 1024;
  void *p = &x;
  return p;
}

bool op_tiling_stub_v4(const Operator &op, const CompileInfoPtr value, OpRunInfoV2 &run_info) {
  return true;
}

CompileInfoPtr op_parse_stub_v4(const Operator &op, const ge::AscendString &compile_info_json) {
  //  static void *p = new int(3);
  CompileInfoPtr info = std::make_shared<CompileInfoJson>("qwer");
  return info;
}

REGISTER_OP_TILING_V2(ReluV2, op_tiling_stub_v2);
REGISTER_OP_TILING_V3(ReluV3, op_tiling_stub_v3, op_parse_stub_v3);
REGISTER_OP_TILING_V4(ReluV4, op_tiling_stub_v4, op_parse_stub_v4);

TEST_F(UtestRegister, test_register_dynamic_outputs_op_only_has_partial_output) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto node_src = builder.AddNode("ParseSingleExample", "ParseSingleExample",
                                  {"serialized", "dense_defaults_0", "dense_defaults_1", "dense_defaults_2"},
                                  {"dense_values_0", "dense_values_1", "dense_values_2"});
  // build op_src attrs
  vector<string> dense_keys = {"image/class/lable", "image/encode", "image/format"};
  vector<DataType> t_dense = {DT_INT64, DT_STRING, DT_STRING};
  AttrUtils::SetListStr(node_src->GetOpDesc(), "dense_keys", dense_keys);
  AttrUtils::SetListStr(node_src->GetOpDesc(), "dense_shapes", {});
  AttrUtils::SetInt(node_src->GetOpDesc(), "num_sparse", 0);
  AttrUtils::SetListStr(node_src->GetOpDesc(), "sparse_keys", {});
  AttrUtils::SetListStr(node_src->GetOpDesc(), "sparse_types", {});
  AttrUtils::SetListDataType(node_src->GetOpDesc(), "Tdense", t_dense);
  auto graph = builder.GetGraph();

  // get op_src
  ge::Operator op_src = OpDescUtils::CreateOperatorFromNode(node_src);
  ge::Operator op_dst = ge::Operator("ParseSingleExample");
  std::shared_ptr<ge::OpDesc> op_desc_dst = ge::OpDescUtils::GetOpDescFromOperator(op_dst);
  op_desc_dst->AddRegisterInputName("dense_defaults");
  op_desc_dst->AddRegisterOutputName("sparse_indices");
  op_desc_dst->AddRegisterOutputName("sparse_values");
  op_desc_dst->AddRegisterOutputName("sparse_shapes");
  op_desc_dst->AddRegisterOutputName("dense_values");

  // simulate parse_single_example plugin
  std::vector<DynamicInputOutputInfo> value;
  DynamicInputOutputInfo input(kInput, "dense_defaults", 14, "Tdense", 6);
  value.push_back(input);
  DynamicInputOutputInfo output(kOutput, "sparse_indices", 14, "num_sparse", 10);
  value.push_back(output);
  DynamicInputOutputInfo output1(kOutput, "sparse_values", 13, "sparse_types", 12);
  value.push_back(output1);
  DynamicInputOutputInfo output2(kOutput, "sparse_shapes", 13, "num_sparse", 10);
  value.push_back(output2);
  DynamicInputOutputInfo output3(kOutput, "dense_values", 12, "Tdense", 6);
  value.push_back(output3);
  DynamicInputOutputInfo invalidput(kInvalid, "Invalid", 7, "Invalid", 7);
  value.push_back(invalidput);

  // pre_check
  EXPECT_EQ(op_dst.GetOutputsSize(), 0);
  auto ret = AutoMappingByOpFnDynamic(op_src, op_dst, value);

  // check add 3 output to op_dst
  EXPECT_EQ(ret, domi::SUCCESS);
  EXPECT_EQ(op_dst.GetOutputsSize(), 3);

  // for AutoMappingByOpFnDynamic failed test
  ge::Operator op_src_fail(nullptr);
  ret = AutoMappingByOpFnDynamic(op_src_fail, op_dst, value);
  EXPECT_EQ(ret, domi::FAILED);

  std::vector<DynamicInputOutputInfo> value_fail;
  ret = AutoMappingByOpFnDynamic(op_src, op_dst, value_fail);
  DynamicInputOutputInfo input_fail(kInput, "", 0, "", 0);
  value_fail.push_back(input_fail);
  ret = AutoMappingByOpFnDynamic(op_src, op_dst, value_fail);
  EXPECT_EQ(ret, domi::FAILED);
}

void GraphInit(domi::tensorflow::GraphDef &graph_def) {
  // add node, set info
  domi::tensorflow::NodeDef *placeholder0 = graph_def.add_node();
  placeholder0->set_name("placeholder0");
  placeholder0->set_op("PlaceHolder");

  // add node, set info, add edges
  domi::tensorflow::NodeDef *add0 = graph_def.add_node();
  add0->set_name("add0");
  add0->set_op("Add");
  add0->add_input("placeholder0");
  add0->add_input("placeholder1");

  // 1. add node
  auto placeholder1 = graph_def.add_node();
  auto add1 = graph_def.add_node();
  auto mul0 = graph_def.add_node();
  auto mul1 = graph_def.add_node();
  auto add2 = graph_def.add_node();
  auto retval0 = graph_def.add_node();
  auto retval1 = graph_def.add_node();

  // 2. set info
  placeholder1->set_name("placeholder1");
  placeholder1->set_op("PlaceHolder");
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

int32_t AutoMappingSubgraphIndexInput(int32_t data_index) {
  return 0;
}
int32_t AutoMappingSubgraphIndexOutput(int32_t netoutput_index) {
  return 0;
}
Status AutoMappingSubgraphIndexInput2(int32_t data_index, int32_t &parent_input_index) {
  return domi::SUCCESS;
}
Status AutoMappingSubgraphIndexOutput2(int32_t netoutput_index, int32_t &parent_output_index) {
  parent_output_index++;
  return domi::SUCCESS;
}
Status AutoMappingSubgraphIndexOutput2Failed(int32_t netoutput_index, int32_t &parent_output_index) {
  return domi::FAILED;
}

TEST_F(UtestRegister, AutoMappingSubgraphIndex) {
  Status stat;
  auto builder = ut::GraphBuilder("root");
  auto output = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  auto input = builder.AddNode("data", DATA, 1, 1);
  input->impl_->op_->impl_->meta_data_.type_ = "Data";
  auto func_node = builder.AddNode("func_node", FRAMEWORKOP, 1, 1);
  func_node->impl_->op_->impl_->meta_data_.type_ = "FrameworkOp";
  builder.AddDataEdge(input, 0, func_node, 0);
  builder.AddDataEdge(func_node, 0, output, 0);

  auto computeGraph = builder.GetGraph();
  Graph graph = GraphUtilsEx::CreateGraphFromComputeGraph(computeGraph);
  stat = AutoMappingSubgraphIndex(graph, AutoMappingSubgraphIndexInput, AutoMappingSubgraphIndexOutput);
  EXPECT_EQ(stat, domi::FAILED);
}

TEST_F(UtestRegister, AutoMappingSubgraphIndexByDataNode) {
  Status stat;
  auto builder = ut::GraphBuilder("root");
  auto output = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  auto func_node = builder.AddNode("func_node", PARTITIONEDCALL, 1, 1);
  builder.AddDataEdge(func_node, 0, output, 0);

  auto computeGraph = builder.GetGraph();
  Graph graph = GraphUtilsEx::CreateGraphFromComputeGraph(computeGraph);
  stat = AutoMappingSubgraphIndex(graph, AutoMappingSubgraphIndexInput2, AutoMappingSubgraphIndexOutput2);
  EXPECT_EQ(stat, domi::SUCCESS);

  auto input = builder.AddNode("Retval", DATA, 1, 1);
  input->impl_->op_->impl_->meta_data_.type_ = "_Retval";
  AttrUtils::SetInt(input->GetOpDesc(), "retval_index", 0);
  builder.AddDataEdge(input, 0, func_node, 0);
  stat = AutoMappingSubgraphIndex(graph, AutoMappingSubgraphIndexInput2, AutoMappingSubgraphIndexOutput2);
  EXPECT_EQ(stat, domi::SUCCESS);
}

TEST_F(UtestRegister, AutoMappingSubgraphIndexByDataNode2) {
  Status stat;
  auto builder = ut::GraphBuilder("root");
  auto input = builder.AddNode("index", DATA, 1, 1);
  input->impl_->op_->impl_->meta_data_.type_ = "Data";
  AttrUtils::SetInt(input->GetOpDesc(), "index", 0);
  auto output = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  auto func_node = builder.AddNode("func_node", PARTITIONEDCALL, 1, 1);
  builder.AddDataEdge(input, 0, func_node, 0);
  builder.AddDataEdge(func_node, 0, output, 0);

  auto computeGraph = builder.GetGraph();
  Graph graph = GraphUtilsEx::CreateGraphFromComputeGraph(computeGraph);
  stat = AutoMappingSubgraphIndex(graph, AutoMappingSubgraphIndexInput2, AutoMappingSubgraphIndexOutput2);
  EXPECT_EQ(stat, domi::SUCCESS);
}

TEST_F(UtestRegister, AutoMappingSubgraphOutputFail) {
  Status stat;
  auto builder = ut::GraphBuilder("root");
  auto output = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  auto input = builder.AddNode("data", DATA, 1, 1);
  input->impl_->op_->impl_->meta_data_.type_ = "Data";
  auto func_node = builder.AddNode("func_node", FRAMEWORKOP, 1, 1);
  func_node->impl_->op_->impl_->meta_data_.type_ = "FrameworkOp";
  builder.AddDataEdge(input, 0, func_node, 0);
  builder.AddDataEdge(func_node, 0, output, 0);

  auto computeGraph = builder.GetGraph();
  Graph graph = GraphUtilsEx::CreateGraphFromComputeGraph(computeGraph);

  stat = AutoMappingSubgraphIndex(graph, AutoMappingSubgraphIndexInput2, AutoMappingSubgraphIndexOutput2Failed);
  EXPECT_EQ(stat, domi::FAILED);
}

TEST_F(UtestRegister, AutoMappingFnDynamicInputTest) {
  Status stat;
  domi::tensorflow::GraphDef graph_def;
  GraphInit(graph_def);
  map<std::string, std::pair<std::string, std::string>> name_attr_value;
  name_attr_value.insert(make_pair(std::string("in"), make_pair(std::string("dynamicName1"), std::string("int"))));
  name_attr_value.insert(make_pair(std::string("out"), make_pair(std::string("dynamicName2"), std::string("float"))));

  ge::Operator op_dst = ge::Operator("Add", "int");
  const domi::tensorflow::NodeDef *node;

  int32_t node_size = graph_def.node_size();
  for (int i = 0; i < node_size; i++) {
    node = graph_def.mutable_node(i);
    stat = AutoMappingFnDynamic(node, op_dst, name_attr_value, 1, 1);
    EXPECT_EQ(stat, domi::SUCCESS);
  }
}

TEST_F(UtestRegister, AutoMappingFnDynamicInput) {
  Status retStat;
  domi::tensorflow::GraphDef graph_def;
  GraphInit(graph_def);

  ge::Operator op_dst = ge::Operator("Add", "int");
  domi::tensorflow::NodeDef *node = graph_def.mutable_node(0);

  // test for add attrs
  map<std::string, std::pair<std::string, std::string>> name_attrs;
  domi::tensorflow::AttrValue inValue;
  inValue.set_s(std::string("stringValue"));
  inValue.set_i(66);
  node->mutable_attr()->insert({"inVal", inValue});
  name_attrs.insert(make_pair(std::string("in"), make_pair(std::string("inName1"), std::string("inVal"))));
  retStat = AutoMappingFnDynamic(node, op_dst, name_attrs, 1, 1);
  EXPECT_EQ(retStat, domi::SUCCESS);
}

TEST_F(UtestRegister, AutoMappingFnDynamicOutput) {
  Status retStat;
  domi::tensorflow::GraphDef graph_def;
  GraphInit(graph_def);

  ge::Operator op_dst = ge::Operator("Add", "int");
  domi::tensorflow::NodeDef *node = graph_def.mutable_node(0);

  // test for add attrs
  map<std::string, std::pair<std::string, std::string>> name_attrs;
  domi::tensorflow::AttrValue outValue;
  outValue.set_b(true);
  outValue.set_i(88);
  node->mutable_attr()->insert({"outVal", outValue});
  name_attrs.insert(make_pair(std::string("out"), make_pair(std::string("outName1"), std::string("outVal"))));
  retStat = AutoMappingFnDynamic(node, op_dst, name_attrs, 1, 1);
  EXPECT_EQ(retStat, domi::SUCCESS);
}

TEST_F(UtestRegister, AutoMappingFunctionkFunc) {
  Status retStat;
  domi::tensorflow::GraphDef graph_def;
  GraphInit(graph_def);

  ge::Operator op_dst = ge::Operator("Add", "int");
  op_dst.SubgraphRegister("subVal", true);
  op_dst.SubgraphCountRegister("subVal", 6);

  // test for add attrs
  domi::tensorflow::NodeDef *node = graph_def.mutable_node(0);
  map<std::string, std::pair<std::string, std::string>> name_attrs;
  domi::tensorflow::AttrValue attrValue;
  attrValue.set_i(88);
  domi::tensorflow::NameAttrList *nameAttrList = new domi::tensorflow::NameAttrList();
  nameAttrList->set_name("nameAttrList");
  attrValue.unsafe_arena_set_allocated_func(nameAttrList);
  node->mutable_attr()->insert({"subVal", attrValue});
  name_attrs.insert(make_pair(std::string("out"), make_pair(std::string("outName1"), std::string("subVal"))));
  retStat = AutoMappingFnDynamic(node, op_dst, name_attrs, 1, 1);
  EXPECT_EQ(retStat, domi::FAILED);
}

TEST_F(UtestRegister, AutoMappingFunctionkList) {
  Status retStat;
  domi::tensorflow::GraphDef graph_def;
  GraphInit(graph_def);

  ge::Operator op_dst = ge::Operator("Add", "int");
  op_dst.SubgraphRegister("subVal", true);

  // test for add attrs
  domi::tensorflow::NodeDef *node = graph_def.mutable_node(0);
  map<std::string, std::pair<std::string, std::string>> name_attrs;
  domi::tensorflow::AttrValue attrValue;
  attrValue.set_i(88);
  domi::tensorflow::AttrValue_ListValue *attrValListVal = new domi::tensorflow::AttrValue_ListValue();
  attrValListVal->add_s("list0");
  attrValListVal->add_s("list1");
  attrValue.unsafe_arena_set_allocated_list(attrValListVal);
  // list.func
  domi::tensorflow::NameAttrList *nameAttrList = new domi::tensorflow::NameAttrList();
  nameAttrList->set_name("nameAttrList");
  attrValListVal->add_func();

  node->mutable_attr()->insert({"subVal", attrValue});
  name_attrs.insert(make_pair(std::string("out"), make_pair(std::string("outName1"), std::string("subVal"))));
  retStat = AutoMappingFnDynamic(node, op_dst, name_attrs, 1, 1);
  EXPECT_EQ(retStat, domi::SUCCESS);
}

domi::Status inputFunc(int32_t data_index, int32_t &parent_input_index) {
  parent_input_index++;
  return (parent_input_index < 0) ? domi::FAILED : domi::SUCCESS;
}

domi::Status outputFunc(int32_t netoutput_index, int32_t &parent_output_index) {
  parent_output_index++;
  return (parent_output_index < 2) ? domi::FAILED : domi::SUCCESS;
}

domi::Status AutoMappingSubgraphIOIndexFuncCB(
    const ge::Graph &graph, const std::function<Status(int32_t data_index, int32_t &parent_input_index)> &input,
    const std::function<Status(int32_t netoutput_index, int32_t &parent_output_index)> &output) {
  static int test_idx = -2;

  switch (test_idx) {
    case -2:
      return input(0, test_idx);
    case -1:
      return input(0, test_idx);
    case 0:
      return output(0, test_idx);
    case 1:
      return output(0, test_idx);
  }
  return domi::SUCCESS;
}

TEST_F(UtestRegister, FrameworkRegistryTest) {
  REGISTER_AUTOMAPPING_SUBGRAPH_IO_INDEX_FUNC(TENSORFLOW, AutoMappingSubgraphIOIndexFuncCB);

  FrameworkRegistry &cur = FrameworkRegistry::Instance();
  cur.AddAutoMappingSubgraphIOIndexFunc(domi::CAFFE, AutoMappingSubgraphIOIndexFuncCB);

  const ge::Graph graph("graph_test");
  AutoMappingSubgraphIOIndexFunc func = cur.GetAutoMappingSubgraphIOIndexFunc(domi::CAFFE);
  EXPECT_EQ(func(graph, inputFunc, outputFunc), domi::FAILED);
  EXPECT_EQ(func(graph, inputFunc, outputFunc), domi::SUCCESS);
  EXPECT_EQ(func(graph, inputFunc, outputFunc), domi::FAILED);
  EXPECT_EQ(func(graph, inputFunc, outputFunc), domi::SUCCESS);
}

TEST_F(UtestRegister, OpRegistrationDataWithNoImpl) {
  OpRegistrationData opRegData(std::string("OmOptype"));
  opRegData.impl_.reset();

  EXPECT_EQ(opRegData.GetOmOptype() == "", true);
  EXPECT_EQ(opRegData.GetFrameworkType(), domi::FRAMEWORK_RESERVED);
  EXPECT_EQ(opRegData.GetOriginOpTypeSet().empty(), true);
  EXPECT_EQ(opRegData.GetParseParamFn(), nullptr);
  EXPECT_EQ(opRegData.GetParseParamByOperatorFn(), nullptr);
  EXPECT_EQ(opRegData.GetFusionParseParamFn(), nullptr);
  EXPECT_EQ(opRegData.GetFusionParseParamByOpFn(), nullptr);
  EXPECT_EQ(opRegData.GetImplyType(), domi::ImplyType::BUILDIN);
  EXPECT_EQ(opRegData.GetParseSubgraphPostFn(), nullptr);
  EXPECT_EQ(opRegData.GetParseOpToGraphFn(), nullptr);
  ParseSubgraphFuncV2 func;
  EXPECT_EQ(opRegData.GetParseSubgraphPostFn(func), domi::FAILED);
}

TEST_F(UtestRegister, OmOptypeTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));
  OpReceiver oprcver(opRegData);
  opRegData.GetOmOptype();

  AscendString OmOptype;
  Status stat = opRegData.GetOmOptype(OmOptype);
  EXPECT_EQ(stat, domi::SUCCESS);
}

TEST_F(UtestRegister, FrameworkTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));

  opRegData.FrameworkType(domi::MINDSPORE);
  EXPECT_EQ(opRegData.GetFrameworkType(), domi::MINDSPORE);
}

TEST_F(UtestRegister, OriOpTypeTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));
  OpRegistrationData opRegData2("OmOptype2");

  std::initializer_list<std::string> OptypeList1{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList1);
  std::vector<AscendString> OptypeList2 = {AscendString("Div"), AscendString("Mul")};
  opRegData.OriginOpType(OptypeList2);

  opRegData2.OriginOpType(std::string("Add"));
  opRegData2.OriginOpType("Sub");

  opRegData.GetOriginOpTypeSet();
  std::set<ge::AscendString> opTypeSet;
  Status stat = opRegData.GetOriginOpTypeSet(opTypeSet);
  EXPECT_EQ(stat, domi::SUCCESS);
}

TEST_F(UtestRegister, OpRegistryImplyTypeTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));

  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);
  std::vector<AscendString> OptypeList2 = {AscendString("Div"), AscendString("Mul")};
  opRegData.OriginOpType(OptypeList2);

  // set ImplyType
  opRegData.ImplyType(domi::ImplyType::CUSTOM);
  EXPECT_EQ(opRegData.GetImplyType(), domi::ImplyType::CUSTOM);

  OpRegistry *opReg = OpRegistry::Instance();
  opReg->Register(opRegData);

  domi::ImplyType implType = opReg->GetImplyTypeByOriOpType(std::string("Add"));
  EXPECT_EQ(implType, domi::ImplyType::CUSTOM);

  implType = opReg->GetImplyType(std::string("OmOptype"));
  EXPECT_EQ(implType, domi::ImplyType::CUSTOM);
  implType = opReg->GetImplyType(std::string("strOmOptype"));
  EXPECT_EQ(implType, domi::ImplyType::BUILDIN);

  vector<std::string> vecOpType;
  vecOpType.clear();
  opReg->GetOpTypeByImplyType(vecOpType, domi::ImplyType::CUSTOM);
  EXPECT_EQ(vecOpType.empty(), false);
  vecOpType.clear();
  opReg->GetOpTypeByImplyType(vecOpType, domi::ImplyType::AI_CPU);
  EXPECT_EQ(vecOpType.empty(), true);
}

TEST_F(UtestRegister, DelInputWithTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));
  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);

  opRegData.ParseParamsFn(domi::AutoMappingFn);
  EXPECT_NE(opRegData.GetParseParamFn(), nullptr);

  // insert input into vector
  const vector<int> input_order{0, 1, 3, 2};
  opRegData.InputReorderVector(input_order);

  opRegData.DelInputWithCond(1, std::string("attrName_1"), true);
  opRegData.DelInputWithCond(2, "attrName_2", false);

  opRegData.DelInputWithOriginalType(3, std::string("Add"));
  opRegData.DelInputWithOriginalType(4, "Sub");

  OpRegistry *opReg = OpRegistry::Instance();
  ASSERT_NE(opReg, nullptr);
  bool retBool = opReg->Register(opRegData);
  ASSERT_EQ(retBool, true);

  std::vector<RemoveInputConfigure> rmConfigVec;
  rmConfigVec = opReg->GetRemoveInputConfigure(std::string("Add"));
  EXPECT_EQ(rmConfigVec.empty(), true);
  rmConfigVec = opReg->GetRemoveInputConfigure(std::string("Mul"));
  EXPECT_EQ(rmConfigVec.empty(), true);
  rmConfigVec = opReg->GetRemoveInputConfigure(std::string("Mul666"));
  EXPECT_EQ(rmConfigVec.empty(), true);
}

TEST_F(UtestRegister, GetOmTypeByOriOpTypeTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));

  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);

  OpRegistry *opReg = OpRegistry::Instance();
  opReg->Register(opRegData);
  std::string om_type;
  EXPECT_EQ(opReg->GetOmTypeByOriOpType(std::string("Sub"), om_type), true);
  EXPECT_EQ(opReg->GetOmTypeByOriOpType(std::string("Sub1"), om_type), false);
}

domi::Status FusionParseParamsFnCB(const std::vector<const google::protobuf::Message *> Msg, ge::Operator &Op) {
  return domi::SUCCESS;
}
domi::Status FusionParseParamsFnCB2(const std::vector<ge::Operator> &VecOp, ge::Operator &Op) {
  return domi::FAILED;
}
domi::Status ParseSubgraphPostFnCB(const std::string &subgraph_name, const ge::Graph &graph) {
  return domi::SUCCESS;
}
domi::Status ParseSubgraphPostFnCB2(const ge::AscendString &subgraph_name, const ge::Graph &graph) {
  return domi::SUCCESS;
}
domi::Status ParseOpToGraphFnCB(const ge::Operator &Op, ge::Graph &Graph) {
  return domi::SUCCESS;
}

TEST_F(UtestRegister, ParseParamFuncTest) {
  const std::string strOmOptype = "OmOptype";
  OpRegistrationData opRegData(strOmOptype);

  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);
  std::vector<AscendString> OptypeListAStr = {AscendString("Div"), AscendString("Mul")};
  opRegData.OriginOpType(OptypeListAStr);

  opRegData.ParseParamsFn(domi::AutoMappingFn);
  EXPECT_NE(opRegData.GetParseParamFn(), nullptr);

  OpRegistry *opReg = OpRegistry::Instance();
  opReg->Register(opRegData);

  EXPECT_EQ(opReg->GetParseParamFunc(std::string("OmOptype1"), std::string("Sub")), nullptr);
  EXPECT_EQ(opReg->GetParseParamFunc(std::string("OmOptype"), std::string("Sub")), nullptr);
}

TEST_F(UtestRegister, FusionParseParamFuncTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));

  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);

  opRegData.FusionParseParamsFn(FusionParseParamsFnCB);
  EXPECT_NE(opRegData.GetFusionParseParamFn(), nullptr);

  OpRegistry *opReg = OpRegistry::Instance();
  opReg->Register(opRegData);

  EXPECT_EQ(opReg->GetFusionParseParamFunc(std::string("OmOptype"), std::string("Sub")), nullptr);
  EXPECT_EQ(opReg->GetFusionParseParamFunc(std::string("OmOptype1"), std::string("Sub")), nullptr);
}

TEST_F(UtestRegister, GetParseOpToGraphFuncTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));

  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);

  opRegData.ParseOpToGraphFn(ParseOpToGraphFnCB);
  EXPECT_NE(opRegData.GetParseOpToGraphFn(), nullptr);

  OpRegistry *opReg = OpRegistry::Instance();
  opReg->Register(opRegData);
  std::string om_type;

  EXPECT_EQ(opReg->GetParseOpToGraphFunc(std::string("OmOptype"), std::string("Add")), nullptr);
  EXPECT_EQ(opReg->GetParseOpToGraphFunc(std::string("OmOptype"), std::string("Mul")), nullptr);
}

TEST_F(UtestRegister, ParseParamByOperatorFuncTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));

  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);

  opRegData.ParseParamsByOperatorFn(domi::AutoMappingByOpFn);
  EXPECT_NE(opRegData.GetParseParamByOperatorFn(), nullptr);

  OpRegistry *opReg = OpRegistry::Instance();
  opReg->Register(opRegData);

  EXPECT_EQ(opReg->GetParseParamByOperatorFunc(std::string("int")), nullptr);
  EXPECT_EQ(opReg->GetParseParamByOperatorFunc(std::string("Add")), nullptr);
}

TEST_F(UtestRegister, FusionParseParamByOpFuncTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));

  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);

  opRegData.FusionParseParamsFn(FusionParseParamsFnCB);
  EXPECT_NE(opRegData.GetFusionParseParamFn(), nullptr);

  opRegData.FusionParseParamsFn(FusionParseParamsFnCB2);
  EXPECT_NE(opRegData.GetFusionParseParamByOpFn(), nullptr);

  OpRegistry *opReg = OpRegistry::Instance();
  opReg->Register(opRegData);

  EXPECT_EQ(opReg->GetFusionParseParamByOpFunc(std::string("strOmOptype"), std::string("Add")), nullptr);
  EXPECT_EQ(opReg->GetFusionParseParamByOpFunc(std::string("OmOptype"), std::string("Add")), nullptr);
}

TEST_F(UtestRegister, ParseSubgraphPostFnTest) {
  OpRegistrationData opRegData(std::string("OmOptype"));

  std::initializer_list<std::string> OptypeList{std::string("Add"), std::string("Sub")};
  opRegData.OriginOpType(OptypeList);

  opRegData.ParseSubgraphPostFn(ParseSubgraphPostFnCB);
  EXPECT_NE(opRegData.GetParseSubgraphPostFn(), nullptr);

  opRegData.ParseSubgraphPostFn(ParseSubgraphPostFnCB2);
  EXPECT_NE(opRegData.GetParseSubgraphPostFn(), nullptr);

  ParseSubgraphFuncV2 Getfunc;
  opRegData.GetParseSubgraphPostFn(Getfunc);

  OpRegistry *opReg = OpRegistry::Instance();
  opReg->Register(opRegData);

  EXPECT_EQ(opReg->GetParseSubgraphPostFunc(std::string("strOmOptype")), nullptr);
  EXPECT_EQ(opReg->GetParseSubgraphPostFunc(std::string("OmOptype")), nullptr);

  domi::ParseSubgraphFuncV2 parse_subgraph_func;
  EXPECT_EQ(opReg->GetParseSubgraphPostFunc(std::string("OmOptype"), parse_subgraph_func), domi::SUCCESS);
  EXPECT_EQ(opReg->GetParseSubgraphPostFunc(std::string("strOmOptype"), parse_subgraph_func), domi::FAILED);
}

TEST_F(UtestRegister, optiling_py_interface) {
  const nlohmann::json j = R"([
      {
          "name": "test_0",
          "dtype": "int8",
          "value": 1,
          "const_value": [
            1,
            1,
            1,
            1
          ],
          "shape": [
            4,
            4,
            4,
            4
          ],
          "format": "ND"
      },
      {
          "name": "test_1",
          "dtype": "list_int",
          "value": [
            1,
            1,
            1,
            1
          ]
      },
      {
          "name": "test_2"
      },
      {
          "name": "test_2",
          "dtype": "list_list_int",
          "value": [
            [1, 2],
            [1, 2],
            [1, 2],
            [1, 2]
          ]
      },
      {
          "name": "test_0",
          "dtype": "list_list_int64",
          "value": [
            [1, 2],
            [1, 2],
            [1, 2],
            [1, 2]
          ]
      },
      {
          "name": "test_3",
          "dtype": "test",
          "value": "1"
      }
      ])"_json;

  std::string json_str = j.dump();
  ge::Operator op("NULL");
  const char *optype = "ReluV2";
  const char *optype_v3 = "ReluV3";
  const char *optype_v4 = "ReluV4";
  const char *cmp_info = "{\"_common_info\":[0,16,48,1,1,0,0],\"_is_ori_last_transpose\":0,\"_pattern\":\"Transdata\","
                         "\"_permute\":[0,2,1,3],\"_sgt_cube_vector_core_type\":\"VectorCore\",\"_src_fuse\":[0,1,3],"
                         "\"_src_pad_mode\":[0,0,2],\"_src_pad_var\":[1,1,16],\"_ub_info"
                         "\":[[48512,24192],[-1],[-1],[-1]],\"device_id\":\"0\"}";
  const char *inputs = "";
  const char *outputs = "";
  char *runinfo = "";
  size_t size = 3;
  const char *cmp_info_hash = "";
  uint64_t *elapse = nullptr;
  const char *attrs = json_str.c_str();
  TbeOpTilingPyInterface(optype, cmp_info, cmp_info_hash, attrs, attrs, attrs, runinfo, size, elapse);
  TbeOpTilingPyInterface(optype_v3, cmp_info, cmp_info_hash, attrs, attrs, attrs, runinfo, size, elapse);
  TbeOpTilingPyInterface(optype_v4, cmp_info, cmp_info_hash, attrs, attrs, attrs, runinfo, size, elapse);
  TbeOpTilingPyInterfaceEx2(optype, cmp_info, attrs, attrs, runinfo, size, cmp_info_hash, elapse);
  TbeOpTilingPyInterfaceEx2(optype_v3, cmp_info, attrs, attrs, runinfo, size, cmp_info_hash, elapse);
  TbeOpTilingPyInterfaceEx2(optype_v4, cmp_info, attrs, attrs, runinfo, size, cmp_info_hash, elapse);
}

TEST_F(UtestRegister, new_optiling_py_interface_ok) {
  const nlohmann::json input = R"([
{"name": "test_0","dtype": "int8", "const_value": [1,2,3,4],"shape": [4,4,4,4],"format": "ND"},
{"name": "test_1","dtype": "int32","shape": [5,5,5,5],"ori_shape": [5,5,5,5],"format": "ND","ori_format": "ND"},
{"name": "test_2","dtype": "int32","shape": [6,6,6,6],"ori_shape": [6,6,6,6],"format": "ND","ori_format": "ND"}])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "int8","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;

  std::string output_str = output.dump();
  const nlohmann::json attrs = R"([
{ "name": "attr_0","dtype": "list_int64","value": [1,2, 3, 4]},
{ "name": "attr_1","dtype": "int","value": 99},
{ "name": "attr_2","dtype": "list_int32","value": [1, 2, 3, 4]},
{ "name": "op_para_size", "dtype": "int", "value": 50}])"_json;
  std::string attrs_str = attrs.dump();
  const char *op_type = "TestReluV2";
  const char *cmp_info = "";
  std::string runinfo(100, 'a');
  size_t size = 100;
  const char *cmp_info_hash = "";
  uint64_t *elapse = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling = OpTilingStubNew;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling_parse = OpTilingParseStubNew;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_creator = CreateCompileInfo;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_deleter = DeleteCompileInfo;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).max_tiling_data_size = 50;
  EXPECT_EQ(TbeOpTilingPyInterface(op_type, cmp_info, cmp_info_hash, input_str.c_str(), output_str.c_str(),
                                   attrs_str.c_str(), const_cast<char *>(runinfo.c_str()), size, elapse),
            1);
  std::string result =
      "{\"block_dim\":2,\"clear_atomic\":true,\"tiling_data\":\"060708090A\",\"tiling_key\":78,\"workspaces\":[12]}";
  EXPECT_EQ(result, runinfo.substr(0, 96));
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling_parse = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_creator = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_deleter = nullptr;
}

TEST_F(UtestRegister, new_optiling_py_interface_fail_with_invalid_const_value) {
  // int9999 is invalid data type
  const nlohmann::json input = R"([
  {"name": "test_0","dtype": "int9999", "const_value": [1,2,3,4],"shape": [4,4,4,4],"format": "ND"}])"_json;
  std::string input_str = input.dump();
  std::string output_str = " ";
  std::string attrs_str = " ";
  const char *op_type = "TestReluV2";
  const char *cmp_info = "";
  std::string runinfo(100, 'a');
  size_t size = 100;
  const char *cmp_info_hash = "";
  uint64_t *elapse = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling = OpTilingStubNew;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling_parse = OpTilingParseStubNew;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_creator = CreateCompileInfo;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_deleter = DeleteCompileInfo;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).max_tiling_data_size = 50;
  EXPECT_EQ(TbeOpTilingPyInterface(op_type, cmp_info, cmp_info_hash, input_str.c_str(), output_str.c_str(),
                                   attrs_str.c_str(), const_cast<char *>(runinfo.c_str()), size, elapse),
            0);
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling_parse = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_creator = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_deleter = nullptr;
}

TEST_F(UtestRegister, new_optiling_py_interface_fail_with_invalid_attr) {
  std::string input_str = " ";
  std::string output_str = " ";
  // int999 is invalid dtype
  const nlohmann::json attrs = R"([
{ "name": "attr_0","dtype": "list_int64","value": [1,2, 3, 4]},
{ "name": "attr_1","dtype": "int9999","value": 99}])"_json;
  std::string attrs_str = attrs.dump();
  const char *op_type = "TestReluV2";
  const char *cmp_info = "";
  std::string runinfo(100, 'a');
  size_t size = 100;
  const char *cmp_info_hash = "";
  uint64_t *elapse = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling = OpTilingStubNew;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling_parse = OpTilingParseStubNew;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_creator = CreateCompileInfo;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_deleter = DeleteCompileInfo;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).max_tiling_data_size = 50;
  EXPECT_EQ(TbeOpTilingPyInterface(op_type, cmp_info, cmp_info_hash, input_str.c_str(), output_str.c_str(),
                                   attrs_str.c_str(), const_cast<char *>(runinfo.c_str()), size, elapse),
            0);
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling_parse = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_creator = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_deleter = nullptr;
}

TEST_F(UtestRegister, new_optiling_py_interface_fail_without_params) {
  EXPECT_EQ(TbeOpTilingPyInterface(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr), 0);
}

TEST_F(UtestRegister, new_optiling_py_interface_ok_with_float_data) {
  const nlohmann::json input = R"([
{"name": "t0", "dtype": "float16","const_value": [1.1,2.1,3.1,4.1] ,"shape": [4,4,4,4], "ori_shape":[4,4,4,4],"format": "ND"},
{"dtype": "int8", "shape": [4,4,4,4], "ori_shape":[4,4,4,4],"format": "ND"}
])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "int8","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;
  std::string output_str = output.dump();
  const char *op_type = "TestReluV2";
  const char *cmp_info = "";
  std::string runinfo(100, 'a');
  size_t size = 100;
  const char *cmp_info_hash = "";
  uint64_t *elapse = nullptr;
  const nlohmann::json attrs = R"([
{ "name": "op_para_size", "dtype": "int", "value": 50}])"_json;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling = OpTilingStubV5;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling_parse = OpTilingParseStubV5;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_creator = CreateCompileInfo;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_deleter = DeleteCompileInfo;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).max_tiling_data_size = 50;
  EXPECT_EQ(TbeOpTilingPyInterface(op_type, cmp_info, cmp_info_hash, input_str.c_str(), output_str.c_str(),
                                   attrs.dump().c_str(), const_cast<char *>(runinfo.c_str()), size, elapse),
            1);
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).tiling_parse = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_creator = nullptr;
  gert::OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type).compile_info_deleter = nullptr;
}

TEST_F(UtestRegister, new_optiling_py_interface_ok_auto_tiling) {
  IMPL_OP_DEFAULT().Tiling(DefaultOptilingStub).TilingParse<StubCompileInfo>(OpTilingParseStubV5);
  // expect rt1 tiling not to work
  REGISTER_OP_TILING_V2(AutoTiling, op_tiling_stub_failed);
  const nlohmann::json input = R"([
{"name": "test_0","dtype": "int8","shape": [4,4,4,4],"format": "ND"}])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "int8","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;
  std::string output_str = output.dump();
  const char *op_type = "AutoTiling";
  const char *cmp_info = "";
  std::string runinfo(100, 'a');
  size_t size = 100;
  const char *cmp_info_hash = "";
  uint64_t *elapse = nullptr;
  const nlohmann::json attrs = R"([
{ "name": "op_para_size", "dtype": "int", "value": 50}])"_json;
  EXPECT_EQ(TbeOpTilingPyInterface(op_type, cmp_info, cmp_info_hash, input_str.c_str(), output_str.c_str(),
                                   attrs.dump().c_str(), const_cast<char *>(runinfo.c_str()), size, elapse),
            1);
}

extern "C" int Tik2PyInterfaceCheckOp(const char *check_type, const char *optype, const char *inputs,
                                      const char *outputs, const char *attrs, char *result_info,
                                      size_t result_info_len);

extern "C" int Tik2PyInterfaceGeneralized(const char *optype, const char *inputs, const char *outputs,
                                          const char *attrs, const char *generalize_config, char *result_info,
                                          size_t result_info_len);

extern "C" int Tik2PyInterfaceGetTilingDefInfo(const char *optype, char *result_info, size_t result_info_len);

int check_supported_stub(const ge::Operator &op, ge::AscendString &result) {
  const nlohmann::json res_json = R"(
{"ret_code": "1","reason": "check_supported_stub"})"_json;
  std::string res_json_str = res_json.dump();
  result = AscendString(res_json_str.c_str());
  return 1;
}

int op_select_format_stub(const ge::Operator &op, ge::AscendString &result) {
  const nlohmann::json res_json = R"({"op_info": "op_select_format_stub"})"_json;
  std::string res_json_str = res_json.dump();
  result = AscendString(res_json_str.c_str());
  return 1;
}

int get_op_support_info_stub(const ge::Operator &op, ge::AscendString &result) {
  const nlohmann::json res_json = R"({"op_info": "get_op_support_info_stub"})"_json;
  std::string res_json_str = res_json.dump();
  result = AscendString(res_json_str.c_str());
  return 1;
}

int get_op_specific_info_stub(const ge::Operator &op, ge::AscendString &result) {
  const nlohmann::json res_json = R"({"op_info": "get_op_specific_info_stub"})"_json;
  std::string res_json_str = res_json.dump();
  result = AscendString(res_json_str.c_str());
  return 1;
}

int check_supported_stub_throw(const ge::Operator &op, ge::AscendString &result) {
  throw "bad callback";
  return 1;
}

TEST_F(UtestRegister, tik2_py_interface_check_cap_ok) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  const nlohmann::json input = R"([
{"name": "test_0","dtype": "int8", "const_value": [1,2,3,4],"shape": [4,4,4,4],"format": "ND"},
{"name": "test_1","dtype": "int32","shape": [5,5,5,5],"ori_shape": [5,5,5,5],"format": "ND","ori_format": "ND"},
{"name": "test_2","dtype": "int32","shape": [6,6,6,6],"ori_shape": [6,6,6,6],"format": "ND","ori_format": "ND"}])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "int8","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;

  std::string output_str = output.dump();
  const nlohmann::json attrs = R"([
{ "name": "attr_0","dtype": "list_int64","value": [1,2, 3, 4]},
{ "name": "attr_1","dtype": "int","value": 99},
{ "name": "attr_2","dtype": "list_int32","value": [1, 2, 3, 4]},
{ "name": "op_para_size", "dtype": "int", "value": 50}])"_json;
  std::string attrs_str = attrs.dump();
  std::string op_type = "tik2_py_interface_check_cap_ok";
  std::string res_info(100, 'a');
  size_t size = 100;
  // check_supported
  REG_CHECK_SUPPORT(tik2_py_interface_check_cap_ok, check_supported_stub);
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_CHECK_SUPPORTED.GetString(), op_type.c_str(), input_str.c_str(), output_str.c_str(),
                                   attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            1);
  std::string check_supported_result = "{\"reason\":\"check_supported_stub\",\"ret_code\":\"1\"}";
  EXPECT_EQ(check_supported_result, res_info.substr(0, check_supported_result.size()));

  // op_select_format
  REG_OP_SELECT_FORMAT(tik2_py_interface_check_cap_ok, op_select_format_stub);
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_OP_SELECT_FORMAT.GetString(), op_type.c_str(), input_str.c_str(),
                                   output_str.c_str(), attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            1);
  std::string op_select_format_result = "{\"op_info\":\"op_select_format_stub\"}";
  EXPECT_EQ(op_select_format_result, res_info.substr(0, op_select_format_result.size()));

  // get_op_support_info
  REG_OP_SUPPORT_INFO(tik2_py_interface_check_cap_ok, get_op_support_info_stub);
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_GET_OP_SUPPORT_INFO.GetString(), op_type.c_str(), input_str.c_str(),
                                   output_str.c_str(), attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            1);
  std::string get_op_support_info_result = "{\"op_info\":\"get_op_support_info_stub\"}";
  EXPECT_EQ(get_op_support_info_result, res_info.substr(0, get_op_support_info_result.size()));

  // get_op_specific_info
  REG_OP_SPEC_INFO(tik2_py_interface_check_cap_ok, get_op_specific_info_stub);
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_GET_SPECIFIC_INFO.GetString(), op_type.c_str(), input_str.c_str(),
                                   output_str.c_str(), attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            1);
  std::string get_op_specific_info_result = "{\"op_info\":\"get_op_specific_info_stub\"}";
  EXPECT_EQ(get_op_specific_info_result, res_info.substr(0, get_op_specific_info_result.size()));
  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_check_cap_fail_without_callback) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  const nlohmann::json input = R"([
{"name": "test_0","dtype": "int8", "const_value": [1,2,3,4],"shape": [4,4,4,4],"format": "ND"},
{"name": "test_1","dtype": "int32","shape": [5,5,5,5],"ori_shape": [5,5,5,5],"format": "ND","ori_format": "ND"},
{"name": "test_2","dtype": "int32","shape": [6,6,6,6],"ori_shape": [6,6,6,6],"format": "ND","ori_format": "ND"}])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "int8","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;

  std::string output_str = output.dump();
  const nlohmann::json attrs = R"([
{ "name": "attr_0","dtype": "list_int64","value": [1,2, 3, 4]},
{ "name": "attr_1","dtype": "int","value": 99},
{ "name": "attr_2","dtype": "list_int32","value": [1, 2, 3, 4]},
{ "name": "op_para_size", "dtype": "int", "value": 50}])"_json;
  std::string attrs_str = attrs.dump();
  std::string op_type = "tik2_py_interface_check_cap_fail_without_callback";
  std::string res_info(100, 'a');
  size_t size = 100;
  // check_supported
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_CHECK_SUPPORTED.GetString(), op_type.c_str(), input_str.c_str(), output_str.c_str(),
                                   attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            0);

  // op_select_format
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_OP_SELECT_FORMAT.GetString(), op_type.c_str(), input_str.c_str(),
                                   output_str.c_str(), attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            0);

  // get_op_support_info
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_GET_OP_SUPPORT_INFO.GetString(), op_type.c_str(), input_str.c_str(),
                                   output_str.c_str(), attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            0);

  // get_op_specific_info
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_GET_SPECIFIC_INFO.GetString(), op_type.c_str(), input_str.c_str(),
                                   output_str.c_str(), attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            0);
  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_check_cap_fail_throw) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  const nlohmann::json input = R"([
{"name": "test_0","dtype": "int8", "const_value": [1,2,3,4],"shape": [4,4,4,4],"format": "ND"},
{"name": "test_1","dtype": "int32","shape": [5,5,5,5],"ori_shape": [5,5,5,5],"format": "ND","ori_format": "ND"},
{"name": "test_2","dtype": "int32","shape": [6,6,6,6],"ori_shape": [6,6,6,6],"format": "ND","ori_format": "ND"}])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "int8","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;

  std::string output_str = output.dump();
  const nlohmann::json attrs = R"([
{ "name": "attr_0","dtype": "list_int64","value": [1,2, 3, 4]},
{ "name": "attr_1","dtype": "int","value": 99},
{ "name": "attr_2","dtype": "list_int32","value": [1, 2, 3, 4]},
{ "name": "op_para_size", "dtype": "int", "value": 50}])"_json;
  std::string attrs_str = attrs.dump();
  std::string op_type = "tik2_py_interface_check_cap_fail_throw";
  std::string res_info(100, 'a');
  size_t size = 100;
  // check_supported
  REG_CHECK_SUPPORT(tik2_py_interface_check_cap_fail_throw, check_supported_stub_throw);
  EXPECT_EQ(Tik2PyInterfaceCheckOp(FUNC_CHECK_SUPPORTED.GetString(), op_type.c_str(), input_str.c_str(), output_str.c_str(),
                                   attrs_str.c_str(), const_cast<char *>(res_info.c_str()), size),
            0);
  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_check_cap_fail_without_params) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  EXPECT_EQ(Tik2PyInterfaceCheckOp(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0), 0);
  unsetenv("ENABLE_RUNTIME_V2");
}

int generalize_stub(const ge::Operator &op, const ge::AscendString &generalize_config, ge::AscendString &result) {
  const nlohmann::json res_json = R"({"op_info": "generalize_stub"})"_json;
  std::string res_json_str = res_json.dump();
  result = AscendString(res_json_str.c_str());
  return 1;
}

int generalize_stub_throw(const ge::Operator &op, const ge::AscendString &generalize_config, ge::AscendString &result) {
  const nlohmann::json res_json = R"({"op_info": "generalize_stub"})"_json;
  std::string res_json_str = res_json.dump();
  result = AscendString(res_json_str.c_str());
  throw "bad callback";
  return 1;
}

TEST_F(UtestRegister, tik2_py_interface_generalize_ok) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  const nlohmann::json input = R"([
{"name": "test_0","dtype": "float16", "const_value": [1,2,3,4],"shape": [4,4,4,4],"format": "ND"},
{"name": "test_1","dtype": "float32","shape": [5,5,5,5],"ori_shape": [5,5,5,5],"format": "ND","ori_format": "ND"},
{"name": "test_2","dtype": "int64","shape": [6,6,6,6],"ori_shape": [6,6,6,6],"format": "ND","ori_format": "ND"}])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "uint32","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;

  std::string output_str = output.dump();
  const nlohmann::json attrs = R"([
{ "name": "attr_0","dtype": "list_list_int64","value": [[1, 2], [3, 4]]},
{ "name": "attr_1","dtype": "uint32","value": 99},
{ "name": "attr_2","dtype": "list_list_int32","value": [[1, 2], [3, 4]]},
{ "name": "op_para_size", "dtype": "uint16", "value": 50}])"_json;
  std::string attrs_str = attrs.dump();
  std::string op_type = "tik2_py_interface_generalize_ok";
  std::string generalize_config = "keep_rank";
  std::string res_info(100, 'a');
  size_t size = 100;
  // shape generalize
  REG_OP_PARAM_GENERALIZE(tik2_py_interface_generalize_ok, generalize_stub);
  EXPECT_EQ(Tik2PyInterfaceGeneralized(op_type.c_str(), input_str.c_str(), output_str.c_str(), attrs_str.c_str(),
                                       generalize_config.c_str(), const_cast<char *>(res_info.c_str()), size),
            1);
  std::string result = "{\"op_info\":\"generalize_stub\"}";
  EXPECT_EQ(result, res_info.substr(0, result.size()));

  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_generalize_fail_without_callback) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  const nlohmann::json input = R"([
{"name": "test_0","dtype": "int8", "const_value": [1,2,3,4],"shape": [4,4,4,4],"format": "ND"},
{"name": "test_1","dtype": "int32","shape": [5,5,5,5],"ori_shape": [5,5,5,5],"format": "ND","ori_format": "ND"},
{"name": "test_2","dtype": "int32","shape": [6,6,6,6],"ori_shape": [6,6,6,6],"format": "ND","ori_format": "ND"}])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "int8","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;

  std::string output_str = output.dump();
  const nlohmann::json attrs = R"([
{ "name": "attr_0","dtype": "list_int64","value": [1,2, 3, 4]},
{ "name": "attr_1","dtype": "int","value": 99},
{ "name": "attr_2","dtype": "list_int32","value": [1, 2, 3, 4]},
{ "name": "op_para_size", "dtype": "int", "value": 50}])"_json;
  std::string attrs_str = attrs.dump();
  std::string op_type = "TestReluV2";
  std::string generalize_config = "keep_rank";
  std::string res_info(100, 'a');
  size_t size = 100;
  // shape generalize
  EXPECT_EQ(Tik2PyInterfaceGeneralized(op_type.c_str(), input_str.c_str(), output_str.c_str(), attrs_str.c_str(),
                                       generalize_config.c_str(), const_cast<char *>(res_info.c_str()), size),
            0);

  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_generalize_fail_throw) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  const nlohmann::json input = R"([
{"name": "test_0","dtype": "float16", "const_value": [1,2,3,4],"shape": [4,4,4,4],"format": "ND"},
{"name": "test_1","dtype": "float32","shape": [5,5,5,5],"ori_shape": [5,5,5,5],"format": "ND","ori_format": "ND"},
{"name": "test_2","dtype": "int64","shape": [6,6,6,6],"ori_shape": [6,6,6,6],"format": "ND","ori_format": "ND"}])"_json;
  std::string input_str = input.dump();
  const nlohmann::json output = R"([ 
{"name": "y_0","dtype": "uint32","shape": [9,9,9,9],"ori_shape" :[9,9,9,9],"format": "ND","ori_format":"ND"}])"_json;

  std::string output_str = output.dump();
  const nlohmann::json attrs = R"([
{ "name": "attr_0","dtype": "list_list_int64","value": [[1, 2], [3, 4]]},
{ "name": "attr_1","dtype": "uint32","value": 99},
{ "name": "attr_2","dtype": "list_list_int32","value": [[1, 2], [3, 4]]},
{ "name": "op_para_size", "dtype": "uint16", "value": 50}])"_json;
  std::string attrs_str = attrs.dump();
  std::string op_type = "tik2_py_interface_generalize_fail_throw";
  std::string generalize_config = "keep_rank";
  std::string res_info(100, 'a');
  size_t size = 100;
  // shape generalize
  REG_OP_PARAM_GENERALIZE(tik2_py_interface_generalize_fail_throw, generalize_stub_throw);
  EXPECT_EQ(Tik2PyInterfaceGeneralized(op_type.c_str(), input_str.c_str(), output_str.c_str(), attrs_str.c_str(),
                                       generalize_config.c_str(), const_cast<char *>(res_info.c_str()), size),
            0);
  
  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_generalize_fail_without_params) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  EXPECT_EQ(Tik2PyInterfaceGeneralized(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0), 0);
  unsetenv("ENABLE_RUNTIME_V2");
}

BEGIN_TILING_DATA_DEF(TestMaxPoolTilingData)
// format: TILING_DATA_FIELD_DEF(data_type, field_name);
TILING_DATA_FIELD_DEF(int32_t, dim_0);
TILING_DATA_FIELD_DEF(uint16_t, var_1);
TILING_DATA_FIELD_DEF(int64_t, factor_1);
END_TILING_DATA_DEF

// register class
REGISTER_TILING_DATA_CLASS(TestMaxPool, TestMaxPoolTilingData)

TEST_F(UtestRegister, tik2_py_interface_get_tiling_def_ok) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  std::string op_type = "TestMaxPool";
  std::string res_info(1024, 'a');
  size_t size = 1024;
  EXPECT_EQ(Tik2PyInterfaceGetTilingDefInfo(op_type.c_str(), const_cast<char *>(res_info.c_str()), size), 1);
  std::string result =
      "{\"class_name\":\"TestMaxPoolTilingData\",\"fields\":[{\"dtype\":\"int32_t\",\"name\":\"dim_0\"},{\"dtype\":"
      "\"uint16_t\",\"name\":\"var_1\"},{\"dtype\":\"int64_t\",\"name\":\"factor_1\"}]}";
  EXPECT_EQ(result, res_info.substr(0, result.size()));

  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_get_tiling_def_without_callback) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  std::string op_type = "TestMaxPoolNotExist";
  std::string res_info(1024, 'a');
  size_t size = 1024;
  // check_supported
  EXPECT_EQ(Tik2PyInterfaceGetTilingDefInfo(op_type.c_str(), const_cast<char *>(res_info.c_str()), size), 0);
  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_get_tiling_def_fail_without_params) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  EXPECT_EQ(Tik2PyInterfaceGetTilingDefInfo(nullptr, nullptr, 0), 0);
  unsetenv("ENABLE_RUNTIME_V2");
}

extern "C" int Tik2PyInterfaceOpReplay(const char *optype, const char *soc_version, int block_dim,
                                       const char *tiling_data, const char *kernel_name, const char *entry_file,
                                       const char *output_kernel_file, int core_type, int task_ration);

int replay_stub(ReplayFuncParam& param, const int core_typ) {
  return 1;
}

int replay_stub_throw(ReplayFuncParam& param, const int core_typ) {
  throw "bad callback";
  return 1;
}

int replay_stub_invalid_ret(ReplayFuncParam& param, const int core_typ) {
  return 0;
}

TEST_F(UtestRegister, tik2_py_interface_op_replay_ok) {
  setenv("ENABLE_RUNTIME_V2", "1", 0); 
  std::string op_type = "tik2_py_interface_op_replay_ok";
  std::string soc_version = "ascend710";
  int blkdim = 32;
  std::string tilingdata = "\x00\x14\x00\x00\x00\n\x00(\x1e\x00\x00\x00\x00\x00\x00\x00";
  std::string kernel_name = "tik2_py_interface_op_replay_ok";
  std::string entry_file = "tik2_py_interface_op_replay_ok_entry_file.h";
  std::string output_kernel_file = "tik2_py_interface_op_replay_ok_kernel_file.cce";
  int core_type = 0;
  int task_ration = 1;
  REG_REPLAY_FUNC(tik2_py_interface_op_replay_ok, ascend710, replay_stub);
  EXPECT_EQ(Tik2PyInterfaceOpReplay(op_type.c_str(), soc_version.c_str(), blkdim, tilingdata.c_str(),
                                    kernel_name.c_str(), entry_file.c_str(), output_kernel_file.c_str(),
                                    core_type, task_ration),
            1);

  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_op_replay_fail_without_callback) {
  setenv("ENABLE_RUNTIME_V2", "1", 0); 
  std::string op_type = "tik2_py_interface_op_replay_fail_without_callback";
  std::string soc_version = "ascend710";
  int blkdim = 32;
  std::string tilingdata = "\x00\x14\x00\x00\x00\n\x00(\x1e\x00\x00\x00\x00\x00\x00\x00";
  std::string kernel_name = "tik2_py_interface_op_replay_fail_without_callback";
  std::string entry_file = "tik2_py_interface_op_replay_fail_without_callback_entry_file.h";
  std::string output_kernel_file = "tik2_py_interface_op_replay_fail_without_callback_kernel_file.cce";
  int core_type = 0;
  int task_ration = 1;
  EXPECT_EQ(Tik2PyInterfaceOpReplay(op_type.c_str(), soc_version.c_str(), blkdim, tilingdata.c_str(),
                                    kernel_name.c_str(), entry_file.c_str(), output_kernel_file.c_str(),
                                    core_type, task_ration),
            0);

  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_op_replay_fail_throw) {
  setenv("ENABLE_RUNTIME_V2", "1", 0); 
  std::string op_type = "tik2_py_interface_op_replay_fail_throw";
  std::string soc_version = "ascend710";
  int blkdim = 32;
  std::string tilingdata = "\x00\x14\x00\x00\x00\n\x00(\x1e\x00\x00\x00\x00\x00\x00\x00";
  std::string kernel_name = "tik2_py_interface_op_replay_fail_throw";
  std::string entry_file = "tik2_py_interface_op_replay_fail_throw_entry_file.h";
  std::string output_kernel_file = "tik2_py_interface_op_replay_fail_throw_kernel_file.cce";
  int core_type = 0;
  int task_ration = 1;
  REG_REPLAY_FUNC(tik2_py_interface_op_replay_fail_throw, ascend710, replay_stub_throw);
  EXPECT_EQ(Tik2PyInterfaceOpReplay(op_type.c_str(), soc_version.c_str(), blkdim,
                                tilingdata.c_str(),kernel_name.c_str(), entry_file.c_str(), output_kernel_file.c_str(),
                                core_type, task_ration),
            0);

  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_op_replay_fail_without_params) {
  setenv("ENABLE_RUNTIME_V2", "1", 0);
  EXPECT_EQ(Tik2PyInterfaceOpReplay(nullptr, nullptr, 0, nullptr, nullptr, nullptr, nullptr, 0, 1), 0);
  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_op_replay_invalid_core_type) {
  setenv("ENABLE_RUNTIME_V2", "1", 0); 
  std::string op_type = "tik2_py_interface_op_replay_invalid_core_type";
  std::string soc_version = "ascend710";
  int blkdim = 32;
  std::string tilingdata = "\x00\x14\x00\x00\x00\n\x00(\x1e\x00\x00\x00\x00\x00\x00\x00";
  std::string kernel_name = "tik2_py_interface_op_replay_invalid_core_type";
  std::string entry_file = "tik2_py_interface_op_replay_invalid_core_type_entry_file.h";
  std::string output_kernel_file = "tik2_py_interface_op_replay_invalid_core_type_kernel_file.cce";
  int core_type = 4;
  int task_ration = 1;
  REG_REPLAY_FUNC(tik2_py_interface_op_replay_invalid_core_type, ascend710, replay_stub);
  EXPECT_EQ(Tik2PyInterfaceOpReplay(op_type.c_str(), soc_version.c_str(), blkdim, tilingdata.c_str(),
                                    kernel_name.c_str(), entry_file.c_str(), output_kernel_file.c_str(),
                                    core_type, task_ration),
            0);

  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_op_replay_invalid_task_ration) {
  setenv("ENABLE_RUNTIME_V2", "1", 0); 
  std::string op_type = "tik2_py_interface_op_replay_invalid_task_ration";
  std::string soc_version = "ascend710";
  int blkdim = 32;
  std::string tilingdata = "\x00\x14\x00\x00\x00\n\x00(\x1e\x00\x00\x00\x00\x00\x00\x00";
  std::string kernel_name = "tik2_py_interface_op_replay_invalid_task_ration";
  std::string entry_file = "tik2_py_interface_op_replay_invalid_task_ration_entry_file.h";
  std::string output_kernel_file = "tik2_py_interface_op_replay_invalid_task_ration_kernel_file.cce";
  int core_type = 0;
  int task_ration = -1;
  REG_REPLAY_FUNC(tik2_py_interface_op_replay_invalid_task_ration, ascend710, replay_stub);
  EXPECT_EQ(Tik2PyInterfaceOpReplay(op_type.c_str(), soc_version.c_str(), blkdim, tilingdata.c_str(),
                                    kernel_name.c_str(), entry_file.c_str(), output_kernel_file.c_str(),
                                    core_type, task_ration),
            0);

  unsetenv("ENABLE_RUNTIME_V2");
}

TEST_F(UtestRegister, tik2_py_interface_op_replay_invalid_ret) {
  setenv("ENABLE_RUNTIME_V2", "1", 0); 
  std::string op_type = "tik2_py_interface_op_replay_invalid_ret";
  std::string soc_version = "ascend710";
  int blkdim = 32;
  std::string tilingdata = "\x00\x14\x00\x00\x00\n\x00(\x1e\x00\x00\x00\x00\x00\x00\x00";
  std::string kernel_name = "tik2_py_interface_op_replay_invalid_ret";
  std::string entry_file = "tik2_py_interface_op_replay_invalid_ret_entry_file.h";
  std::string output_kernel_file = "tik2_py_interface_op_replay_invalid_ret_kernel_file.cce";
  int core_type = 1;
  int task_ration = 2;
  REG_REPLAY_FUNC(tik2_py_interface_op_replay_invalid_ret, ascend710, replay_stub_invalid_ret);
  EXPECT_EQ(Tik2PyInterfaceOpReplay(op_type.c_str(), soc_version.c_str(), blkdim, tilingdata.c_str(),
                                    kernel_name.c_str(), entry_file.c_str(), output_kernel_file.c_str(),
                                    core_type, task_ration),
            0);

  unsetenv("ENABLE_RUNTIME_V2");
}