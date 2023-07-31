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
#include <cstring>
#include <securec.h>
#include "common/ge_common/debug/ge_log.h"
#include "graph/ascend_string.h"

namespace optiling {
void TilingDef::GeLogError(const std::string& str) const {
  GELOGE(ge::GRAPH_FAILED, "%s", str.c_str());
}

void TilingDef::SaveToBuffer(void *pdata, size_t capacity) {
  // copy tilingdata to buffer without struct tiling data.
  auto mem_ret = memcpy_s(pdata, capacity, data_ptr_, data_size_);
  if (mem_ret != EOK) {
    GELOGE(ge::GRAPH_FAILED,
           "TilingDef::SaveToBuffer failed: memcpy_s return [%d], capacity = [%zu], data_size_ = [%zu].", mem_ret,
           capacity, data_size_);
  }
  // save struct tiling data to buffer
  for (auto ptr : saveBufferPtr) {
    TilingDef* sub_ptr = (TilingDef *)ptr.first;
    size_t offset = ptr.second;
    uint8_t* struct_ptr = (uint8_t*)pdata + offset;
    mem_ret = memcpy_s(struct_ptr, sub_ptr->data_size_, sub_ptr->data_ptr_, sub_ptr->data_size_);
    if (mem_ret != EOK) {
    GELOGE(ge::GRAPH_FAILED,
           "TilingDef::SaveToBuffer failed: memcpy_s return [%d], capacity = [%zu], data_size_ = [%zu].", mem_ret,
           sub_ptr->data_size_, sub_ptr->data_size_);
   }
  }
}

std::vector<FieldInfo> TilingDef::GetFieldInfo() const {
  return field_info_;
}

const char *TilingDef::GetTilingClassName() const {
  return class_name_;
}

size_t TilingDef::GetDataSize() const {
  return data_size_;
}

void TilingDef::InitData() {
    GELOGI("TilingDef::InitData, init data size %d.", data_size_);
    data_ptr_ = new uint8_t[data_size_]();
    if (data_ptr_ == nullptr) {
          GELOGE(ge::GRAPH_FAILED, "TilingDef::InitData failed: init data size %d.", data_size_);
    }
}

StructSizeInfoBase &StructSizeInfoBase::GetInstance()
{
  static StructSizeInfoBase instance;
  return instance;
}

CTilingDataClassFactory &CTilingDataClassFactory::GetInstance()
{
  static CTilingDataClassFactory instance;
  return instance;
}

void CTilingDataClassFactory::RegisterTilingData(const char *op_type,
                                                 const TilingDataConstructor constructor) {
  instance_.emplace(op_type, constructor);
  GELOGI("RegisterTilingData: op_type:%s, constructor:%p, registered count:%zu", op_type, constructor,
         instance_.size());
}

std::shared_ptr<TilingDef> CTilingDataClassFactory::CreateTilingDataInstance(const char *op_type) {
  const auto it = instance_.find(op_type);
  if (it == instance_.end()) {
    GELOGW("CreateTilingDataInstance: cannot find op_type:%s.", op_type);
    return nullptr;
  }
  const TilingDataConstructor constructor = it->second;

  if (constructor == nullptr) {
    GELOGW("CreateTilingDataInstance: constructor is nullptr.");
    return nullptr;
  }

  return (*constructor)();
}
}  // end of namespace optiling
