/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#include "register/prototype_pass_registry.h"
#include "graph/debug/ge_log.h"

namespace ge {
class ProtoTypePassRegistry::ProtoTypePassRegistryImpl {
public:
    void RegisterProtoTypePass(ProtoTypePassRegistry::CreateFn create_fn, const domi::FrameworkType &fmk_type) {
      std::lock_guard<std::mutex> lock(mu_);

      auto iter = create_fns_.find(fmk_type);
      if (iter != create_fns_.end()) {
        create_fns_[fmk_type].push_back(create_fn);
        return;
      }

      std::vector<ProtoTypePassRegistry::CreateFn> create_fn_vector;
      create_fn_vector.push_back(create_fn);
      create_fns_[fmk_type] = create_fn_vector;
    }

    std::vector<ProtoTypePassRegistry::CreateFn> GetCreateFnByType(const domi::FrameworkType &fmk_type) {
      std::lock_guard<std::mutex> lock(mu_);
      auto iter = create_fns_.find(fmk_type);
      if (iter == create_fns_.end()) {
        return std::vector<ProtoTypePassRegistry::CreateFn>{};
      }
      return iter->second;
    }

private:
    std::mutex mu_;
    std::map<domi::FrameworkType, std::vector<ProtoTypePassRegistry::CreateFn>> create_fns_;
};

ProtoTypePassRegistry::ProtoTypePassRegistry() {
    impl_ = std::unique_ptr<ProtoTypePassRegistryImpl>(new (std::nothrow) ProtoTypePassRegistryImpl);
}

ProtoTypePassRegistry::~ProtoTypePassRegistry() {}

ProtoTypePassRegistry& ProtoTypePassRegistry::GetInstance() {
  static ProtoTypePassRegistry instance;
  return instance;
}

void ProtoTypePassRegistry::RegisterProtoTypePass(CreateFn create_fn, const domi::FrameworkType &fmk_type) {
  if (impl_ == nullptr) {
    GELOGE(MEMALLOC_FAILED, "ProtoTypePassRegistry is not properly initialized.");
    return;
  }
  impl_->RegisterProtoTypePass(create_fn, fmk_type);
}

std::vector<ProtoTypePassRegistry::CreateFn> ProtoTypePassRegistry::GetCreateFnByType(const domi::FrameworkType &fmk_type) {
  if (impl_ == nullptr) {
    GELOGE(MEMALLOC_FAILED, "ProtoTypePassRegistry is not properly initialized.");
    return std::vector<ProtoTypePassRegistry::CreateFn>{};
  }
  impl_->GetCreateFnByType(fmk_type);
}

ProtoTypePassRegistrar::ProtoTypePassRegistrar(ProtoTypeBasePass *(*create_fn)(),
                                                 const domi::FrameworkType &fmk_type) {
  ProtoTypePassRegistry::GetInstance().RegisterProtoTypePass(create_fn, fmk_type);
}
}