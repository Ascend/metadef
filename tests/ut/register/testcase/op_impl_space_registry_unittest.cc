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
#include "register/op_impl_registry.h"
#include "register/op_impl_space_registry.h"
#include "register/op_impl_registry_holder_manager.h"
//#include "graph/utils/file_utils.h"
#include "common/util/mem_utils.h"
#include "mmpa/mmpa_api.h"
#include "tests/depends/mmpa/src/mmpa_stub.h"
#include <gtest/gtest.h>

namespace gert_test {
namespace {
ge::OpSoBinPtr CreateSoBinPtr(std::string &so_name, std::string &vendor_name) {
  std::unique_ptr<char[]> so_bin = std::unique_ptr<char[]>(new(std::nothrow) char[so_name.length()]);
  (void) memcpy_s(so_bin.get(), so_name.length(), so_name.data(), so_name.length());
  ge::OpSoBinPtr so_bin_ptr = ge::MakeShared<ge::OpSoBin>(so_name, vendor_name, std::move(so_bin), so_name.length());
  return so_bin_ptr;
}

size_t g_impl_num = 3;
char *type[] = {"Add_0", "Add_1", "Add_2"};
size_t GetRegisteredOpNum() { return g_impl_num; }
uint32_t GetOpImplFunctions(TypesToImpl *impl, size_t g_impl_num) {
  gert::OpImplKernelRegistry::OpImplFunctions funcs;
  for (int i = 0; i < g_impl_num; ++i) {
    funcs.tiling = (gert::OpImplKernelRegistry::TilingKernelFunc) (0x10 + i);
    funcs.infer_shape = (gert::OpImplKernelRegistry::InferShapeKernelFunc) (0x20 + i);
    funcs.infer_datatype = nullptr;
    funcs.tiling_parse = nullptr;
    funcs.compile_info_deleter = nullptr;
    funcs.compile_info_creator = nullptr;
    funcs.inputs_dependency = 0;
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

class OpImplSpaceRegistryUT : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {
    gert::OpImplRegistryHolderManager::GetInstance().ClearOpImplRegistries();
    ge::MmpaStub::GetInstance().Reset();
  }
};

ge::graphStatus TestInferShapeFunc(gert::InferShapeContext *) {
  return ge::GRAPH_SUCCESS;
}
TEST_F(OpImplSpaceRegistryUT, OpImplSpaceRegistry_GetOrCreateRegistry_1so_Succeed) {
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xffffffff;
  g_impl_num = 1;

  std::string so_name("libopmaster.so");
  std::string vendor_name("MDC");
  ge::OpSoBinPtr so_bin_ptr = CreateSoBinPtr(so_name, vendor_name);
  std::vector<ge::OpSoBinPtr> bins;
  bins.emplace_back(so_bin_ptr);
  ge::SoInOmInfo so_info;
  so_info.cpu_info = "x86";
  so_info.os_info = "Linux";
  so_info.opp_version = "1.84";
  gert::OpImplSpaceRegistry space_registry;
  EXPECT_EQ(space_registry.GetOrCreateRegistry(bins, so_info), ge::GRAPH_SUCCESS);
  std::string so_data = so_name;
  auto registry_holder = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_data);
  EXPECT_NE(registry_holder, nullptr);
#ifndef ONLY_COMPILE_OPEN_SRC
  EXPECT_NE(space_registry.GetOpImpl("Add_0"), nullptr);
  EXPECT_EQ(space_registry.GetPrivateAttrs("Add_0").size(), 0);
#else
  IMPL_OP(Add_0).InferShape(TestInferShapeFunc).PrivateAttr("attr1");
  EXPECT_EQ(space_registry.GetOpImpl("Add_0")->infer_shape, TestInferShapeFunc);
  EXPECT_EQ(space_registry.GetPrivateAttrs("Add_0").size(), 1);
#endif
}

TEST_F(OpImplSpaceRegistryUT, OpImplSpaceRegistry_GetOrCreateRegistry_2so_Succeed) {
  g_impl_num = 2;
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xffffffff;

  ge::SoInOmInfo so_info;
  so_info.cpu_info = "x86";
  so_info.os_info = "Linux";
  so_info.opp_version = "1.84";

  std::string so_name1("libopmaster.so");
  std::string vendor_name1("MDC");
  ge::OpSoBinPtr so_bin_ptr1 = CreateSoBinPtr(so_name1, vendor_name1);

  gert::OpImplSpaceRegistry space_registry;
  EXPECT_EQ(space_registry.GetOrCreateRegistry({so_bin_ptr1}, so_info), ge::GRAPH_SUCCESS);
  auto registry_holder1 = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_name1);
  EXPECT_NE(registry_holder1, nullptr);
#ifndef ONLY_COMPILE_OPEN_SRC
  EXPECT_NE(space_registry.GetOpImpl("Add_0"), nullptr);
  EXPECT_EQ(space_registry.GetPrivateAttrs("Add_0").size(), 0);
#endif

  std::string so_name2("libopsproto.so");
  std::string vendor_name2("DC");
  ge::OpSoBinPtr so_bin_ptr2 = CreateSoBinPtr(so_name2, vendor_name2);
  EXPECT_EQ(space_registry.GetOrCreateRegistry({so_bin_ptr2}, so_info), ge::GRAPH_SUCCESS);
  auto registry_holder2 = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_name2);
  EXPECT_NE(registry_holder2, nullptr);

  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 2);
  // 再次校验 registry_holder1
  registry_holder1 = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_name1);
  EXPECT_NE(registry_holder1, nullptr);
}
TEST_F(OpImplSpaceRegistryUT, OpImplSpaceRegistry_GetOrCreateRegistry_3so_Succeed) {
  g_impl_num = 2;
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xffffffff;

  ge::SoInOmInfo so_info;
  so_info.cpu_info = "x86";
  so_info.os_info = "Linux";
  so_info.opp_version = "1.84";

  std::string so_name1("libopmaster.so");
  std::string vendor_name1("MDC");
  ge::OpSoBinPtr so_bin_ptr1 = CreateSoBinPtr(so_name1, vendor_name1);

  gert::OpImplSpaceRegistry space_registry;
  EXPECT_EQ(space_registry.GetOrCreateRegistry({so_bin_ptr1}, so_info), ge::GRAPH_SUCCESS);
  auto registry_holder1 = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_name1);
  EXPECT_NE(registry_holder1, nullptr);
#ifndef ONLY_COMPILE_OPEN_SRC
  EXPECT_NE(space_registry.GetOpImpl("Add_0"), nullptr);
  EXPECT_EQ(space_registry.GetPrivateAttrs("Add_0").size(), 0);
#endif

  std::string so_name2("libopsproto.so");
  std::string vendor_name2("DC");
  ge::OpSoBinPtr so_bin_ptr2 = CreateSoBinPtr(so_name2, vendor_name2);
  EXPECT_EQ(space_registry.GetOrCreateRegistry({so_bin_ptr2}, so_info), ge::GRAPH_SUCCESS);
  auto registry_holder2 = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_name2);
  EXPECT_NE(registry_holder2, nullptr);
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 2);

  std::string so_name3("libopsproto.so");
  std::string vendor_name3("DC");
  ge::OpSoBinPtr so_bin_ptr3 = CreateSoBinPtr(so_name2, vendor_name2);
  EXPECT_EQ(space_registry.GetOrCreateRegistry({so_bin_ptr2}, so_info), ge::GRAPH_SUCCESS);
  auto registry_holder3 = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_name2);
  EXPECT_NE(registry_holder2, nullptr);
  EXPECT_EQ(gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistrySize(), 2);

  // 再次校验 registry_holder1
  registry_holder1 = gert::OpImplRegistryHolderManager::GetInstance().GetOpImplRegistryHolder(so_name1);
  EXPECT_NE(registry_holder1, nullptr);
}

TEST_F(OpImplSpaceRegistryUT, OpImplSpaceRegistry_AddRegistry_Succeed) {
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xffffffff;
  g_impl_num = 3;

  gert::OpImplSpaceRegistry space_registry;
  auto registry_holder = std::make_shared<gert::OmOpImplRegistryHolder>();

  std::string so_name("libopmaster.so");
  std::string vendor_name("MDC");
  ge::OpSoBinPtr so_bin_ptr = CreateSoBinPtr(so_name, vendor_name);
  auto ret = registry_holder->LoadSo(so_bin_ptr);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);

  auto types_to_impl = registry_holder->GetTypesToImpl();
  ret = space_registry.AddRegistry(registry_holder);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);

#ifndef ONLY_COMPILE_OPEN_SRC
  auto func = space_registry.GetOpImpl("Add_0");
  EXPECT_EQ(func->tiling, (gert::OpImplKernelRegistry::TilingKernelFunc) 0x10);
  EXPECT_EQ(func->infer_shape, (gert::OpImplKernelRegistry::InferShapeKernelFunc) 0x20);

  func = space_registry.GetOpImpl("Add_1");
  EXPECT_EQ(func->tiling, (gert::OpImplKernelRegistry::TilingKernelFunc) 0x11);
  EXPECT_EQ(func->infer_shape, (gert::OpImplKernelRegistry::InferShapeKernelFunc) 0x21);

  func = space_registry.GetOpImpl("Add_2");
  EXPECT_EQ(func->tiling, (gert::OpImplKernelRegistry::TilingKernelFunc) 0x12);
  EXPECT_EQ(func->infer_shape, (gert::OpImplKernelRegistry::InferShapeKernelFunc) 0x22);

  func = space_registry.GetOpImpl("Add_3");
  EXPECT_EQ(func, nullptr);
#endif
}

TEST_F(OpImplSpaceRegistryUT, DefaultOpImplSpaceRegistry_SetSpaceRegistry_Succeed) {
  auto space_registry = std::make_shared<gert::OpImplSpaceRegistry>();
  gert::DefaultOpImplSpaceRegistry::GetInstance().SetDefaultSpaceRegistry(space_registry);
  EXPECT_NE(gert::DefaultOpImplSpaceRegistry::GetInstance().GetDefaultSpaceRegistry(), nullptr);
}
}  // namespace gert_test