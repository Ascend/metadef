/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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
#include "op_tiling_registry_impl.h"
#include "graph/debug/ge_log.h"
#include <map>
#include <memory>
#include <securec.h>
#include <string>
#include <vector>

namespace optiling {
namespace utils {

OpCompileInfoImpl::OpCompileInfoImpl(const std::string &key, const std::string &value) : str(value), key(key) {}

OpRunInfoImpl &OpRunInfoImpl::operator=(const OpRunInfoImpl &runinfo) {
  if (&runinfo != this) {
  // Copy
    block_dim = runinfo.block_dim;
    clear_atomic = runinfo.clear_atomic;
    tiling_key = runinfo.tiling_key;
    tiling_data = runinfo.tiling_data;
    workspaces = runinfo.workspaces; 
    return *this;
  }
}

void OpRunInfoImpl::SetBlockDim(uint32_t input_block_dim) {
  block_dim = input_block_dim;
}

void OpRunInfoImpl::AddWorkspace(int64_t workspace) {
  workspaces.push_back(workspace);
}

uint32_t OpRunInfoImpl::GetBlockDim() {
  return block_dim;
}

size_t OpRunInfoImpl::GetWorkspaceNum() {
  return workspaces.size();
}

ge::graphStatus OpRunInfoImpl::GetWorkspace(size_t idx, int64_t &workspace) {
  if (!workspaces.empty() && idx < workspaces.size()) {
    workspace = workspaces[idx];
    return ge::GRAPH_SUCCESS;
  }
  return ge::GRAPH_FAILED;
}

ge::graphStatus OpRunInfoImpl::GetAllWorkspaces(std::vector<int64_t> &_workspaces) {
  if (!workspaces.empty()) {
    _workspaces = workspaces;
    return ge::GRAPH_SUCCESS;
  }
  return ge::GRAPH_FAILED;
}

void OpRunInfoImpl::AddTilingData(const char *_value, size_t _size) {
  tiling_data.write(_value, _size);
  tiling_data.flush();
}

ByteBuffer &OpRunInfoImpl::GetAllTilingData() {
  if (!tiling_data.str().empty()) {
    return tiling_data;
  }
}

void OpRunInfoImpl::SetAllTilingData(ByteBuffer &value) {
  tiling_data.clear();
  std::string temp = value.str();
  tiling_data << temp;
}

void OpRunInfoImpl::SetClearAtomic(bool clear_atomic_input) {
  clear_atomic = clear_atomic_input;
}

bool OpRunInfoImpl::GetClearAtomic() const {
  return clear_atomic;
}

void OpRunInfoImpl::SetTilingKey(uint32_t _tiling_key) {
  tiling_key = _tiling_key;
}

uint32_t OpRunInfoImpl::GetTilingKey() const {
  return tiling_key;
}

void OpCompileInfoImpl::SetKey(const std::string &_key) {
  key = _key;
}

void OpCompileInfoImpl::SetValue(const std::string &value) {
  str = value;
}

const std::string &OpCompileInfoImpl::GetKey() const {
  return key;
}

const std::string &OpCompileInfoImpl::GetValue() const {
  return str;
}
}  // namespace utils
}  // namespace optiling