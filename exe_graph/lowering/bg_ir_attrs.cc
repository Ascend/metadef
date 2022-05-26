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
#include "exe_graph/lowering/bg_ir_attrs.h"

#include <type_traits>

#include <securec.h>
#include "framework/common/debug/ge_log.h"
#include "graph/utils/math_util.h"
#include "common/checker.h"

#include "exe_graph/runtime/runtime_attrs.h"
#include "exe_graph/runtime/continuous_vector.h"
#include "exe_graph/runtime/tensor.h"
#include "exe_graph/runtime/runtime_attrs_def.h"

namespace gert {
namespace bg {
namespace {
void GeShapeToGertShape(const ge::GeShape &ge_shape, gert::Shape &gert_shape) {
  gert_shape.SetDimNum(ge_shape.GetDimNum());
  for (size_t i = 0; i < ge_shape.GetDimNum(); ++i) {
    gert_shape.SetDim(i, ge_shape.GetDim(i));
  }
}
template<typename T, typename std::enable_if<std::is_fundamental<T>::value, int>::type = 0>
bool AppendFundAttr(const ge::AnyValue &attr, std::vector<std::vector<uint8_t>> &attrs) {
  auto val = attr.Get<T>();
  GE_ASSERT_NOTNULL(val);
  std::vector<uint8_t> runtime_attr(sizeof(*val));
  GE_ASSERT_EOK(memcpy_s(runtime_attr.data(), sizeof(*val), val, sizeof(*val)));
  attrs.emplace_back(std::move(runtime_attr));
  return true;
}
bool AppendStrAttr(const ge::AnyValue &attr, std::vector<std::vector<uint8_t>> &attrs) {
  auto str = attr.Get<std::string>();
  GE_ASSERT_NOTNULL(str);
  std::vector<uint8_t> runtime_attr(str->size() + 1);
  GE_ASSERT_EOK(strcpy_s(reinterpret_cast<char *>(runtime_attr.data()), str->size() + 1, str->c_str()));
  attrs.emplace_back(std::move(runtime_attr));
  return true;
}
template<typename T, typename std::enable_if<std::is_fundamental<T>::value, int>::type = 0>
bool AppendVectorAttr(const ge::AnyValue &attr, std::vector<std::vector<uint8_t>> &attrs) {
  auto val = attr.Get<std::vector<T>>();
  GE_ASSERT_NOTNULL(val);
  size_t total_size;
  auto cv_holder = ContinuousVector::Create<T>(val->size(), total_size);
  GE_ASSERT_NOTNULL(cv_holder);
  auto cv = reinterpret_cast<ContinuousVector *>(cv_holder.get());
  size_t copy_size = val->size() * sizeof(T);
  GE_ASSERT_EOK(memcpy_s(cv->MutableData(), cv->GetCapacity() * sizeof(T), val->data(), copy_size));
  cv->SetSize(val->size());

  // todo 拷贝了两次，后面优化
  std::vector<uint8_t> buf(total_size);
  GE_ASSERT_EOK(memcpy_s(buf.data(), buf.size(), cv_holder.get(), total_size));
  attrs.emplace_back(std::move(buf));
  return true;
}
bool AppendTensorAttr(const ge::AnyValue &attr, std::vector<std::vector<uint8_t>> &attrs) {
  auto val = attr.Get<ge::GeTensor>();
  GE_ASSERT_NOTNULL(val);
  auto &tensor_desc = val->GetTensorDesc();
  auto shape_size = tensor_desc.GetShape().GetShapeSize();
  if (shape_size < 0) {
    GELOGE(ge::PARAM_INVALID, "Failed to append tensor attr, shape size less than 0");
    return false;
  }
  size_t total_size;
  auto tensor_holder = Tensor::CreateFollowing(shape_size, tensor_desc.GetDataType(), total_size);
  GE_ASSERT_NOTNULL(tensor_holder);
  auto tensor = reinterpret_cast<Tensor *>(tensor_holder.get());
  GeShapeToGertShape(tensor_desc.GetShape(), tensor->MutableStorageShape());
  GeShapeToGertShape(tensor_desc.GetOriginShape(), tensor->MutableOriginShape());
  tensor->SetOriginFormat(tensor_desc.GetOriginFormat());
  tensor->SetStorageFormat(tensor_desc.GetFormat());
  GE_ASSERT_EOK(memcpy_s(tensor->GetData<uint8_t>(), total_size - sizeof(Tensor), val->GetData().GetData(),
                         val->GetData().GetSize()));

  std::vector<uint8_t> buf(total_size);
  GE_ASSERT_EOK(memcpy_s(buf.data(), total_size, tensor_holder.get(), total_size));
  attrs.emplace_back(std::move(buf));
  return true;
}
bool AppendAttr(const ge::AnyValue &attr, std::vector<std::vector<uint8_t>> &attrs) {
  switch (attr.GetValueType()) {
    case ge::AnyValue::VT_FLOAT:
      return AppendFundAttr<float>(attr, attrs);
    case ge::AnyValue::VT_BOOL:
      return AppendFundAttr<bool>(attr, attrs);
    case ge::AnyValue::VT_INT:
      return AppendFundAttr<int64_t>(attr, attrs);
    case ge::AnyValue::VT_LIST_INT:
      return AppendVectorAttr<int64_t>(attr, attrs);
    case ge::AnyValue::VT_STRING:
      return AppendStrAttr(attr, attrs);
    case ge::AnyValue::VT_TENSOR:
      return AppendTensorAttr(attr, attrs);
    case ge::AnyValue::VT_LIST_FLOAT:
      return AppendVectorAttr<float>(attr, attrs);
      // todo ListListInt
    default:
      GELOGE(ge::FAILED, "Does not support the attr type now, attr type %d", attr.GetValueType());
      return false;
  }
}
bool GetAllIrAttrs(const ge::NodePtr &node, std::vector<std::vector<uint8_t>> &runtime_attrs) {
  // todo 这里可能存在兼容性问题，
  //    metadef仓在当前版本新增了ir_attr_names能力，这意味着在老版本中，生成的OpDesc中没有保存ir_attr_nams
  //    考虑反序列化老版本产生的om场景：此时om的opdesc中没有ir_attr_names这个字段，这里也就拿不到ir_attr_names
  //    兼容的解决方案时，如果反序列化时发现没有这个字段，那么需要从当前IR中构造一遍，强行生成这个属性
  // todo 序列化、反序列化场景，需要把ir_attr_names保存下来，补充ST验证反序列化场景
  const auto &ir_attr_names = node->GetOpDesc()->GetIrAttrNames();
  auto all_attrs = ge::AttrUtils::GetAllAttrs(node->GetOpDesc());
  for (size_t i = 0; i < ir_attr_names.size(); ++i) {
    auto &attr_name = node->GetOpDesc()->GetIrAttrNames()[i];
    auto iter = all_attrs.find(attr_name);
    if (iter == all_attrs.end()) {
      GELOGE(ge::FAILED, "Can not find the IR attr %s from node %s", attr_name.c_str(), node->GetName().c_str());
      return false;
    }
    GE_ASSERT_TRUE(AppendAttr(iter->second, runtime_attrs));
  }
  return true;
}
std::unique_ptr<uint8_t[]> CreateAttrBuffer(const std::vector<std::vector<uint8_t>> &attrs, size_t &total_size) {
  total_size = sizeof(RuntimeAttrsDef);
  size_t offset_size;
  if (ge::MulOverflow(sizeof(size_t), attrs.size(), offset_size)) {
    GELOGE(ge::FAILED, "Failed to create attr buffer, total size overflow, attrs size may invalid %zu", attrs.size());
    return nullptr;
  }
  if (ge::AddOverflow(total_size, offset_size, total_size)) {
    GELOGE(ge::FAILED, "Failed to create attr buffer, total size overflow, attrs offset may invalid %zu", offset_size);
    return nullptr;
  }
  for (const auto &attr : attrs) {
    if (ge::AddOverflow(total_size, attr.size(), total_size)) {
      GELOGE(ge::FAILED,
             "Failed to create attr buffer, total size overflow, attr size may invalid %zu, current total size %zu",
             attr.size(), total_size);
      return nullptr;
    }
  }
  auto attr_holder = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_size]);
  GE_ASSERT_NOTNULL(attr_holder);
  auto attr_def = reinterpret_cast<RuntimeAttrsDef *>(attr_holder.get());
  attr_def->attr_num = attrs.size();
  size_t current_offset = sizeof(RuntimeAttrsDef) + sizeof(size_t) * attr_def->attr_num;
  auto attr_pos = reinterpret_cast<uint8_t *>(attr_holder.get());
  for (size_t i = 0; i < attrs.size(); ++i) {
    attr_def->offset[i] = current_offset;
    GE_ASSERT_EOK(memcpy_s(attr_pos + current_offset, total_size - current_offset, attrs[i].data(), attrs[i].size()));
    current_offset += attrs[i].size();
  }
  return attr_holder;
}
}  // namespace
std::unique_ptr<uint8_t[]> CreateAttrBuffer(const ge::NodePtr &node, size_t &size) {
  std::vector<std::vector<uint8_t>> runtime_attrs;
  GE_ASSERT_TRUE(GetAllIrAttrs(node, runtime_attrs));
  return CreateAttrBuffer(runtime_attrs, size);
}
}  // namespace bg
}  // namespace gert