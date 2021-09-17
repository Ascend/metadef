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

#include "list_value_serializer.h"
#include <vector>
#include <string>
#include <functional>

#include "graph/debug/ge_log.h"
#include "graph/types.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_util.h"
#include "tensor_desc_serializer.h"
#include "tensor_serializer.h"
#include "named_attrs_serializer.h"
#include "graph_serializer.h"
#include "graph/ge_tensor.h"
#include "utils/serialization_util.h"

namespace ge {
using ComputeGraphPtr = std::shared_ptr<ComputeGraph>;
using GeTensorPtr = std::shared_ptr<GeTensor>;
using ListValue = proto::AttrDef::ListValue;
using std::placeholders::_1;
using std::placeholders::_2;

graphStatus ListValueSerializer::Serialize(const AnyValue &av, proto::AttrDef &def) {
  const static std::map<AnyValue::ValueType, std::function<graphStatus(const AnyValue &, proto::AttrDef &)>>
      type_serializer_map =
      {{AnyValue::VT_LIST_INT, std::bind(&ListValueSerializer::SerializeListInt, this, _1, _2)},
       {AnyValue::VT_LIST_FLOAT, std::bind(&ListValueSerializer::SerializeListFloat, this, _1, _2)},
       {AnyValue::VT_LIST_BOOL, std::bind(&ListValueSerializer::SerializeListBool, this, _1, _2)},
       {AnyValue::VT_LIST_BYTES, std::bind(&ListValueSerializer::SerializeListBuffer, this, _1, _2)},
       {AnyValue::VT_LIST_DATA_TYPE, std::bind(&ListValueSerializer::SerializeListDataType, this, _1, _2)},
       {AnyValue::VT_LIST_STRING, std::bind(&ListValueSerializer::SerializeListString, this, _1, _2)},
       {AnyValue::VT_LIST_NAMED_ATTRS, std::bind(&ListValueSerializer::SerializeListNamedAttrs, this, _1, _2)},
       {AnyValue::VT_LIST_TENSOR_DESC, std::bind(&ListValueSerializer::SerializeListGeTensorDesc, this, _1, _2)},
       {AnyValue::VT_LIST_TENSOR, std::bind(&ListValueSerializer::SerializeListGeTensor, this, _1, _2)},
       {AnyValue::VT_LIST_GRAPH, std::bind(&ListValueSerializer::SerializeListGraphDef, this, _1, _2)},
      };

  auto iter = type_serializer_map.find(av.GetValueType());
  if (iter == type_serializer_map.end()) {
    GELOGE(GRAPH_FAILED, "Value type [%d] not support.", static_cast<int32_t>(av.GetValueType()));
    return GRAPH_FAILED;
  }
  return iter->second(av, def);
}
graphStatus ListValueSerializer::Deserialize(const proto::AttrDef &def, AnyValue &av) {
  const static std::map<ListValue::ListValueType, std::function<graphStatus(const proto::AttrDef &def, AnyValue &av)>>
      type_deserializer_map =
      {{ListValue::VT_LIST_INT, std::bind(&ListValueSerializer::DeserializeListInt, this, _1, _2)},
       {ListValue::VT_LIST_FLOAT, std::bind(&ListValueSerializer::DeserializeListFloat, this, _1, _2)},
       {ListValue::VT_LIST_STRING, std::bind(&ListValueSerializer::DeserializeListString, this, _1, _2)},
       {ListValue::VT_LIST_BYTES, std::bind(&ListValueSerializer::DeserializeListBuffer, this, _1, _2)},
       {ListValue::VT_LIST_BOOL, std::bind(&ListValueSerializer::DeserializeListBool, this, _1, _2)},
       {ListValue::VT_LIST_DATA_TYPE, std::bind(&ListValueSerializer::DeserializeListDataType, this, _1, _2)},
       {ListValue::VT_LIST_NAMED_ATTRS, std::bind(&ListValueSerializer::DeserializeListNamedAttrs, this, _1, _2)},
       {ListValue::VT_LIST_TENSOR_DESC, std::bind(&ListValueSerializer::DeserializeListGeTensorDesc, this, _1, _2)},
       {ListValue::VT_LIST_TENSOR, std::bind(&ListValueSerializer::DeserializeListGeTensor, this, _1, _2)},
       {ListValue::VT_LIST_GRAPH, std::bind(&ListValueSerializer::DeserializeListGraphDef, this, _1, _2)},
      };

  auto iter = type_deserializer_map.find(def.list().val_type());
  if (iter == type_deserializer_map.end()) {
    GELOGE(GRAPH_FAILED, "Value type [%d] not support.", static_cast<int32_t>(def.list().val_type()));
    return GRAPH_FAILED;
  }
  return iter->second(def, av);
}

graphStatus ListValueSerializer::SerializeListInt(const AnyValue &av, proto::AttrDef &def) {
  std::vector<int64_t> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_int attr.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_i();
  for (auto value:list_value) {
    mutable_list->add_i(value);
  }
  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_INT);
  return GRAPH_SUCCESS;
}

graphStatus ListValueSerializer::SerializeListString(const AnyValue &av, proto::AttrDef &def) {
  std::vector<std::string> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_string attr.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_s();
  for (const auto &value:list_value) {
    mutable_list->add_s(value);
  }
  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_STRING);
  return GRAPH_SUCCESS;
}

graphStatus ListValueSerializer::SerializeListFloat(const AnyValue &av, proto::AttrDef &def) {
  std::vector<float> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_float attr.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_f();
  for (auto value:list_value) {
    mutable_list->add_f(value);
  }
  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_FLOAT);

  return GRAPH_SUCCESS;
}

graphStatus ListValueSerializer::SerializeListBool(const AnyValue &av, proto::AttrDef &def) {
  std::vector<bool> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_bool attr.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_b();
  for (auto value:list_value) {
    mutable_list->add_b(value);
  }
  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_BOOL);

  return GRAPH_SUCCESS;
}

graphStatus ListValueSerializer::SerializeListGeTensorDesc(const AnyValue &av, proto::AttrDef &def) {
  std::vector<ge::GeTensorDesc> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_tensor_desc attr.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_td();
  for (const auto &value : list_value) {
    auto attr_proto = mutable_list->add_td();
    GE_CHECK_NOTNULL(attr_proto);
    GeTensorSerializeUtils::GeTensorDescAsProto(value, attr_proto);
  }

  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_TENSOR_DESC);

  return GRAPH_SUCCESS;
}

graphStatus ListValueSerializer::SerializeListGeTensor(const AnyValue &av, proto::AttrDef &def) {
  std::vector<GeTensor> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_tensor attr_value.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_t();
  for (const auto &value : list_value) {
    auto attr_proto = mutable_list->add_t();
    GE_CHECK_NOTNULL(attr_proto);
    GeTensorSerializeUtils::GeTensorAsProto(value, attr_proto);
  }

  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_TENSOR);

  return GRAPH_SUCCESS;
}
graphStatus ListValueSerializer::SerializeListBuffer(const AnyValue &av, proto::AttrDef &def) {
  std::vector<Buffer> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_buffer attr.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_bt();
  for (auto value : list_value) {
    if (value.GetData() != nullptr && value.size() > 0) {
      mutable_list->add_bt(value.GetData(), value.GetSize());
    }
  }
  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_BYTES);

  return GRAPH_SUCCESS;
}

graphStatus ListValueSerializer::SerializeListGraphDef(const AnyValue &av, proto::AttrDef &def) {
  std::vector<proto::GraphDef> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_graph attr_value.");
    return GRAPH_FAILED;
  }

  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_g();
  for (const auto &graph : list_value) {
    auto mutable_graph = mutable_list->add_g();
    GE_CHECK_NOTNULL(mutable_graph);
    *mutable_graph = graph;
  }

  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_GRAPH);

  return GRAPH_SUCCESS;
}

graphStatus ListValueSerializer::SerializeListNamedAttrs(const AnyValue &av, proto::AttrDef &def) {
  std::vector<ge::NamedAttrs> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_named_attr attr.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_na();
  auto attr_serializer = AttrSerializerRegistry::GetInstance().GetSerializer(GetTypeId<ge::NamedAttrs>());
  auto named_attr_serializer = dynamic_cast<NamedAttrsSerializer *>(attr_serializer);
  GE_CHECK_NOTNULL(named_attr_serializer);

  for (const auto &value : list_value) {
    auto attr_proto = mutable_list->add_na();
    GE_CHECK_NOTNULL(attr_proto);
    if (named_attr_serializer->Serialize(value, attr_proto) != GRAPH_SUCCESS) {
      GELOGE(GRAPH_FAILED, "NamedAttr [%s] serialize failed.", value.GetName().c_str());
      return GRAPH_FAILED;
    }
  }

  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_NAMED_ATTRS);

  return GRAPH_SUCCESS;
}
graphStatus ListValueSerializer::SerializeListDataType(const AnyValue &av, proto::AttrDef &def) {
  std::vector<ge::DataType> list_value;
  graphStatus ret = av.GetValue(list_value);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get list_datatype attr.");
    return GRAPH_FAILED;
  }
  auto mutable_list = def.mutable_list();
  GE_CHECK_NOTNULL(mutable_list);
  mutable_list->clear_dt();
  for (auto value : list_value) {
    proto::DataType dt;
    SerializationUtil::GeDataTypeToProto(value, dt);
    mutable_list->add_dt(dt);
  }
  mutable_list->set_val_type(proto::AttrDef::ListValue::VT_LIST_DATA_TYPE);

  return GRAPH_SUCCESS;
}

graphStatus ListValueSerializer::DeserializeListInt(const proto::AttrDef &def, AnyValue &av) {
  std::vector<int64_t> values(def.list().i_size());
  for (int idx = 0; idx < def.list().i_size(); ++idx) {
    values[idx] = def.list().i(idx);
  }
  return av.SetValue(std::move(values));
}

graphStatus ListValueSerializer::DeserializeListString(const proto::AttrDef &def, AnyValue &av) {
  std::vector<std::string> values(def.list().s_size());
  for (int idx = 0; idx < def.list().s_size(); ++idx) {
    values[idx] = def.list().s(idx);
  }
  return av.SetValue(std::move(values));
}

graphStatus ListValueSerializer::DeserializeListFloat(const proto::AttrDef &def, AnyValue &av) {
  std::vector<float> values(def.list().f_size());
  for (int idx = 0; idx < def.list().f_size(); ++idx) {
    values[idx] = def.list().f(idx);
  }

  return av.SetValue(std::move(values));
}

graphStatus ListValueSerializer::DeserializeListBool(const proto::AttrDef &def, AnyValue &av) {
  std::vector<bool> values(def.list().b_size());
  for (int idx = 0; idx < def.list().b_size(); ++idx) {
    values[idx] = def.list().b(idx);
  }
  return av.SetValue(std::move(values));
}

graphStatus ListValueSerializer::DeserializeListGeTensorDesc(const proto::AttrDef &def, AnyValue &av) {
  std::vector<ge::GeTensorDesc> values(def.list().td_size());
  for (int idx = 0; idx < def.list().td_size(); ++idx) {
    GeTensorSerializeUtils::AssembleGeTensorDescFromProto(&def.list().td(idx), values[idx]);
  }

  return av.SetValue(std::move(values));
}

graphStatus ListValueSerializer::DeserializeListGeTensor(const proto::AttrDef &def, AnyValue &av) {
  std::vector<GeTensor> values(def.list().t_size());
  for (int idx = 0; idx < def.list().t_size(); ++idx) {
    GeTensorSerializeUtils::AssembleGeTensorFromProto(&def.list().t(idx), values[idx]);
  }

  return av.SetValue(std::move(values));
}

graphStatus ListValueSerializer::DeserializeListBuffer(const proto::AttrDef &def, AnyValue &av) {
  std::vector<Buffer> values(def.list().bt_size());
  for (int idx = 0; idx < def.list().bt_size(); ++idx) {
    values[idx] =
        Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(def.list().bt(idx).data()), def.list().bt(idx).size());
  }

  return av.SetValue(std::move(values));
}
graphStatus ListValueSerializer::DeserializeListGraphDef(const proto::AttrDef &def, AnyValue &av) {
  std::vector<proto::GraphDef> values(def.list().g_size());
  for (int idx = 0; idx < def.list().g_size(); ++idx) {
    values[idx] = def.list().g(idx);
  }
  return av.SetValue(std::move(values));
}

graphStatus ListValueSerializer::DeserializeListNamedAttrs(const proto::AttrDef &def, AnyValue &av) {
  auto attr_deserializer = AttrSerializerRegistry::GetInstance().
      GetDeserializer(proto::AttrDef::ValueCase::kFunc);
  auto named_attr_deserializer = dynamic_cast<NamedAttrsSerializer *>(attr_deserializer);
  GE_CHECK_NOTNULL(named_attr_deserializer);

  std::vector<ge::NamedAttrs> values(def.list().na_size());
  for (int idx = 0; idx < def.list().na_size(); ++idx) {
    if (named_attr_deserializer->Deserialize(def.list().na(idx), values[idx]) != GRAPH_SUCCESS) {
      GELOGE(GRAPH_FAILED, "NamedAttr [%s] deserialize failed.", def.list().na(idx).name().c_str());
      return GRAPH_FAILED;
    }
  }

  return av.SetValue(std::move(values));
}
graphStatus ListValueSerializer::DeserializeListDataType(const proto::AttrDef &def, AnyValue &av) {
  std::vector<ge::DataType> values(def.list().dt_size());
  for (int idx = 0; idx < def.list().dt_size(); ++idx) {
    SerializationUtil::ProtoDataTypeToGe(static_cast<proto::DataType>(def.list().dt(idx)), values[idx]);
  }

  return av.SetValue(std::move(values));
}

REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<int64_t>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<std::string>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<float>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<bool>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<GeTensorDesc>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<GeTensor>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<Buffer>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<proto::GraphDef>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<ge::NamedAttrs>>(), proto::AttrDef::kList);
REG_GEIR_SERIALIZER(ListValueSerializer, GetTypeId<std::vector<ge::DataType>>(), proto::AttrDef::kList);
}  // namespace ge
