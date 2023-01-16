/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#include "flow_graph/process_point.h"
#include "common/checker.h"
#include "debug/ge_util.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/serialization/attr_serializer_registry.h"
#include "proto/dflow.pb.h"

namespace ge {
namespace dflow {
class ProcessPointImpl {
public:
  ProcessPointImpl(const char *pp_name, ProcessPointType pp_type) : pp_name_(pp_name), pp_type_(pp_type),
                   json_file_path_() {}
  ~ProcessPointImpl() = default;

  ProcessPointType GetProcessPointType() const {
    return pp_type_;
  }

  const char *GetProcessPointName() const {
    return pp_name_.c_str();
  }

  void SetCompileConfig(const char *json_file_path) {
    json_file_path_ = json_file_path;
  }

  const char *GetCompileConfig() const {
    return json_file_path_.c_str();
  }

private:
  std::string pp_name_;
  const ProcessPointType pp_type_;
  std::string json_file_path_;
};

ProcessPoint::ProcessPoint(const char *pp_name, ProcessPointType pp_type) {
  if (pp_name == nullptr) {
    impl_ = nullptr;
    GELOGE(ge::FAILED, "ProcessPoint name is nullptr.");
  } else {
    impl_ = std::make_shared<ProcessPointImpl>(pp_name, pp_type);
    if (impl_ == nullptr) {
      GELOGE(ge::FAILED, "ProcessPointImpl make shared failed.");
    }
  }
}
ProcessPoint::~ProcessPoint() {}

ProcessPointType ProcessPoint::GetProcessPointType() const {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] ProcessPointImpl is nullptr, check failed");
    return ProcessPointType::INVALID;
  }
  return impl_->GetProcessPointType();
}

const char *ProcessPoint::GetProcessPointName() const {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] ProcessPointImpl is nullptr, check failed");
    return nullptr;
  }
  return impl_->GetProcessPointName();
}

void ProcessPoint::SetCompileConfig(const char *json_file_path) {
  GE_RETURN_IF_NULL(impl_, "[Check][Param] ProcessPointImpl is nullptr, check failed");
  GE_RETURN_IF_NULL(json_file_path, "[Check][Param] ProcessPoint(%s)'s compile config json is nullptr.",
                    this->GetProcessPointName());
  return impl_->SetCompileConfig(json_file_path);
}

const char *ProcessPoint::GetCompileConfig() const {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] ProcessPointImpl is nullptr, check failed");
    return nullptr;
  }
  return impl_->GetCompileConfig();
}

class GraphPpImpl {
public:
  GraphPpImpl(const char *pp_name, const GraphBuilder &builder) : pp_name_(pp_name), builder_(builder) {}
  ~GraphPpImpl() = default;

  GraphBuilder GetGraphBuilder() const {
    auto builder = builder_;
    auto pp_name = pp_name_;
    GraphBuilder GraphBuild = [builder, pp_name]() {
      auto graph = builder();
      auto compute_graph = ge::GraphUtilsEx::GetComputeGraph(graph);
      if (compute_graph == nullptr) {
        GELOGE(ge::FAILED, "graph is invalid.");
        return graph;
      }
      compute_graph->SetName(pp_name);
      return graph;
    };
    return GraphBuild;
  }
private:
  std::string pp_name_;
  GraphBuilder builder_;
};

GraphPp::GraphPp(const char *pp_name, const GraphBuilder &builder) : ProcessPoint(pp_name, ProcessPointType::GRAPH) {
  if (builder == nullptr) {
    GELOGE(ge::FAILED, "GraphPp(%s) graph builder is null.", this->GetProcessPointName());
    impl_ = nullptr;
  } else {
    impl_ = std::make_shared<GraphPpImpl>(pp_name, builder);
    if (impl_ == nullptr) {
      GELOGE(ge::FAILED, "GraphPpImpl make shared failed.");
    }
  }
}
GraphPp::~GraphPp() = default;

GraphPp &GraphPp::SetCompileConfig(const char *json_file_path) {
  if (json_file_path == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] ProcessPoint(%s)'s compile config json is nullptr.",
           this->GetProcessPointName());
    return *this;
  }

  ProcessPoint::SetCompileConfig(json_file_path);
  GELOGI("SetCompileConfig, json_file_path=%s.", json_file_path);
  return *this;
}

void GraphPp::Serialize(ge::AscendString &str) const {
  dataflow::ProcessPoint process_point;
  process_point.set_name(this->GetProcessPointName());
  process_point.set_type(dataflow::ProcessPoint_ProcessPointType_GRAPH);
  process_point.set_compile_cfg_file(this->GetCompileConfig());
  process_point.add_graphs(this->GetProcessPointName());
  std::string target_str;
  process_point.SerializeToString(&target_str);
  str = ge::AscendString(target_str.c_str(), target_str.length());
  return;
}

GraphBuilder GraphPp::GetGraphBuilder() const {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] GraphPpImpl is nullptr, check failed");
    return nullptr;
  }
  return impl_->GetGraphBuilder();
}

class FunctionPpImpl {
public:
  FunctionPpImpl() = default;
  ~FunctionPpImpl() = default;

  void AddInvokedClosure(const char *name, const GraphPp graph_pp) {
    GE_RETURN_IF_NULL(name, "[Check][Param] AddInvokedClosure failed for name is nullptr.");
    GE_RETURN_IF_TRUE(invoked_closures_.find(name) != invoked_closures_.end(),
                      "AddInvokedClosure failed for duplicate name(%s).", name);

    invoked_closures_.emplace(name, std::move(graph_pp));
    GELOGI("AddInvokedClosure key(%s), pp name(%s).", name, graph_pp.GetProcessPointName());
    return;
  }

  const std::map<const std::string, const GraphPp> &GetInvokedClosures() const {
    return invoked_closures_;
  }

  template<typename T>
  bool SetAttrValue(const char *name, T &&value) {
    return attrs_.SetByName(name, std::forward<T>(value));
  }

  const ge::AttrStore &GetAttrMap() const {
    return attrs_;
  }

  void UpdataProcessPoint(dataflow::ProcessPoint &process_point) {
    AddInvokedPps(process_point);
    AddFunctionPpInitPara(process_point);
  }
private:
  void AddInvokedPps(dataflow::ProcessPoint &process_point);
  void AddFunctionPpInitPara(dataflow::ProcessPoint &process_point);
  ge::AttrStore attrs_;
  std::map<const std::string, const GraphPp> invoked_closures_;
};

void FunctionPpImpl::AddInvokedPps(dataflow::ProcessPoint &process_point) {
  for (auto iter = invoked_closures_.cbegin(); iter != invoked_closures_.cend(); ++iter) {
    auto invoke_pps = process_point.mutable_invoke_pps();
    GE_RT_VOID_CHECK_NOTNULL(invoke_pps);
    const GraphPp &graph_pp = iter->second;
    dataflow::ProcessPoint invoked_pp;
    invoked_pp.set_name(graph_pp.GetProcessPointName());
    invoked_pp.set_type(dataflow::ProcessPoint_ProcessPointType_GRAPH);
    invoked_pp.set_compile_cfg_file(graph_pp.GetCompileConfig());

    const auto builder = graph_pp.GetGraphBuilder();
    if (builder == nullptr) {
      GELOGE(ge::FAILED, "GraphBuilder is null.");
      return;
    }

    invoked_pp.add_graphs(graph_pp.GetProcessPointName());
    (*invoke_pps)[iter->first] = std::move(invoked_pp);
    GELOGI("Add invoke pp success. key:%s, invoked pp name:%s", (iter->first).c_str(), graph_pp.GetProcessPointName());
  }
  return;
}

void FunctionPpImpl::AddFunctionPpInitPara(dataflow::ProcessPoint &process_point) {
  auto init_attr = process_point.mutable_attrs();
  const auto attrs = attrs_.GetAllAttrs();
  for (const auto &attr : attrs) {
    const AnyValue attr_value = attr.second;
    const auto serializer = AttrSerializerRegistry::GetInstance().GetSerializer(attr_value.GetValueTypeId());
    GE_RT_VOID_CHECK_NOTNULL(serializer);

    proto::AttrDef attr_def;
    if (serializer->Serialize(attr_value, attr_def) != GRAPH_SUCCESS) {
      GELOGE(ge::FAILED, "Attr serialized failed, name:[%s].", attr.first.c_str());
      return;
    }
    (*init_attr)[attr.first] = attr_def;
  }
  return;
}

FunctionPp::FunctionPp(const char *pp_name) : ProcessPoint(pp_name, ProcessPointType::FUNCTION) {
  impl_ = std::make_shared<FunctionPpImpl>();
  if (impl_ == nullptr) {
    GELOGW("FunctionPpImpl make shared failed.");
  }
}
FunctionPp::~FunctionPp() = default;

FunctionPp &FunctionPp::SetCompileConfig(const char *json_file_path) {
  if (json_file_path == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] ProcessPoint(%s)'s compile config json is nullptr.",
           this->GetProcessPointName());
    return *this;
  }

  ProcessPoint::SetCompileConfig(json_file_path);
  GELOGI("SetCompileConfig, json_file_path=%s.", json_file_path);
  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const ge::AscendString &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  std::string str_value(value.GetString());
  if (!impl_->SetAttrValue(attr_name, str_value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const char *value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (value == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] Set init param value is nullptr, check failed.");
    return *this;
  }

  std::string str_value(value);
  if (!impl_->SetAttrValue(attr_name, str_value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const std::vector<ge::AscendString> &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  std::vector<std::string> vec_value;
  for (auto iter = value.cbegin(); iter != value.cend(); iter++) {
    vec_value.emplace_back(iter->GetString());
  }
  if (!impl_->SetAttrValue(attr_name, vec_value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const int64_t &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const std::vector<int64_t> &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const std::vector<std::vector<int64_t>> &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const float &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const std::vector<float> &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const bool &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const std::vector<bool> &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const ge::DataType &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::SetInitParam(const char *attr_name, const std::vector<ge::DataType> &value) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  if (!impl_->SetAttrValue(attr_name, value)) {
    GELOGE(ge::FAILED, "set attr name(%s) failed.", attr_name);
  }

  return *this;
}

FunctionPp &FunctionPp::AddInvokedClosure(const char *name, const GraphPp &graph_pp) {
  if (impl_ == nullptr) {
    GELOGE(ge::FAILED, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
    return *this;
  }

  impl_->AddInvokedClosure(name, graph_pp);
  return *this;
}

const std::map<const std::string, const GraphPp> &FunctionPp::GetInvokedClosures() const {
  if (impl_ == nullptr) {
    static std::map<const std::string, const GraphPp> empty_map;
    return empty_map;
  }

  return impl_->GetInvokedClosures();
}

void FunctionPp::Serialize(ge::AscendString &str) const {
  GE_RETURN_IF_NULL(impl_, "[Check][Param] FunctionPpImpl is nullptr, check failed.");
  dataflow::ProcessPoint process_point;
  process_point.set_name(this->GetProcessPointName());
  process_point.set_type(dataflow::ProcessPoint_ProcessPointType_FUNCTION);
  process_point.set_compile_cfg_file(this->GetCompileConfig());
  impl_->UpdataProcessPoint(process_point);
  std::string target_str;
  process_point.SerializeToString(&target_str);
  str = ge::AscendString(target_str.c_str(), target_str.length());
  return;
}
}  // namespace dflow
}  // namespace ge