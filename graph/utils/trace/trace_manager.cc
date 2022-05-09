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
#include "common/util/trace_manager/trace_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <ctime>
#include <cstdarg>
#include "mmpa/mmpa_api.h"
#include "graph/def_types.h"
#include "graph/ge_context.h"
#include "graph/utils/file_utils.h"
#include "toolchain/slog.h"

namespace ge {
thread_local char_t TraceManager::trace_header_[TRACE_PART_LEN] = {};
thread_local std::string TraceManager::graph_name_;
const char_t TraceManager::kTraceEnv[TRACE_ENV_LEN] = "NPU_COLLECT_PATH";
const std::string TraceManager::kTraceRecordPath = "/extra-info/graph_trace/";

TraceManager &TraceManager::GetInstance() {
  static TraceManager instance;
  return instance;
}
bool TraceManager::is_trace_enable_ = false;
// Get env and path
void TraceManager::CheckTraceEnv(char_t *env_path, const uint32_t &length) {
  (void)env_path;
  (void)length;
}

void TraceManager::AddTrace(std::string &&trace_info) {
  (void)trace_info;
  return;
}

// set owner, and update date, start time
void TraceManager::SetTraceOwner(const std::string &owner, const std::string &stage, const std::string &graph_name) {
  (void)owner;
  (void)stage;
  (void)graph_name;
}

void TraceManager::ClearTraceOwner() {
}

void TraceManager::ClearTraceBody() {
}

std::string TraceManager::GetSeqString(const uint32_t &seq) {
  (void)seq;
  return(std::string(""));
}

std::string TraceManager::GetFileName(const std::string &save_time) {
  return save_time;
}
Status TraceManager::OpenFile(int32_t &fd, const std::string &file_path) {
  (void)fd;
  (void)file_path;
  return GRAPH_SUCCESS;
}

void TraceManager::WriteData(const int32_t fd,  const char_t * const data) {
  (void)fd;
  (void)data;
}
void TraceManager::SaveFileFunc() {
}

void TraceManager::SaveInDestructor() {
}

TraceManager::TraceManager() {
  be_in_save_ = false;
  trace_array_[0][0] = 0;
}

TraceManager::~TraceManager() {
}
} // End of class TraceManager
