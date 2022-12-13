// Copyright (c) 2021 Huawei Technologies Co., Ltd
// All rights reserved.
//
// Licensed under the BSD 3-Clause License  (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#define private public
#define protected public
#include "common/plugin/plugin_manager.h"

#include <iostream>

using namespace testing;
using namespace std;

namespace ge {
namespace {
const char *const kEnvName = "ASCEND_OPP_PATH";
}
class UtestPluginManager : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(UtestPluginManager, test_plugin_manager_load) {
  PluginManager manager;
  auto test_so_name = "test_fail.so";
  manager.handles_[test_so_name] = nullptr;
  manager.ClearHandles_();
  ASSERT_EQ(manager.handles_.size(), 0);
  EXPECT_EQ(manager.LoadSo("./", {}), SUCCESS);
  int64_t file_size = 1;
  EXPECT_EQ(manager.ValidateSo("./", 1, file_size), SUCCESS);

  std::string path = GetModelPath();
  std::vector<std::string> pathList;
  pathList.push_back(path);

  std::vector<std::string> funcChkList;
  const std::string file_name = "libcce.so";
  funcChkList.push_back(file_name);

  system(("touch " + path + file_name).c_str());

  EXPECT_EQ(manager.Load("", pathList), SUCCESS);
  EXPECT_EQ(manager.Load(path, {}), SUCCESS);
  EXPECT_EQ(manager.LoadSo(path, {}), SUCCESS);

  path += file_name + "/";
  pathList.push_back(path);
  EXPECT_EQ(manager.LoadSo(path, pathList), SUCCESS);

  unlink(path.c_str());
}

TEST_F(UtestPluginManager, test_plugin_manager_getopp_plugin_vendors_01) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_vendors).c_str());
  system(("echo 'load_priority=customize,mdc,lhisi' > " + path_config).c_str());

  std::vector<std::string> vendors;
  Status ret = PluginManager::GetOppPluginVendors(path_config, vendors);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(vendors[0], "customize");
  EXPECT_EQ(vendors[1], "mdc");
  EXPECT_EQ(vendors[2], "lhisi");
}

TEST_F(UtestPluginManager, test_plugin_manager_getopp_plugin_vendors_02) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_vendors).c_str());
  system(("echo '' > " + path_config).c_str());

  std::vector<std::string> vendors;
  Status ret = PluginManager::GetOppPluginVendors(path_config, vendors);
  EXPECT_NE(ret, SUCCESS);
}

TEST_F(UtestPluginManager, test_plugin_manager_getopp_plugin_vendors_03) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_vendors).c_str());
  system(("echo 'load_priority' > " + path_config).c_str());

  std::vector<std::string> vendors;
  Status ret = PluginManager::GetOppPluginVendors(path_config, vendors);
  EXPECT_NE(ret, SUCCESS);
}

TEST_F(UtestPluginManager, test_plugin_manager_getopp_plugin_vendors_04) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_vendors).c_str());
  system(("rm -rf " + path_config).c_str());

  std::vector<std::string> vendors;
  Status ret = PluginManager::GetOppPluginVendors(path_config, vendors);
  EXPECT_NE(ret, SUCCESS);
}

TEST_F(UtestPluginManager, test_plugin_manager_GetOpsProtoPath_01) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  system(("rm -rf " + path_builtin).c_str());

  std::string opsproto_path;
  Status ret = PluginManager::GetOpsProtoPath(opsproto_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(opsproto_path,
      opp_path + "op_proto/custom/:" + opp_path + "op_proto/built-in/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetOpsProtoPath_02) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_builtin).c_str());
  system(("mkdir -p " + path_vendors).c_str());
  system(("echo 'load_priority=customize,mdc,lhisi' > " + path_config).c_str());

  std::string opsproto_path;
  Status ret = PluginManager::GetOpsProtoPath(opsproto_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(opsproto_path,
    path_vendors + "/customize/op_proto/:" +
    path_vendors + "/mdc/op_proto/:" +
    path_vendors + "/lhisi/op_proto/:" +
    opp_path + "built-in/op_proto/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetOpsProtoPath_03) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_builtin).c_str());
  system(("mkdir -p " + path_vendors).c_str());
  system(("echo 'load_priority' > " + path_config).c_str());

  std::string opsproto_path;
  Status ret = PluginManager::GetOpsProtoPath(opsproto_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(opsproto_path,
    opp_path + "op_proto/custom/:" +
    opp_path + "built-in/op_proto/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetOpTilingPath_01) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  system(("rm -rf " + path_builtin).c_str());

  std::string op_tiling_path;
  Status ret = PluginManager::GetOpTilingPath(op_tiling_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_tiling_path,
      opp_path + "op_impl/built-in/ai_core/tbe/:" + opp_path + "op_impl/custom/ai_core/tbe/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetOpTilingPath_02) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_builtin).c_str());
  system(("mkdir -p " + path_vendors).c_str());
  system(("echo 'load_priority=customize,mdc,lhisi' > " + path_config).c_str());

  std::string op_tiling_path;
  Status ret = PluginManager::GetOpTilingPath(op_tiling_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_tiling_path,
    opp_path + "built-in/op_impl/ai_core/tbe/:" +
    path_vendors + "/lhisi/op_impl/ai_core/tbe/:" +
    path_vendors + "/mdc/op_impl/ai_core/tbe/:" +
    path_vendors + "/customize/op_impl/ai_core/tbe/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetOpTilingPath_03) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_builtin).c_str());
  system(("mkdir -p " + path_vendors).c_str());
  system(("echo 'load_priority' > " + path_config).c_str());

  std::string op_tiling_path;
  Status ret = PluginManager::GetOpTilingPath(op_tiling_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_tiling_path,
    opp_path + "built-in/op_impl/ai_core/tbe/:" +
    opp_path + "op_impl/custom/ai_core/tbe/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetCustomOpPath_01) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  system(("rm -rf " + path_builtin).c_str());

  std::string customop_path;
  Status ret = PluginManager::GetCustomOpPath("tensorflow", customop_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(customop_path,
    opp_path + "framework/custom/:" + opp_path + "framework/built-in/tensorflow/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetCustomOpPath_02) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_builtin).c_str());
  system(("mkdir -p " + path_vendors).c_str());
  system(("echo 'load_priority=customize,mdc,lhisi' > " + path_config).c_str());

  std::string customop_path;
  Status ret = PluginManager::GetCustomOpPath("tensorflow", customop_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(customop_path,
    path_vendors + "/customize/framework/:" +
    path_vendors + "/mdc/framework/:" +
    path_vendors + "/lhisi/framework/:" +
    opp_path + "built-in/framework/tensorflow/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetCustomOpPath_03) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  std::string path_vendors = opp_path + "vendors";
  std::string path_config = path_vendors + "/config.ini";
  system(("mkdir -p " + path_builtin).c_str());
  system(("mkdir -p " + path_vendors).c_str());
  system(("rm -rf " + path_config).c_str());

  std::string customop_path;
  Status ret = PluginManager::GetCustomOpPath("tensorflow", customop_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(customop_path,
    opp_path + "framework/custom/:" +
    opp_path + "built-in/framework/tensorflow/"
  );
}

TEST_F(UtestPluginManager, test_plugin_manager_GetConstantFoldingOpsPath_01) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  system(("rm -rf " + path_builtin).c_str());

  std::string customop_path;
  Status ret = PluginManager::GetConstantFoldingOpsPath("/tmp", customop_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(customop_path, opp_path + "/op_impl/built-in/host_cpu");
}

TEST_F(UtestPluginManager, test_plugin_manager_GetConstantFoldingOpsPath_02) {
  std::string opp_path = __FILE__;
  opp_path = opp_path.substr(0, opp_path.rfind("/") + 1);
  mmSetEnv(kEnvName, opp_path.c_str(), 1);

  std::string path_builtin = opp_path + "built-in";
  system(("mkdir -p " + path_builtin).c_str());

  std::string customop_path;
  Status ret = PluginManager::GetConstantFoldingOpsPath("/tmp", customop_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(customop_path, opp_path + "/built-in/op_impl/host_cpu");
}

TEST_F(UtestPluginManager, GetOppSupportedOsAndCpuType_SUCCESS) {
  std::unordered_map<std::string, std::unordered_set<std::string>> opp_supported_os_cpu;
  opp_supported_os_cpu["linux"] = {"x86_64"};
  opp_supported_os_cpu["windows"] = {"x86_64", "aarch64"};

  std::string opp_path = "./test_os_and_cpu/";
  // construct dir
  for (auto it0 : opp_supported_os_cpu) {
    std::string first_layer = opp_path + "built-in/op_proto/lib/" + it0.first;
    cout<<"====first layer: "<<first_layer<<endl;
    for (auto it1 : it0.second) {
      std::string second_layer = first_layer + "/" + it1;
      system(("mkdir -p " + second_layer).c_str());
      cout<<"====second layer: "<<second_layer<<endl;
    }
  }

  // get dir and check
  mmSetEnv(kEnvName, opp_path.c_str(), 1);
  std::unordered_map<std::string, std::unordered_set<std::string>> opp_supported_os_cpu_tmp;
  PluginManager::GetOppSupportedOsAndCpuType(opp_supported_os_cpu_tmp);
  for (auto it0 : opp_supported_os_cpu_tmp) {
    cout<<"====get first layer: "<<it0.first<<endl;
    ASSERT_NE(opp_supported_os_cpu.count(it0.first), 0);
    for (auto it1 : opp_supported_os_cpu_tmp[it0.first]) {
      cout<<"====get second layer: "<<it1<<endl;
      ASSERT_NE(opp_supported_os_cpu[it0.first].count(it1), 0);
    }
  }

  // del dir
  system(("rm -rf " + opp_path).c_str());
}

TEST_F(UtestPluginManager, GetOppSupportedOsAndCpuType_FAIL) {
  dlog_setlevel(0, 0, 0);
  std::unordered_map<std::string, std::unordered_set<std::string>> opp_supported_os_cpu_tmp;
  PluginManager::GetOppSupportedOsAndCpuType(opp_supported_os_cpu_tmp, "./", "linux", 2);
  ASSERT_EQ(opp_supported_os_cpu_tmp.empty(), true);

  PluginManager::GetOppSupportedOsAndCpuType(opp_supported_os_cpu_tmp, "./not_exit", "linux", 1);
  ASSERT_EQ(opp_supported_os_cpu_tmp.empty(), true);

  std::string path = "./scene.info";
  system(("touch " + path).c_str());
  PluginManager::GetOppSupportedOsAndCpuType(opp_supported_os_cpu_tmp, path, "linux", 1);
  ASSERT_EQ(opp_supported_os_cpu_tmp.empty(), true);
  system(("rm -f " + path).c_str());
  dlog_setlevel(0, 3, 0);
}

TEST_F(UtestPluginManager, GetVersionFromPath_fail) {
  std::string opp_version = "./version.info";
  system(("touch " + opp_version).c_str());
  std::string version;
  ASSERT_EQ(PluginManager::GetVersionFromPath(opp_version, version), false);
  ASSERT_EQ(version.empty(), true);

  system(("echo '=3.20.T100.0.B356' > " + opp_version).c_str());
  ASSERT_EQ(PluginManager::GetVersionFromPath(opp_version, version), false);
  ASSERT_EQ(version.empty(), true);

  system(("echo 'Version=' > " + opp_version).c_str());
  ASSERT_EQ(PluginManager::GetVersionFromPath(opp_version, version), false);
  ASSERT_EQ(version.empty(), true);

  system(("rm -f " + opp_version).c_str());
}

TEST_F(UtestPluginManager, GetCurEnvPackageOsAndCpuType) {
  std::string path ="./opp";
  system(("mkdir -p " + path).c_str());
  path += "/scene.info";
  system(("touch " + path).c_str());
  system(("echo 'os=linux' > " + path).c_str());
  system(("echo 'arch=x86_64' >> " + path).c_str());

  std::string path1 ="./../opp";
  system(("mkdir -p " + path1).c_str());
  path1 += "/scene.info";
  system(("touch " + path1).c_str());
  system(("echo 'os=linux' > " + path1).c_str());
  system(("echo 'arch=x86_64' >> " + path1).c_str());

  system("pwd");
  system(("realpath " + path).c_str());
  system(("realpath " + path1).c_str());

  std::string cur_env_os;
  std::string cur_env_cpu;
  PluginManager::GetCurEnvPackageOsAndCpuType(cur_env_os, cur_env_cpu);
  ASSERT_EQ(cur_env_os, "linux");
  ASSERT_EQ(cur_env_cpu, "x86_64");

  system(("echo -e 'error' > " + path).c_str());
  system(("echo -e 'error' > " + path1).c_str());
  cur_env_os.clear();
  cur_env_cpu.clear();
  PluginManager::GetCurEnvPackageOsAndCpuType(cur_env_os, cur_env_cpu);
  ASSERT_EQ(cur_env_os.empty(), true);
  ASSERT_EQ(cur_env_cpu.empty(), true);

  system(("rm -f " + path).c_str());
  system(("rm -f " + path1).c_str());
  cur_env_os.clear();
  cur_env_cpu.clear();
  PluginManager::GetCurEnvPackageOsAndCpuType(cur_env_os, cur_env_cpu);
  ASSERT_EQ(cur_env_os.empty(), true);
  ASSERT_EQ(cur_env_cpu.empty(), true);
}
}  // namespace ge
