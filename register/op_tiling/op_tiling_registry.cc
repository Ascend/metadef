/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
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

#include "register/op_tiling_registry.h"
#include "framework/common/debug/ge_log.h"

namespace optiling {
size_t ByteBufferGetAll(ByteBuffer &buf, char *dest, size_t dest_len) {
  size_t nread = 0;
  size_t rn = 0;
  do {
    rn = buf.readsome(dest + nread, dest_len - nread);
    nread += rn;
  } while (rn > 0 && dest_len > nread);

  return nread;
}

ByteBuffer &ByteBufferPut(ByteBuffer &buf, const uint8_t *data, size_t data_len) {
  buf.write(reinterpret_cast<const char *>(data), data_len);
  buf.flush();
  return buf;
}

std::unordered_map<std::string, OpTilingFunc> &OpTilingRegistryInterf::RegisteredOpInterf() {
  static std::unordered_map<std::string, OpTilingFunc> interf;
  return interf;
}

OpTilingRegistryInterf::OpTilingRegistryInterf(std::string op_type, OpTilingFunc func) {
  auto &interf = RegisteredOpInterf();
  interf.emplace(op_type, func);
  GELOGI("Register tiling function: op_type:%s, funcPointer:%p, registered count:%zu", op_type.c_str(),
         func.target<OpTilingFuncPtr>(), interf.size());
}

namespace utils {
std::unordered_map<std::string, OpTilingFuncV2> &OpTilingRegistryInterf_V2::RegisteredOpInterf() {
  static std::unordered_map<std::string, OpTilingFuncV2> interf;
  GELOGI("Generate interf by new method, registered count: %zu", interf.size());
  return interf;
}

OpTilingRegistryInterf_V2::OpTilingRegistryInterf_V2(const std::string &op_type, OpTilingFuncV2 func) {
  auto &interf = RegisteredOpInterf();
  interf.emplace(op_type, std::move(func));
  GELOGI("Register tiling function by new method: op_type:%s, registered count:%zu", op_type.c_str(), interf.size());
}
}  // namespace utils
}  // namespace optiling
