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

#include <string>
#include <thread>
#include <atomic>
#include <cstring>
#include <securec.h>
#include "framework/common/util.h"
#include "graph/ge_error_codes.h"

namespace ge {
using char_t = char;

constexpr uint8_t TRACE_ITEM_LEN  = 150U;
constexpr uint8_t TRACE_PART_LEN  = 30U;
constexpr uint8_t TRACE_TIME0_LEN = 16U;
constexpr uint8_t TRACE_TIME1_LEN = 16U;
constexpr uint8_t TRACE_TIME2_LEN = 4U;
constexpr uint8_t TRACE_ENV_LEN   = 32U;

struct NowDate {
  char_t tmp0[TRACE_TIME0_LEN];    // year month date
  char_t tmp1[TRACE_TIME1_LEN];    // hour minute second
  char_t tmp2[TRACE_TIME2_LEN];    // millisecond
};

enum ActType {
  TRACE_ACT_NULL  = 0,
  TRACE_ADD       = 1,
  TRACE_MOD       = 2,    // modify
  TRACE_DEL       = 3,
  TRACE_SET       = 4,
  TRACE_UPD       = 5,    // update
  TRACE_ACT_SIZE,
};

enum InputOutputType {
  TRACE_IN_OUT_NULL = 0,
  TRACE_INPUT       = 1,
  TRACE_OUTPUT      = 2,
  TRACE_IN_OUT_SIZE,
};

class TraceManager {
 public:
  static const std::string kTraceAct[TRACE_ACT_SIZE];
  static const std::string kTraceInOut[TRACE_IN_OUT_SIZE];

  static TraceManager &GetInstance();
  void SetTraceOwner(const std::string &owner, const std::string &stage, const std::string &graph_name);
  void ClearTraceOwner();
  void GetTraceItem(uint32_t &index, char_t * &item);
  char_t* GetTraceHeader();
  void AddTrace(const uint32_t index);

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
  void CreateTraceDirectory(const std::string &env_path, std::string &trace_path);
  void SaveFileFunc();
  void SaveInDestructor();
  void ClearTraceBody();
  std::string GetTimeStr();
  std::string GetSeqString(const uint32_t &seq);
  std::string GetFileName(const std::string &save_time);
  void SaveToFile();
  Status OpenFile(int32_t &fd, const std::string &file_path);
  void WriteData(const int32_t fd, const char_t * const data);

  static constexpr uint8_t TRACE_THREAD_SLEEP_TIME_SECOND = 3U;
  static constexpr uint32_t TRACE_SAVE_TRIGGER_NUM = 1000U;
  static constexpr uint32_t TRACE_SAVE_ARRAY_SIZE = TRACE_SAVE_TRIGGER_NUM << 1;

  static const char_t kTraceEnv[TRACE_ENV_LEN];
  static const std::string kTraceRecordPath;
  static thread_local char_t trace_header_[TRACE_PART_LEN];

  bool enable_ = false;
  volatile bool be_in_save_ = false;
  char_t trace_array_[TRACE_SAVE_ARRAY_SIZE][TRACE_ITEM_LEN] = {};
  std::atomic<uint32_t> trace_index_ {0};
  std::string start_time_;
  std::string save_time_last_;
  std::string trace_save_file_path_;
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

/* trace info form
  owner:stage:graph_name, graph_name_in, action, node_name, node_data_name, archor_in_out:index,
  tensor_in_out:index, tensor_data_name, content_value,  dest_node_name:anchor_in_out:index */
} // end of namespace ge
#endif  // TRACE_MANAGER_H_
