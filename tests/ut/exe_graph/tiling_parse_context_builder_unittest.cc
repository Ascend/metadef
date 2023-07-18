/**
 * Copyright 2023 Huawei Technologies Co., Ltd
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
#define private public
#include "runtime/tiling_parse_context_builder.h"
#undef private
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/graph_utils.h"
#include "exe_graph/lowering/value_holder.h"
#include "platform/platform_infos_def.h"
#include "framework/common/util.h"
#include "register/op_impl_space_registry.h"
#include "register/op_impl_registry.h"

namespace gert {
class TilingParseContextBuilderUT : public testing::Test {};

TEST_F(TilingParseContextBuilderUT, FindImplFuncNullptr) {
  std::string op_compile_info_json = "{}";
  fe::PlatFormInfos platform_infos;
  auto builder = TilingParseContextBuilder();

  bg::ValueHolder::PopGraphFrame();
  (void)bg::ValueHolder::PushGraphFrame();
  auto foo = bg::ValueHolder::CreateVoid("Foo", {});
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 0);
  auto outputs = foo->AppendOutputs(2);
  EXPECT_EQ(outputs.size(), 2);
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 2);
  auto bar = bg::ValueHolder::CreateSingleDataOutput("Bar", outputs);
  EXPECT_NE(bar, nullptr);
  auto node = bar->GetNode();
  EXPECT_EQ(node->GetAllInDataAnchorsSize(), 2);
  auto op = ge::OpDescUtils::CreateOperatorFromNode(node->shared_from_this());
  auto holder = builder
                .CompileInfo(&op_compile_info_json)
                .PlatformInfo(&platform_infos)
                .Build(op);
  EXPECT_NE(holder.context_, nullptr);
}

TEST_F(TilingParseContextBuilderUT, CompileInfoNullptr) {
  fe::PlatFormInfos platform_infos;
  auto builder = TilingParseContextBuilder();

  bg::ValueHolder::PopGraphFrame();
  (void)bg::ValueHolder::PushGraphFrame();
  auto foo = bg::ValueHolder::CreateVoid("Foo", {});
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 0);
  auto outputs = foo->AppendOutputs(2);
  EXPECT_EQ(outputs.size(), 2);
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 2);
  auto bar = bg::ValueHolder::CreateSingleDataOutput("Bar", outputs);
  EXPECT_NE(bar, nullptr);
  auto node = bar->GetNode();
  EXPECT_EQ(node->GetAllInDataAnchorsSize(), 2);
  auto op = ge::OpDescUtils::CreateOperatorFromNode(node->shared_from_this());
  auto holder = builder
                .CompileInfo(nullptr)
                .PlatformInfo(&platform_infos)
                .Build(op);
  EXPECT_NE(holder.context_, nullptr);
}

TEST_F(TilingParseContextBuilderUT, PlatformInfosNullptr) {
  std::string op_compile_info_json = "{}";
  fe::PlatFormInfos platform_infos;
  auto builder = TilingParseContextBuilder();

  bg::ValueHolder::PopGraphFrame();
  (void)bg::ValueHolder::PushGraphFrame();
  auto foo = bg::ValueHolder::CreateVoid("Foo", {});
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 0);
  auto outputs = foo->AppendOutputs(2);
  EXPECT_EQ(outputs.size(), 2);
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 2);
  auto bar = bg::ValueHolder::CreateSingleDataOutput("Bar", outputs);
  EXPECT_NE(bar, nullptr);
  auto node = bar->GetNode();
  EXPECT_EQ(node->GetAllInDataAnchorsSize(), 2);
  auto op = ge::OpDescUtils::CreateOperatorFromNode(node->shared_from_this());
  auto holder = builder
                .CompileInfo(&op_compile_info_json)
                .PlatformInfo(nullptr)
                .Build(op);
  EXPECT_NE(holder.context_, nullptr);
}

TEST_F(TilingParseContextBuilderUT, TilingFuncNullptr) {
  std::string op_compile_info_json = "{}";
  fe::PlatFormInfos platform_infos;
  auto builder = TilingParseContextBuilder();

  // construct op impl registry
  IMPL_OP(Bar_0)
    .TilingParse<int32_t>([](gert::KernelContext *kernel_context) -> UINT32 {
        return ge::GRAPH_SUCCESS;
      });
  auto space_registry = std::make_shared<gert::OpImplSpaceRegistry>();
  auto registry_holder = std::make_shared<gert::OpImplRegistryHolder>();
  auto funcs = gert::OpImplRegistry::GetInstance().GetOpImpl("Bar_0");
  registry_holder->AddTypesToImpl("Bar_0", *funcs);
  space_registry->AddRegistry(registry_holder);
  DefaultOpImplSpaceRegistry::GetInstance().SetDefaultSpaceRegistry(space_registry);

  // construct op
  bg::ValueHolder::PopGraphFrame();
  (void)bg::ValueHolder::PushGraphFrame();
  auto foo = bg::ValueHolder::CreateVoid("Foo", {});
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 0);
  auto outputs = foo->AppendOutputs(2);
  EXPECT_EQ(outputs.size(), 2);
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 2);
  auto bar = bg::ValueHolder::CreateSingleDataOutput("Bar_0", outputs);
  EXPECT_NE(bar, nullptr);
  auto node = bar->GetNode();
  EXPECT_EQ(node->GetAllInDataAnchorsSize(), 2);
  ge::OpDescPtr op_desc = node->GetOpDesc();
  ge::GeTensorDesc tensor_desc(ge::GeShape({1}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AddInputDesc("y", tensor_desc);
  op_desc->AddOutputDesc("z", tensor_desc);
  op_desc->MutableInputDesc(1)->SetDataType(ge::DT_INT32);
  op_desc->MutableInputDesc(1)->SetShape(ge::GeShape({1}));
  op_desc->MutableInputDesc(1)->SetOriginShape(ge::GeShape({1}));
  auto op = ge::OpDescUtils::CreateOperatorFromNode(node->shared_from_this());

  auto holder = builder
                .CompileInfo(&op_compile_info_json)
                .PlatformInfo(&platform_infos)
                .Build(op);
  EXPECT_NE(holder.context_, nullptr);
  DefaultOpImplSpaceRegistry::GetInstance().SetDefaultSpaceRegistry(nullptr);
}

TEST_F(TilingParseContextBuilderUT, BuildSuccess) {
  std::string op_compile_info_json = "{}";
  fe::PlatFormInfos platform_infos;
  auto builder = TilingParseContextBuilder();

  // construct op impl registry
  IMPL_OP(Bar)
    .Tiling([](gert::TilingContext *kernel_context) -> UINT32 {
        return ge::GRAPH_SUCCESS;
      })
    .TilingParse<int32_t>([](gert::KernelContext *kernel_context) -> UINT32 {
        return ge::GRAPH_SUCCESS;
      });
  auto space_registry = std::make_shared<gert::OpImplSpaceRegistry>();
  auto registry_holder = std::make_shared<gert::OpImplRegistryHolder>();
  auto funcs = gert::OpImplRegistry::GetInstance().GetOpImpl("Bar");
  registry_holder->AddTypesToImpl("Bar", *funcs);
  space_registry->AddRegistry(registry_holder);
  DefaultOpImplSpaceRegistry::GetInstance().SetDefaultSpaceRegistry(space_registry);

  // construct op
  bg::ValueHolder::PopGraphFrame();
  (void)bg::ValueHolder::PushGraphFrame();
  auto foo = bg::ValueHolder::CreateVoid("Foo", {});
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 0);
  auto outputs = foo->AppendOutputs(2);
  EXPECT_EQ(outputs.size(), 2);
  EXPECT_EQ(foo->GetNode()->GetAllOutDataAnchorsSize(), 2);
  auto bar = bg::ValueHolder::CreateSingleDataOutput("Bar", outputs);
  EXPECT_NE(bar, nullptr);
  auto node = bar->GetNode();
  EXPECT_EQ(node->GetAllInDataAnchorsSize(), 2);
  ge::OpDescPtr op_desc = node->GetOpDesc();
  ge::GeTensorDesc tensor_desc(ge::GeShape({1}));
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AddInputDesc("y", tensor_desc);
  op_desc->AddOutputDesc("z", tensor_desc);
  op_desc->MutableInputDesc(1)->SetDataType(ge::DT_INT32);
  op_desc->MutableInputDesc(1)->SetShape(ge::GeShape({1}));
  op_desc->MutableInputDesc(1)->SetOriginShape(ge::GeShape({1}));
  auto op = ge::OpDescUtils::CreateOperatorFromNode(node->shared_from_this());

  auto holder = builder
                .CompileInfo(&op_compile_info_json)
                .PlatformInfo(&platform_infos)
                .Build(op);
  EXPECT_NE(holder.context_, nullptr);
  auto tiling_parse_context = reinterpret_cast<TilingParseContext *>(holder.context_);
  EXPECT_NE(tiling_parse_context->GetCompiledInfo<int32_t>(), nullptr);
  EXPECT_NE(tiling_parse_context->GetPlatformInfo(), nullptr);
  EXPECT_EQ(*tiling_parse_context->GetCompiledInfo<int32_t>(), 0);
  DefaultOpImplSpaceRegistry::GetInstance().SetDefaultSpaceRegistry(nullptr);
}
}  // namespace gert
