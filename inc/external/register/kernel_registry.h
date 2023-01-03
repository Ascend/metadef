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
#ifndef B369E37D560547C2B8DC137404F9713E_H
#define B369E37D560547C2B8DC137404F9713E_H
#include <functional>
#include <string>
#include <memory>
#include "graph/ge_error_codes.h"
#include "exe_graph/runtime/base_type.h"
#include "exe_graph/runtime/kernel_context.h"
#include "graph/node.h"

namespace gert {
class KernelRegistry {
 public:
  static KernelRegistry &GetInstance();
  static void ReplaceKernelRegistry(std::shared_ptr<KernelRegistry> registry);

  using CreateOutputsFunc = std::function<ge::graphStatus(const ge::Node *, KernelContext *)>;
  typedef UINT32 (*KernelFunc)(KernelContext *context);
  typedef UINT32 (*OutputsCreatorFunc)(const ge::Node *, KernelContext *);
  typedef std::vector<std::string> (*TracePrinter)(const KernelContext *);
  struct KernelFuncs {
    KernelFunc run_func;
    CreateOutputsFunc outputs_creator; // to be deleted
    CreateOutputsFunc outputs_initializer; // to be deleted
    OutputsCreatorFunc outputs_creator_func;
    TracePrinter trace_printer;
  };

  virtual ~KernelRegistry() = default;
  virtual const KernelFuncs *FindKernelFuncs(const std::string &kernel_type) const = 0;
  virtual void RegisterKernel(const std::string kernel_type, KernelFuncs func) {
    (void) kernel_type;
    (void) func;
  };
};

// todo delete this class after the next synchronization from yellow to blue
class KernelRegister {
 public:
  explicit KernelRegister(const ge::char_t *kernel_type);
  KernelRegister(const KernelRegister &other);
  ~KernelRegister();
  KernelRegister &operator=(const KernelRegister &other) = default;

  KernelRegister &RunFunc(KernelRegistry::KernelFunc func);

  KernelRegister &OutputsCreator(KernelRegistry::CreateOutputsFunc func); // to be deleted
  KernelRegister &OutputsCreatorFunc(KernelRegistry::OutputsCreatorFunc func);
  KernelRegister &OutputsInitializer(KernelRegistry::CreateOutputsFunc func); // to be deleted
  KernelRegister &TracePrinter(KernelRegistry::TracePrinter func);

 private:
  std::string kernel_type_;
  KernelRegistry::KernelFuncs kernel_funcs_;
};

class KernelRegisterData;
class KernelRegisterV2 {
 public:
  explicit KernelRegisterV2(const ge::char_t *kernel_type);
  KernelRegisterV2(const KernelRegisterV2 &other);
  ~KernelRegisterV2();
  KernelRegisterV2 &operator=(const KernelRegisterV2 &other) = delete;
  KernelRegisterV2 &operator=(KernelRegisterV2 &&other) = delete;
  KernelRegisterV2(KernelRegisterV2 &&other) = delete;

  KernelRegisterV2 &RunFunc(KernelRegistry::KernelFunc func);

  KernelRegisterV2 &OutputsCreator(KernelRegistry::CreateOutputsFunc func); // to be deleted
  KernelRegisterV2 &OutputsCreatorFunc(KernelRegistry::OutputsCreatorFunc func);
  KernelRegisterV2 &OutputsInitializer(KernelRegistry::CreateOutputsFunc func); // to be deleted
  KernelRegisterV2 &TracePrinter(KernelRegistry::TracePrinter func);

 private:
  std::unique_ptr<KernelRegisterData> register_data_;
};
}  // namespace gert

#define REGISTER_KERNEL_COUNTER2(type, counter) static auto g_register_kernel_##counter = gert::KernelRegisterV2(#type)
#define REGISTER_KERNEL_COUNTER(type, counter) REGISTER_KERNEL_COUNTER2(type, counter)
#define REGISTER_KERNEL(type) REGISTER_KERNEL_COUNTER(type, __COUNTER__)

#endif
