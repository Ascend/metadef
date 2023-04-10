/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
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

#include "graph/utils/file_utils.h"

#include <cerrno>
#include "graph/types.h"
#include "graph/debug/ge_log.h"
#include "mmpa/mmpa_api.h"
#include "graph/def_types.h"
#include <fstream>
#include "common/checker.h"

namespace {
  const int32_t kFileSuccess = 0;
  const uint32_t kMaxWriteSize = 1 * 1024 * 1024 * 1024U; // 1G
}
namespace ge {
std::string RealPath(const char_t *path) {
  if (path == nullptr) {
    REPORT_INNER_ERROR("E18888", "path is nullptr, check invalid");
    GELOGE(FAILED, "[Check][Param] path pointer is NULL.");
    return "";
  }
  if (strnlen(path, static_cast<size_t>(MMPA_MAX_PATH)) >= static_cast<size_t>(MMPA_MAX_PATH)) {
    ErrorManager::GetInstance().ATCReportErrMessage("E19002", {"filepath", "size"},
                                                    {path, std::to_string(MMPA_MAX_PATH)});
    GELOGE(FAILED, "[Check][Param]Path[%s] len is too long, it must be less than %d", path, MMPA_MAX_PATH);
    return "";
  }

  // Nullptr is returned when the path does not exist or there is no permission
  // Return absolute path when path is accessible
  std::string res;
  char_t resolved_path[MMPA_MAX_PATH] = {};
  if (mmRealPath(path, &(resolved_path[0U]), MMPA_MAX_PATH) == EN_OK) {
    res = &(resolved_path[0]);
  } else {
    GELOGW("[Util][realpath] Get real_path for %s failed, reason:%s", path, strerror(errno));
  }
  return res;
}

std::string GetRegulatedName(const std::string name) {
  std::string regulate_name = name;
  replace(regulate_name.begin(), regulate_name.end(), '/', '_');
  replace(regulate_name.begin(), regulate_name.end(), '\\', '_');
  replace(regulate_name.begin(), regulate_name.end(), '.', '_');
  GELOGD("Get regulated name[%s] success", regulate_name.c_str());
  return regulate_name;
}

inline int32_t CheckAndMkdir(const char_t *tmp_dir_path, mmMode_t mode) {
  if (mmAccess2(tmp_dir_path, M_F_OK) != EN_OK) {
    const int32_t ret = mmMkdir(tmp_dir_path, mode);  // 700
    if (ret != 0) {
      REPORT_CALL_ERROR("E18888",
                        "Can not create directory %s. Make sure the directory "
                        "exists and writable. errmsg:%s",
                        tmp_dir_path, strerror(errno));
      GELOGW(
          "[Util][mkdir] Create directory %s failed, reason:%s. Make sure the "
          "directory exists and writable.",
          tmp_dir_path, strerror(errno));
      return ret;
    }
  }
  return 0;
}

/**
 *  @ingroup domi_common
 *  @brief Create directory, support to create multi-level directory
 *  @param [in] directory_path  Path, can be multi-level directory
 *  @return -1 fail
 *  @return 0 success
 */
int32_t CreateDir(const std::string &directory_path) {
  GE_CHK_BOOL_EXEC(!directory_path.empty(),
                   REPORT_INNER_ERROR("E18888", "directory path is empty, check invalid");
                       return -1, "[Check][Param] directory path is empty.");
  const auto dir_path_len = directory_path.length();
  if (dir_path_len >= static_cast<size_t>(MMPA_MAX_PATH)) {
    ErrorManager::GetInstance().ATCReportErrMessage("E19002", {"filepath", "size"},
                                                    {directory_path, std::to_string(MMPA_MAX_PATH)});
    GELOGW("[Util][mkdir] Path %s len is too long, it must be less than %d", directory_path.c_str(), MMPA_MAX_PATH);
    return -1;
  }
  char_t tmp_dir_path[MMPA_MAX_PATH] = {};
  const auto mkdir_mode = static_cast<mmMode_t>(static_cast<uint32_t>(M_IRUSR) |
                                          static_cast<uint32_t>(M_IWUSR) |
                                          static_cast<uint32_t>(M_IXUSR));
  for (size_t i = 0U; i < dir_path_len; i++) {
    tmp_dir_path[i] = directory_path[i];
    if ((tmp_dir_path[i] == '\\') || (tmp_dir_path[i] == '/')) {
      const int32_t ret = CheckAndMkdir(&(tmp_dir_path[0U]), mkdir_mode);
      if (ret != 0) {
        return ret;
      }
    }
  }
  return CheckAndMkdir(directory_path.c_str(), mkdir_mode);
}

/**
 *  @ingroup domi_common
 *  @brief Create directory, support to create multi-level directory
 *  @param [in] directory_path  Path, can be multi-level directory
 *  @return -1 fail
 *  @return 0 success
 */
int32_t CreateDirectory(const std::string &directory_path) {
  return CreateDir(directory_path);
}

std::unique_ptr<char_t[]> GetBinFromFile(std::string &path, uint32_t &data_len) {
  GE_ASSERT_TRUE(!path.empty());

  std::ifstream ifs(path, std::ifstream::binary);
  if (!ifs.is_open()) {
    GELOGW("path:%s not open", path.c_str());
    return nullptr;
  }

  (void)ifs.seekg(0, std::ifstream::end);
  const uint32_t len = static_cast<uint32_t>(ifs.tellg());
  (void)ifs.seekg(0, std::ifstream::beg);
  auto bin_data = std::unique_ptr<char_t[]>(new(std::nothrow) char_t[len]);
  if (bin_data == nullptr) {
    GELOGE(FAILED, "[Allocate][Mem]Allocate mem failed");
    ifs.close();
    return nullptr;
  }
  (void)ifs.read(reinterpret_cast<char_t*>(bin_data.get()), static_cast<std::streamsize>(len));
  data_len = len;
  ifs.close();
  return bin_data;
}

graphStatus GetBinFromFile(const std::string &path, char_t *buffer, size_t &data_len) {
  GE_ASSERT_TRUE(!path.empty());
  GE_ASSERT_TRUE(buffer != nullptr);
  std::string real_path = RealPath(path.c_str());
  GE_ASSERT_TRUE(!real_path.empty(),
      "Path: %s is invalid, file or directory is not exist", path.c_str());
  std::ifstream ifs(real_path, std::ifstream::binary);
  if (!ifs.is_open()) {
    GELOGE(GRAPH_FAILED, "path:%s not open", real_path.c_str());
    return GRAPH_FAILED;
  }

  (void)ifs.seekg(0, std::ifstream::end);
  const size_t len = static_cast<size_t>(ifs.tellg());
  (void)ifs.seekg(0, std::ifstream::beg);
  (void)ifs.read(buffer, static_cast<std::streamsize>(len));
  data_len = len;
  ifs.close();
  return GRAPH_SUCCESS;
}

graphStatus WriteBinToFile(std::string &path, char_t *data, uint32_t &data_len) {
  GE_ASSERT_TRUE(!path.empty());
  std::ofstream ofs(path, std::ios::out | std::ifstream::binary);
  GE_ASSERT_TRUE(ofs.is_open(), "path:%s open failed", path.c_str());
  (void)ofs.write(data, static_cast<std::streamsize>(data_len));
  ofs.close();
  return GRAPH_SUCCESS;
}

graphStatus WriteBinToFile(const int32_t fd, const char_t * const data, size_t data_len) {
  if ((data == nullptr) || (data_len == 0UL)) {
    GELOGE(GRAPH_FAILED, "check param failed, data is nullptr or length is zero.");
    return GRAPH_FAILED;
  }
  int64_t write_count = 0L;
  auto seek = PtrToPtr<void, uint8_t>(static_cast<void *>(const_cast<char_t *>(data)));
  while (data_len > kMaxWriteSize) {
    write_count = mmWrite(fd, reinterpret_cast<void *>(seek), static_cast<uint32_t>(kMaxWriteSize));
    GE_ASSERT_TRUE(((write_count != EN_INVALID_PARAM) && (write_count != EN_ERROR)),
        "Write data failed, data_len: %llu", data_len);
    seek = PtrAdd<uint8_t>(seek, static_cast<size_t>(data_len), kMaxWriteSize);
    data_len -= kMaxWriteSize;
  }
  if (data_len != 0UL) {
    write_count = mmWrite(fd, reinterpret_cast<void *>(seek), static_cast<uint32_t>(data_len));
    GE_ASSERT_TRUE(((write_count != EN_INVALID_PARAM) && (write_count != EN_ERROR)),
        "Write data failed, data_len: %llu", data_len);
  }
  return GRAPH_SUCCESS;
}

void SplitFilePath(const std::string &file_path, std::string &dir_path, std::string &file_name) {
  if (file_path.empty()) {
    GELOGD("file_path is empty, no need split");
    return;
  }
  int32_t split_pos = static_cast<int32_t>(file_path.length() - 1UL);
  for (; split_pos >= 0; split_pos--) {
    if ((file_path[static_cast<size_t>(split_pos)] == '\\') ||
        (file_path[static_cast<size_t>(split_pos)] == '/')) {
      break;
    }
  }
  if (split_pos < 0) {
    file_name = file_path;
    return;
  }
  dir_path = std::string(file_path).substr(0U, static_cast<size_t>(split_pos));
  file_name = std::string(file_path.substr(split_pos + 1U, file_path.length()));
  return;
}

graphStatus SaveBinToFile(const char * const data, size_t length, const std::string &file_path) {
  if (data == nullptr || length == 0UL) {
    GELOGE(GRAPH_FAILED, "check param failed, data is nullptr or length is zero.");
    return GRAPH_FAILED;
  }
  std::string dir_path;
  std::string file_name;
  SplitFilePath(file_path, dir_path, file_name);
  if (!dir_path.empty()) {
    GE_ASSERT_TRUE((CreateDir(dir_path) == kFileSuccess),
                   "Create direct failed, path: %s.", file_path.c_str());
  }
  std::string real_path = RealPath(dir_path.c_str());
  GE_ASSERT_TRUE(!real_path.empty(), "Path: %s is empty", file_path.c_str());
  real_path = real_path + "/" + file_name;
  // Open file
  const mmMode_t mode = static_cast<mmMode_t>(static_cast<uint32_t>(M_IRUSR) | static_cast<uint32_t>(M_IWUSR));
  const int32_t open_flag = static_cast<int32_t>(static_cast<uint32_t>(M_RDWR) |
                                                 static_cast<uint32_t>(M_CREAT) |
                                                 static_cast<uint32_t>(O_TRUNC));
  int32_t fd = mmOpen2(&real_path[0UL], open_flag, mode);
  GE_ASSERT_TRUE(((fd != EN_INVALID_PARAM) && (fd != EN_ERROR)), "Open file failed, path: %s", real_path.c_str());
  Status ret = GRAPH_SUCCESS;
  if (WriteBinToFile(fd, data, length) != GRAPH_SUCCESS) {
    GELOGE(GRAPH_FAILED, "Write data to file: %s failed.", real_path.c_str());
    ret = GRAPH_FAILED;
  }
  if (mmClose(fd) != 0) { // mmClose:0 success
    GELOGE(GRAPH_FAILED, "Close file failed.");
    return GRAPH_FAILED;
  }
  return ret;
}
}
