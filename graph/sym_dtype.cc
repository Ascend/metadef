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

#include "sym_dtype.h"
#include "common/checker.h"
#include "graph/utils/attr_utils.h"
#include "graph/tensor_type_impl.h"
#include "graph/types.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "op_common/data_type_utils.h"

namespace ge {
namespace {
graphStatus GetDtypeFromAttr(const OpDesc &op, const std::string &attr, DataType &dtype) {
  GELOGI("Trying get dtype from attr %s of op %s", attr.c_str(), op.GetName().c_str());
  if (AttrUtils::GetDataType(op, attr, dtype)) {
    return GRAPH_SUCCESS;
  }
  int32_t numeric_dtype = -1;
  if (AttrUtils::GetInt(op, attr, numeric_dtype)) {
    GE_ASSERT(numeric_dtype >= 0 && numeric_dtype < DT_MAX, "Invalid numeric dtype %d for sym %s of op %s",
              numeric_dtype, attr.c_str(), op.GetName().c_str());
    dtype = static_cast<DataType>(numeric_dtype);
    return GRAPH_SUCCESS;
  }
  GELOGE(GRAPH_FAILED, "Op %s has no attr named %s", op.GetName().c_str(), attr.c_str());
  return GRAPH_FAILED;
}

graphStatus GetListDtypeFromAttr(const OpDesc &op, const std::string &attr, std::vector<DataType> &dtypes) {
  GELOGI("Trying get list-dtype from attr %s of op %s", attr.c_str(), op.GetName().c_str());
  if (AttrUtils::GetListDataType(op, attr, dtypes)) {
    return GRAPH_SUCCESS;
  }
  std::vector<int32_t> numeric_dtypes;
  if (AttrUtils::GetListInt(op, attr, numeric_dtypes)) {
    for (auto &numeric_dtype : numeric_dtypes) {
      GE_ASSERT(numeric_dtype >= 0 && numeric_dtype < DT_MAX, "Invalid numeric dtype %d for sym %s of op %s",
                numeric_dtype, attr.c_str(), op.GetName().c_str());
      dtypes.push_back(static_cast<DataType>(numeric_dtype));
    }
    return GRAPH_SUCCESS;
  }
  GELOGE(GRAPH_FAILED, "Op %s has no attr named %s", op.GetName().c_str(), attr.c_str());
  return GRAPH_FAILED;
}

std::string ToString(const TensorType &types) {
  std::string s = "[";
  for (auto &dtype : types.tensor_type_impl_->GetMutableDateTypeSet()) {
    s += TypeUtils::DataTypeToSerialString(dtype);
    s += ",";
  }
  s += "]";
  return s;
}

const char *ToString(const IrInputType &type) {
  if (type == kIrInputRequired) {
    return "Required";
  }
  if (type == kIrInputOptional) {
    return "Optional";
  }
  if (type == kIrInputDynamic) {
    return "Dynamic";
  }
  return "Unknown";
}

graphStatus PromoteDtype(const DataType &left, const DataType &right, DataType &promoted_dtype) {
  GE_ASSERT(left >= 0 && left < DT_MAX, "Invalid left dtype %d", left);
  GE_ASSERT(right >= 0 && right < DT_MAX, "Invalid right dtype %d", right);

  promoted_dtype = opcommon::PromoteType(left, right);
  GELOGD("Promoted dtype %s from %s and %s", TypeUtils::DataTypeToSerialString(promoted_dtype).c_str(),
         TypeUtils::DataTypeToSerialString(left).c_str(), TypeUtils::DataTypeToSerialString(right).c_str());
  return GRAPH_SUCCESS;
}

graphStatus PromoteDtype(const TypeOrTypes &left, const TypeOrTypes &right, TypeOrTypes &promoted_dtype) {
  GE_ASSERT(left.IsListType() == right.IsListType(), "Trying promote %s with %s", left.DebugString().c_str(),
            right.DebugString().c_str());
  if (left.IsListType()) {
    std::vector<DataType> left_dtypes;
    std::vector<DataType> right_dtypes;

    GE_ASSERT_GRAPH_SUCCESS(left.GetTypes(left_dtypes));
    GE_ASSERT_GRAPH_SUCCESS(right.GetTypes(right_dtypes));

    GE_ASSERT_EQ(left_dtypes.size(), right_dtypes.size());

    std::vector<DataType> data_types;
    data_types.resize(left_dtypes.size());
    for (size_t i = 0U; i < left_dtypes.size(); i++) {
      GE_ASSERT_GRAPH_SUCCESS(PromoteDtype(left_dtypes[i], right_dtypes[i], data_types[i]));
    }
    promoted_dtype.SetTypes(data_types);
    return GRAPH_SUCCESS;
  }

  DataType left_dtype;
  DataType right_dtype;
  GE_ASSERT_GRAPH_SUCCESS(left.GetType(left_dtype));
  GE_ASSERT_GRAPH_SUCCESS(right.GetType(right_dtype));

  DataType dtype;
  GE_ASSERT_GRAPH_SUCCESS(PromoteDtype(left_dtype, right_dtype, dtype));
  promoted_dtype.SetType(dtype);
  return GRAPH_SUCCESS;
}
}  // namespace

graphStatus TypeOrTypes::GetType(DataType &type) const {
  if (!initialized_ || is_list_ || types_.empty()) {
    return GRAPH_FAILED;
  }
  type = types_[0];
  return GRAPH_SUCCESS;
}

graphStatus TypeOrTypes::GetTypes(std::vector<DataType> &types) const {
  if (!initialized_ || !is_list_) {
    return GRAPH_FAILED;
  }
  types = types_;
  return GRAPH_SUCCESS;
}

const DataType &TypeOrTypes::UnsafeGetType() const {
  if (!initialized_ || is_list_ || (types_.size() != 1)) {
    const static DataType kUndefined = DT_UNDEFINED;
    return kUndefined;
  }
  return types_[0];
}

const std::vector<DataType> &TypeOrTypes::UnsafeGetTypes() const {
  if (!initialized_ || !is_list_) {
    const static std::vector<DataType> kUndefined{};
    return kUndefined;
  }
  return types_;
}

void TypeOrTypes::SetType(const DataType &type) {
  initialized_ = true;
  is_list_ = false;
  types_.clear();
  types_.emplace_back(type);
}

void TypeOrTypes::SetTypes(const std::vector<DataType> &types) {
  initialized_ = true;
  is_list_ = true;
  types_ = types;
}

std::string TypeOrTypes::DebugString() const {
  if (!initialized_) {
    return "Uninitialized";
  }
  std::string ret = is_list_ ? "List[" : "";
  for (auto &type : types_) {
    ret += TypeUtils::DataTypeToSerialString(type) + ",";
  }
  if (is_list_) {
    ret += "]";
  }
  return ret;
}

// 不使用DATATYPE指定sym的取值范围时，sym的取值范围为所有数据类型
SymDtype::SymDtype(const std::string &id) : id_(id), is_legacy_(true), is_list_(false), tensor_type_({}) {}

const std::string &SymDtype::Id() const {
  return id_;
}

bool SymDtype::IsLegacy() const {
  return is_legacy_;
}

void SymDtype::BindIrInput(const std::string &ir_input, const IrInputType &input_type, size_t input_index) {
  ir_inputs_.emplace_back(ir_input, input_type, input_index);
}

void SymDtype::BindAllowedDtypes(const TensorType &types) {
  is_legacy_ = false;
  is_list_ = false;
  tensor_type_ = types;
}

void SymDtype::BindAllowedDtypes(const ListTensorType &types) {
  is_legacy_ = false;
  is_list_ = true;
  tensor_type_ = types.tensor_type;
}

void SymDtype::BindExpression(const std::shared_ptr<SymDtypeExpression> &expression) {
  is_legacy_ = false;
  expression_ = expression;
}

bool SymDtype::IsListType() const {
  if (expression_ != nullptr) {
    return expression_->IsListType();
  }
  return is_list_;
}

const std::string &SymDtype::DebugString() const {
  std::string ret = id_ + ":";
  ret += (is_list_ ? "List" : "Oneof");
  ret += ToString(tensor_type_);
  return id_;
}

graphStatus SymDtype::Eval(const OpDesc &op, TypeOrTypes &type_or_types) const {
  GE_ASSERT(!is_legacy_, "Trying eval legacy sym dtype %s", id_.c_str());
  if (expression_ != nullptr) {
    GELOGI("Eval sym dtype from expression of op %s", id_.c_str(), op.GetType().c_str());
    return expression_->Eval(op, type_or_types);
  }

  if (IsListType()) {
    std::vector<DataType> dtypes;
    GE_ASSERT_GRAPH_SUCCESS(Eval(op, dtypes));
    type_or_types.SetTypes(dtypes);
    return GRAPH_SUCCESS;
  }

  DataType single_dtype;
  GE_ASSERT_GRAPH_SUCCESS(Eval(op, single_dtype));
  type_or_types.SetType(single_dtype);
  return GRAPH_SUCCESS;
}

std::string SymDtype::SymBackend::DebugString() const {
  return std::string(ToString(type)) + "[" + std::to_string(index) + "] " + name;
}

graphStatus SymDtype::Eval(const OpDesc &op, DataType &dtype) const {
  if (ir_inputs_.empty()) {
    GELOGI("Trying eval sym dtype from attr %s of op %s", id_.c_str(), op.GetType().c_str());
    if (AttrUtils::HasAttr(op, id_)) {
      GE_ASSERT_GRAPH_SUCCESS(GetDtypeFromAttr(op, id_, dtype));
      GE_ASSERT(tensor_type_.tensor_type_impl_->IsDataTypeInRange(dtype));
      return GRAPH_SUCCESS;
    }
    GE_ASSERT(tensor_type_.tensor_type_impl_->GetMutableDateTypeSet().size() == 1,
              "Op %s has no attr %s and sym %s allowed dtypes range is not one", op.GetType().c_str(), id_.c_str());
    dtype = *tensor_type_.tensor_type_impl_->GetMutableDateTypeSet().begin();
    return GRAPH_SUCCESS;
  }

  std::map<size_t, std::pair<size_t, size_t>> ir_input_2_range;
  GE_ASSERT_GRAPH_SUCCESS(
      OpDescUtils::GetIrInputRawDescRange(const_cast<OpDesc *>(&op)->shared_from_this(), ir_input_2_range));
  GE_ASSERT(ir_input_2_range.size() == op.GetIrInputsSize(), "Failed get input instance info of %s %s",
            op.GetName().c_str(), op.GetType().c_str());

  std::set<DataType> infered_dtypes;
  for (auto &backend : ir_inputs_) {
    auto &input_range = ir_input_2_range[backend.index];
    size_t start = input_range.first;
    size_t end = input_range.first + input_range.second;
    GELOGD("Sym %s of %s backend %s mapping to input desc[%zu:%zu)", id_.c_str(), op.GetName().c_str(),
           backend.DebugString().c_str(), start, end);

    for (size_t i = start; i < end; i++) {
      auto desc = op.MutableInputDesc(i);
      GE_ASSERT_NOTNULL(desc);
      GELOGI("Get dtype %s from %s input %s:%zu of op %s",
             TypeUtils::DataTypeToSerialString(desc->GetDataType()).c_str(), ToString(backend.type),
             backend.name.c_str(), i - start, op.GetName().c_str());
      infered_dtypes.insert(desc->GetDataType());
    }
  }

  GE_ASSERT(infered_dtypes.size() == 1, "Infer dtype failed for op %s as %zu types infered", op.GetName().c_str(),
            infered_dtypes.size());
  dtype = *infered_dtypes.begin();

  GE_ASSERT(tensor_type_.tensor_type_impl_->IsDataTypeInRange(dtype), "Sym %s infered dtype %s not in range %s",
            id_.c_str(), TypeUtils::DataTypeToSerialString(dtype).c_str(), ToString(tensor_type_).c_str());

  return GRAPH_SUCCESS;
}

graphStatus SymDtype::Eval(const OpDesc &op, std::vector<DataType> &dtypes) const {
  if (ir_inputs_.empty()) {
    GELOGI("Eval sym list-dtype from attr %s of op %s", id_.c_str(), op.GetType().c_str());
    GE_ASSERT_GRAPH_SUCCESS(GetListDtypeFromAttr(op, id_, dtypes));
    for (auto &dtype : dtypes) {
      GE_ASSERT(tensor_type_.tensor_type_impl_->IsDataTypeInRange(dtype),
                "Sym %s infered one of list-dtype %s not in range %s", id_.c_str(),
                TypeUtils::DataTypeToSerialString(dtype).c_str(), ToString(tensor_type_).c_str());
    }
    return GRAPH_SUCCESS;
  }

  std::map<size_t, std::pair<size_t, size_t>> ir_input_2_range;
  GE_ASSERT_GRAPH_SUCCESS(
      OpDescUtils::GetIrInputRawDescRange(const_cast<OpDesc *>(&op)->shared_from_this(), ir_input_2_range));
  GE_ASSERT(ir_input_2_range.size() == op.GetIrInputsSize(), "Failed get input instance info of %s %s",
            op.GetName().c_str(), op.GetType().c_str());

  for (auto &backend : ir_inputs_) {
    GE_ASSERT(backend.type == kIrInputDynamic, "List-type sym %s can not bind to %s input %s", id_.c_str(),
              ToString(backend.type), backend.name.c_str());
    auto &input_range = ir_input_2_range[backend.index];
    size_t start = input_range.first;
    size_t end = input_range.first + input_range.second;
    GELOGD("Sym %s of %s backend %s mapping to input desc[%zu:%zu)", id_.c_str(), op.GetName().c_str(),
           backend.DebugString().c_str(), start, end);

    std::vector<DataType> input_dtypes;
    for (size_t i = start; i < end; i++) {
      auto desc = op.MutableInputDesc(i);
      GE_ASSERT_NOTNULL(desc);
      GELOGI("Get dtype %s from dynamic input %s:%zu of op %s",
             TypeUtils::DataTypeToSerialString(desc->GetDataType()).c_str(), backend.name.c_str(), i - start,
             op.GetName().c_str());
      input_dtypes.push_back(desc->GetDataType());
    }

    if (dtypes.empty()) {
      dtypes = input_dtypes;
    } else {
      GE_ASSERT_EQ(input_dtypes.size(), dtypes.size());
      for (size_t i = 0U; i < input_dtypes.size(); i++) {
        GE_ASSERT(input_dtypes[i] == dtypes[i], "Sym list-dtype mismatch");
      }
    }
  }

  for (auto &dtype : dtypes) {
    GE_ASSERT(tensor_type_.tensor_type_impl_->IsDataTypeInRange(dtype), "Sym %s infered list-dtype %s not in range %s",
              id_.c_str(), TypeUtils::DataTypeToSerialString(dtype).c_str(), ToString(tensor_type_).c_str());
  }

  return GRAPH_SUCCESS;
}

PromotionSymDtypeExpression::PromotionSymDtypeExpression(const std::vector<SymDtype *> &syms) : syms_(syms) {}

graphStatus PromotionSymDtypeExpression::Eval(const OpDesc &op, TypeOrTypes &type_or_types) const {
  GE_ASSERT(syms_.size() > 1U, "Trying eval promotion sym with %zu syms", syms_.size());

  GE_ASSERT_GRAPH_SUCCESS(syms_[0]->Eval(op, type_or_types));
  GELOGI("Promoting start with %s from sym %s", type_or_types.DebugString().c_str(), syms_[0]->DebugString().c_str());

  TypeOrTypes next;
  for (size_t i = 1U; i < syms_.size(); i++) {
    GE_ASSERT_GRAPH_SUCCESS(syms_[i]->Eval(op, next));
    GELOGI("Promoting %s with %s from sym %s", type_or_types.DebugString().c_str(), next.DebugString().c_str(),
           syms_[i]->DebugString().c_str());
    GE_ASSERT_GRAPH_SUCCESS(PromoteDtype(type_or_types, next, type_or_types));
  }

  return GRAPH_SUCCESS;
}
}  // namespace ge