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

#include "float_serializer.h"
#include "proto/ge_ir.pb.h"
#include "graph/debug/ge_log.h"
#include "graph/types.h"

namespace ge {
graphStatus FloatSerializer::Serialize(const AnyValue &av, proto::AttrDef &def) {
  float32_t val;
  const graphStatus ret = av.GetValue(val);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "Failed to get float attr.");
    return GRAPH_FAILED;
  }
  def.set_f(val);
  return GRAPH_SUCCESS;
}

graphStatus FloatSerializer::Deserialize(const proto::AttrDef &def, AnyValue &av) {
  return av.SetValue(def.f());
}

REG_GEIR_SERIALIZER(float_serializer, FloatSerializer, GetTypeId<float>(), proto::AttrDef::kF);
}  // namespace ge
