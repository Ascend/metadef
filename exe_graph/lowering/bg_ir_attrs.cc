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

#include "securec.h"
#include "framework/common/debug/ge_log.h"
#include "graph/utils/math_util.h"

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
bool AppendAttr(const ge::AnyValue &attr, std::vector<std::vector<uint8_t>> &attrs) {
  switch (attr.GetValueType()) {
    case ge::AnyValue::VT_STRING: {
      auto str = attr.Get<std::string>();
      if (str == nullptr) {
        return false;
      }
      std::vector<uint8_t> runtime_attr(str->size() + 1);
      if (strcpy_s(reinterpret_cast<char *>(runtime_attr.data()), str->size() + 1, str->c_str()) != EOK) {
        return false;
      }
      attrs.emplace_back(std::move(runtime_attr));
      break;
    }
    case ge::AnyValue::VT_FLOAT: {
      auto val = attr.Get<float>();
      if (val == nullptr) {
        return false;
      }
      std::vector<uint8_t> runtime_attr(sizeof(*val));
      if (memcpy_s(runtime_attr.data(), sizeof(*val), val, sizeof(*val)) != EOK) {
        return false;
      }
      attrs.emplace_back(std::move(runtime_attr));
      break;
    }
    case ge::AnyValue::VT_BOOL: {
      auto b = attr.Get<bool>();
      if (b == nullptr) {
        return false;
      }
      std::vector<uint8_t> runtime_attr(sizeof(*b));
      if (memcpy_s(runtime_attr.data(), sizeof(*b), b, sizeof(*b)) != EOK) {
        return false;
      }
      attrs.emplace_back(std::move(runtime_attr));
      break;
    }
    case ge::AnyValue::VT_INT: {
      auto i = attr.Get<int64_t>();
      if (i == nullptr) {
        return false;
      }
      std::vector<uint8_t> runtime_attr(sizeof(*i));
      if (memcpy_s(runtime_attr.data(), sizeof(*i), i, sizeof(*i)) != EOK) {
        return false;
      }
      attrs.emplace_back(std::move(runtime_attr));
      break;
    }
    case ge::AnyValue::VT_LIST_INT: {
      auto val = attr.Get<std::vector<int64_t>>();
      if (val == nullptr) {
        return false;
      }
      size_t total_size;
      auto cv_holder = ContinuousVector::Create<int64_t>(val->size(), total_size);
      if (cv_holder == nullptr) {
        return false;
      }
      auto cv = reinterpret_cast<ContinuousVector *>(cv_holder.get());
      size_t copy_size = val->size() * sizeof(int64_t);
      if (memcpy_s(cv->MutableData(), cv->GetCapacity() * sizeof(int64_t), val->data(), copy_size) != EOK) {
        return false;
      }
      cv->SetSize(val->size());

      // todo 拷贝了两次，后面优化
      std::vector<uint8_t> buf(total_size);
      if (memcpy_s(buf.data(), buf.size(), cv_holder.get(), total_size) != EOK) {
        return false;
      }
      attrs.emplace_back(std::move(buf));
      break;
    }
    case ge::AnyValue::VT_TENSOR: {
      auto val = attr.Get<ge::GeTensor>();
      if (val == nullptr) {
        return false;
      }
      auto &tensor_desc = val->GetTensorDesc();
      auto shape_size = tensor_desc.GetShape().GetShapeSize();
      if (shape_size < 0) {
        return false;
      }
      size_t total_size;
      auto tensor_holder = Tensor::CreateFollowing(shape_size, tensor_desc.GetDataType(), total_size);
      if (tensor_holder == nullptr) {
        return false;
      }
      auto tensor = reinterpret_cast<Tensor *>(tensor_holder.get());
      GeShapeToGertShape(tensor_desc.GetShape(), tensor->MutableStorageShape());
      GeShapeToGertShape(tensor_desc.GetOriginShape(), tensor->MutableOriginShape());
      tensor->SetOriginFormat(tensor_desc.GetOriginFormat());
      tensor->SetStorageFormat(tensor_desc.GetFormat());
      if (memcpy_s(tensor->GetData<uint8_t>(), total_size - sizeof(Tensor), val->GetData().GetData(),
                   val->GetData().GetSize()) != EOK) {
        return false;
      }

      std::vector<uint8_t> buf(total_size);
      if (memcpy_s(buf.data(), total_size, tensor_holder.get(), total_size) != EOK) {
        return false;
      }
      attrs.emplace_back(std::move(buf));
      break;
    }
      // todo ListListInt

    default:
      GELOGE(ge::FAILED, "Does not support the attr type now, attr type %d", attr.GetValueType());
      return false;
  }
  return true;
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
    if (!AppendAttr(iter->second, runtime_attrs)) {
      GELOGE(ge::FAILED, "Failed to convert attr %s from node to kernel", attr_name.c_str());
      return false;
    }
  }
  return true;
}
std::unique_ptr<uint8_t[]> CreateAttrBuffer(const std::vector<std::vector<uint8_t>> &attrs, size_t &total_size) {
  total_size = sizeof(RuntimeAttrsDef);
  size_t offset_size;
  if (ge::MulOverflow(sizeof(size_t), attrs.size(), offset_size)) {
    return nullptr;
  }
  if (ge::AddOverflow(total_size, offset_size, total_size)) {
    return nullptr;
  }
  for (const auto &attr : attrs) {
    if (ge::AddOverflow(total_size, attr.size(), total_size)) {
      return nullptr;
    }
  }
  auto attr_holder = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_size]);
  if (attr_holder == nullptr) {
    return nullptr;
  }
  auto attr_def = reinterpret_cast<RuntimeAttrsDef *>(attr_holder.get());
  attr_def->attr_num = attrs.size();
  size_t current_offset = sizeof(RuntimeAttrsDef) + sizeof(size_t) * attr_def->attr_num;
  auto attr_pos = reinterpret_cast<uint8_t *>(attr_holder.get());
  for (size_t i = 0; i < attrs.size(); ++i) {
    attr_def->offset[i] = current_offset;
    if (memcpy_s(attr_pos + current_offset, total_size - current_offset, attrs[i].data(), attrs[i].size()) != EOK) {
      return nullptr;
    }
    current_offset += attrs[i].size();
  }
  return attr_holder;
}
}  // namespace
std::unique_ptr<uint8_t[]> CreateAttrBuffer(const ge::NodePtr &node, size_t &size) {
  std::vector<std::vector<uint8_t>> runtime_attrs;
  if (!GetAllIrAttrs(node, runtime_attrs)) {
    return nullptr;
  }
  return CreateAttrBuffer(runtime_attrs, size);
}
}  // namespace bg
}  // namespace gert