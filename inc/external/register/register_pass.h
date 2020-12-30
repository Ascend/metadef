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

#ifndef INC_EXTERNAL_REGISTER_REGISTER_PASS_H_
#define INC_EXTERNAL_REGISTER_REGISTER_PASS_H_

#include <functional>
#include <memory>
#include <string>
#include <set>

#include "graph/graph.h"
#include "ge/ge_api_error_codes.h"
#include "register/register_types.h"

namespace ge {
class PassRegistrationDataImpl;
using CustomPassFunc = std::function<Status(ge::GraphPtr &)>;

class FMK_FUNC_HOST_VISIBILITY FMK_FUNC_DEV_VISIBILITY PassRegistrationData {
 public:
  PassRegistrationData() = default;
  ~PassRegistrationData() = default;

  PassRegistrationData(std::string pass_name);

  PassRegistrationData &Priority(const int32_t &);

  PassRegistrationData &CustomPassFn(const CustomPassFunc &);

  std::string GetPassName() const;
  int32_t GetPriority() const;
  CustomPassFunc GetCustomPassFn() const;

 private:
  std::shared_ptr<PassRegistrationDataImpl> impl_;
};

class DataGreater : std::greater<PassRegistrationData> {
 public:
  bool opertor()(const PassRegistrationData &a, const PassRegistrationData &b) const {
    return a.GetPriority() < b.GetPriority();
  }
};

class CustomPassHelper {
 public:
  static CustomPassHelper *Instance();

  std::multiset<PassRegistrationData, DataGreater> registration_datas_;

  Status Run(ge::GraphPtr &);

  ~CustomPassHelper() = default;

 private:
  CustomPassHelper() = default;
};

class FMK_FUNC_HOST_VISIBILITY FMK_FUNC_DEV_VISIBILITY PassReceiver {
 public:
  PassReceiver(PassRegistrationData &reg_data);
  ~PassReceiver() = default;
};

#define REGISTER_CUSTOM_PASS(name) REGISTER_CUSTOM_PASS_UNIQ_HELPER(__COUNTER__, name)
#define REGISTER_CUSTOM_PASS_UNIQ_HELPER(ctr, name) REGISTER_CUSTOM_PASS_UNIQ(ctr, name)
#define REGISTER_CUSTOM_PASS_UNIQ(ctr, name)        \
  static ::ge::PassReceiver register_pass##ctr      \
      __attribute__((unused)) =                     \
          ::ge::PassRegistrationData(name)
} // namespace ge
#endif  // INC_EXTERNAL_REGISTER_REGISTER_PASS_H_
