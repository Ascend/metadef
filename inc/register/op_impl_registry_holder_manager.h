/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
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

#ifndef INC_OP_IMPL_REGISTRY_HOLDER_MANAGER_H_
#define INC_OP_IMPL_REGISTRY_HOLDER_MANAGER_H_

#include "graph/op_so_bin.h"
#include "external/register/op_impl_kernel_registry.h"
#include "register/op_impl_registry_api.h"
#include <string>
#include <map>

namespace gert {
class OpImplRegistryHolder {
 public:
  OpImplRegistryHolder() = default;

  virtual ~OpImplRegistryHolder();

  std::map<OpImplKernelRegistry::OpType, OpImplKernelRegistry::OpImplFunctions> &GetTypesToImpl() {
    return types_to_impl_;
  }

  void SetHandle(void *handle) { handle_ = handle; }

  std::unique_ptr<TypesToImpl[]> GetOpImplFunctionsByHandle(void *handle, const string &so_path, size_t &impl_num);

  void AddTypesToImpl(gert::OpImplKernelRegistry::OpType op_type, gert::OpImplKernelRegistry::OpImplFunctions funcs);
 protected:
  std::map<OpImplKernelRegistry::OpType, OpImplKernelRegistry::OpImplFunctions> types_to_impl_;
  void *handle_ = nullptr;
};
using OpImplRegistryHolderPtr = std::shared_ptr<OpImplRegistryHolder>;

class OmOpImplRegistryHolder : public OpImplRegistryHolder {
 public:
  OmOpImplRegistryHolder() = default;

  ~OmOpImplRegistryHolder();

  ge::graphStatus LoadSo(const std::shared_ptr<ge::OpSoBin> &so_bin);

 private:
  ge::graphStatus CreateOmOppDir(std::string &opp_dir);

  ge::graphStatus RmOmOppDir(const std::string &opp_dir);

  ge::graphStatus SaveToFile(const std::shared_ptr<ge::OpSoBin> &so_bin, const std::string &opp_path);

 private:
  std::string so_dir_;
};

class OpImplRegistryHolderManager {
 public:
  OpImplRegistryHolderManager() = default;

  ~OpImplRegistryHolderManager() = default;

  static OpImplRegistryHolderManager &GetInstance();

  void AddRegistry(std::string &so_data, const std::shared_ptr<OpImplRegistryHolder> &registry_holder);

  void UpdateOpImplRegistries();

  const OpImplRegistryHolderPtr GetOpImplRegistryHolder (std::string &so_data);

  OpImplRegistryHolderPtr GetOrCreateOpImplRegistryHolder(std::string &so_data,
                                                          const std::string &so_name,
                                                          const ge::SoInOmInfo &so_info,
                                                          std::function<OpImplRegistryHolderPtr()> create_func);

  size_t GetOpImplRegistrySize() { return op_impl_registries_.size(); }
 private:
  std::unordered_map<std::string, std::weak_ptr<OpImplRegistryHolder>> op_impl_registries_;
  std::mutex map_mutex_;
};
}  // namespace gert
#endif  // INC_OP_IMPL_REGISTRY_HOLDER_MANAGER_H_
