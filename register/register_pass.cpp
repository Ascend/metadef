/**
 * Copyright 2020 Huawei Technologies Co., Ltd

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

#include "external/register/register_pass.h"
#include <algorithm>
#include <climits>
#include "graph/debug/ge_log.h"

namespace ge {
PassReceiver::PassReceiver(PassRegistrationData &reg_data) {
  CustomPassHelper::Instance()->registration_datas_.insert(reg_data);
}

class PassRegistrationDataImpl {
 public:
  PassRegistrationDataImpl() = default;
  ~PassRegistrationDataImpl() = default;

  explicit PassRegistrationDataImpl(const std::string &pass_name);

  std::string pass_name_;
  int32_t priority_ = INT_MAX;
  CustomPassFunc custom_pass_fn_ = nullptr;
}

PassRegistrationDataImpl::PassRegistrationDataImpl(const std::string &pass_name)
    : pass_name_(pass_name),
      priority_(INT_MAX),
      custom_pass_fn_(nullptr) {}

PassRegistrationData::PassRegistrationData(std::string pass_name) {
  impl_ = std::shared_ptr<PassRegistrationDataImpl>(new (std::nothrow) PassRegistrationDataImpl(pass_name));
  if (impl_ == nullptr) {
    GELOGW("Custom pass [%s] PassRegistrationDataImpl make shared failed!", pass_name.c_str());
  }
}

std::string PassRegistrationData::GetPassName() const {
  if (impl_ == nullptr) {
    return "";
  }
  return impl_->pass_name_;
}

PassRegistrationData &PassRegistrationData::Priority(const int32_t &priority) {
  if (impl_ != nullptr) {
    if (priority < 0) {
      GELOGW("Custom pass [%s] priority must greater than or equal to 0, but got %d",
             impl_->pass_name_.c_str(), priority);
    } else {
      impl_->priority_ = priority;
    }
  }
  return *this;
}

int32_t PassRegistrationData::GetPriority() const {
  if (impl_ == nullptr) {
    return INT_MAX;
  }
  return impl_->priority_;
}

PassRegistrationData &PassRegistrationData::CustomPassFn(const CustomPassFunc &custom_pass_fn) {
  if (impl_ != nullptr) {
    impl_->custom_pass_fn_ = custom_pass_fn;
  }
  return *this;
}

CustomPassFunc PassRegistrationData::GetCustomPassFn() const {
  if (impl_ == nullptr) {
    return nullptr;
  }
  return impl_->custom_pass_fn_;
}

CustomPassHelper *CustomPassHelper::Instance() {
  static CustomPassHelper instance;
  return &instance;
}

Status CustomPassHelper::Run(ge::GraphPtr &graph) {
  for (auto &item : registration_datas_) {
    GELOGD("Start to run custom pass [%s]", item.GetPassName().c_str());
    auto custom_pass_fn = item.GetCustomPassFn();
    if (custom_pass_fn == nullptr) {
      GELOGW("Custom pass [%s] doesn't have custom_pass_fn!", item.GetPassName().c_str());
      continue;
    }
    if (custom_pass_fn(graph) != SUCCESS) {
      GE_LOGE("Custom pass [%s] run failed!", item.GetPassName().c_str());
      return FAILED;
    }
    GELOGD("Run custom pass [%s] success!", item.GetPassName().c_str());
  }
  return SUCCESS;
}
}  // namespace ge
