/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#ifndef GRAPH_OP_DESC_IMPL_H_
#define GRAPH_OP_DESC_IMPL_H_

#include <string>
#include <utility>
#include <vector>
#include <set>
#include "graph/types.h"
#include "graph/op_desc.h"
#include "graph/small_vector.h"
#include "graph/ascend_limits.h"
#include "graph/tensor_type_impl.h"
#include "graph/ir_meta.h"

namespace ge {
enum class DataTypeInferStrategy {
  kInferFromAttr,
  kInferFromInput,
  kInferFromOutput,
  kInvalidStrategy
};

class OpDescImpl {
 public:
  OpDescImpl();
  OpDescImpl(const std::string &name, const std::string &type);
  explicit OpDescImpl(const ge::proto::OpDef &op_def);

  ~OpDescImpl() = default;

  std::string GetName() const;
  void SetName(const std::string &name);

  std::string GetType() const;
  void SetType(const std::string &type, OpDescImplPtr &impl_of_target_type);
  void SetIrRelated(const OpDescImpl *r_op_desc);

  graphStatus AddInputDesc(const ge::GeTensorDesc &input_desc);
  graphStatus AddInputDesc(const uint32_t index, const ge::GeTensorDesc &input_desc);
  graphStatus AddInputDesc(const std::string &name, const ge::GeTensorDesc &input_desc);
  graphStatus AddInputDescMiddle(const std::string &name, const uint32_t num, const size_t index);
  graphStatus AddOutputDescMiddle(const std::string &name, const uint32_t num, const size_t index);
  graphStatus AddInputDescForward(const std::string &name, const uint32_t num);
  graphStatus AddOutputDescForward(const std::string &name, const uint32_t num);
  graphStatus AddOptionalInputDesc(const std::string &name, const ge::GeTensorDesc &input_desc);

  graphStatus UpdateInputDesc(const uint32_t index, const ge::GeTensorDesc &tensor_Desc);
  graphStatus UpdateInputDesc(const std::string &name, const ge::GeTensorDesc &tensor_Desc);

  bool OpDescMembersAreEqual(const OpDescImpl &r_op_desc) const;
  bool OpDescAttrsAreEqual(const OpDescImpl &r_op_desc) const;
  bool OpDescGenTensorDescsAreEqual(const OpDescImpl &r_op_desc) const;

  bool InputIsSet(const std::string &name) const;

  const GeTensorDesc &GetInputDesc(const uint32_t index) const;
  const GeTensorDesc &GetInputDesc(const std::string &name) const;
  GeTensorDescPtr MutableInputDesc(const uint32_t index) const;
  GeTensorDescPtr MutableInputDesc(const std::string &name) const;
  OpDesc::Vistor<string> GetAllInputNames(const ConstOpDescPtr &op_desc) const;

  void SetOpKernelLibName(const std::string &name);
  std::string GetOpKernelLibName() const;
  void SetOpEngineName(const std::string &name);
  std::string GetOpEngineName() const;

  OpDesc::Vistor<GeTensorDesc> GetAllInputsDesc(const ConstOpDescPtr &op_desc) const;
  OpDesc::Vistor<GeTensorDescPtr> GetAllInputsDescPtr(const ConstOpDescPtr &op_desc) const;

  size_t GetInputsSize() const;
  size_t GetAllInputsSize() const;

  graphStatus AddOutputDesc(const ge::GeTensorDesc &output_desc);
  graphStatus AddOutputDesc(const std::string &name, const ge::GeTensorDesc &output_desc);
  graphStatus UpdateOutputDesc(const uint32_t index, const ge::GeTensorDesc &tensor_Desc);
  graphStatus UpdateOutputDesc(const std::string &name, const ge::GeTensorDesc &tensor_Desc);
  const GeTensorDesc &GetOutputDesc(const uint32_t index) const;
  const GeTensorDesc &GetOutputDesc(const std::string &name) const;
  GeTensorDescPtr MutableOutputDesc(const uint32_t index) const;
  GeTensorDescPtr MutableOutputDesc(const std::string &name) const;

  uint32_t GetAllOutputsDescSize() const;
  OpDesc::Vistor<GeTensorDesc> GetAllOutputsDesc(const ConstOpDescPtr &op_desc) const;
  OpDesc::Vistor<GeTensorDescPtr> GetAllOutputsDescPtr(const ConstOpDescPtr &op_desc) const;
  ConstGeTensorDescPtr GetOutputDescPtr(const uint32_t index) const;
  size_t GetOutputsSize() const;

  ConstGeTensorDescPtr GetInputDescPtr(const uint32_t index) const;
  ConstGeTensorDescPtr GetInputDescPtrDfault(const uint32_t index) const;
  ConstGeTensorDescPtr GetInputDescPtr(const std::string &name) const;

  graphStatus AddDynamicInputDesc(const std::string &name, const uint32_t num, const bool is_push_back);
  graphStatus AddDynamicInputDescByIndex(const std::string &name, const uint32_t num, const size_t index);

  graphStatus AddDynamicOutputDesc(const std::string &name, const uint32_t num, const bool is_push_back);
  bool IsOptionalInput(const uint32_t index) const;
  std::map<std::string, uint32_t> GetAllInputName() const;
  std::map<std::string, uint32_t> GetAllOutputName();
  std::map<std::string, uint32_t>& MutableAllInputName();
  std::map<std::string, uint32_t>& MutableAllOutputName();
  bool UpdateInputName(std::map<std::string, uint32_t> input_name_idx);
  bool UpdateOutputName(std::map<std::string, uint32_t> output_name_idx);

  std::function<graphStatus(Operator &)> GetInferFunc() const;
  std::function<graphStatus(Operator &)> GetVerifyFunc() const;
  std::function<graphStatus(Operator &)> GetInferFormatFunc() const;
  std::function<graphStatus(Operator &)> GetInferValueRangeFunc() const;
  std::function<graphStatus(Operator &)> GetInferDataSliceFunc() const;

  void AddInferFunc(const std::function<graphStatus(Operator &)> &func);
  void AddInferFormatFunc(const std::function<graphStatus(Operator &)> &func);
  void AddInferValueRangeFunc(const std::function<graphStatus(Operator &)> &func);
  void AddVerifierFunc(const std::function<graphStatus(Operator &)> &func);
  void AddInferDataSliceFunc(const std::function<graphStatus(Operator &)> &func);

  graphStatus VerifyIR() const;
  graphStatus DefaultInferDataType(const OpDescPtr &op_desc);
  graphStatus DefaultInferFormat(const ConstOpDescPtr &op_desc) const;

  std::string GetInputNameByIndex(const uint32_t index) const;
  int32_t GetInputIndexByName(const std::string &name) const;
  std::string GetValidInputNameByIndex(const uint32_t index) const;

  std::string GetOutputNameByIndex(const uint32_t index) const;
  int32_t GetOutputIndexByName(const std::string &name) const;

  ProtoAttrMap &MutableAttrMap();
  ConstProtoAttrMap &GetAttrMap() const;

  IRMetaData &MutableIRMeta();
  const IRMetaData &GetIRMeta() const;

  void SetId(const int64_t id);
  int64_t GetId() const;

  void SetStreamId(const int64_t stream_id);
  int64_t GetStreamId() const;

  void SetInputName(const std::vector<std::string> &input_name);
  std::vector<std::string> GetInputName() const;

  void SetSrcName(const std::vector<std::string> &src_name);
  std::vector<std::string> GetSrcName() const;

  void SetSrcIndex(const std::vector<int64_t> &src_index);
  std::vector<int64_t> GetSrcIndex() const;

  void SetInputOffset(const std::vector<int64_t> &input);
  std::vector<int64_t> GetInputOffset() const;

  void SetOutputOffset(const std::vector<int64_t> &output);
  std::vector<int64_t> GetOutputOffset() const;

  void SetDstName(const std::vector<std::string> &dst_name);
  std::vector<std::string> GetDstName() const;

  void SetDstIndex(const std::vector<int64_t> &dst_index);

  void SetWorkspace(const std::vector<int64_t> &workspace);
  std::vector<int64_t> GetWorkspace() const;

  void SetWorkspaceBytes(const std::vector<int64_t> &workspace_bytes);
  std::vector<int64_t> GetWorkspaceBytes() const;

  void SetIsInputConst(const std::vector<bool> &is_input_const);
  std::vector<bool> GetIsInputConst() const;

  std::string GetSubgraphInstanceName(const size_t index) const;
  const std::vector<std::string> &GetSubgraphInstanceNames() const;
  void RemoveSubgraphInstanceName(const std::string &name);
  graphStatus AddSubgraphName(const std::string &name);
  const std::map<std::string, uint32_t> &GetSubgraphNameIndexes() const;
  graphStatus SetSubgraphInstanceName(const size_t index, const std::string &name);

  graphStatus GetSubgraphNameByInstanceName(const std::string &instance_name, std::string &subgraph_name) const;

  void *GetTilingFuncInfo() const;
  void SetTilingFuncInfo(void *tiling_func_info);
  void *GetAtomicTilingFuncInfo() const;
  void SetAtomicTilingFuncInfo(void *atomic_tiling_func_info);

 private:
  void DeSerializeOpDefToMetaData(const proto::OpDef &op_def);
  void SerializeMetaDataToOpDef(proto::OpDef * const op_def);
  // infer datatype
  graphStatus VerifyInputDataType() const;
  graphStatus CollectInputDataTypeBySymbol(std::unordered_map<std::string, DataType> &symbol_2_input_dtype) const;
  graphStatus VerifyInputDataTypeConsistent(const std::unordered_map<std::string,
                                                                     DataType> &symbol_2_input_dtype) const;
  graphStatus VerifyInputDataTypeInRange(const std::unordered_map<std::string, DataType> &symbol_2_input_dtype) const;
  DataTypeInferStrategy GetDataTypeInferStrategy(const string &datatype_symbol) const;
  graphStatus InferDataTypeForOutput(const std::string &ir_output, std::vector<DataType> &dst_types);
  graphStatus InferDataTypeForOutputs(const OpDescPtr &op_desc);
  graphStatus TryInferDataTypeFromAttr(const string &datatype_symbol, DataType &dst_type);
  graphStatus TryInferDataTypeFromInput(const string &datatype_symbol, std::vector<DataType> &dst_types);
  graphStatus TryInferDataTypeFromRequiredInput(const std::string &ir_input_name, const string &datatype_symbol,
                                                DataType &dst_type);
  graphStatus TryInferDataTypeFromDynamicInput(const std::string &ir_input_name, const string &datatype_symbol,
                                               std::vector<DataType> &dst_types);
  graphStatus TryInferDataTypeFromOutput(const string &datatype_symbol, DataType &dst_type);

  friend class AttrUtils;
  friend class OpDescUtils;
  friend class ModelSerializeImp;
  friend class OnnxUtils;
  friend class GraphUtils;
  friend class NodeUtils;
  std::vector<std::string> subgraph_instance_names_;

  // subgraph names to index, for a `if` operator:
  // then_branch: 0
  // else_branch: 1
  // or for a `case` node:
  // branches0: 0
  // branches1: 1
  // branches2: 2
  std::map<std::string, uint32_t> subgraph_names_to_index_;
  std::vector<GeTensorDescPtr> inputs_desc_{};
  std::map<std::string, uint32_t> input_name_idx_{};
  std::vector<GeTensorDescPtr> outputs_desc_{};
  std::map<std::string, uint32_t> output_name_idx_{};
  std::function<graphStatus(Operator &)> infer_func_ = nullptr;
  std::function<graphStatus(Operator &)> infer_format_func_ = nullptr;
  std::function<graphStatus(Operator &)> infer_value_range_func_ = nullptr;
  std::function<graphStatus(Operator &)> verifier_func_ = nullptr;
  std::function<graphStatus(Operator &)> infer_data_slice_func_ = nullptr;
  std::string op_kernel_lib_name_;
  std::string engine_name_;
  OpMetadata meta_data_;
  AttrStore attrs_;
  void *tiling_func_info_ = nullptr;
  void *atomic_tiling_func_info_ = nullptr;
};
}  // namespace ge
#endif  // GRAPH_OP_DESC_IMPL_H_
