/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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

#ifndef METADEF_CXX_INC_DFX_INFO_FILLER_H
#define METADEF_CXX_INC_DFX_INFO_FILLER_H

#include <string>
#include <vector>
#include <graph/types.h>
#include <graph/ge_error_codes.h>

namespace gert {
class ProfNodeInfoWrapperBasic {
 public:
  virtual ~ProfNodeInfoWrapperBasic() {}
  virtual void SetBlockDim(const uint32_t block_dim) = 0;
  virtual ge::graphStatus FillShapeInfo(std::vector<std::vector<uint32_t>> &input_shapes,
                                        std::vector<std::vector<uint32_t>> &output_shapes) = 0;
};

class DataDumpInfoWrapperBasic {
 public:
  virtual ~DataDumpInfoWrapperBasic() {}
  virtual ge::graphStatus CreateCtxInfo(const uint32_t context_id, const uint32_t thread_id) = 0;
  virtual ge::graphStatus AddCtxAddr(const uint32_t thread_id, const bool is_input,
                                     const uint64_t address, const uint64_t size) = 0;
  virtual void ClearWorkspace() = 0;
  virtual void AddWorkspace(const uintptr_t addr) = 0;
  virtual bool SetStrAttr(const std::string &name, const std::string &value) = 0;
};

class ExceptionDumpInfoWrapperBasic {
 public:
  virtual ~ExceptionDumpInfoWrapperBasic() {}
  virtual void SetTilingData(const std::string &data) = 0;
  virtual void SetTilingKey(const uint32_t data) = 0;
  virtual void SetMemoryLogFlag(const bool flag) = 0;
  virtual void SetArgs(const uintptr_t addr, const size_t size, bool host_flag = true) = 0;
  virtual void ClearWorkspace() = 0;
  virtual void AddWorkspace(const uintptr_t addr, const int64_t bytes) = 0;
};
}

#endif
