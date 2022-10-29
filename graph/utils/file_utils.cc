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
#include "graph/def_types.h"
#include "graph/debug/ge_log.h"
#include "mmpa/mmpa_api.h"

namespace ge {
namespace {
const size_t kMaxErrStrLen = 128U;

int32_t CheckPathValid(const std::string &file_path) {
  if (file_path.size() >= static_cast<size_t>(MMPA_MAX_PATH)) {
    GELOGE(FAILED, "[Check][FilePath]Failed, file path's length:%zu >= mmpa_max_path:%d",
           file_path.size(), MMPA_MAX_PATH);
    REPORT_INNER_ERROR("E19999", "Check file path failed, file path's length:%zu >= "
                                 "mmpa_max_path:%d", file_path.size(), MMPA_MAX_PATH);
    return -1;
  }
  int32_t path_split_pos = static_cast<int32_t>(file_path.size() - 1U);
  for (; path_split_pos >= 0; path_split_pos--) {
    if ((file_path[static_cast<size_t>(path_split_pos)] == '\\') ||
        (file_path[static_cast<size_t>(path_split_pos)] == '/')) {
      break;
    }
  }
  if (path_split_pos == 0) {
    return 0;
  }
  // If there is a path before the file name, create the path
  if (path_split_pos != -1) {
    if (CreateDirectory(std::string(file_path).substr(0U, static_cast<size_t>(path_split_pos))) != 0) {
      GELOGE(FAILED, "[Create][Directory]Failed, file path:%s.", file_path.c_str());
      return -1;
    }
  }
  return 0;
}

int32_t OpenFile(int32_t &fd, const std::string &file_path) {
  if (CheckPathValid(file_path) != 0) {
    GELOGE(FAILED, "[Check][FilePath]Check output file failed, file_path:%s.", file_path.c_str());
    REPORT_CALL_ERROR("E19999", "Check output file failed, file_path:%s.", file_path.c_str());
    return -1;
  }
  char_t real_path[MMPA_MAX_PATH] = {};
  if (mmRealPath(file_path.c_str(), &real_path[0], MMPA_MAX_PATH) != EN_OK) {
    GELOGI("File %s is not exist, it will be created.", file_path.c_str());
  }
  const mmMode_t mode = static_cast<mmMode_t>(static_cast<uint32_t>(M_IRUSR) | static_cast<uint32_t>(M_IWUSR));
  const int32_t open_flag = static_cast<int32_t>(static_cast<uint32_t>(M_RDWR) |
      static_cast<uint32_t>(M_CREAT) |
      static_cast<uint32_t>(O_TRUNC));
  fd = mmOpen2(&real_path[0], open_flag, mode);
  if ((fd == EN_INVALID_PARAM) || (fd == EN_ERROR)) {
    // -1: Failed to open file; - 2: Illegal parameter
    char_t err_buf[kMaxErrStrLen + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrStrLen);
    GELOGE(FAILED, "[Open][File]Failed. errno:%d, errmsg:%s", fd, err_msg);
    REPORT_INNER_ERROR("E19999", "Open file failed, errno:%d, errmsg:%s.", fd, err_msg);
    return -1;
  }
  return 0;
}

int32_t WriteData(const void * const data, uint64_t size, const int32_t fd) {
  if ((size == 0U) || (data == nullptr)) {
    return -1;
  }
  int64_t write_count;
  const uint32_t size_2g = 2147483648U;  // 0x1 << 31
  // Write data
  if (size > size_2g) {
    const uint32_t size_1g = 1073741824U;  // 0x1 << 30
    auto seek = PtrToPtr<void, uint8_t>(const_cast<void *>(data));
    while (size > size_1g) {
      write_count = mmWrite(fd, reinterpret_cast<void *>(seek), size_1g);
      if ((write_count == EN_INVALID_PARAM) || (write_count == EN_ERROR)) {
        char_t err_buf[kMaxErrStrLen + 1U] = {};
        const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrStrLen);
        GELOGE(FAILED, "[Write][Data]Failed, errno:%ld, errmsg:%s", write_count, err_msg);
        REPORT_INNER_ERROR("E19999", "Write data failed, errno:%" PRId64 ", errmsg:%s.", write_count, err_msg);
        return -1;
      }
      seek = PtrAdd<uint8_t>(seek, static_cast<size_t>(size), size_1g);
      size -= size_1g;
    }
    write_count = mmWrite(fd, reinterpret_cast<void *>(seek), static_cast<uint32_t>(size));
  } else {
    write_count = mmWrite(fd, const_cast<void *>(data), static_cast<uint32_t>(size));
  }

  // -1: Failed to write to file; - 2: Illegal parameter
  if ((write_count == EN_INVALID_PARAM) || (write_count == EN_ERROR)) {
    char_t err_buf[kMaxErrStrLen + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrStrLen);
    GELOGE(FAILED, "[Write][Data]Failed. mmpa_errorno = %ld, error:%s", write_count, err_msg);
    REPORT_INNER_ERROR("E19999", "Write data failed, mmpa_errorno = %" PRId64 ", error:%s.", write_count, err_msg);
    return -1;
  }

  return 0;
}
}  // namespace

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
int32_t CreateDirectory(const std::string &directory_path) {
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

int32_t SaveDataToFile(const std::string &file_path, const void * const data, const uint64_t len) {
  if ((data == nullptr) || (len <= 0)) {
    GELOGE(FAILED, "[Check][Param]Failed, model_data is null or the length[%lu] is less than 1.", len);
    REPORT_INNER_ERROR("E19999", "Save file failed, the model_data is null or "
                                 "its length:%" PRIu64 " is less than 1.", len);
    return -1;
  }
  int32_t fd = 0;
  if (OpenFile(fd, file_path) != 0) {
    GELOGE(FAILED, "OpenFile FAILED");
    return -1;
  }
  if (WriteData(data, len, fd) != 0) {
    GELOGE(FAILED, "WriteData FAILED");
    return -1;
  }
  if (mmClose(fd) != 0) {
    char_t err_buf[kMaxErrStrLen + 1U] = {};
    const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrStrLen);
    GELOGE(FAILED, "[Close][File]Failed, errmsg:%s", err_msg);
    REPORT_CALL_ERROR("E19999", "Close file failed, errmsg:%s", err_msg);
    return -1;
  }
  return 0;
}
}
