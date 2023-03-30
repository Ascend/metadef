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

#include "register/tilingdata_base.h"
#include <securec.h>
#include "framework/common/debug/ge_log.h"
#include "graph/ascend_string.h"

namespace optiling {
std::map<ge::AscendString, TilingDataConstructor> CTilingDataClassFactory::instance_;

void TilingDef::SaveToBuffer(void *pdata, size_t capacity) const {
  auto  mem_ret = memcpy_s(pdata, capacity, data_ptr_, data_size_);
  if (mem_ret != EOK) {
    GELOGW("TilingDef::SaveToBuffer failed: memcpy_s return [%d].", mem_ret);
  }
}

std::vector<FieldInfo> TilingDef::GetFieldInfo() const {
  return field_info_;
}

ge::AscendString TilingDef::GetTilingClassName() const {
  return class_name_;
}

size_t TilingDef::GetDataSize() const {
  return data_size_;
}

void TilingDef::InitData() {
  if (data_ptr_ != nullptr) {
    delete data_ptr_;
    data_ptr_ = nullptr;
  }
  if (data_size_ > 0) {
    data_ptr_ = new uint8_t[data_size_];
  }
}

void CTilingDataClassFactory::RegisterTilingData(ge::AscendString op_type, TilingDataConstructor constructor) {
  instance_[op_type] = constructor;
  GELOGI("RegisterTilingData: op_type:%s, constructor:%p, registered count:%zu", op_type.GetString(), constructor,
         instance_.size());
}

std::shared_ptr<TilingDef> CTilingDataClassFactory::CreateTilingDataInstance(const ge::AscendString &op_type) {
  auto it = instance_.find(op_type);
  if (it == instance_.end()) {
    GELOGW("CreateTilingDataInstance: cannot find op_type:%s.", op_type.GetString());
    return nullptr;
  }
  TilingDataConstructor constructor = it->second;

  if (constructor == nullptr) {
    GELOGW("CreateTilingDataInstance: constructor is nullptr.");
    return nullptr;
  }

  return (*constructor)();
}
}  // end of namespace optiling
