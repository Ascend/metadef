/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "register/graph_optimizer/buffer_fusion/buffer_fusion_pass_registry.h"
#include <algorithm>
#include <map>
#include <mutex>
#include <utility>
#include <vector>
#include "graph/debug/ge_log.h"

namespace fe {
class BufferFusionPassRegistry::BufferFusionPassRegistryImpl {
 public:
#ifndef ONLY_COMPILE_OPEN_SRC
  void RegisterPass(const BufferFusionPassType &pass_type, const std::string &pass_name,
                    const BufferFusionPassRegistry::CreateFn create_fn, uint64_t attr) {
    const std::lock_guard<std::mutex> lock(mu_);
    const auto iter = pass_descs_.find(pass_type);
    if (iter != pass_descs_.end()) {
      pass_descs_[pass_type][pass_name].attr = attr;
      pass_descs_[pass_type][pass_name].create_fn = create_fn;
      GELOGI("UbFusionPass[type=%d, name=%s, attr=%lu]: the pass type already exists.",
             pass_type, pass_name.c_str(), attr);
      return;
    }

    std::map<std::string, PassDesc> pass_desc;
    pass_desc[pass_name] = {attr, create_fn};
    pass_descs_[pass_type] = pass_desc;
    GELOGI("UbFusionPass[type=%d, name=%s, attr=%lu]: the pass type does not exists.",
           pass_type, pass_name.c_str(), attr);
  }

  std::map<std::string, BufferFusionPassRegistry::CreateFn> GetCreateFn(const BufferFusionPassType &pass_type) {
    const std::lock_guard<std::mutex> lock(mu_);
    std::map<std::string, BufferFusionPassRegistry::CreateFn> ret;
    const auto iter = pass_descs_.find(pass_type);
    if (iter == pass_descs_.end()) {
      return ret;
    }
    for (const auto &ele : iter->second) {
      ret.emplace(std::make_pair(ele.first, ele.second.create_fn));
    }
    return ret;
  }

  map<std::string, PassDesc> GetPassDesc(const BufferFusionPassType &pass_type) {
    const std::lock_guard<std::mutex> lock(mu_);
    map<std::string, PassDesc> result;
    const auto iter = pass_descs_.find(pass_type);
    if (iter == pass_descs_.end()) {
      return result;
    }
    return iter->second;
  }
 private:
  std::mutex mu_;
  std::map<BufferFusionPassType, map<std::string, PassDesc>> pass_descs_;
#else
void RegisterPass(const BufferFusionPassType &pass_type, const std::string &pass_name,
                    const BufferFusionPassRegistry::CreateFn create_fun) {
    const std::lock_guard<std::mutex> lock(mu_);
    const auto iter = create_fns_.find(pass_type);
    if (iter != create_fns_.end()) {
      create_fns_[pass_type][pass_name] = create_fun;
      GELOGI("UbFusionPass[type=%d,name=%s]: the pass type already exists.", pass_type, pass_name.c_str());
      return;
    }

    std::map<std::string, BufferFusionPassRegistry::CreateFn> create_fn_map;
    create_fn_map[pass_name] = create_fun;
    create_fns_[pass_type] = create_fn_map;
    GELOGI("UbFusionPass[type=%d,name=%s]: the pass type does not exists.", pass_type, pass_name.c_str());
  }

  std::map<std::string, BufferFusionPassRegistry::CreateFn> GetCreateFn(const BufferFusionPassType &pass_type) {
    const std::lock_guard<std::mutex> lock(mu_);
    std::map<std::string, BufferFusionPassRegistry::CreateFn> result;
    const auto iter = create_fns_.find(pass_type);
    if (iter == create_fns_.end()) {
      return result;
    }
    return iter->second;
  }

 private:
  std::mutex mu_;
  std::map<BufferFusionPassType, map<std::string, BufferFusionPassRegistry::CreateFn>> create_fns_;
#endif
};

BufferFusionPassRegistry::BufferFusionPassRegistry() {
  impl_ = std::unique_ptr<BufferFusionPassRegistryImpl>(new (std::nothrow) BufferFusionPassRegistryImpl);
}

BufferFusionPassRegistry::~BufferFusionPassRegistry() {}

BufferFusionPassRegistry &BufferFusionPassRegistry::GetInstance() {
  static BufferFusionPassRegistry instance;
  return instance;
}
#ifndef ONLY_COMPILE_OPEN_SRC
void BufferFusionPassRegistry::RegisterPass(const BufferFusionPassType &pass_type, const std::string &pass_name,
                                            const CreateFn &create_fun, uint64_t attr) {
  if (impl_ == nullptr) {
    GELOGE(ge::MEMALLOC_FAILED, "[Check][Param]UbFusionPass[type=%d,name=%s]: failed to register the ub fusion pass",
           pass_type, pass_name.c_str());
    return;
  }
  impl_->RegisterPass(pass_type, pass_name, create_fun, attr);
}

std::map<std::string, BufferFusionPassRegistry::PassDesc> BufferFusionPassRegistry::GetPassDesc(
    const BufferFusionPassType &pass_type) {
  if (impl_ == nullptr) {
    GELOGE(ge::MEMALLOC_FAILED, "[Check][Param]UbFusionPass[type=%d]: failed to get pass desc", pass_type);
    std::map<std::string, PassDesc> ret;
    return ret;
  }
  return impl_->GetPassDesc(pass_type);
}
#else
void BufferFusionPassRegistry::RegisterPass(const BufferFusionPassType &pass_type, const std::string &pass_name,
                                            const CreateFn &create_fun) {
  if (impl_ == nullptr) {
    GELOGE(ge::MEMALLOC_FAILED, "[Check][Param]UbFusionPass[type=%d,name=%s]: failed to register the ub fusion pass",
           pass_type, pass_name.c_str());
    return;
  }
  impl_->RegisterPass(pass_type, pass_name, create_fun);
}
#endif

std::map<std::string, BufferFusionPassRegistry::CreateFn> BufferFusionPassRegistry::GetCreateFnByType(
    const BufferFusionPassType &pass_type) {
  if (impl_ == nullptr) {
    GELOGE(ge::MEMALLOC_FAILED, "[Check][Param]UbFusionPass[type=%d]: failed to create the ub fusion pass", pass_type);
    return std::map<std::string, CreateFn>{};
  }
  return impl_->GetCreateFn(pass_type);
}

#ifndef ONLY_COMPILE_OPEN_SRC
BufferFusionPassRegistrar::BufferFusionPassRegistrar(const BufferFusionPassType &pass_type,
                                                     const std::string &pass_name,
                                                     BufferFusionPassBase *(*create_fun)(),
                                                     uint64_t attr) {
  if ((pass_type < BUILT_IN_AI_CORE_BUFFER_FUSION_PASS) || (pass_type >= BUFFER_FUSION_PASS_TYPE_RESERVED)) {
    GELOGE(ge::PARAM_INVALID, "[Check][Param:pass_type] value %d is not supported.", pass_type);
    return;
  }

  if (pass_name.empty()) {
    GELOGE(ge::PARAM_INVALID, "[Check][Param:pass_name]Failed to register the ub fusion pass, the pass name is empty.");
    return;
  }

  BufferFusionPassRegistry::GetInstance().RegisterPass(pass_type, pass_name, create_fun, attr);
}
#else
BufferFusionPassRegistrar::BufferFusionPassRegistrar(const BufferFusionPassType &pass_type,
                                                     const std::string &pass_name,
                                                     BufferFusionPassBase *(*create_fun)()) {
  if ((pass_type < BUILT_IN_AI_CORE_BUFFER_FUSION_PASS) || (pass_type >= BUFFER_FUSION_PASS_TYPE_RESERVED)) {
    GELOGE(ge::PARAM_INVALID, "[Check][Param:pass_type] value %d is not supported.", pass_type);
    return;
  }

  if (pass_name.empty()) {
    GELOGE(ge::PARAM_INVALID, "[Check][Param:pass_name]Failed to register the ub fusion pass, the pass name is empty.");
    return;
  }

  BufferFusionPassRegistry::GetInstance().RegisterPass(pass_type, pass_name, create_fun);
}
#endif
}  // namespace fe