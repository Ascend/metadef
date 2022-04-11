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

#ifndef AIR_CXX_RUNTIME_V2_KERNEL_KERNEL_REGISTRY_H_
#define AIR_CXX_RUNTIME_V2_KERNEL_KERNEL_REGISTRY_H_
#include <functional>
#include <unordered_map>
#include <string>

#include "kernel_registry.h"

namespace gert {
struct KernelRegistryImpl : KernelRegistry {
 public:
  static KernelRegistryImpl &GetInstance();
  void RegisterKernel(std::string kernel_type, KernelFuncs func);
  const KernelFuncs *FindKernelFuncs(const std::string &kernel_type) const override;

 private:
  std::unordered_map<std::string, KernelFuncs> types_to_func_;
};

class KernelRegister {
 public:
  explicit KernelRegister(const char *kernel_type);
  KernelRegister(const KernelRegister &other);

  KernelRegister &RunFunc(KernelRegistry::KernelFunc func);

  KernelRegister &OutputsCreator(KernelRegistryImpl::CreateOutputsFunc func);
  KernelRegister &OutputsDestroyer(KernelRegistryImpl::DestroyOutputsFunc func);

  private:
  std::string kernel_type_;
  KernelRegistryImpl::KernelFuncs kernel_funcs_;
};
}

#define REGISTER_KERNEL_COUNTER2(type, counter) static auto g_register_kernel_##counter = gert::KernelRegister(#type)
#define REGISTER_KERNEL_COUNTER(type, counter) REGISTER_KERNEL_COUNTER2(type, counter)
#define REGISTER_KERNEL(type) REGISTER_KERNEL_COUNTER(type, __COUNTER__)

#endif //AIR_CXX_RUNTIME_V2_KERNEL_KERNEL_REGISTRY_H_
