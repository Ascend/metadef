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

#ifndef ERROR_MANAGER_H_
#define ERROR_MANAGER_H_

#include <map>
#include <string>
#include <vector>
#include <mutex>

namespace ErrorMessage {
  // first stage
  const std::string INITIALIZE = "INIT";
  const std::string MODEL_COMPILE = "COMP";
  const std::string MODEL_LOAD = "LOAD";
  const std::string MODEL_EXECUTE = "EXEC";
  const std::string FINALIZE = "FINAL";

  // SecondStage
  // INITIALIZE
  const std::string OPS_PROTO = "OPS_PRO";
  const std::string SYSTEM = "SYS";
  const std::string ENGINE = "ENGINE";
  const std::string OPS_KERNEL = "OPS_KER";
  const std::string OPS_KERNEL_BUILDER = "OPS_KER_BLD";
  // MODEL_COMPILE
  const std::string PREPARE_OPTIMIZE = "PRE_OPT";
  const std::string ORIGIN_OPTIMIZE = "ORI_OPT";
  const std::string SUB_GRAPH_OPTIMIZE = "SUB_OPT";
  const std::string MERGE_GRAPH_OPTIMIZE = "MERGE_OPT";
  const std::string PREBUILD = "PRE_BLD";
  const std::string STREAM_ALLOC = "STM_ALLOC";
  const std::string MEMORY_ALLOC = "MEM_ALLOC";
  const std::string TASK_GENERATE = "TASK_GEN";
  // COMMON
  const std::string OTHER = "OTHER";

  struct Context {
    uint64_t work_stream_id;
    std::string first_stage;
    std::string second_stage;
    std::string log_header;
  };
}

class ErrorManager {
 public:
  ///
  /// @brief Obtain  ErrorManager instance
  /// @return ErrorManager instance
  ///
  static ErrorManager &GetInstance();

  ///
  /// @brief init
  /// @return int 0(success) -1(fail)
  ///
  int Init();

  ///
  /// @brief init
  /// @param [in] path: current so path
  /// @return int 0(success) -1(fail)
  ///
  int Init(std::string path);

  ///
  /// @brief Report error message
  /// @param [in] error_code: error code
  /// @param [in] args_map: parameter map
  /// @return int 0(success) -1(fail)
  ///
  int ReportErrMessage(std::string error_code, const std::map<std::string, std::string> &args_map);

  ///
  /// @brief output error message
  /// @param [in] handle: print handle
  /// @return int 0(success) -1(fail)
  ///
  int OutputErrMessage(int handle);

  ///
  /// @brief output  message
  /// @param [in] handle: print handle
  /// @return int 0(success) -1(fail)
  ///
  int OutputMessage(int handle);

  std::string GetErrorMessage();

  std::string GetWarningMessage();

  ///
  /// @brief Report error message
  /// @param [in] key: vector parameter key
  /// @param [in] value: vector parameter value
  ///
  void ATCReportErrMessage(std::string error_code, const std::vector<std::string> &key = {},
                           const std::vector<std::string> &value = {});

  ///
  /// @brief report graph compile failed message such as error code and op_name in mstune case
  /// @param [in] graph_name: root graph name
  /// @param [in] msg: failed message map, key is error code, value is op_name
  /// @return int 0(success) -1(fail)
  ///
  int ReportMstuneCompileFailedMsg(const std::string &root_graph_name,
                                   const std::map<std::string, std::string> &msg);

  ///
  /// @brief get graph compile failed message in mstune case
  /// @param [in] graph_name: graph name
  /// @param [out] msg_map: failed message map, key is error code, value is op_name list
  /// @return int 0(success) -1(fail)
  ///
  int GetMstuneCompileFailedMsg(const std::string &graph_name,
                                std::map<std::string,
                                std::vector<std::string>> &msg_map);

  // @brief generate work_stream_id by current pid and tid, clear error_message stored by same work_stream_id
  // used in external api entrance, all sync api can use
  void GenWorkStreamIdDefault();

  // @brief generate work_stream_id by args sessionid and graphid, clear error_message stored by same work_stream_id
  // used in external api entrance
  void GenWorkStreamIdBySessionGraph(uint64_t session_id, uint64_t graph_id);

  const std::string &GetLogHeader();

  struct ErrorMessage::Context &GetErrorContext();

  void SetErrorContext(struct ErrorMessage::Context error_context);

  void SetStage(const std::string &first_stage, const std::string &second_stage);

 private:
  struct ErrorInfo {
    std::string error_id;
    std::string error_message;
    std::vector<std::string> arg_list;
  };

  ErrorManager() {}
  ~ErrorManager() {}

  ErrorManager(const ErrorManager &) = delete;
  ErrorManager(ErrorManager &&) = delete;
  ErrorManager &operator=(const ErrorManager &) = delete;
  ErrorManager &operator=(ErrorManager &&) = delete;

  int ParseJsonFile(std::string path);

  int ReadJsonFile(const std::string &file_path, void *handle);

  void ClassifyCompileFailedMsg(const std::map<std::string, std::string> &msg,
                                std::map<std::string,
                                std::vector<std::string>> &classfied_msg);

  std::vector<std::string>& GetErrorMsgContainerByWorkId(uint64_t work_id);
  std::vector<std::string>& GetWarningMsgContainerByWorkId(uint64_t work_id);

  bool is_init_ = false;
  std::mutex mutex_;
  std::map<std::string, ErrorInfo> error_map_;
  std::vector<std::string> error_messages_;
  std::vector<std::string> warning_messages_;
  std::map<std::string, std::map<std::string, std::vector<std::string>>> compile_failed_msg_map_;

  std::map<uint64_t, std::vector<std::string>> error_message_per_work_id_;
  std::map<uint64_t, std::vector<std::string>> warning_messages_per_work_id_;

  thread_local static struct ErrorMessage::Context error_context_;
};

#endif  // ERROR_MANAGER_H_
