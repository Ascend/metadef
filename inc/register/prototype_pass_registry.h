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

#ifndef METADEF_PROTOTYPE_PASS_REGISTRY_H
#define METADEF_PROTOTYPE_PASS_REGISTRY_H

#include <google/protobuf/message.h>
#include "register/register_fmk_types.h"
#include "register/register_types.h"
#include "external/ge/ge_api_error_codes.h"
#include "common/ge_inner_error_codes.h"

namespace ge {
class ProtoTypeBasePass {
 public:
    ProtoTypeBasePass(std::string name) {
      name_ = name;
    };
    virtual Status Run(google::protobuf::Message *message) = 0;
    std::string GetName() const { return name_; }
private:
    std::string name_;
};

class ProtoTypePassRegistry {
 public:
  using CreateFn = ProtoTypeBasePass *(*)();
  ~ProtoTypePassRegistry();

  static ProtoTypePassRegistry& GetInstance();

  void RegisterProtoTypePass(CreateFn create_fn, const domi::FrameworkType &fmk_type);

  std::vector<CreateFn> GetCreateFnByType(const domi::FrameworkType &fmk_type);

private:
  ProtoTypePassRegistry();
  class ProtoTypePassRegistryImpl;
  /*lint -e148*/
  std::unique_ptr<ProtoTypePassRegistryImpl> impl_;
};

class ProtoTypePassRegistrar {
 public:
   ProtoTypePassRegistrar(ProtoTypeBasePass *(*create_fn)(), const domi::FrameworkType &fmk_type);
   ~ProtoTypePassRegistrar() {}
};

#define REGISTER_PROTOTYPE_PASS(pass_name, pass, fmk_type) \
  REGISTER_PROTOTYPE_PASS_UNIQ_HELPER(__COUNTER__, pass_name, pass, fmk_type)

#define REGISTER_PROTOTYPE_PASS_UNIQ_HELPER(ctr, pass_name, pass, fmk_type) \
  REGISTER_PROTOTYPE_PASS_UNIQ(ctr, pass_name, pass, fmk_type)

#define REGISTER_PROTOTYPE_PASS_UNIQ(ctr, pass_name, pass, fmk_type)                                 \
  static ::ge::ProtoTypePassRegistrar register_prototype_pass##ctr __attribute__((unused)) = \
      ::ge::ProtoTypePassRegistrar([]() -> ::ge::ProtoTypeBasePass * { return new (std::nothrow) pass(pass_name); }, \
                                     fmk_type)
}
#endif //METADEF_PROTOTYPE_PASS_REGISTRY_H
