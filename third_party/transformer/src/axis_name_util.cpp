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


#include "transformer/inc/axis_name_util.h"

namespace common {
namespace transformer {
uint32_t AxisNameUtil::ReshapeTypeToUint(const ge::Format &format, const std::string &reshape_type) {
  if (reshape_type.empty()) {
    GELOGD("Reshape type is empty, return default reshape type.");
    return RESHAPE_DEFAULT;
  }

  // get reshape type according to axis name.
  auto iter_format = ALL_RESHAPE_TYPE_MAP.find(format);
  if (iter_format == ALL_RESHAPE_TYPE_MAP.end()) {
    return RESHAPE_DEFAULT;
  }

  auto iter = iter_format->second.find(reshape_type);
  if (iter != iter_format->second.end()) {
    GELOGD("new reshape type is : %u, axis_names is : %s!", (uint32_t)iter->second, reshape_type.c_str());
    return iter->second;
  } else {
    GELOGW("Can not get reshape type by name : %s!", reshape_type.c_str());
  }
  return RESHAPE_DEFAULT;
}
} // namespace transformer
} // namespace common
