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
#include "register/op_impl_registry_holder_manager.h"
#include "graph/utils/file_utils.h"
#include "common/util/mem_utils.h"
#include "mmpa/mmpa_api.h"
#include "tests/depends/mmpa/src/mmpa_stub.h"
#include <gtest/gtest.h>

namespace gert_test {
namespace {
#define MMPA_MAX_PATH 4096
constexpr const char *kHomeEnvName = "HOME";
void GetOppDir(std::string &opp_dir) {
  char path_env[MMPA_MAX_PATH] = {0};
  int32_t ret = mmGetEnv(kHomeEnvName, path_env, MMPA_MAX_PATH);
  if ((ret != EN_OK) || (strlen(path_env) == 0)) {
    return;
  }
  const char *env = getenv(kHomeEnvName);
  opp_dir = env;
  opp_dir += "/.ascend_temp/.om_exe_data/";
  return;
}

ge::OpSoBinPtr CreateSoBinPtr(std::string &so_name, std::string &vendor_name) {
  std::unique_ptr<char[]> so_bin = std::unique_ptr<char[]>(new(std::nothrow) char[so_name.length()]);
  (void) memcpy_s(so_bin.get(), so_name.length(), so_name.data(), so_name.length());
  ge::OpSoBinPtr so_bin_ptr = ge::MakeShared<ge::OpSoBin>(so_name, vendor_name, std::move(so_bin), so_name.length());
  return so_bin_ptr;
}

int super_system(const char *cmd, char *retmsg, int msg_len) {
  FILE *fp;
  int res = -1;
  if (cmd == NULL || retmsg == NULL || msg_len < 0) {
    GELOGD("Err: Fuc:%s system paramer invalid! ", __func__);
    return 1;
  }
  if ((fp = popen(cmd, "r")) == NULL) {
    perror("popen");
    GELOGD("Err: Fuc:%s popen error: %s ", __func__, strerror(errno));
    return 2;
  } else {
    memset(retmsg, 0, msg_len);
    while (fgets(retmsg, msg_len, fp));
    {
      GELOGD("Fuc: %s fgets buf is %s ", __func__, retmsg);
    }
    if ((res = pclose(fp)) == -1) {
      GELOGD("Fuc:%s close popen file pointer fp error! ", __func__);
      return 3;
    }
    retmsg[strlen(retmsg) - 1] = '\0';
    return 0;
  }
}

size_t g_impl_num = 3;
char *type[] = {"Add_0", "Add_1", "Add_2"};
size_t GetRegisteredOpNum() { return g_impl_num; }
uint32_t GetOpImplFunctions(gert::TypesToImpl *impl, size_t g_impl_num) {
  gert::OpImplKernelRegistry::OpImplFunctions funcs;
  for (int i = 0; i < g_impl_num; ++i) {
    funcs.tiling = (gert::OpImplKernelRegistry::TilingKernelFunc) (0x10 + i);
    funcs.infer_shape = (gert::OpImplKernelRegistry::InferShapeKernelFunc) (0x20 + i);
    impl[i].op_type = type[i];
    impl[i].funcs = funcs;
  }
  return 0;
}

void *mock_handle = nullptr;
class MockMmpa : public ge::MmpaStubApi {
 public:
  void *DlSym(void *handle, const char *func_name) override {
    if (std::string(func_name) == "GetRegisteredOpNum") {
      return (void *) &GetRegisteredOpNum;
    } else if (std::string(func_name) == "GetOpImplFunctions") {
      return (void *) &GetOpImplFunctions;
    }
    return nullptr;
  }
  void *DlOpen(const char *fileName, int32_t mode) override {
    if (mock_handle == nullptr) {
      return nullptr;
    }
    return (void *) mock_handle;
  }
  int32_t DlClose(void *handle) override {
    return 0L;
  }
};
}

class OpImplRegistryHolderManagerUT : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() { gert::OpImplRegistryHolderManager::GetInstance().UpdateOpImplRegistries(); }
};

TEST_F(OpImplRegistryHolderManagerUT, OmOpImplRegistryHolder_LoadSo_Succeed) {
  dlog_setlevel(GE_MODULE_NAME, DLOG_DEBUG, 1);
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xffffffff;
  g_impl_num = 1;

  std::string so_name("libopmaster.so");
  std::string vendor_name("MDC");
  ge::OpSoBinPtr so_bin_ptr = CreateSoBinPtr(so_name, vendor_name);
  gert::OmOpImplRegistryHolder om_op_impl_registry_holder;
  auto ret = om_op_impl_registry_holder.LoadSo(so_bin_ptr);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
  EXPECT_EQ(om_op_impl_registry_holder.GetTypesToImpl().size(), 1);

  //校验文件是否落盘
  std::string opp_dir;
  GetOppDir(opp_dir);
  std::string pid = std::to_string(mmGetPid());
  std::string tid = std::to_string(mmGetTid());
  std::string command = "ls " + opp_dir + "| grep " + pid + "| grep " + tid + " | wc -l";

  char retmsg[1024];
  int ret1 = super_system(command.c_str(), retmsg, sizeof(retmsg));
  EXPECT_EQ(ret1, 0);
  EXPECT_EQ(atoi(retmsg), 1);

  std::string command2 = "ls " + opp_dir + "| grep " + pid;
  int ret2 = super_system(command2.c_str(), retmsg, sizeof(retmsg));
  opp_dir += retmsg;
  std::string command3 = "ls " + opp_dir;
  int ret3 = super_system(command3.c_str(), retmsg, sizeof(retmsg));
  std::string opp_path = opp_dir + "/" + retmsg;
  GELOGD("opp_path: %s", opp_path.c_str());

  // 比较落盘数据内容是否和罗落盘前一致
  uint32_t data_len;
  auto bin_data = ge::GetBinFromFile(opp_path, data_len);
  EXPECT_EQ(so_bin_ptr->GetBinDataSize(), data_len);
  EXPECT_EQ(memcmp(so_bin_ptr->GetBinData(), bin_data.get(), data_len), 0);
}

TEST_F(OpImplRegistryHolderManagerUT, OmOpImplRegistryHolder_LoadSo_DlopenFailed) {
  dlog_setlevel(GE_MODULE_NAME, DLOG_DEBUG, 1);
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = nullptr;
  g_impl_num = 1;

  std::string so_name("libopmaster.so");
  std::string vendor_name("MDC");
  ge::OpSoBinPtr so_bin_ptr = CreateSoBinPtr(so_name, vendor_name);
  gert::OmOpImplRegistryHolder om_op_impl_registry_holder;
  auto ret = om_op_impl_registry_holder.LoadSo(so_bin_ptr);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
  EXPECT_EQ(om_op_impl_registry_holder.GetTypesToImpl().size(), 0);
}

TEST_F(OpImplRegistryHolderManagerUT, OmOpImplRegistryHolder_LoadSo_DlsymFailed) {
  dlog_setlevel(GE_MODULE_NAME, DLOG_DEBUG, 0);
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0x7000;
  g_impl_num = 1;

  std::string so_name("libopmaster.so");
  std::string vendor_name("MDC");
  ge::OpSoBinPtr so_bin_ptr = CreateSoBinPtr(so_name, vendor_name);
  gert::OmOpImplRegistryHolder om_op_impl_registry_holder;
  auto ret = om_op_impl_registry_holder.LoadSo(so_bin_ptr);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
  EXPECT_EQ(om_op_impl_registry_holder.GetTypesToImpl().size(), 0);
}

TEST_F(OpImplRegistryHolderManagerUT, GetOrCreateOpImplRegistryHolder_Succeed_HaveRegisted) {
  std::string so_data("libopmaster.so");
  std::string so_name("libopmaster.so");
  auto registry_holder = std::make_shared<gert::OpImplRegistryHolder>();
  gert::OpImplRegistryHolderManager::GetInstance().AddRegistry(so_data, registry_holder);
  ge::SoInOmInfo so_info;
  so_info.cpu_info = "x86";
  so_info.os_info = "Linux";
  so_info.opp_version = "1.84";
  auto registry_holder1 = gert::OpImplRegistryHolderManager::GetInstance().GetOrCreateOpImplRegistryHolder(so_data,
                                                                                                          so_name,
                                                                                                          so_info,
                                                                                                          nullptr);
  EXPECT_NE(registry_holder1, nullptr);
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 1);
}

TEST_F(OpImplRegistryHolderManagerUT, GetOrCreateOpImplRegistryHolder_Succeed_NoRegisted) {
  std::string so_data("libopmaster.so");
  std::string so_name("libopmaster.so");
  std::string vendor_name("MDC");
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xffffffff;
  g_impl_num = 1;
  ge::OpSoBinPtr so_bin_ptr = CreateSoBinPtr(so_name, vendor_name);
  ge::SoInOmInfo so_info;
  so_info.cpu_info = "x86";
  so_info.os_info = "Linux";
  so_info.opp_version = "1.84";
  auto create_func = [&so_bin_ptr]() ->gert::OpImplRegistryHolderPtr {
    auto om_registry_holder = std::make_shared<gert::OmOpImplRegistryHolder>();
    if (om_registry_holder == nullptr) {
      GELOGE(ge::FAILED, "make_shared om op impl registry holder failed");
      return nullptr;
    }
    if((om_registry_holder->LoadSo(so_bin_ptr)) != ge::GRAPH_SUCCESS) {
      GELOGE(ge::FAILED, "om registry holder load so failed");
      return nullptr;
    }
    return om_registry_holder;
  };
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 0);
  auto registry_holder1 = gert::OpImplRegistryHolderManager::GetInstance().GetOrCreateOpImplRegistryHolder(so_data,
                                                                                                           so_name,
                                                                                                           so_info,
                                                                                                           create_func);
  EXPECT_NE(registry_holder1, nullptr);
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 1);
}

TEST_F(OpImplRegistryHolderManagerUT, GetOrCreateOpImplRegistryHolder_Failed_CreateFuncFailed) {
  std::string so_data("libopmaster.so");
  std::string so_name("libopmaster.so");
  std::string vendor_name("MDC");
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xffffffff;
  g_impl_num = 1;
  ge::OpSoBinPtr so_bin_ptr = CreateSoBinPtr(so_name, vendor_name);
  ge::SoInOmInfo so_info;
  so_info.cpu_info = "x86";
  so_info.os_info = "Linux";
  so_info.opp_version = "1.84";
  auto create_func = [&so_bin_ptr]() ->gert::OpImplRegistryHolderPtr {
    return nullptr;
  };
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 0);
  auto registry_holder1 = gert::OpImplRegistryHolderManager::GetInstance().GetOrCreateOpImplRegistryHolder(so_data,
                                                                                                           so_name,
                                                                                                           so_info,
                                                                                                           create_func);
  EXPECT_EQ(registry_holder1, nullptr);
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 0);
}

TEST_F(OpImplRegistryHolderManagerUT, GetOrCreateOpImplRegistryHolder_Failed_CreateFuncNull) {
  std::string so_data("libopmaster.so");
  std::string so_name("libopmaster.so");
  std::string vendor_name("MDC");
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xffffffff;
  g_impl_num = 1;
  ge::OpSoBinPtr so_bin_ptr = CreateSoBinPtr(so_name, vendor_name);
  ge::SoInOmInfo so_info;
  so_info.cpu_info = "x86";
  so_info.os_info = "Linux";
  so_info.opp_version = "1.84";
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 0);
  auto registry_holder1 = gert::OpImplRegistryHolderManager::GetInstance().GetOrCreateOpImplRegistryHolder(so_data,
                                                                                                           so_name,
                                                                                                           so_info,
                                                                                                           nullptr);
  EXPECT_EQ(registry_holder1, nullptr);
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 0);
}

TEST_F(OpImplRegistryHolderManagerUT, OpImplRegistryManager_UpdateOpImplRegistries_Succeed) {
  std::string so_data("libopmaster.so");
  {
    auto registry_holder = std::make_shared<gert::OpImplRegistryHolder>();
    gert::OpImplRegistryHolderManager::GetInstance().AddRegistry(so_data, registry_holder);
    auto tmp_registry_holder = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_data);
    EXPECT_NE(tmp_registry_holder, nullptr);
    EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 1);
  }
  auto tmp_registry_holder = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_data);
  EXPECT_EQ(tmp_registry_holder, nullptr);
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 1);

  gert::OpImplRegistryHolderManager::GetInstance().UpdateOpImplRegistries();
  tmp_registry_holder = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_data);
  EXPECT_EQ(tmp_registry_holder, nullptr);
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 0);
}
}  // namespace gert_test
