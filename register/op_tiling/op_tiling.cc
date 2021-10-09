/**
 * Copyright Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
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

#include "register/op_tiling.h"

#include <nlohmann/json.hpp>
#include "common/util/error_manager/error_manager.h"
#include "external/graph/operator.h"
#include "graph/debug/ge_log.h"
#include "graph/debug/ge_util.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "op_tiling/op_tiling_constants.h"
#include "op_tiling/op_tiling_utils.h"

namespace optiling {
using Status = domi::Status;
using DataBuf = std::tuple<const uint8_t *, size_t>;
using OpCompileInfoV2 = utils::OpCompileInfo;
using utils::OpTilingFuncV2;
using utils::OpTilingRegistryInterf_V2;

class AnyValueBase {
public:
  virtual ~AnyValueBase() = default;
  virtual DataBuf GetDataBuf() const = 0;
};

template<typename VT>
class AnyValue : public AnyValueBase {
public:
  explicit AnyValue(const VT &value) : value_(value) {}
  ~AnyValue() override = default;
  DataBuf GetDataBuf() const override {
    return DataBuf(reinterpret_cast<const uint8_t *>(&value_), sizeof(value_));
  }

private:
  VT value_;
};

template<typename VT>
class AnyVecValue : public AnyValueBase {
public:
  explicit AnyVecValue(std::vector<VT> &value) : value_(std::move(value)) {}
  ~AnyVecValue() override = default;
  DataBuf GetDataBuf() const override {
    return DataBuf(reinterpret_cast<const uint8_t *>(value_.data()), sizeof(VT) * value_.size());
  }

private:
  std::vector<VT> value_;
};

template<typename T, typename Enabled = void>
struct Getter;

template<typename T>
struct Getter<T, typename std::enable_if<std::is_integral<T>::value>::type> {
  using ST = int64_t;
  static constexpr bool (*func)(ge::AttrUtils::ConstAttrHolderAdapter &&, const string &,
                                int64_t &) = ge::AttrUtils::GetInt;
  static constexpr bool (*list_func)(ge::AttrUtils::ConstAttrHolderAdapter &&, const string &,
                                     vector<int64_t> &) = ge::AttrUtils::GetListInt;
};
template<typename T>
struct Getter<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
  using ST = float;
  static constexpr bool (*func)(ge::AttrUtils::ConstAttrHolderAdapter &&, const string &,
                                float &) = ge::AttrUtils::GetFloat;
  static constexpr bool (*list_func)(ge::AttrUtils::ConstAttrHolderAdapter &&, const string &,
                                     vector<float> &) = ge::AttrUtils::GetListFloat;
};

class TeOpVarAttrArgsImpl {
  using DataKeyType = std::pair<std::string, std::string>;

public:
  explicit TeOpVarAttrArgsImpl(const ge::OpDescPtr &op_desc) : op_desc_(op_desc){};
  ~TeOpVarAttrArgsImpl() = default;

  Status GetDataByName(const string &name, const string &dtype, DataBuf &data);

private:
  template<typename T>
  Status GetNodeAttrDataIntListList(const std::string &name, DataBuf &data) {
    std::vector<std::vector<int64_t>> value;
    bool res = ge::AttrUtils::GetListListInt(op_desc_, name, value);
    if (!res) {
      GE_LOGE("attr not found. %s", name.c_str());
      return domi::FAILED;
    }

    std::vector<T> dest;
    for (const auto &vec : value) {
      for (auto elem : vec) {
        dest.emplace_back(static_cast<T>(elem));
      }
    }
    auto dest_ptr = std::make_shared<AnyVecValue<T>>(dest);
    data_map_.emplace(name + '_' + typeid(T).name(), dest_ptr);
    data = dest_ptr->GetDataBuf();
    GELOGI("IntListList attr found. %s", name.c_str());
    return domi::SUCCESS;
  }

  template<typename T, bool IsList = false, typename std::enable_if<!IsList, bool>::type = true>
  Status GetNodeAttrDataTmpl(const std::string &name, DataBuf &data) {
    auto func = Getter<T>::func;
    typename Getter<T>::ST value;
    bool res = func(op_desc_, name, value);
    if (!res) {
      GE_LOGE("attr not found. %s", name.c_str());
      return domi::FAILED;
    }

    auto dest_ptr = std::make_shared<AnyValue<T>>(static_cast<T>(value));
    data_map_.emplace(name + '_' + typeid(T).name(), dest_ptr);
    data = dest_ptr->GetDataBuf();
    GELOGI("Single attr found. %s", name.c_str());
    return domi::SUCCESS;
  }

  template<typename T, bool IsList = false, typename std::enable_if<IsList, bool>::type = true>
  Status GetNodeAttrDataTmpl(const std::string &name, DataBuf &data) {
    auto func = Getter<T>::list_func;
    std::vector<typename Getter<T>::ST> value;
    bool res = func(op_desc_, name, value);
    if (!res) {
      GE_LOGE("List attr not found. %s", name.c_str());
      return domi::FAILED;
    }

    std::vector<T> dest;
    for (auto elem : value) {
      dest.emplace_back(static_cast<T>(elem));
    }
    auto dest_ptr = std::make_shared<AnyVecValue<T>>(dest);
    data_map_.emplace(name + '_' + typeid(T).name(), dest_ptr);
    data = dest_ptr->GetDataBuf();
    GELOGI("attr found. %s", name.c_str());
    return domi::SUCCESS;
  }

private:
  static std::map<std::string, std::function<Status(TeOpVarAttrArgsImpl *, const std::string &, DataBuf &)>>
          data_getter_;
  ge::OpDescPtr op_desc_;
  std::map<std::string, std::shared_ptr<AnyValueBase>> data_map_;
};

std::map<std::string, std::function<Status(TeOpVarAttrArgsImpl *, const std::string &, DataBuf &)>>
        TeOpVarAttrArgsImpl::data_getter_ = {{"Int8", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<int8_t>},
                                             {"Int16", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<int16_t>},
                                             {"Int32", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<int32_t>},
                                             {"Int64", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<int64_t>},
                                             {"UInt8", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<uint8_t>},
                                             {"UInt16", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<uint16_t>},
                                             {"UInt32", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<uint32_t>},
                                             {"UInt64", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<uint64_t>},
                                             {"Float", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<float>},
                                             {"ListInt8", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<int8_t, true>},
                                             {"ListInt16", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<int16_t, true>},
                                             {"ListInt32", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<int32_t, true>},
                                             {"ListInt64", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<int64_t, true>},
                                             {"ListUInt8", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<uint8_t, true>},
                                             {"ListUInt16", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<uint16_t, true>},
                                             {"ListUInt32", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<uint32_t, true>},
                                             {"ListUInt64", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<uint64_t, true>},
                                             {"ListFloat", &TeOpVarAttrArgsImpl::GetNodeAttrDataTmpl<float, true>}};

Status TeOpVarAttrArgsImpl::GetDataByName(const std::string &name, const std::string &dtype, DataBuf &data) {
  auto iter = data_getter_.find(dtype);
  if (iter == data_getter_.end()) {
    GE_LOGE("wrong dtype: %s", dtype.c_str());
    return domi::FAILED;
  } else {
    return iter->second(this, name, data);
  }
}

const uint8_t *TeOpVarAttrArgs::GetData(const std::string &name, const std::string &dtype, size_t &size) const {
  DataBuf data(nullptr, 0);
  auto rc = impl_->GetDataByName(name, dtype, data);
  if (rc == domi::SUCCESS) {
    GELOGI("attr found. %s, %s, %p, %ld", name.c_str(), dtype.c_str(), std::get<0>(data), std::get<1>(data));
  }
  size = std::get<1>(data);
  return std::get<0>(data);
}

class VarAttrHelper {
public:
  static bool InitTeOpVarAttr(const ge::OpDescPtr &op_desc_ptr, TeOpVarAttrArgs &attr) {
    OP_TILING_MAKE_SHARED(attr.impl_ = std::make_shared<TeOpVarAttrArgsImpl>(op_desc_ptr), return false);
    return true;
  }
};

bool FeedTeOpTensorArg(ge::OpDesc::Vistor<ge::GeTensorDescPtr> &tensor_desc_vec,
                       std::vector<TeOpTensorArg> &tensor_arg, ge::OpDescPtr &op_desc) {
  size_t index = 0;
  for (ge::GeTensorDescPtr &tensor_desc_ptr : tensor_desc_vec) {
    TeOpTensorArg arg_tensor;
    TeOpTensor tensor;
    arg_tensor.arg_type = TA_SINGLE;
    tensor.shape = tensor_desc_ptr->MutableShape().GetDims();
    if (tensor.shape.empty()) {
      tensor.shape = {1};
    }
    tensor.ori_shape = tensor_desc_ptr->GetOriginShape().GetDims();
    tensor.name = op_desc->GetInputNameByIndex(index);

    ge::Format primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(tensor_desc_ptr->GetFormat()));
    tensor.format = ge::TypeUtils::FormatToSerialString(primary_format);
    tensor.ori_format = ge::TypeUtils::FormatToSerialString(tensor_desc_ptr->GetOriginFormat());

    ge::DataType dtype = tensor_desc_ptr->GetDataType();
    auto dataTypeIter = DATATYPE_STRING_MAP.find(dtype);
    if (dataTypeIter == DATATYPE_STRING_MAP.end()) {
      GE_LOGE("datatype error %d", static_cast<int>(dtype));
      return false;
    }
    tensor.dtype = dataTypeIter->second;
    if (IsLogEnable(GE_MODULE_NAME, DLOG_INFO)) {
      std::stringstream shapestr;
      shapestr << "shape:[";
      for (auto &i : tensor.shape) {
        shapestr << i << ",";
      }
      shapestr << "], ori_shape:[";
      for (auto &i : tensor.ori_shape) {
        shapestr << i << ",";
      }
      shapestr << "], format:" << tensor.format;
      shapestr << ", ori_format:" << tensor.ori_format;
      shapestr << ", dtype: " << tensor.dtype;
      GELOGI("calling optiling shape info: %s", shapestr.str().c_str());
    }

    arg_tensor.tensor.emplace_back(tensor);
    tensor_arg.emplace_back(arg_tensor);
    index++;
  }
  return true;
}

void FeedTeOpConstTensor(const ge::Node &node, const ge::OpDescPtr &op_desc,
                         std::map<std::string, TeConstTensorData> &const_inputs) {
  ge::Operator op = ge::OpDescUtils::CreateOperatorFromNode(node.shared_from_this());
  std::vector<std::string> depend_names;
  (void)ge::AttrUtils::GetListStr(op_desc, ATTR_NAME_OP_INFER_DEPENDS, depend_names);
  for (const std::string &depend : depend_names) {
    ge::Tensor data;
    ge::graphStatus rc = op.GetInputConstData(depend.c_str(), data);
    GELOGI("GetInputConstData: %s, %d", depend.c_str(), rc);
    if (rc != ge::GRAPH_SUCCESS) {
      continue;
    }

    const uint8_t *pbuf = data.GetData();
    size_t buflen = data.GetSize();

    GELOGI("Const input tensor data: %s, %p %zu", depend.c_str(), pbuf, buflen);
    const_inputs.emplace(depend, TeConstTensorData{pbuf, buflen, data});
  }
}

ge::graphStatus OpParaCalculate(const ge::Node &node, OpRunInfo &run_info,
                                std::unordered_map<std::string, OpTilingFunc>::iterator iter) {
  ge::OpDescPtr op_desc = node.GetOpDesc();
  GELOGI("Do optiling, op_type:%s, op_name:%s", op_desc->GetType().c_str(), op_desc->GetName().c_str());
  TeOpParas op_param;
  op_param.op_type = op_desc->GetType();
  VarAttrHelper::InitTeOpVarAttr(op_desc, op_param.var_attrs);

  ge::OpDesc::Vistor<ge::GeTensorDescPtr> inputs = op_desc->GetAllInputsDescPtr();
  if (!FeedTeOpTensorArg(inputs, op_param.inputs, op_desc)) {
    GE_LOGE("Do optiling, op_type:%s, op_name:%s", op_desc->GetType().c_str(), op_desc->GetName().c_str());
    return ge::GRAPH_FAILED;
  }
  ge::OpDesc::Vistor<ge::GeTensorDescPtr> outputs = op_desc->GetAllOutputsDescPtr();
  if (!FeedTeOpTensorArg(outputs, op_param.outputs, op_desc)) {
    return ge::GRAPH_FAILED;
  }
  FeedTeOpConstTensor(node, op_desc, op_param.const_inputs);

  OpCompileInfo op_compile_info;
  if (!ge::AttrUtils::GetStr(op_desc, COMPILE_INFO_KEY, op_compile_info.key)) {
    GE_LOGE("Op[%s] does not have attr[%s].", op_desc->GetName().c_str(), COMPILE_INFO_KEY.c_str());
    return ge::GRAPH_FAILED;
  }
  if (!ge::AttrUtils::GetStr(op_desc, COMPILE_INFO_JSON, op_compile_info.str)) {
    GE_LOGE("Op[%s] does not have attr[%s].", op_desc->GetName().c_str(), COMPILE_INFO_JSON.c_str());
    return ge::GRAPH_FAILED;
  }

  bool ret = (iter->second)(op_param, op_compile_info, run_info);
  if (ret) {
    GELOGI("Optiling succeed. op_type:%s, op_name:%s", op_desc->GetType().c_str(), op_desc->GetName().c_str());
  } else {
    REPORT_CALL_ERROR("E19999", "Fail to call op tiling function of op[%s, %s].",
                      op_desc->GetType().c_str(), op_desc->GetName().c_str());
    GE_LOGE("Fail to call op tiling function of op[%s, %s].", op_desc->GetName().c_str(), op_desc->GetType().c_str());
  }
  return ret ? ge::GRAPH_SUCCESS : ge::GRAPH_FAILED;
}

ge::graphStatus TurnToOpParaCalculateV1(const ge::Node &node, OpRunInfoV2 &run_info,
                                        std::unordered_map<std::string, OpTilingFunc>::iterator iter) {
  OpRunInfo run_info_struct;
  run_info_struct.block_dim = run_info.GetBlockDim();
  run_info_struct.clear_atomic = run_info.GetClearAtomic();
  run_info_struct.tiling_key = run_info.GetTilingKey();
  if (OpParaCalculate(node, run_info_struct, iter) != ge::GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "OpParaCalculate failed, op_type[%s], op_name[%s]",
                      node.GetType().c_str(), node.GetName().c_str());
    return ge::GRAPH_FAILED;
  }

  run_info.SetBlockDim(run_info_struct.block_dim);
  run_info.SetClearAtomic(run_info_struct.clear_atomic);
  run_info.SetTilingKey(run_info_struct.tiling_key);
  run_info.InternelSetTiling(run_info_struct.tiling_data);
  if (!run_info_struct.workspaces.empty()) {
    for (const int64_t &workspace : run_info_struct.workspaces) {
      run_info.AddWorkspace(workspace);
    }
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus TurnToOpParaCalculateV2(const ge::Node &node, OpRunInfoV2 &run_info,
                                        std::unordered_map<std::string, OpTilingFuncV2>::iterator iter) {
  ge::OpDescPtr op_desc = node.GetOpDesc();
  GELOGI("Do optiling, op_type:%s, op_name:%s", op_desc->GetType().c_str(), op_desc->GetName().c_str());
  std::string op_compile_info_key;
  if (!ge::AttrUtils::GetStr(op_desc, COMPILE_INFO_KEY, op_compile_info_key)) {
    GE_LOGE("Op[%s] does not have attr[%s].", op_desc->GetName().c_str(), COMPILE_INFO_KEY.c_str());
    return ge::GRAPH_FAILED;
  }
  std::string op_compile_info_json;
  if (!ge::AttrUtils::GetStr(op_desc, COMPILE_INFO_JSON, op_compile_info_json)) {
    GE_LOGE("Op[%s] does not have attr[%s].", op_desc->GetName().c_str(), COMPILE_INFO_JSON.c_str());
    return ge::GRAPH_FAILED;
  }
  OpCompileInfoV2 op_compile_info(op_compile_info_key, op_compile_info_json);

  std::vector<int32_t> indexes;
  ReplaceEmptyShapeOfTensorDesc(op_desc, indexes);
  AddNameToTensordesc(op_desc);

  ge::Operator op_param = ge::OpDescUtils::CreateOperatorFromNode(node.shared_from_this());
  bool ret = (iter->second)(op_param, op_compile_info, run_info);
  if (ret) {
    GELOGI("Optiling succeed. op_type:%s, op_name:%s", op_desc->GetType().c_str(), op_desc->GetName().c_str());
  } else {
    REPORT_CALL_ERROR("E19999", "Fail to call op tiling function of op[%s, %s].",
                      op_desc->GetType().c_str(), op_desc->GetName().c_str());
    GE_LOGE("Fail to call op tiling function of op[%s, %s].", op_desc->GetName().c_str(), op_desc->GetType().c_str());
  }
  RecoveryEmptyShapeOfTensorDesc(op_desc, indexes);
  return ret ? ge::GRAPH_SUCCESS : ge::GRAPH_FAILED;
}

extern "C" ge::graphStatus OpParaCalculateV2(const ge::Node &node, OpRunInfoV2 &run_info) {
  std::string op_type = node.GetType();
  auto &interf_v2 = OpTilingRegistryInterf_V2::RegisteredOpInterf();
  auto &interf_v1 = OpTilingRegistryInterf::RegisteredOpInterf();
  bool v2_flag = true;
  auto iter_2 = interf_v2.find(op_type);
  auto iter_1 = interf_v1.end();
  if (iter_2 == interf_v2.end()) {
    GELOGI("Optiling func of op[%s] is not found in V2, try to find it in V1.", op_type.c_str());
    v2_flag = false;
    iter_1 = interf_v1.find(op_type);
    if (iter_1 == interf_v1.end()) {
      GELOGI("Optiling func of op[%s] not found in V1, try to find it in V2 by Autotiling.", op_type.c_str());
      iter_2 = interf_v2.find(OP_TYPE_AUTO_TILING);
      v2_flag = true;
      if (iter_2 == interf_v2.end()) {
        GELOGI("Optiling func of op[%s] is not found in V2 by Autotiling, try to find it in V1 by Autotiling.",
               op_type.c_str());
        iter_1 = interf_v1.find(OP_TYPE_AUTO_TILING);
        v2_flag = false;
        if (iter_1 == interf_v1.end()) {
          REPORT_CALL_ERROR("E19999", "Optiling func not found. op_type:%s", op_type.c_str());
          return ge::GRAPH_FAILED;
        }
      }
    }
  }
  ge::graphStatus ret = ge::GRAPH_FAILED;
  if (v2_flag) {
    ret = TurnToOpParaCalculateV2(node, run_info, iter_2);
  } else {
    ret = TurnToOpParaCalculateV1(node, run_info, iter_1);
  }
  return ret;
}

ge::graphStatus OpAtomicCalculateV1(const ge::OpDescPtr &op_desc_ptr, OpRunInfo &run_info,
                                    std::unordered_map<std::string, OpTilingFunc>::iterator iter) {
  GELOGI("Begin to do Atomic optiling. op_type:%s, op_name:%s",
         OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN.c_str(), op_desc_ptr->GetName().c_str());
  std::vector<int64_t> atomic_output_indices;
  (void) ge::AttrUtils::GetListInt(op_desc_ptr, ge::ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_indices);
  std::map<string, std::map<int64_t, int64_t>> atomic_workspace_info;
  atomic_workspace_info = op_desc_ptr->TryGetExtAttr(ge::EXT_ATTR_ATOMIC_WORKSPACE_INFO, atomic_workspace_info);
  bool atomic_flag = atomic_output_indices.empty() && atomic_workspace_info.empty();
  if (atomic_flag) {
    GE_LOGE("No ATOMIC_ATTR_OUTPUT_INDEX and EXT_ATTR_ATOMIC_WORKSPACE_INFO found, op_type:%s, op_name:%s",
            OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN.c_str(), op_desc_ptr->GetName().c_str());
    return ge::GRAPH_FAILED;
  }

  OpCompileInfo op_compile_info;
  if (!ge::AttrUtils::GetStr(op_desc_ptr, ATOMIC_COMPILE_INFO_KEY, op_compile_info.key)) {
    GE_LOGE("Op[%s] does not have attr[%s].", op_desc_ptr->GetName().c_str(), ATOMIC_COMPILE_INFO_KEY.c_str());
    return ge::GRAPH_FAILED;
  }
  if (!ge::AttrUtils::GetStr(op_desc_ptr, ATOMIC_COMPILE_INFO_JSON, op_compile_info.str)) {
    GE_LOGE("Op[%s] does not have attr[%s].", op_desc_ptr->GetName().c_str(), ATOMIC_COMPILE_INFO_JSON.c_str());
    return ge::GRAPH_FAILED;
  }

  nlohmann::json compile_info_json;
  try {
    compile_info_json = nlohmann::json::parse(op_compile_info.str);
  } catch (nlohmann::json::parse_error& ex) {
    REPORT_CALL_ERROR("E19999", "Failed to set compile_info_value to json of op[%s]. op_compile_info_json:%s",
                      op_desc_ptr->GetName().c_str(), op_compile_info.str.c_str());
    GE_LOGE("Failed to set compile_info_value to json of op[%s]. op_compile_info_json:%s",
            op_desc_ptr->GetName().c_str(), op_compile_info.str.c_str());
    return ge::GRAPH_FAILED;
  }

  int64_t clean_size = 0;
  int64_t first_clean_size = 0;
  if (!atomic_output_indices.empty()) {
    bool is_first_index = true;
    for (const int64_t &atomic_output_indice : atomic_output_indices) {
      ge::ConstGeTensorDescPtr tensor = op_desc_ptr->GetOutputDescPtr(atomic_output_indice);
      if (tensor == nullptr) {
        GE_LOGE("Get MutableOutputDesc failed. op_type:%s, op_name:%s",
                OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN.c_str(), op_desc_ptr->GetName().c_str());
        return ge::GRAPH_FAILED;
      }

      if (ge::TensorUtils::GetSize(*tensor, clean_size) != ge::GRAPH_SUCCESS) {
        GE_LOGE("Get size of tensor desc failed. op_type:%s, op_name:%s",
                OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN.c_str(), op_desc_ptr->GetName().c_str());
        return ge::GRAPH_FAILED;
      }
      compile_info_json[COMPILE_INFO_WORKSPACE_SIZE_LIST].push_back(clean_size);
      if (is_first_index) {
        first_clean_size = clean_size;
        is_first_index = false;
      }
    }
  }

  GELOGI("Atomic clean size: %ld, op_name:%s", clean_size, op_desc_ptr->GetName().c_str());

  TeOpParas op_param;
  op_param.op_type = OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN;
  op_param.const_inputs.emplace("workspace_size",
                                TeConstTensorData(nullptr, static_cast<size_t>(first_clean_size), ge::Tensor()));

  if (!atomic_workspace_info.empty()) {
    clean_size = 0;
    auto workspace_bytes = op_desc_ptr->GetWorkspaceBytes();
    for (auto byte : workspace_bytes) {
      clean_size += byte;
    }
    compile_info_json[COMPILE_INFO_WORKSPACE_SIZE_LIST].push_back(clean_size);
  }
  GELOGI("op_compile_info's value: %s", compile_info_json.dump().c_str());
  op_compile_info.str = compile_info_json.dump();
  op_compile_info.key = op_compile_info.key.append(compile_info_json[COMPILE_INFO_WORKSPACE_SIZE_LIST].dump());

  bool ret = (iter->second)(op_param, op_compile_info, run_info);
  if (ret) {
    GELOGI("Atomic optiling v1 succeed. op_type:%s, op_name:%s.",
           op_desc_ptr->GetType().c_str(), op_desc_ptr->GetName().c_str());
  } else {
    REPORT_CALL_ERROR("E19999", "Fail to call op tiling v1 function of atomic op[%s, %s].",
                      op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
    GE_LOGE("Fail to call op tiling v1 function of atomic op[%s, %s].",
            op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
  }
  return ret ? ge::GRAPH_SUCCESS : ge::GRAPH_FAILED;
}

ge::graphStatus TurnToOpAtomicCalculateV1(const ge::OpDescPtr &op_desc_ptr, OpRunInfoV2 &run_info,
                                          std::unordered_map<std::string, OpTilingFunc>::iterator iter) {
  OpRunInfo run_info_struct;
  run_info_struct.block_dim = run_info.GetBlockDim();
  run_info_struct.clear_atomic = run_info.GetClearAtomic();
  run_info_struct.tiling_key = run_info.GetTilingKey();
  if (OpAtomicCalculateV1(op_desc_ptr, run_info_struct, iter) != ge::GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "OpAtomicCalculateV1 failed, op_type[%s], op_name[%s]",
                      op_desc_ptr->GetType().c_str(), op_desc_ptr->GetName().c_str());
    return ge::GRAPH_FAILED;
  }
  run_info.InternelSetTiling(run_info_struct.tiling_data);
  run_info.SetBlockDim(run_info_struct.block_dim);
  run_info.SetClearAtomic(run_info_struct.clear_atomic);
  run_info.SetTilingKey(run_info_struct.tiling_key);
  if (!run_info_struct.workspaces.empty()) {
    for (const int64_t &workspace : run_info_struct.workspaces) {
      run_info.AddWorkspace(workspace);
    }
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus TurnToOpAtomicCalculateV2(const ge::OpDescPtr &op_desc_ptr, OpRunInfoV2 &run_info,
                                          std::unordered_map<std::string, OpTilingFuncV2>::iterator iter) {
  GELOGI("Begin to do Atomic optiling for op[%s, %s].",
         OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN.c_str(), op_desc_ptr->GetName().c_str());
  std::vector<int64_t> atomic_output_indices;
  (void) ge::AttrUtils::GetListInt(op_desc_ptr, ge::ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_indices);
  std::map<string, std::map<int64_t, int64_t>> atomic_workspace_info;
  atomic_workspace_info = op_desc_ptr->TryGetExtAttr(ge::EXT_ATTR_ATOMIC_WORKSPACE_INFO, atomic_workspace_info);
  bool atomic_flag = atomic_output_indices.empty() && atomic_workspace_info.empty();
  if (atomic_flag) {
    REPORT_CALL_ERROR("E19999",
                      "No ATOMIC_ATTR_OUTPUT_INDEX and EXT_ATTR_ATOMIC_WORKSPACE_INFO found,op_type:%s, op_name:%s",
                      OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN.c_str(), op_desc_ptr->GetName().c_str());
    return ge::GRAPH_FAILED;
  }

  std::string op_compile_info_key;
  if (!ge::AttrUtils::GetStr(op_desc_ptr, ATOMIC_COMPILE_INFO_KEY, op_compile_info_key)) {
    GE_LOGE("Op[%s] does not have attr[%s].", op_desc_ptr->GetName().c_str(), ATOMIC_COMPILE_INFO_KEY.c_str());
    return ge::GRAPH_FAILED;
  }
  std::string op_compile_info_json;
  if (!ge::AttrUtils::GetStr(op_desc_ptr, ATOMIC_COMPILE_INFO_JSON, op_compile_info_json)) {
    GE_LOGE("Op[%s] does not have attr[%s].", op_desc_ptr->GetName().c_str(), ATOMIC_COMPILE_INFO_JSON.c_str());
    return ge::GRAPH_FAILED;
  }

  nlohmann::json compile_info_json;
  try {
    compile_info_json = nlohmann::json::parse(op_compile_info_json);
  } catch (nlohmann::json::parse_error& ex) {
    REPORT_CALL_ERROR("E19999", "Failed to set compile_info_value to json of op[%s]. op_compile_info_json:%s",
                      op_desc_ptr->GetName().c_str(), op_compile_info_json.c_str());
    GE_LOGE("Failed to set compile_info_value to json of op[%s]. op_compile_info_json:%s",
            op_desc_ptr->GetName().c_str(), op_compile_info_json.c_str());
    return ge::GRAPH_FAILED;
  }

  vector<int64_t> workspace_list;
  int64_t clean_size = 0;
  if (!atomic_output_indices.empty()) {
    bool is_first_index = true;
    for (const int64_t &atomic_output_indice : atomic_output_indices) {
      ge::ConstGeTensorDescPtr tensor = op_desc_ptr->GetOutputDescPtr(atomic_output_indice);
      if (tensor == nullptr) {
                REPORT_CALL_ERROR("E19999",
                                  "Get MutableOutputDesc failed. op_type:%s, op_name:%s",
                                  OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN.c_str(), op_desc_ptr->GetName().c_str());
        return ge::GRAPH_FAILED;
      }
      if (ge::TensorUtils::GetSize(*tensor, clean_size) != ge::GRAPH_SUCCESS) {
                REPORT_CALL_ERROR("E19999",
                                  "Get size of tensor desc failed. op_type:%s, op_name:%s",
                                  OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN.c_str(), op_desc_ptr->GetName().c_str());
        return ge::GRAPH_FAILED;
      }
      compile_info_json[COMPILE_INFO_WORKSPACE_SIZE_LIST].push_back(clean_size);
      if (is_first_index) {
        workspace_list.push_back(clean_size);
        is_first_index = false;
      }
    }
  }

  if (!atomic_workspace_info.empty()) {
    clean_size = 0;
    std::vector<int64_t> workspace_bytes = op_desc_ptr->GetWorkspaceBytes();
    for (const int64_t &byte : workspace_bytes) {
      clean_size += byte;
    }
    compile_info_json[COMPILE_INFO_WORKSPACE_SIZE_LIST].push_back(clean_size);
  }
  workspace_list.push_back(clean_size);

  GELOGI("Atomic clean size: %ld, op_name:%s", clean_size, op_desc_ptr->GetName().c_str());
  ge::Operator op_param(OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN);
  op_param.SetAttr(ATTR_NAME_ATOMIC_CLEAN_WORKSPACE, workspace_list);
  op_compile_info_json = compile_info_json.dump();
  op_compile_info_key = op_compile_info_key.append(compile_info_json[COMPILE_INFO_WORKSPACE_SIZE_LIST].dump());

  OpCompileInfoV2 op_compile_info(op_compile_info_key, op_compile_info_json);
  bool ret = (iter->second)(op_param, op_compile_info, run_info);
  if (ret) {
    GELOGI("Atomic optiling v2 succeed. op_type:%s, op_name:%s.",
           op_desc_ptr->GetType().c_str(), op_desc_ptr->GetName().c_str());
  } else {
    REPORT_CALL_ERROR("E19999", "Fail to call op tiling v2 function of atomic op[%s, %s].",
                      op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
    GE_LOGE("Fail to call op tiling v2 function of atomic op[%s, %s].",
            op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
  }
  return ret ? ge::GRAPH_SUCCESS : ge::GRAPH_FAILED;
}

extern "C" ge::graphStatus OpAtomicCalculateV2(const ge::Node &node, OpRunInfoV2 &run_info) {
  ge::OpDescPtr op_desc_ptr = node.GetOpDesc();
  auto &interf_v2 = OpTilingRegistryInterf_V2::RegisteredOpInterf();
  auto iter_v2 = interf_v2.find(OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN);
  ge::graphStatus status = ge::GRAPH_FAILED;
  if (iter_v2 != interf_v2.end()) {
    status = TurnToOpAtomicCalculateV2(op_desc_ptr, run_info, iter_v2);
  } else {
    auto &interf_v1 = OpTilingRegistryInterf::RegisteredOpInterf();
    auto iter_v1 = interf_v1.find(OP_TYPE_DYNAMIC_ATOMIC_ADDR_CLEAN);
    if (iter_v1 != interf_v1.end()) {
      GELOGI("Atomic optiling func on the new way is not found, turn to the old way, op_name:%s",
             op_desc_ptr->GetName().c_str());
      status = TurnToOpAtomicCalculateV1(op_desc_ptr, run_info, iter_v1);
    } else {
      GE_LOGE("Atomic optiling func not found. op_name:%s", op_desc_ptr->GetName().c_str());
      return ge::GRAPH_FAILED;
    }
  }

  return status;
}
}  // namespace optiling