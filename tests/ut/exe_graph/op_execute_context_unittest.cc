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
#include "exe_graph/runtime/op_execute_context.h"
#include "exe_graph/runtime/gert_mem_allocator.h"
#include "graph/ge_error_codes.h"
#include <gtest/gtest.h>
#include "faker/kernel_run_context_faker.h"
#include "faker/allocator_faker.h"
#include "exe_graph/runtime/storage_shape.h"
#include "register/kernel_registry.h"
namespace gert {
class OpExecuteContextUT : public testing::Test {};
TEST_F(OpExecuteContextUT, MallocFreeWorkSpaceOk) {
  std::vector<ge::MemBlock *> mem_block;
  ge::Allocator *allocator = new AllocatorFaker();
  memory::GertMemAllocator gert_allocator;
  gert_allocator.allocator = allocator;
  gert_allocator.placement = kOnDeviceHbm;
  auto context_holder = KernelRunContextFaker()
                            .IrInputNum(2)
                            .IrInstanceNum({1, 1})
                            .KernelIONum(7, 1)
                            .NodeIoNum(2, 1)
                            .Inputs({nullptr, nullptr, nullptr, &gert_allocator, nullptr, nullptr, nullptr})
                            .Outputs({&mem_block})
                            .Build();
  auto context = context_holder.GetContext<OpExecuteContext>();
  ASSERT_NE(context, nullptr);
  ASSERT_NE(context->MallocWorkspace(1024), nullptr);
  context->FreeWorkspace();
  delete allocator;
}
}