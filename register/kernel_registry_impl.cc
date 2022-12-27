/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "register/kernel_registry_impl.h"
#include <utility>
#include "graph/debug/ge_log.h"
#include "kernel_register_data.h"
namespace gert {
namespace {
std::shared_ptr<KernelRegistry> g_user_defined_registry = nullptr;
}  // namespace

KernelRegistry &KernelRegistry::GetInstance() {
  if (g_user_defined_registry != nullptr) {
    return *g_user_defined_registry;
  } else {
    return KernelRegistryImpl::GetInstance();
  }
}
void KernelRegistry::ReplaceKernelRegistry(std::shared_ptr<KernelRegistry> registry) {
  g_user_defined_registry = std::move(registry);
}

KernelRegistryImpl &KernelRegistryImpl::GetInstance() {
  static KernelRegistryImpl registry;
  return registry;
}
void KernelRegistryImpl::RegisterKernel(std::string kernel_type, KernelRegistry::KernelFuncs func) {
  types_to_func_[std::move(kernel_type)] = std::move(func);
}

const KernelRegistry::KernelFuncs *KernelRegistryImpl::FindKernelFuncs(const std::string &kernel_type) const {
  auto iter = types_to_func_.find(kernel_type);
  if (iter == types_to_func_.end()) {
    return nullptr;
  }
  return &iter->second;
}
const std::unordered_map<std::string, KernelRegistry::KernelFuncs> &KernelRegistryImpl::GetAll() const {
  return types_to_func_;
}
KernelRegister::KernelRegister(const char *kernel_type)
    : register_data_(new(std::nothrow) KernelRegisterData(kernel_type)) {}
KernelRegister::~KernelRegister() {
  delete register_data_;
}
KernelRegister &KernelRegister::RunFunc(KernelRegistry::KernelFunc func) {
  if (register_data_ != nullptr) {
    register_data_->GetFuncs().run_func = func;
  }
  return *this;
}
KernelRegister &KernelRegister::OutputsCreator(KernelRegistry::CreateOutputsFunc func) {
  if (register_data_ != nullptr) {
    register_data_->GetFuncs().outputs_creator = std::move(func);
  }
  return *this;
}
KernelRegister &KernelRegister::OutputsCreatorFunc(KernelRegistry::OutputsCreatorFunc func) {
  if (register_data_ != nullptr) {
    register_data_->GetFuncs().outputs_creator_func = func;
  }
  return *this;
}
KernelRegister &KernelRegister::OutputsInitializer(KernelRegistry::CreateOutputsFunc func) {
  if (register_data_ != nullptr) {
    register_data_->GetFuncs().outputs_initializer = std::move(func);
  }
  return *this;
}
KernelRegister &KernelRegister::TracePrinter(KernelRegistry::TracePrinter func) {
  if (register_data_ != nullptr) {
    register_data_->GetFuncs().trace_printer = func;
  }
  return *this;
}
KernelRegister::KernelRegister(const KernelRegister &other) : register_data_(nullptr) {
  auto register_data = other.register_data_;
  if (register_data == nullptr) {
    GE_LOGE("The register_data_ in register object is nullptr, failed to register funcs");
    return;
  }
  GELOGD("GERT kernel type %s registered", register_data->GetKernelType().c_str());
  KernelRegistry::GetInstance().RegisterKernel(register_data->GetKernelType(), register_data->GetFuncs());
}
}  // namespace gert