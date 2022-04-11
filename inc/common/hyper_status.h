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

#ifndef AIR_CXX_BASE_COMMON_HYPER_STATUS_H_
#define AIR_CXX_BASE_COMMON_HYPER_STATUS_H_
#include <securec.h>
#include <memory>
namespace gert {
char *CreateMessage(const char *format, va_list arg);
class HyperStatus {
 public:
  bool IsSuccess() const {
    return status_ == nullptr;
  }
  const char *GetErrorMessage() const noexcept {
    return status_;
  }
  ~HyperStatus() {
    delete[] status_;
  }

  HyperStatus() : status_(nullptr) {}
  HyperStatus(const HyperStatus &other);
  HyperStatus(HyperStatus &&other) noexcept;
  HyperStatus &operator=(const HyperStatus &other);
  HyperStatus &operator=(HyperStatus &&other) noexcept;

  static HyperStatus Success();
  static HyperStatus ErrorStatus(char *message, ...);
  static HyperStatus ErrorStatus(std::unique_ptr<char[]> message);
 private:
  char *status_;
};
}

#endif //AIR_CXX_BASE_COMMON_HYPER_STATUS_H_
