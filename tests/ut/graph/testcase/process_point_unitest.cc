/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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
#include "flow_graph/data_flow.h"
#include "proto/dflow.pb.h"
#include "graph/serialization/attr_serializer_registry.h"

using namespace ge::dflow;

namespace ge {
class ProcessPointUTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(ProcessPointUTest, GraphPpNormalTest) {
  ge::Graph graph("user_graph");
  GraphBuilder graph_build = [graph]() { return graph; };
  auto pp1 = GraphPp("pp1", graph_build).SetCompileConfig("./pp1.json");

  ASSERT_EQ(strcmp(pp1.GetProcessPointName(), "pp1"), 0);
  ASSERT_EQ(pp1.GetProcessPointType(), ProcessPointType::GRAPH);
  ASSERT_EQ(strcmp(pp1.GetCompileConfig(), "./pp1.json"), 0);
  auto builder = pp1.GetGraphBuilder();
  auto sub_graph = builder();
  ASSERT_EQ(sub_graph.GetName(), "user_graph");
}

TEST_F(ProcessPointUTest, GraphPpNullBuilder) {
  auto pp1 = GraphPp("pp1", nullptr).SetCompileConfig("./pp1.json");
  auto pp2 = GraphPp("pp2", nullptr).SetCompileConfig(nullptr);
  ASSERT_EQ(strcmp(pp1.GetProcessPointName(), "pp1"), 0);
  ASSERT_EQ(pp1.GetProcessPointType(), ProcessPointType::GRAPH);
  ASSERT_EQ(strcmp(pp1.GetCompileConfig(), "./pp1.json"), 0);
  ASSERT_EQ(pp1.GetGraphBuilder(), nullptr);
  ASSERT_EQ(strcmp(pp2.GetCompileConfig(), ""), 0);
}

TEST_F(ProcessPointUTest, GraphPpNullPpName) {
  auto pp1 = GraphPp(nullptr, nullptr).SetCompileConfig("./pp1.json");

  ASSERT_EQ(pp1.GetProcessPointName(), nullptr);
  ASSERT_EQ(pp1.GetProcessPointType(), ProcessPointType::INVALID);
  ASSERT_EQ(pp1.GetCompileConfig(), nullptr);
  ASSERT_EQ(pp1.GetGraphBuilder(), nullptr);
}

TEST_F(ProcessPointUTest, FunctionPpNormalTest) {
  int64_t batch_value = 124;
  std::vector<int64_t> list_int = {10, 20, 30};
  std::vector<std::vector<int64_t>> list_list_int = {{0, 1}, {-1, 1}};
  bool bool_attr = true;
  std::vector<bool> list_bool = {true, false, true};
  float float_attr = 2.1;
  std::vector<float> list_float = {1.1, 2.2, 3.3};
  ge::DataType dt = DT_DOUBLE;
  std::vector<ge::DataType> list_dt = {DT_BOOL, DT_DOUBLE, DT_INT64};
  ge::AscendString str_value("str_value");
  std::vector<ge::AscendString> list_str_value;
  list_str_value.emplace_back(str_value);
  list_str_value.emplace_back(str_value);

  auto pp1 = FunctionPp("pp1")
                 .SetCompileConfig("./pp1.json")
                 .SetInitParam("_batchsize", batch_value)
                 .SetInitParam("_list_int", list_int)
                 .SetInitParam("_list_list_int", list_list_int)
                 .SetInitParam("_bool_attr", bool_attr)
                 .SetInitParam("_list_bool", list_bool)
                 .SetInitParam("_float_attr", float_attr)
                 .SetInitParam("_list_float", list_float)
                 .SetInitParam("_data_type_attr", dt)
                 .SetInitParam("_list_dt", list_dt)
                 .SetInitParam("_c_str_value", "c_str_value")
                 .SetInitParam("_c_str_null_value", nullptr)
                 .SetInitParam("_str_value", str_value)
                 .SetInitParam("_list_str_value", list_str_value);

  ge::AscendString str;
  pp1.Serialize(str);
  std::string tar_str(str.GetString(), str.GetLength());
  ASSERT_EQ(strcmp(pp1.GetProcessPointName(), "pp1"), 0);
  ASSERT_EQ(pp1.GetProcessPointType(), ProcessPointType::FUNCTION);
  ASSERT_EQ(strcmp(pp1.GetCompileConfig(), "./pp1.json"), 0);

  auto process_point = dataflow::ProcessPoint();
  auto flag = process_point.ParseFromString(tar_str);
  ASSERT_TRUE(flag);
  ASSERT_EQ(process_point.name(), "pp1");
  ASSERT_EQ(process_point.type(), dataflow::ProcessPoint_ProcessPointType_FUNCTION);
  ASSERT_EQ(process_point.compile_cfg_file(), "./pp1.json");

  auto int_attr = process_point.attrs();
  // int check
  auto *int_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kI);
  ASSERT_NE(int_deserializer, nullptr);
  AnyValue value;
  int_deserializer->Deserialize(int_attr["_batchsize"], value);
  int64_t get_batch_value;
  value.GetValue(get_batch_value);
  ASSERT_EQ(get_batch_value, batch_value);
  value.Clear();
  // list int check
  auto *list_int_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kList);
  ASSERT_NE(list_int_deserializer, nullptr);
  list_int_deserializer->Deserialize(int_attr["_list_int"], value);
  std::vector<int64_t> get_list_int_value;
  value.GetValue(get_list_int_value);
  ASSERT_EQ(get_list_int_value, list_int);
  value.Clear();
  // bool check
  auto *bool_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kB);
  ASSERT_NE(bool_deserializer, nullptr);
  bool_deserializer->Deserialize(int_attr["_bool_attr"], value);
  bool get_bool_value;
  value.GetValue(get_bool_value);
  ASSERT_EQ(get_bool_value, bool_attr);
  value.Clear();
  // list bool check
  auto *list_bool_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kList);
  ASSERT_NE(list_bool_deserializer, nullptr);
  list_bool_deserializer->Deserialize(int_attr["_list_bool"], value);
  std::vector<bool> get_list_bool_value;
  value.GetValue(get_list_bool_value);
  ASSERT_EQ(get_list_bool_value, list_bool);
  value.Clear();
  // float check
  auto *float_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kF);
  ASSERT_NE(float_deserializer, nullptr);
  float_deserializer->Deserialize(int_attr["_float_attr"], value);
  float get_float_value;
  value.GetValue(get_float_value);
  ASSERT_EQ(get_float_value, float_attr);
  value.Clear();
  // list float check
  auto *list_float_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kList);
  ASSERT_NE(list_float_deserializer, nullptr);
  list_float_deserializer->Deserialize(int_attr["_list_float"], value);
  std::vector<float> get_list_float_value;
  value.GetValue(get_list_float_value);
  ASSERT_EQ(get_list_float_value, list_float);
  value.Clear();
  // data type check
  auto *dt_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kDt);
  ASSERT_NE(dt_deserializer, nullptr);
  dt_deserializer->Deserialize(int_attr["_data_type_attr"], value);
  ge::DataType get_dt_value;
  value.GetValue(get_dt_value);
  ASSERT_EQ(get_dt_value, dt);
  value.Clear();
  // list data check
  auto *list_dt_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kList);
  ASSERT_NE(list_dt_deserializer, nullptr);
  list_dt_deserializer->Deserialize(int_attr["_list_dt"], value);
  std::vector<ge::DataType> get_list_dt_value;
  value.GetValue(get_list_dt_value);
  ASSERT_EQ(get_list_dt_value, list_dt);
  value.Clear();
  // c-string check
  auto *c_str_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kS);
  ASSERT_NE(c_str_deserializer, nullptr);
  c_str_deserializer->Deserialize(int_attr["_c_str_value"], value);
  std::string get_c_str_value;
  value.GetValue(get_c_str_value);
  ASSERT_EQ(get_c_str_value, "c_str_value");
  value.Clear();
  // c-string null check
  auto *c_str_null_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kS);
  ASSERT_NE(c_str_null_deserializer, nullptr);
  c_str_null_deserializer->Deserialize(int_attr["_c_str_null_value"], value);
  std::string get_c_str_null_value;
  value.GetValue(get_c_str_null_value);
  ASSERT_EQ(get_c_str_null_value, "");
  value.Clear();
  // string check
  auto *str_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kS);
  ASSERT_NE(str_deserializer, nullptr);
  str_deserializer->Deserialize(int_attr["_str_value"], value);
  std::string get_str_value;
  value.GetValue(get_str_value);
  ASSERT_EQ(get_str_value, str_value.GetString());
  value.Clear();
  // list string check
  auto *list_str_deserializer = AttrSerializerRegistry::GetInstance().GetDeserializer(proto::AttrDef::kList);
  ASSERT_NE(list_str_deserializer, nullptr);
  list_str_deserializer->Deserialize(int_attr["_list_str_value"], value);
  std::vector<std::string> get_list_str_value;
  value.GetValue(get_list_str_value);
  ASSERT_EQ(get_list_str_value[0], str_value.GetString());
  ASSERT_EQ(get_list_str_value[1], str_value.GetString());
  value.Clear();
}

TEST_F(ProcessPointUTest, FunctionPpNullPpName) {
  int64_t batch_value = 124;
  std::vector<int64_t> list_int = {10, 20, 30};
  std::vector<std::vector<int64_t>> list_list_int = {{0, 1}, {-1, 1}};
  bool bool_attr = true;
  std::vector<bool> list_bool = {true, false, true};
  float float_attr = 2.1;
  std::vector<float> list_float = {1.1, 2.2, 3.3};
  ge::DataType dt = DT_DOUBLE;
  std::vector<ge::DataType> list_dt = {DT_BOOL, DT_DOUBLE, DT_INT64};
  ge::AscendString str_value("str_value");
  std::vector<ge::AscendString> list_str_value;
  list_str_value.emplace_back(str_value);
  list_str_value.emplace_back(str_value);

  auto pp1 = FunctionPp(nullptr).SetCompileConfig("./pp1.json")
                                .SetInitParam("_batchsize", batch_value)
                                .SetInitParam("_list_int", list_int)
                                .SetInitParam("_list_list_int", list_list_int)
                                .SetInitParam("_bool_attr", bool_attr)
                                .SetInitParam("_list_bool", list_bool)
                                .SetInitParam("_float_attr", float_attr)
                                .SetInitParam("_list_float", list_float)
                                .SetInitParam("_data_type_attr", dt)
                                .SetInitParam("_list_dt", list_dt)
                                .SetInitParam("_str_value", str_value)
                                .SetInitParam("_list_str_value", list_str_value);
  ASSERT_EQ(pp1.GetProcessPointName(), nullptr);
  ASSERT_EQ(pp1.GetProcessPointType(), ProcessPointType::INVALID);
  ASSERT_EQ(pp1.GetCompileConfig(), nullptr);
}

TEST_F(ProcessPointUTest, FunctionPpInvokedPp) {
  ge::Graph ge_graph("ge_graph");
  GraphBuilder graph_build = [ge_graph]() { return ge_graph; };
  GraphBuilder graph_build2 = []() { return ge::Graph("ge_graph2"); };
  auto graphPp1 = GraphPp("graphPp_1", graph_build).SetCompileConfig("./graph.json");
  auto graphPp2 = GraphPp("graphPp_2", graph_build2).SetCompileConfig("./graph2.json");
  auto pp1 = FunctionPp("pp1").AddInvokedClosure("graph1", graphPp1)
                              .AddInvokedClosure("graph1", graphPp2);
  auto invoked_pp = pp1.GetInvokedClosures();
  ASSERT_EQ(invoked_pp.size(), 1);

  auto pp2 = FunctionPp("pp2").AddInvokedClosure("graph1", graphPp1)
                              .AddInvokedClosure("graph2", graphPp2);
  auto invoked_pp2 = pp2.GetInvokedClosures();
  ASSERT_EQ(invoked_pp2.size(), 2);

  ge::AscendString str;
  pp2.Serialize(str);
  std::string tar_str(str.GetString(), str.GetLength());
  auto process_point = dataflow::ProcessPoint();
  (void)process_point.ParseFromString(tar_str);
  ASSERT_EQ(strcmp(pp2.GetProcessPointName(), "pp2"), 0);
  ASSERT_EQ(pp1.GetProcessPointType(), ProcessPointType::FUNCTION);

  auto invoke_pps = process_point.invoke_pps();
  auto invoke_pp0 = invoke_pps["graph1"];
  ASSERT_EQ(invoke_pp0.name(), "graphPp_1");
  ASSERT_EQ(invoke_pp0.type(), dataflow::ProcessPoint_ProcessPointType_GRAPH);
  ASSERT_EQ(invoke_pp0.compile_cfg_file(), "./graph.json");
  ASSERT_EQ(invoke_pp0.graphs(0), "graphPp_1");

  auto invoke_pp1 = invoke_pps["graph2"];
  ASSERT_EQ(invoke_pp1.name(), "graphPp_2");
  ASSERT_EQ(invoke_pp1.type(), dataflow::ProcessPoint_ProcessPointType_GRAPH);
  ASSERT_EQ(invoke_pp1.compile_cfg_file(), "./graph2.json");
  ASSERT_EQ(invoke_pp1.graphs(0), "graphPp_2");
}
} // namespace ge