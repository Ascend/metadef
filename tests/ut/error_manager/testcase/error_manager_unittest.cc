/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#include <gtest/gtest.h>
#include <memory>
#define protected public
#define private public
#include "inc/common/util/error_manager/error_manager.h"
#define protected public
#define private public

namespace ge
{
  class UtestErrorManager : public testing::Test {
    protected:
    void SetUp() {}
    void TearDown() {
        ErrorManager::GetInstance().is_init_ = false;
        ErrorManager::GetInstance().compile_failed_msg_map_.clear();
        ErrorManager::GetInstance().compile_failed_msg_map_.clear();
        ErrorManager::GetInstance().error_message_per_work_id_.clear();
        ErrorManager::GetInstance().warning_messages_per_work_id_.clear();
    }
  };
  
TEST_F(UtestErrorManager, Init_faild) {
  auto &instance = ErrorManager::GetInstance();
  EXPECT_EQ(instance.Init(""), -1);
  instance.is_init_ = true;
  EXPECT_EQ(instance.Init(""), 0);
}

TEST_F(UtestErrorManager, FormatErrorMessage) {
  char buf[10] = {0};
  error_message::FormatErrorMessage(buf, 10, "test");
}

TEST_F(UtestErrorManager, GetInstance) {
  ErrorManager::GetInstance();
}

TEST_F(UtestErrorManager, ReportInterErrMessage) {
  auto &instance = ErrorManager::GetInstance();
  instance.error_context_.work_stream_id = 0;
  EXPECT_EQ(instance.ReportInterErrMessage("errcode", "errmsg"), -1);
  instance.is_init_ = true;
  EXPECT_EQ(instance.ReportInterErrMessage("600000", "errmsg"), -1);
}

TEST_F(UtestErrorManager, ReportInterErrMessage_WorkStreamId) {
  auto &instance = ErrorManager::GetInstance();
  instance.is_init_ = true;
  auto id = instance.error_context_.work_stream_id;
  instance.error_context_.work_stream_id = 0;
  EXPECT_EQ(instance.ReportInterErrMessage("609999", "errmsg"), 0);
  instance.error_context_.work_stream_id = id;
  for (int i = 0; i < 1002; i++){
  	instance.error_message_per_work_id_[i] = std::vector<ErrorManager::ErrorItem>();
  }
  EXPECT_EQ(instance.ReportInterErrMessage("609999", "errmsg"), -1);
}

TEST_F(UtestErrorManager, ReportErrMessage) {
  auto &instance = ErrorManager::GetInstance();
  std::string error_code = "error";
  std::map<std::string, std::string> args_map;
  EXPECT_EQ(instance.ReportErrMessage(error_code, args_map), 0);
  instance.is_init_ = true;
  EXPECT_EQ(instance.ReportErrMessage(error_code, args_map), -1);
}

TEST_F(UtestErrorManager, ReportErrMessage_Normal) {
  auto &instance = ErrorManager::GetInstance();
  instance.is_init_ = true;
  std::string error_code = "error";
  std::map<std::string, std::string> args_map;
  args_map["argv1"] = "val1";
  auto conf = ErrorManager::ErrorInfoConfig();
  instance.error_map_[error_code] = conf;
  EXPECT_EQ(instance.ReportErrMessage(error_code, args_map), 0);
  instance.error_map_[error_code].arg_list.push_back("argv1");
  EXPECT_EQ(instance.ReportErrMessage(error_code, args_map), -1);
  instance.error_map_[error_code].arg_list.clear();
  instance.error_map_[error_code].arg_list.push_back("");
  EXPECT_EQ(instance.ReportErrMessage(error_code, args_map), 0);
}

TEST_F(UtestErrorManager, GetErrorMessage) {
  auto &instance = ErrorManager::GetInstance();
  EXPECT_EQ(instance.GetErrorMessage(), "");
}


TEST_F(UtestErrorManager, GetWarningMessage) {
  auto &instance = ErrorManager::GetInstance();
  EXPECT_EQ(instance.GetWarningMessage(), "");
}

TEST_F(UtestErrorManager, OutputErrMessage) {
  auto &instance = ErrorManager::GetInstance();
  EXPECT_EQ(instance.OutputErrMessage(1), 0);
}

TEST_F(UtestErrorManager, OutputMessage) {
  auto &instance = ErrorManager::GetInstance();
  EXPECT_EQ(instance.OutputMessage(1), 0);
}

TEST_F(UtestErrorManager, ATCReportErrMessage) {
  auto &instance = ErrorManager::GetInstance();
  std::vector<std::string> key;
  std::vector<std::string> value;
  instance.ATCReportErrMessage("error", key, value);
  instance.is_init_ = true;
  instance.ATCReportErrMessage("error", key, value);
  key.push_back("key");
  instance.ATCReportErrMessage("error", key, value);
  value.push_back("123");
  instance.ATCReportErrMessage("error", key, value);
}

TEST_F(UtestErrorManager, ClassifyCompileFailedMsg) {
  auto &instance = ErrorManager::GetInstance();
  std::map<std::string, std::string> msg;
  std::map<std::string, std::vector<std::string>> classified_msg;
  msg["code"] = "error";
  instance.ClassifyCompileFailedMsg(msg, classified_msg);
  classified_msg["code"] = std::vector<std::string>();
  instance.ClassifyCompileFailedMsg(msg, classified_msg);
}


TEST_F(UtestErrorManager, SetStage) {
  auto &instance = ErrorManager::GetInstance();
  instance.SetStage("first", "second");
  char first[10] = {"first"};
  char second[10] = {"second"};
  instance.SetStage(first, 10, second, 10);
}

TEST_F(UtestErrorManager, Context) {
  auto &instance = ErrorManager::GetInstance();
  auto context = instance.GetErrorManagerContext();
  instance.SetErrorContext(context);
}

TEST_F(UtestErrorManager, GenWorkStreamIdBySessionGraph) {
  auto &instance = ErrorManager::GetInstance();
  instance.GenWorkStreamIdBySessionGraph(1, 2);
}

TEST_F(UtestErrorManager, GetMstuneCompileFailedMsg) {
  auto &instance = ErrorManager::GetInstance();
  std::string graph_name = "graph";
  std::map<std::string, std::vector<std::string>> msg_map;
  EXPECT_EQ(instance.GetMstuneCompileFailedMsg(graph_name, msg_map), 0);
  msg_map["msg"] = std::vector<std::string>();
  EXPECT_EQ(instance.GetMstuneCompileFailedMsg(graph_name, msg_map), 0);
  instance.is_init_ = true;
  EXPECT_EQ(instance.GetMstuneCompileFailedMsg(graph_name, msg_map), -1);
  msg_map.clear();
  instance.compile_failed_msg_map_["graph"] = std::map<std::string, std::vector<std::string>>();
  EXPECT_EQ(instance.GetMstuneCompileFailedMsg(graph_name, msg_map), 0);
}

TEST_F(UtestErrorManager, ReportMstuneCompileFailedMsg) {
  auto &instance = ErrorManager::GetInstance();
  std::string root_graph_name = "root_graph";
  std::map<std::string, std::string> msg;
  EXPECT_EQ(instance.ReportMstuneCompileFailedMsg(root_graph_name, msg), 0);
  instance.is_init_ = true;
  EXPECT_EQ(instance.ReportMstuneCompileFailedMsg(root_graph_name, msg), -1);
}

TEST_F(UtestErrorManager, ReportMstuneCompileFailedMsg_Success) {
  auto &instance = ErrorManager::GetInstance();
  instance.is_init_ = true;
  std::string root_graph_name = "root_graph_name";
  std::map<std::string, std::string> msg;
  msg["root_graph_name"] = "message";
  EXPECT_EQ(instance.ReportMstuneCompileFailedMsg(root_graph_name, msg), 0);
  instance.compile_failed_msg_map_["root_graph_name"] = std::map<std::string, std::vector<std::string>>();
  EXPECT_EQ(instance.ReportMstuneCompileFailedMsg(root_graph_name, msg), 0);
}

}