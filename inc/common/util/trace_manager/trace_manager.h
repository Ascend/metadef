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

#ifndef TRACE_MANAGER_H_
#define TRACE_MANAGER_H_

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstring>
#include <securec.h>
#include "framework/common/util.h"
#include "graph/ge_error_codes.h"

using char_t = char;

#define TRACE_GEN_RECORD(owner, action, graph_name, node_name, node_data, tensor_index, tensor_data, content)      \
do {                                                                                                               \
  if (TraceManager::GetInstance().IsTraceEnable()) {                                                               \
      std::stringstream ss;                                                                                        \
      ss << owner << "," << action << "," << graph_name << "," << node_name << "," << node_data << ","             \
         << tensor_index << "," << tensor_data << "," << content;                                                  \
      TraceManager::GetInstance().AddTrace(ss.str());                                                              \
  }                                                                                                                \
} while (false)

namespace ge
{
using char_t = char;
constexpr uint8_t TRACE_ITEM_LEN  = 150U;
constexpr uint8_t TRACE_PART_LEN  = 30U;
constexpr uint8_t TRACE_TIME0_LEN = 16U;
constexpr uint8_t TRACE_TIME1_LEN = 16U;
constexpr uint8_t TRACE_TIME2_LEN = 4U;
constexpr uint8_t TRACE_ENV_LEN   = 32U;
class TraceManager {
 public:
  static TraceManager &GetInstance();
  void SetTraceOwner(const std::string &owner, const std::string &stage, const std::string &graph_name);
  void ClearTraceOwner();
  void AddTrace(std::string &&trace_info);
  static inline char_t* GetTraceHeader() {
    return trace_header_;
  }
  static inline std::string GetOutGraphName() {
    return graph_name_;
  }

  inline bool IsTraceEnable() const {
    return enable_;
  }
  inline bool DoesHasOwner() const {
    return true;                      // to be replaced
  }

 private:
  TraceManager();
  ~TraceManager();
  TraceManager(const TraceManager &) = delete;
  TraceManager(TraceManager &&) = delete;
  TraceManager &operator=(const TraceManager &) = delete;
  TraceManager &operator=(TraceManager &&) = delete;
  void CheckTraceEnv(char_t *env_path, const uint32_t &length);
  void SaveFileFunc();
  void SaveInDestructor();
  void ClearTraceBody();
  std::string GetSeqString(const uint32_t &seq);
  std::string GetFileName(const std::string &save_time);
  Status OpenFile(int32_t &fd, const std::string &file_path);
  void WriteData(const int32_t fd, const char_t * const data);
  static constexpr uint8_t TRACE_THREAD_SLEEP_TIME_SECOND = 3U;
  static constexpr uint32_t TRACE_SAVE_TRIGGER_NUM = 1000U;
  static constexpr uint32_t TRACE_SAVE_ARRAY_SIZE = TRACE_SAVE_TRIGGER_NUM << 1;

  static const char_t kTraceEnv[TRACE_ENV_LEN];
  static const std::string kTraceRecordPath;
  static thread_local char_t trace_header_[TRACE_PART_LEN];
  static thread_local std::string graph_name_;
  bool enable_ = false;
  volatile bool be_in_save_ = false;
  char_t trace_array_[TRACE_SAVE_ARRAY_SIZE][TRACE_ITEM_LEN] = {};
  std::atomic<uint32_t> trace_index_ {0};
  std::string start_time_;
  std::string save_time_last_;
  std::string trace_save_file_path_;
  static bool is_trace_enable_;
};

class TraceOwnerGuard {
 public:
  TraceOwnerGuard(const std::string &owner, const std::string &stage, const std::string &graph_name) {
    TraceManager::GetInstance().SetTraceOwner(owner, stage, graph_name);
  }
  ~TraceOwnerGuard() {
    TraceManager::GetInstance().ClearTraceOwner();
  }
  TraceOwnerGuard(const TraceOwnerGuard &) = delete;
  TraceOwnerGuard(TraceOwnerGuard &&) = delete;
  TraceOwnerGuard &operator=(const TraceOwnerGuard &) = delete;
  TraceOwnerGuard &operator=(TraceOwnerGuard &&) = delete;
};

#define trace TraceManager::GetInstance()
} // end of namespace ge
#endif  // TRACE_MANAGER_H_
