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
#include "exe_graph/lowering/bg_kernel_context_extend.h"
#include "exe_graph/lowering/bg_ir_attrs.h"
#include "exe_graph/runtime/context_extend.h"
#include "framework/common/debug/ge_log.h"
namespace gert {
namespace bg {
namespace {
ge::graphStatus GetInstanceNum(const ge::NodePtr &node, const std::string &ir_name, ge::IrInputType ir_type,
                               size_t start_index, size_t &instance_num) {
  auto node_indegree = node->GetAllInDataAnchorsSize();
  if (ir_type == ge::kIrInputRequired) {
    auto name = node->GetOpDesc()->GetValidInputNameByIndex(start_index);
    if (name != ir_name) {
      GELOGE(ge::FAILED,
             "Failed to get instance num for node %s, can not find the input for ir name %s, current index %zu, "
             "current name %s",
             node->GetName().c_str(), ir_name.c_str(), start_index, name.c_str());
      return ge::FAILED;
    }
    instance_num = 1;
    return ge::SUCCESS;
  }
  if (ir_type == ge::kIrInputOptional) {
    auto name = node->GetOpDesc()->GetValidInputNameByIndex(start_index);
    if (name == ir_name) {
      instance_num = 1;
    } else {
      instance_num = 0;
    }
    return ge::SUCCESS;
  }
  if (ir_type == ge::kIrInputDynamic) {
    size_t dyn_i = 0;
    for (size_t i = start_index; i < node_indegree; ++i, ++dyn_i) {
      auto name = node->GetOpDesc()->GetValidInputNameByIndex(i);
      if (name != ir_name + std::to_string(dyn_i)) {
        break;
      }
    }
    instance_num = dyn_i;
    return ge::SUCCESS;
  }
  GELOGE(ge::FAILED, "Failed to get instance num for node %s, unknown ir input type %d, ir name %s",
         node->GetName().c_str(), ir_type, ir_name.c_str());
  return ge::FAILED;
}
ge::graphStatus InitInputInstanceInfo(const ge::NodePtr &node, ComputeNodeInfo &compute_node_info) {
  const auto &ir_inputs = node->GetOpDesc()->GetIrInputs();
  size_t input_index = 0;
  for (size_t i = 0; i < ir_inputs.size(); ++i) {
    auto ins_info = compute_node_info.MutableInputInstanceInfo(i);
    if (ins_info == nullptr) {
      return ge::GRAPH_FAILED;
    }
    size_t instance_num = 0;
    auto ret = GetInstanceNum(node, ir_inputs[i].first, ir_inputs[i].second, input_index, instance_num);
    if (ret != ge::SUCCESS) {
      return ret;
    }
    compute_node_info.MutableInputInstanceInfo(i)->SetInstantiationNum(instance_num);
    compute_node_info.MutableInputInstanceInfo(i)->SetInstanceStart(input_index);
    input_index += instance_num;
  }
  //  // TODO 001： test graph has no irputs stub here
  //  if (input_index != node->GetAllInDataAnchorsSize()) {
  //    return ge::GRAPH_FAILED;
  //  }
  return ge::GRAPH_SUCCESS;
}

void SetCompileTimeTd(const ge::ConstGeTensorDescPtr &desc, CompileTimeTensorDesc &td) {
  td.SetDataType(desc->GetDataType());
  td.SetOriginFormat(desc->GetOriginFormat());
  td.SetStorageFormat(desc->GetFormat());
  // todo set expand dims type
  td.SetExpandDimsType(ExpandDimsType(""));
}
ge::graphStatus InitCompileTimeTD(const ge::NodePtr &node, ComputeNodeInfo &compute_node_info) {
  for (size_t i = 0; i < node->GetAllInDataAnchorsSize(); ++i) {
    auto desc = node->GetOpDesc()->GetInputDescPtr(i);
    if (desc == nullptr) {
      return ge::FAILED;
    }
    auto td = compute_node_info.MutableInputTdInfo(i);
    if (td == nullptr) {
      return ge::FAILED;
    }
    SetCompileTimeTd(desc, *td);
  }

  for (size_t i = 0; i < node->GetAllOutDataAnchorsSize(); ++i) {
    auto desc = node->GetOpDesc()->GetOutputDescPtr(i);
    if (desc == nullptr) {
      return ge::FAILED;
    }
    auto td = compute_node_info.MutableOutputTdInfo(i);
    if (td == nullptr) {
      return ge::FAILED;
    }
    SetCompileTimeTd(desc, *td);
  }
  return ge::SUCCESS;
}

}  // namespace
std::unique_ptr<uint8_t[]> CreateComputeNodeInfo(const ge::NodePtr &node, BufferPool &buffer_pool, size_t &total_size) {
  size_t attr_size;
  auto attr_buf = CreateAttrBuffer(node, attr_size);
  if (attr_buf == nullptr) {
    return nullptr;
  }

  auto ir_input_num = node->GetOpDesc()->GetIrInputs().size();
  auto input_num = node->GetAllInDataAnchorsSize();
  auto output_num = node->GetAllOutDataAnchorsSize();
  if (ComputeNodeInfo::CalcSize(ir_input_num, input_num, output_num, total_size) != ge::GRAPH_SUCCESS) {
    return nullptr;
  }
  if (ge::AddOverflow(total_size, attr_size, total_size)) {
    return nullptr;
  }
  auto compute_node_info_holder = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_size]());
  if (compute_node_info_holder == nullptr) {
    return nullptr;
  }

  auto node_name = buffer_pool.AddStr(node->GetName().c_str());
  auto node_type = buffer_pool.AddStr(node->GetType().c_str());
  auto compute_node_info = reinterpret_cast<ComputeNodeInfo *>(compute_node_info_holder.get());
  compute_node_info->Init(ir_input_num, input_num, output_num, reinterpret_cast<const char *>(node_name),
                          reinterpret_cast<const char *>(node_type));

  auto ret = InitInputInstanceInfo(node, *compute_node_info);
  if (ret != ge::SUCCESS) {
    return nullptr;
  }

  ret = InitCompileTimeTD(node, *compute_node_info);
  if (ret != ge::SUCCESS) {
    return nullptr;
  }

  auto attr = compute_node_info->MutableAttrs();
  auto offset = reinterpret_cast<uint8_t *>(attr) - compute_node_info_holder.get();
  if (offset > total_size) {
    GELOGE(
        ge::FAILED,
        "Failed to create kernel context extend info, the offset of attr %zu beyond the total size of ExtendInfo %zu",
        offset, attr_size);
    return nullptr;
  }
  if (memcpy_s(attr, total_size - offset, attr_buf.get(), attr_size) != EOK) {
    GELOGE(ge::FAILED, "Failed to create kernel context extend info, the remain size %zu for attr is not enough %zu",
           total_size - offset, attr_size);
    return nullptr;
  }

  return compute_node_info_holder;
}
std::unique_ptr<uint8_t[]> CreateComputeNodeInfo(const ge::NodePtr &node, BufferPool &buffer_pool) {
  size_t total_size;
  return CreateComputeNodeInfo(node, buffer_pool, total_size);
}
}  // namespace bg
}  // namespace gert