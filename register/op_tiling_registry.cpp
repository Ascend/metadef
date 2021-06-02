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

#include "register/op_tiling_registry.h"
#include "framework/common/debug/ge_log.h"
#include "op_tiling_registry_impl.h"
#include <random>
using std::make_shared;

namespace optiling {

thread_local int64_t last_op_tiling_perf = -1;

std::map<std::string, OpTilingFunc> &OpTilingRegistryInterf::RegisteredOpInterf() {
  static std::map<std::string, OpTilingFunc> interf;
  return interf;
}

OpTilingRegistryInterf::OpTilingRegistryInterf(std::string op_type, OpTilingFunc func) {
  auto &interf = RegisteredOpInterf();
  interf.emplace(op_type, func);
  GELOGI("Register tiling function: op_type:%s, funcPointer:%p, registered count:%zu", op_type.c_str(),
         func.target<OpTilingFuncPtr>(), interf.size());
}

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

namespace utils {

OpRunInfo::OpRunInfo() {
  impl_ = make_shared<OpRunInfoImpl>();
}

OpRunInfo::OpRunInfo(uint32_t block_dim, bool clear_atomic, uint32_t tiling_key) {
  impl_ = make_shared<OpRunInfoImpl>(block_dim, clear_atomic, tiling_key);
}

OpRunInfo::OpRunInfo(const OpRunInfo &runinfo) {
  // Copy
  impl_ = make_shared<OpRunInfoImpl>();
  if (runinfo.impl_ != nullptr && impl_ != nullptr) {
    *impl_ = *runinfo.impl_;
  }
}

OpRunInfo::OpRunInfo(OpRunInfo &&runinfo) {
  // Move
  impl_ = std::move(runinfo.impl_);
}

OpRunInfo &OpRunInfo::operator=(const OpRunInfo &runinfo) {
  // Copy
  if (&runinfo != this) {
    impl_ = make_shared<OpRunInfoImpl>();
    if (runinfo.impl_ != nullptr && impl_ != nullptr) {
      *impl_ = *runinfo.impl_;
    }
  }
  return *this;
}

OpRunInfo &OpRunInfo::operator=(OpRunInfo &&runinfo) {
  if (&runinfo != this) {
    impl_ = std::move(runinfo.impl_);
  }
  return *this;
}

OpCompileInfo::OpCompileInfo(const std::string &key, const std::string &value) {
  impl_ = make_shared<OpCompileInfoImpl>(key, value);
}

OpCompileInfo::OpCompileInfo(const OpCompileInfo &compileinfo) {
  // Copy
  impl_ = make_shared<OpCompileInfoImpl>();
  if (compileinfo.impl_ != nullptr && impl_ != nullptr) {
    *impl_ = *compileinfo.impl_;
  }
}

OpCompileInfo::OpCompileInfo(OpCompileInfo &&compileinfo) {
  // Move
  impl_ = std::move(compileinfo.impl_);
}

OpCompileInfo &OpCompileInfo::operator=(const OpCompileInfo &compileinfo) {
  // Copy
  if (&compileinfo != this) {
    impl_ = make_shared<OpCompileInfoImpl>();
    if (compileinfo.impl_ != nullptr && impl_ != nullptr) {
      *impl_ = *compileinfo.impl_;
    }
  }
  return *this;
}

OpCompileInfo &OpCompileInfo::operator=(OpCompileInfo &&compileinfo) {
  if (&compileinfo != this) {
    impl_ = std::move(compileinfo.impl_);
  }
  return *this;
}

OpCompileInfo::~OpCompileInfo() {}

std::map<std::string, OpTilingFuncV2> &OpTilingRegistryInterf_V2::RegisteredOpInterf() {
  static std::map<std::string, OpTilingFuncV2> interf;
  GELOGI("Generate interf by new method, registered count: %zu", interf.size());
  return interf;
}

OpTilingRegistryInterf_V2::OpTilingRegistryInterf_V2(std::string op_type, OpTilingFuncV2 func) {
  auto &interf = RegisteredOpInterf();
  interf.emplace(op_type, func);
  GELOGI("Register tiling function by new method: op_type:%s, funcPointer:%p, registered ount:%zu", op_type.c_str(),
         func.target<OpTilingFuncV2Ptr>(), interf.size());
}

void OpRunInfo::SetBlockDim(uint32_t input_block_dim) {
  if (impl_ != nullptr) {
    impl_->SetBlockDim(input_block_dim);
  }
}

void OpRunInfo::AddWorkspace(int64_t workspace) {
  if (impl_ != nullptr) {
    impl_->AddWorkspace(workspace);
  }
}

uint32_t OpRunInfo::GetBlockDim() {
  if (impl_ != nullptr) {
    return impl_->GetBlockDim();
  }
}

size_t OpRunInfo::GetWorkspaceNum() {
  if (impl_ != nullptr) {
    return impl_->GetWorkspaceNum();
  }
}

ge::graphStatus OpRunInfo::GetWorkspace(size_t idx, int64_t &workspace) {
  if (impl_ != nullptr) {
    return impl_->GetWorkspace(idx, workspace);
  }
}

ge::graphStatus OpRunInfo::GetAllWorkspaces(std::vector<int64_t> &_workspaces) {
  if (impl_ != nullptr) {
    return impl_->GetAllWorkspaces(_workspaces);
  }
}

void OpRunInfo::InternelSetTiling(ByteBuffer &value) {
  if (impl_ != nullptr) {
    impl_->SetAllTilingData(value);
  }
}

void OpRunInfo::AddTilingData(const char *_value, size_t _size) {
  if (impl_ != nullptr) {
    impl_->AddTilingData(_value, _size);
  }
}

ByteBuffer &OpRunInfo::GetAllTilingData() {
  if (impl_ != nullptr) {
    return impl_->GetAllTilingData();
  }
}

void OpRunInfo::SetClearAtomic(bool clear_atomic_input) {
  if (impl_ != nullptr) {
    impl_->SetClearAtomic(clear_atomic_input);
  }
}

bool OpRunInfo::GetClearAtomic() const {
  if (impl_ != nullptr) {
    return impl_->GetClearAtomic();
  }
}

void OpRunInfo::SetTilingKey(uint32_t _tiling_key) {
  if (impl_ != nullptr) {
    impl_->SetTilingKey(_tiling_key);
  }
}

uint32_t OpRunInfo::GetTilingKey() const {
  if (impl_ != nullptr) {
    return impl_->GetTilingKey();
  }
}

void OpCompileInfo::SetKey(const std::string &_key) {
  if (impl_ != nullptr) {
    impl_->SetKey(_key);
  }
}

void OpCompileInfo::SetValue(const std::string &_value) {
  if (impl_ != nullptr) {
    impl_->SetValue(_value);
  }
}

const std::string &OpCompileInfo::GetKey() const {
  if (impl_ != nullptr) {
    return impl_->GetKey();
  }
}

const std::string &OpCompileInfo::GetValue() const {
  if (impl_ != nullptr) {
    return impl_->GetValue();
  }
}
}  // namespace utils
}  // namespace optiling
