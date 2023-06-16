/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "register/op_impl_space_registry.h"
#include "common/checker.h"
#include "common/util/mem_utils.h"
#include "graph/any_value.h"
#include "graph/debug/ge_log.h"
#include "graph/utils/file_utils.h"
#include "register/op_impl_registry.h"
#include "register/op_impl_registry_holder_manager.h"
#include "mmpa/mmpa_api.h"

#define MERGE_FUNCTION(merged_funcs, src_funcs, op_type, func_name)           \
  if ((merged_funcs).func_name == nullptr) {                                  \
    (merged_funcs).func_name = (src_funcs).func_name;                         \
  } else if ((src_funcs).func_name != nullptr) {                              \
    GELOGW("op type %s %s func has been registered", op_type, #func_name);    \
  } else {                                                                    \
  }

namespace gert {
namespace {
void CloseHandle(void * const handle) {
  if (handle != nullptr) {
    if (mmDlclose(handle) != 0) {
      const ge::char_t *error = mmDlerror();
      error = (error == nullptr) ? "" : error;
      GELOGW("[Close][Handle] failed, reason:%s", error);
    }
  }
}
}
ge::graphStatus OpImplSpaceRegistry::GetOrCreateRegistry(const std::vector<ge::OpSoBinPtr> &bins,
                                                         const ge::SoInOmInfo &so_info) {
  for (const auto &so_bin : bins) {
    GE_ASSERT_NOTNULL(so_bin, "so bin must not be nullptr");
    std::string so_data(so_bin->GetBinData(), so_bin->GetBinData() + so_bin->GetBinDataSize());
    const auto create_func = [&so_bin]() -> OpImplRegistryHolderPtr {
      auto om_registry_holder = std::make_shared<OmOpImplRegistryHolder>();
      if (om_registry_holder == nullptr) {
        GELOGE(ge::FAILED, "make_shared om op impl registry holder failed");
        return nullptr;
      }
      if ((om_registry_holder->LoadSo(so_bin)) != ge::GRAPH_SUCCESS) {
        GELOGE(ge::FAILED, "om registry holder load so failed");
        return nullptr;
      }
      return om_registry_holder;
    };
    const auto registry_holder =
        OpImplRegistryHolderManager::GetInstance().GetOrCreateOpImplRegistryHolder(so_data,
                                                                                   so_bin->GetSoName(),
                                                                                   so_info,
                                                                                   create_func);
    GE_CHECK_NOTNULL(registry_holder);
    GE_ASSERT_SUCCESS(AddRegistry(registry_holder));
  }
  return ge::GRAPH_SUCCESS;
}

void OpImplSpaceRegistry::MergeFunctions(OpImplKernelRegistry::OpImplFunctions &merged_funcs,
                                         const OpImplKernelRegistry::OpImplFunctions &src_funcs,
                                         const std::string &op_type) const {
  MERGE_FUNCTION(merged_funcs, src_funcs, op_type.c_str(), infer_shape)
  MERGE_FUNCTION(merged_funcs, src_funcs, op_type.c_str(), infer_shape_range)
  MERGE_FUNCTION(merged_funcs, src_funcs, op_type.c_str(), infer_datatype)
  MERGE_FUNCTION(merged_funcs, src_funcs, op_type.c_str(), tiling_parse)
  MERGE_FUNCTION(merged_funcs, src_funcs, op_type.c_str(), compile_info_creator)
  MERGE_FUNCTION(merged_funcs, src_funcs, op_type.c_str(), compile_info_deleter)
  MERGE_FUNCTION(merged_funcs, src_funcs, op_type.c_str(), tiling)

  if (merged_funcs.max_tiling_data_size == 0U) {
    merged_funcs.max_tiling_data_size = src_funcs.max_tiling_data_size;
  } else if (src_funcs.max_tiling_data_size != 0U) {
    GELOGW("op type %s max_tiling_data_size has been registered", op_type.c_str());
  } else {
    // 已经注册且没有重复注册
  }

  if (merged_funcs.inputs_dependency == 0U) {
    merged_funcs.inputs_dependency = src_funcs.inputs_dependency;
  } else if (src_funcs.inputs_dependency != 0U) {
    GELOGW("op type %s inputs_dependency has been registered", op_type.c_str());
  } else {
    // 已经注册且没有重复注册
  }
  if (merged_funcs.private_attrs.size() == 0U) {
    merged_funcs.private_attrs = src_funcs.private_attrs;
  } else if (src_funcs.private_attrs.size() != 0U) {
    GELOGW("op type %s private_attrs has been registered", op_type.c_str());
  } else {
    // 已经注册且没有重复注册
  }
  if (merged_funcs.unique_private_attrs.size() == 0U) {
    merged_funcs.unique_private_attrs = src_funcs.unique_private_attrs;
  } else if (src_funcs.unique_private_attrs.size() != 0U) {
    GELOGW("op type %s unique_private_attrs has been registered", op_type.c_str());
  } else {
    // 已经注册且没有重复注册
  }
}

void OpImplSpaceRegistry::MergeTypesToImpl(OpTypesToImplMap &merged_impl, OpTypesToImplMap &src_impl) const {
  for (auto iter = src_impl.cbegin(); iter != src_impl.cend(); ++iter) {
    const auto op_type = iter->first;
    GELOGD("Merge types to impl, op type %s", op_type.GetString());
    if (merged_impl.find(op_type) == merged_impl.end()) {
      merged_impl[op_type] = src_impl[op_type];
      continue;
    } else {
      const auto src_funcs = iter->second;
      MergeFunctions(merged_impl[op_type], src_funcs, op_type.GetString());
    }
  }
}

ge::graphStatus OpImplSpaceRegistry::AddRegistry(const std::shared_ptr<OpImplRegistryHolder> &registry_holder) {
  if (registry_holder != nullptr) {
    op_impl_registries_.emplace_back(registry_holder);
    MergeTypesToImpl(merged_types_to_impl_, registry_holder->GetTypesToImpl());
  }
  return ge::GRAPH_SUCCESS;
}

const OpImplKernelRegistry::OpImplFunctions *OpImplSpaceRegistry::GetOpImpl(const std::string &op_type) const {
  const auto iter = merged_types_to_impl_.find(op_type.c_str());
  if (iter == merged_types_to_impl_.cend()) {
    return nullptr;
  }
  return &iter->second;
}

const OpImplKernelRegistry::PrivateAttrList &OpImplSpaceRegistry::GetPrivateAttrs(const std::string &op_type) const {
  const auto op_impl_ptr = GetOpImpl(op_type.c_str());
  if (op_impl_ptr == nullptr) {
    static OpImplKernelRegistry::PrivateAttrList emptyPrivateAttr;
    return emptyPrivateAttr;
  }
  return op_impl_ptr->private_attrs;
}

DefaultOpImplSpaceRegistry &DefaultOpImplSpaceRegistry::GetInstance() {
  static DefaultOpImplSpaceRegistry instance;
  return instance;
}

ge::graphStatus OpImplSpaceRegistry::LoadSoAndSaveToRegistry(const std::string &so_path, void **so_handle) {
  uint32_t len = 0U;
  const auto so_data = ge::GetBinFromFile(const_cast<std::string &>(so_path), len);
  std::string str_so_data(so_data.get(), so_data.get()+len);
  if (gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(str_so_data) != nullptr) {
    GELOGI("So already loaded!");
    return ge::GRAPH_FAILED;
  }
  void * const handle = mmDlopen(so_path.c_str(),
                                 static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
                                     static_cast<uint32_t>(MMPA_RTLD_GLOBAL)));
  if (handle == nullptr) {
    GELOGW("Failed to dlopen %s! errmsg:%s", so_path.c_str(), mmDlerror());
    return ge::GRAPH_FAILED;
  }
  const std::function<void()> callback = [&handle]() {
    CloseHandle(handle);
  };
  GE_DISMISSABLE_GUARD(close_handle, callback);
  const auto om_registry_holder = ge::MakeShared<gert::OpImplRegistryHolder>();
  GE_CHECK_NOTNULL(om_registry_holder);
  size_t impl_num = 0U;
  const auto impl_funcs = om_registry_holder->GetOpImplFunctionsByHandle(handle, so_path, impl_num);
  if (impl_funcs == nullptr) {
    GELOGW("Failed to get funcs from so!");
    return ge::GRAPH_FAILED;
  }
  for (size_t i = 0U; i < impl_num; i++) {
    om_registry_holder->GetTypesToImpl()[impl_funcs[i].op_type] = impl_funcs[i].funcs;
  }
  gert::OpImplRegistryHolderManager::GetInstance().AddRegistry(str_so_data, om_registry_holder);
  auto space_registry = gert::DefaultOpImplSpaceRegistry::GetInstance().GetDefaultSpaceRegistry();
  if (space_registry == nullptr) {
    space_registry = std::make_shared<gert::OpImplSpaceRegistry>();
    GE_CHECK_NOTNULL(space_registry);
    gert::DefaultOpImplSpaceRegistry::GetInstance().SetDefaultSpaceRegistry(space_registry);
  }
  const auto ret = space_registry->AddRegistry(om_registry_holder);
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGW("Space registry add new holder failed!");
    return ge::GRAPH_FAILED;
  }
  om_registry_holder->SetHandle(handle);
  *so_handle = handle;
  GELOGI("Save so symbol and handle in path[%s] success!", so_path.c_str());
  GE_DISMISS_GUARD(close_handle);
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert
