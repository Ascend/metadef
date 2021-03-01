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

#include "common/util/error_manager/error_manager.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sstream>
#include "framework/common/debug/ge_log.h"
#include "framework/common/string_util.h"
#include "graph/ge_context.h"
#include "mmpa/mmpa_api.h"

namespace {
const char *const kErrorCodePath = "../conf/error_manager/error_code.json";
const char *const kErrorList = "error_info_list";
const char *const kErrCode = "ErrCode";
const char *const kErrMessage = "ErrMessage";
const char *const kArgList = "Arglist";
const uint64_t kLength = 2;
}  // namespace

///
/// @brief Obtain error manager self library path
/// @return store liberror_manager.so path
///
static std::string GetSelfLibraryDir(void) {
  mmDlInfo dl_info;
  if (mmDladdr(reinterpret_cast<void *>(GetSelfLibraryDir), &dl_info) != EN_OK) {
    GELOGW("Failed to read the shared library file path!");
    return std::string();
  } else {
    std::string so_path = dl_info.dli_fname;
    char path[MMPA_MAX_PATH] = {0};
    if (so_path.length() >= MMPA_MAX_PATH) {
        GELOGW("The shared library file path is too long!");
        return std::string();
    }
    if (mmRealPath(so_path.c_str(), path, MMPA_MAX_PATH) != EN_OK) {
      GELOGW("Failed to get realpath of %s", so_path.c_str());
      return std::string();
    }

    so_path = path;
    so_path = so_path.substr(0, so_path.rfind('/') + 1);
    return so_path;
  }
}

///
/// @brief Obtain ErrorManager instance
/// @return ErrorManager instance
///
ErrorManager &ErrorManager::GetInstance() {
  static ErrorManager instance;
  return instance;
}

///
/// @brief init
/// @param [in] path: current so path
/// @return int 0(success) -1(fail)
///
int ErrorManager::Init(std::string path) {
  if (is_init_) {
    return 0;
  }
  int ret = ParseJsonFile(path + kErrorCodePath);
  if (ret != 0) {
    GELOGE(ge::FAILED, "Parser json file failed");
    return -1;
  }
  is_init_ = true;
  return 0;
}

///
/// @brief init
/// @return int 0(success) -1(fail)
///
int ErrorManager::Init() {
    return Init(GetSelfLibraryDir());
}
///
/// @brief report error message
/// @param [in] error_code: error code
/// @param [in] args_map: parameter map
/// @return int 0(success) -1(fail)
///
int ErrorManager::ReportErrMessage(std::string error_code, const std::map<std::string, std::string> &args_map) {
  if (!is_init_) {
    GELOGI("ErrorManager has not inited, can't report error message");
    return 0;
  }
  auto it = error_map_.find(error_code);
  if (it == error_map_.end()) {
    GELOGE(ge::FAILED, "Error code %s is not registered", error_code.c_str());
    return -1;
  }
  const ErrorInfo &error_info = it->second;
  std::string error_message = error_info.error_message;
  const std::vector<std::string> &arg_list = error_info.arg_list;
  for (const std::string &arg : arg_list) {
    if (arg.empty()) {
      GELOGI("arg is null");
      break;
    }
    auto arg_it = args_map.find(arg);
    if (arg_it == args_map.end()) {
      GELOGE(ge::FAILED, "error_code: %s, arg %s is not existed in map", error_code.c_str(), arg.c_str());
      return -1;
    }
    const std::string &arg_value = arg_it->second;
    auto index = error_message.find("%s");
    if (index == std::string::npos) {
      GELOGE(ge::FAILED, "error_code: %s, %s location in error_message is not found", error_code.c_str(), arg.c_str());
      return -1;
    }
    error_message.replace(index, kLength, arg_value);
  }

  uint64_t work_id = ge::GetContext().WorkStreamId();
  if (work_id == 0) {
    GELOGW("work_id in this work stream is zero, work_id set action maybe forgeted in some externel api.");
  }
  auto& error_messages = GetErrorMsgContainerByWorkId(work_id);
  auto& warning_messages = GetWarningMsgContainerByWorkId(work_id);

  std::string message = error_code + ": " + error_message;
  std::unique_lock<std::mutex> lock(mutex_);
  if (error_code[0] == 'W') {
    auto it = find(warning_messages.begin(), warning_messages.end(), message);
    if (it == warning_messages.end()) {
      warning_messages.emplace_back(message);
    }
  } else {
    auto it = find(error_messages.begin(), error_messages.end(), message);
    if (it == error_messages.end()) {
      error_messages.emplace_back(message);
    }
  }

  return 0;
}

std::string ErrorManager::GetErrorMessage() {
  uint64_t work_id = ge::GetContext().WorkStreamId();
  auto& error_messages = GetErrorMsgContainerByWorkId(work_id);

  if (error_messages.empty()) {
    error_messages.push_back("E19999: Unknown error occurred. Please check the log.");
  }

  std::stringstream err_stream;
  for (auto &info : error_messages) {
    err_stream << info << std::endl;
  }

  return err_stream.str();
}

std::string ErrorManager::GetWarningMessage() {
  uint64_t work_id = ge::GetContext().WorkStreamId();
  auto& warning_messages = GetWarningMsgContainerByWorkId(work_id);

  std::stringstream warning_stream;
  for (auto &info : warning_messages) {
    warning_stream << info << std::endl;
  }
  return warning_stream.str();
}

///
/// @brief output error message
/// @param [in] handle: print handle
/// @return int 0(success) -1(fail)
///
int ErrorManager::OutputErrMessage(int handle) {
  uint64_t work_id = ge::GetContext().WorkStreamId();
  auto& error_messages = GetErrorMsgContainerByWorkId(work_id);

  if (error_messages.empty()) {
    error_messages.push_back("E19999: Unknown error occurred. Please check the log.");
  }
  if (handle <= fileno(stderr)) {
    for (auto &info : error_messages) {
      std::cout << info << std::endl;
    }
  } else {
    for (auto &info : error_messages) {
      mmSsize_t ret = mmWrite(handle, const_cast<char *>(info.c_str()), info.length());
      if (ret == -1) {
        GELOGE(ge::FAILED, "write file fail");
        return -1;
      }
    }
  }
  return 0;
}

///
/// @brief output message
/// @param [in] handle: print handle
/// @return int 0(success) -1(fail)
///
int ErrorManager::OutputMessage(int handle) {
  uint64_t work_id = ge::GetContext().WorkStreamId();
  auto& warning_messages = GetWarningMsgContainerByWorkId(work_id);

  for (auto &info : warning_messages) {
    std::cout << info << std::endl;
  }
  return 0;
}

///
/// @brief parse json file
/// @param [in] path: json path
/// @return int 0(success) -1(fail)
///
int ErrorManager::ParseJsonFile(std::string path) {
  GELOGI("Begin to parser json file");
  nlohmann::json json_file;
  int status = ReadJsonFile(path, &json_file);
  if (status != 0) {
    GELOGE(ge::FAILED, "Read json file failed and the file path is %s", path.c_str());
    return -1;
  }

  try {
    const nlohmann::json &error_list_json = json_file[kErrorList];
    if (error_list_json.is_null()) {
      GELOGE(ge::FAILED, "The message of error_info_list is not found in %s", path.c_str());
      return -1;
    }
    if (!error_list_json.is_array()) {
      GELOGE(ge::FAILED, "The message of error_info_list is not array in %s", path.c_str());
      return -1;
    }

    for (size_t i = 0; i < error_list_json.size(); i++) {
      ErrorInfo error_info;
      error_info.error_id = error_list_json[i][kErrCode];
      error_info.error_message = error_list_json[i][kErrMessage];
      error_info.arg_list = ge::StringUtils::Split(error_list_json[i][kArgList], ',');
      auto it = error_map_.find(error_info.error_id);
      if (it != error_map_.end()) {
        GELOGE(ge::FAILED, "There are the same error code %s in %s", error_info.error_id.c_str(), path.c_str());
        return -1;
      }
      error_map_.emplace(error_info.error_id, error_info);
    }
  } catch (const nlohmann::json::exception &e) {
    GELOGE(ge::FAILED, "Parse json file failed and the file path is %s, exception message: %s", path.c_str(), e.what());
    return -1;
  }

  GELOGI("Parse json file success");
  return 0;
}

///
/// @brief read json file
/// @param [in] file_path: json path
/// @param [in] handle:  print handle
/// @return int 0(success) -1(fail)
///
int ErrorManager::ReadJsonFile(const std::string &file_path, void *handle) {
  GELOGI("Begin to read json file");
  if (file_path.empty()) {
    GELOGE(ge::FAILED, "Json path %s is not valid", file_path.c_str());
    return -1;
  }
  nlohmann::json *json_file = reinterpret_cast<nlohmann::json *>(handle);
  if (json_file == nullptr) {
    GELOGE(ge::FAILED, "JsonFile is nullptr");
    return -1;
  }
  const char *file = file_path.data();
  if ((mmAccess2(file, M_F_OK)) != EN_OK) {
    GELOGE(ge::FAILED, "The json file %s is not exist, error %s", file_path.c_str(), strerror(errno));
    return -1;
  }

  std::ifstream ifs(file_path);
  if (!ifs.is_open()) {
    GELOGE(ge::FAILED, "Open json file %s failed", file_path.c_str());
    return -1;
  }

  try {
    ifs >> *json_file;
  } catch (const nlohmann::json::exception &e) {
    GELOGE(ge::FAILED, "Something wrong in %s", file_path.c_str());
    ifs.close();
    return -1;
  }

  ifs.close();
  GELOGI("Read json file success");
  return 0;
}

///
/// @brief report error message
/// @param [in] error_code: error code
/// @param [in] vector parameter key, vector parameter value
/// @return int 0(success) -1(fail)
///
void ErrorManager::ATCReportErrMessage(std::string error_code, const std::vector<std::string> &key,
                                       const std::vector<std::string> &value) {
  if (!is_init_) {
    GELOGI("ErrorManager has not inited, can't report error message");
    return;
  }
  std::map<std::string, std::string> args_map;
  if (key.empty()) {
    (void)ErrorManager::GetInstance().ReportErrMessage(error_code, args_map);
  } else if (key.size() == value.size()) {
    for (size_t i = 0; i < key.size(); ++i) {
      args_map.insert(std::make_pair(key[i], value[i]));
    }
    (void)ErrorManager::GetInstance().ReportErrMessage(error_code, args_map);
  } else {
    GELOGW("ATCReportErrMessage wrong, vector key and value size is not equal");
  }
}

///
/// @brief report graph compile failed message such as error code and op_name in mustune case
/// @param [in] msg: failed message map, key is error code, value is op_name
/// @param [out] classified_msg: classified_msg message map, key is error code, value is op_name vector
///
void ErrorManager::ClassifyCompileFailedMsg(const std::map<std::string, std::string> &msg,
                                            std::map<std::string,
                                            std::vector<std::string>> &classified_msg) {
  for (const auto &itr : msg) {
    const std::string &error_code = itr.first;
    const std::string &op_name = itr.second;
    GELOGD("msg is error_code:%s, op_name:%s", error_code.c_str(), op_name.c_str());
    auto err_code_itr = classified_msg.find(error_code);
    if (err_code_itr == classified_msg.end()) {
      classified_msg.emplace(error_code, std::vector<std::string>{op_name});
    } else {
      std::vector<std::string> &op_name_list = err_code_itr->second;
      op_name_list.emplace_back(op_name);
    }
  }
}

///
/// @brief report graph compile failed message such as error code and op_name in mustune case
/// @param [in] root_graph_name: root graph name
/// @param [in] msg: failed message map, key is error code, value is op_name
/// @return int 0(success) -1(fail)
///
int ErrorManager::ReportMstuneCompileFailedMsg(const std::string &root_graph_name,
                                               const std::map<std::string, std::string> &msg) {
  if (!is_init_) {
    GELOGI("ErrorManager has not inited, can't report compile message");
    return 0;
  }
  if (msg.empty() || root_graph_name.empty()) {
    GELOGW("Msg or root graph name is empty, msg size is %u, \
           root graph name is %s", msg.size(), root_graph_name.c_str());
    return -1;
  }
  GELOGD("Report graph:%s compile failed msg", root_graph_name.c_str());
  std::unique_lock<std::mutex> lock(mutex_);
  auto itr = compile_failed_msg_map_.find(root_graph_name);
  if (itr != compile_failed_msg_map_.end()) {
    std::map<std::string, std::vector<std::string>> &classified_msg = itr->second;
    ClassifyCompileFailedMsg(msg, classified_msg);
  } else {
    std::map<std::string, std::vector<std::string>> classified_msg;
    ClassifyCompileFailedMsg(msg, classified_msg);
    compile_failed_msg_map_.emplace(root_graph_name, classified_msg);
  }
  return 0;
}

///
/// @brief get graph compile failed message in mustune case
/// @param [in] graph_name: graph name
/// @param [out] msg_map: failed message map, key is error code, value is op_name list
/// @return int 0(success) -1(fail)
///
int ErrorManager::GetMstuneCompileFailedMsg(const std::string &graph_name, std::map<std::string,
                                            std::vector<std::string>> &msg_map) {
  if (!is_init_) {
    GELOGI("ErrorManager has not inited, can't report compile failed message");
    return 0;
  }
  if (!msg_map.empty()) {
    GELOGW("msg_map is not empty, exist msg");
    return -1;
  }
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = compile_failed_msg_map_.find(graph_name);
  if (iter == compile_failed_msg_map_.end()) {
    GELOGW("can not find graph, name is:%s", graph_name.c_str());
    return -1;
  } else {
    auto &compile_failed_msg = iter->second;
    msg_map.swap(compile_failed_msg);
    compile_failed_msg_map_.erase(graph_name);
  }
  GELOGI("get graph:%s compile failed msg success", graph_name.c_str());

  return 0;
}

std::vector<std::string>& ErrorManager::GetErrorMsgContainerByWorkId(uint64_t work_id) {
  auto iter = error_message_per_work_id_.find(work_id);
  if (iter == error_message_per_work_id_.end()) {
    error_message_per_work_id_.emplace(work_id, std::vector<std::string>());
    iter = error_message_per_work_id_.find(work_id);
  }
  return iter->second;
}

std::vector<std::string>& ErrorManager::GetWarningMsgContainerByWorkId(uint64_t work_id) {
  auto iter = warning_messages_per_work_id_.find(work_id);
  if (iter == warning_messages_per_work_id_.end()) {
    warning_messages_per_work_id_.emplace(work_id, std::vector<std::string>());
    iter = warning_messages_per_work_id_.find(work_id);
  }
  return iter->second;
}

void ErrorManager::GenWorkStreamIdDefault() {
  // system getpid and gettid is always successful
  int32_t pid = mmGetPid();
  int32_t tid = mmGetTid();

  const uint64_t kPidOffset = 100000;
  uint64_t work_stream_id = pid * kPidOffset + tid;
  ge::GetContext().SetWorkStreamId(work_stream_id);

  auto iter = error_message_per_work_id_.find(work_stream_id);
  if (iter != error_message_per_work_id_.end()) {
    error_message_per_work_id_.erase(iter);
  }
}

void ErrorManager::GenWorkStreamIdBySessionGraph(uint64_t session_id, uint64_t graph_id) {
  const uint64_t kSessionIdOffset = 100000;
  uint64_t work_stream_id = session_id * kSessionIdOffset + graph_id;
  ge::GetContext().SetWorkStreamId(work_stream_id);

  auto iter = error_message_per_work_id_.find(work_stream_id);
  if (iter != error_message_per_work_id_.end()) {
    error_message_per_work_id_.erase(iter);
  }
}