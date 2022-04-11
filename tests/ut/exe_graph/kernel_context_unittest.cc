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
#include "exe_graph/runtime/kernel_context.h"
#include <gtest/gtest.h>
#include "faker/kernel_run_context_facker.h"
namespace gert {
class KernelContextUT : public testing::Test {};
namespace {
struct TestT {
  int64_t a;
  int32_t b;
};
}  // namespace
TEST_F(KernelContextUT, ChainGetInnerOk) {
  Chain c;
  EXPECT_EQ(c.GetPointer<uint64_t>(), reinterpret_cast<uint64_t *>(&c));
  EXPECT_EQ(c.GetPointer<uint8_t>(), reinterpret_cast<uint8_t *>(&c));

  const Chain &cc = c;
  EXPECT_EQ(cc.GetPointer<uint64_t>(), reinterpret_cast<uint64_t *>(&c));
  EXPECT_EQ(cc.GetPointer<uint8_t>(), reinterpret_cast<uint8_t *>(&c));
}

TEST_F(KernelContextUT, ChainGetAllocOk) {
  AsyncAnyValue av = {nullptr, nullptr};
  Chain *c = reinterpret_cast<Chain *>(&av);
  const Chain *cc = reinterpret_cast<Chain *>(&av);

  EXPECT_EQ(c->GetPointer<TestT>(), reinterpret_cast<TestT *>(av.data));
  EXPECT_EQ(c->GetPointer<TestT>(), reinterpret_cast<TestT *>(av.data));

  EXPECT_EQ(cc->GetPointer<TestT>(), reinterpret_cast<TestT *>(av.data));
  EXPECT_EQ(cc->GetPointer<TestT>(), reinterpret_cast<TestT *>(av.data));
}
TEST_F(KernelContextUT, ChainGetInnerValueOk) {
  AsyncAnyValue av = {nullptr, nullptr};
  av.data = reinterpret_cast<void *>(10);

  Chain *c = reinterpret_cast<Chain *>(&av);
  const Chain *cc = reinterpret_cast<Chain *>(&av);

  EXPECT_EQ(c->GetValue<int64_t>(), 10);
  EXPECT_EQ(cc->GetValue<int64_t>(), 10);

  c->GetValue<int64_t>() = 20;
  EXPECT_EQ(reinterpret_cast<int64_t>(av.data), 20);
}

TEST_F(KernelContextUT, ChainSetDeleterOk) {
  AsyncAnyValue av = {nullptr, nullptr};
  Chain *c = reinterpret_cast<Chain *>(&av);

  c->SetWithDefaultDeleter(new TestT());
  ASSERT_NE(av.data, nullptr);
  ASSERT_NE(av.deleter, nullptr);
  av.deleter(av.data);

  c->SetWithDefaultDeleter(new std::vector<int64_t>());
  ASSERT_NE(av.data, nullptr);
  ASSERT_NE(av.deleter, nullptr);
  av.deleter(av.data);
}

TEST_F(KernelContextUT, ChainSetAndUseStructOk) {
  AsyncAnyValue av = {nullptr, nullptr};
  Chain *c = reinterpret_cast<Chain *>(&av);

  c->SetWithDefaultDeleter(new std::vector<int64_t>());

  auto vec = c->GetPointer<std::vector<int64_t>>();
  vec->push_back(10);
  vec->push_back(20);
  EXPECT_EQ(c->GetPointer<std::vector<int64_t>>()->size(), 2);
  EXPECT_EQ(*c->GetPointer<std::vector<int64_t>>(), std::vector<int64_t>({10, 20}));

  av.deleter(av.data);
}

TEST_F(KernelContextUT, GetInputNumOk) {
  auto context_holder = KernelRunContextFaker().KernelIONum(5, 6).Build();
  auto context = context_holder.GetContext<KernelContext>();
  EXPECT_EQ(context->GetInputNum(), 5);
  EXPECT_EQ(context->GetOutputNum(), 6);
}

TEST_F(KernelContextUT, GetInnerInputOk) {
  auto context_holder = KernelRunContextFaker().KernelIONum(5, 6).Build();
  auto context = context_holder.GetContext<KernelContext>();

  EXPECT_EQ(context->GetInputPointer<int64_t>(0),
            reinterpret_cast<const int64_t *>(&(context_holder.value_holder[0].data)));
  EXPECT_EQ(context->GetInputPointer<int64_t>(1),
            reinterpret_cast<const int64_t *>(&(context_holder.value_holder[1].data)));
  EXPECT_EQ(context->GetInputPointer<int32_t>(2),
            reinterpret_cast<const int32_t *>(&(context_holder.value_holder[2].data)));

  EXPECT_EQ(context->GetInputValue<int64_t>(4), reinterpret_cast<int64_t>(context_holder.value_holder[4].data));

  EXPECT_EQ(context->GetInput(0), reinterpret_cast<const Chain *>(&context_holder.value_holder[0]));
  EXPECT_EQ(context->GetInput(4), reinterpret_cast<const Chain *>(&context_holder.value_holder[4]));
}

TEST_F(KernelContextUT, GetAllocInputOk) {
  auto context_holder = KernelRunContextFaker().KernelIONum(5, 6).Build();
  std::vector<TestT> t_holder(11);
  for (size_t i = 0; i < 11; ++i) {
    context_holder.value_holder[i].data = &t_holder[i];
  }
  auto context = context_holder.GetContext<KernelContext>();

  EXPECT_EQ(context->GetInputPointer<TestT>(0),
            reinterpret_cast<const TestT *>((context_holder.value_holder[0].data)));
  EXPECT_EQ(context->GetInputPointer<TestT>(1),
            reinterpret_cast<const TestT *>((context_holder.value_holder[1].data)));
  EXPECT_EQ(context->GetInputPointer<TestT>(4),
            reinterpret_cast<const TestT *>((context_holder.value_holder[4].data)));

  EXPECT_EQ(context->GetInput(0), reinterpret_cast<const Chain *>(&context_holder.value_holder[0]));
  EXPECT_EQ(context->GetInput(4), reinterpret_cast<const Chain *>(&context_holder.value_holder[4]));
}

TEST_F(KernelContextUT, GetInnerOutputOk) {
  auto context_holder = KernelRunContextFaker().KernelIONum(5, 6).Build();
  auto context = context_holder.GetContext<KernelContext>();

  EXPECT_EQ(context->GetOutputPointer<int64_t>(0),
            reinterpret_cast<const int64_t *>(&(context_holder.value_holder[5].data)));
  EXPECT_EQ(context->GetOutputPointer<int64_t>(1),
            reinterpret_cast<const int64_t *>(&(context_holder.value_holder[6].data)));
  EXPECT_EQ(context->GetOutputPointer<int32_t>(2),
            reinterpret_cast<const int32_t *>(&(context_holder.value_holder[7].data)));

  EXPECT_EQ(context->GetOutput(0), reinterpret_cast<const Chain *>(&context_holder.value_holder[5]));
  EXPECT_EQ(context->GetOutput(5), reinterpret_cast<const Chain *>(&context_holder.value_holder[10]));
}

TEST_F(KernelContextUT, GetAllocOutputOk) {
  auto context_holder = KernelRunContextFaker().KernelIONum(5, 6).Build();
  std::vector<TestT> t_holder(11);
  for (size_t i = 0; i < 11; ++i) {
    context_holder.value_holder[i].data = &t_holder[i];
  }
  auto context = context_holder.GetContext<KernelContext>();

  EXPECT_EQ(context->GetOutputPointer<TestT>(0),
            reinterpret_cast<const TestT *>((context_holder.value_holder[5].data)));
  EXPECT_EQ(context->GetOutputPointer<TestT>(1),
            reinterpret_cast<const TestT *>((context_holder.value_holder[6].data)));
  EXPECT_EQ(context->GetOutputPointer<TestT>(4),
            reinterpret_cast<const TestT *>((context_holder.value_holder[9].data)));

  EXPECT_EQ(context->GetOutput(0), reinterpret_cast<const Chain *>(&context_holder.value_holder[5]));
  EXPECT_EQ(context->GetOutput(4), reinterpret_cast<const Chain *>(&context_holder.value_holder[9]));
}
}  // namespace gert