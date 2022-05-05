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
#include "common/checker.h"
#include <gtest/gtest.h>
namespace {
ge::graphStatus StatusFuncUseStatus(ge::graphStatus val) {
  GE_ASSERT_SUCCESS(val);
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus StatusFuncUseBool(bool val) {
  GE_ASSERT_TRUE(val);
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus StatusFuncUsePointer(void *val) {
  GE_ASSERT_NOTNULL(val);
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus StatusFuncUseUniquePtr(const std::unique_ptr<uint8_t[]> &val) {
  GE_ASSERT_NOTNULL(val);
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus StatusFuncUseSharedPtr(const std::shared_ptr<uint8_t[]> &val) {
  GE_ASSERT_NOTNULL(val);
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus StatusFuncUseEOK(int val) {
  GE_ASSERT_EOK(val);
  return ge::GRAPH_SUCCESS;
}

bool BoolFuncUseStatus(ge::graphStatus val) {
  GE_ASSERT_SUCCESS(val);
  return true;
}
bool BoolFuncUseBool(bool val) {
  GE_ASSERT_TRUE(val);
  return true;
}
bool BoolFuncUsePointer(void *val) {
  GE_ASSERT_NOTNULL(val);
  return true;
}
bool BoolFuncUseUniquePtr(const std::unique_ptr<uint8_t[]> &val) {
  GE_ASSERT_NOTNULL(val);
  return true;
}
bool BoolFuncUseSharedPtr(const std::shared_ptr<uint8_t[]> &val) {
  GE_ASSERT_NOTNULL(val);
  return true;
}
bool BoolFuncUseEOK(int val) {
  GE_ASSERT_EOK(val);
  return true;
}

int64_t g_a = 0xff;
void *PointerFuncUseStatus(ge::graphStatus val) {
  GE_ASSERT_SUCCESS(val);
  return (void*)&g_a;
}
void *PointerFuncUseBool(bool val) {
  GE_ASSERT_TRUE(val);
  return (void*)&g_a;
}
void *PointerFuncUsePointer(void *val) {
  GE_ASSERT_NOTNULL(val);
  return (void*)&g_a;
}
void *PointerFuncUseUniquePtr(const std::unique_ptr<uint8_t[]> &val) {
  GE_ASSERT_NOTNULL(val);
  return (void*)&g_a;
}
void *PointerFuncUseSharedPtr(const std::shared_ptr<uint8_t[]> &val) {
  GE_ASSERT_NOTNULL(val);
  return (void*)&g_a;
}
void *PointerFuncUseEOK(int val) {
  GE_ASSERT_EOK(val);
  return (void*)&g_a;
}
}  // namespace
class CheckerUT : public testing::Test {};
TEST_F(CheckerUT, ReturnStatusOk) {
  ASSERT_NE(StatusFuncUseStatus(ge::FAILED), ge::GRAPH_SUCCESS);
  ASSERT_NE(StatusFuncUseStatus(ge::GRAPH_FAILED), ge::GRAPH_SUCCESS);
  ASSERT_EQ(StatusFuncUseStatus(ge::GRAPH_SUCCESS), ge::GRAPH_SUCCESS);

  ASSERT_NE(StatusFuncUseBool(false), ge::GRAPH_SUCCESS);
  ASSERT_EQ(StatusFuncUseBool(true), ge::GRAPH_SUCCESS);

  int64_t a;
  ASSERT_NE(StatusFuncUsePointer(nullptr), ge::GRAPH_SUCCESS);
  ASSERT_EQ(StatusFuncUsePointer(&a), ge::GRAPH_SUCCESS);

  std::unique_ptr<uint8_t[]> b = std::unique_ptr<uint8_t[]>(new uint8_t[100]);
  ASSERT_NE(StatusFuncUseUniquePtr(nullptr), ge::GRAPH_SUCCESS);
  ASSERT_EQ(StatusFuncUseUniquePtr(b), ge::GRAPH_SUCCESS);

  auto c = std::shared_ptr<uint8_t[]>(new uint8_t[100]);
  ASSERT_NE(StatusFuncUseSharedPtr(nullptr), ge::GRAPH_SUCCESS);
  ASSERT_EQ(StatusFuncUseSharedPtr(c), ge::GRAPH_SUCCESS);

  ASSERT_NE(StatusFuncUseEOK(EINVAL), ge::GRAPH_SUCCESS);
  ASSERT_EQ(StatusFuncUseEOK(EOK), ge::GRAPH_SUCCESS);
}

TEST_F(CheckerUT, ReturnBoolOk) {
  ASSERT_NE(BoolFuncUseStatus(ge::FAILED), true);
  ASSERT_NE(BoolFuncUseStatus(ge::GRAPH_FAILED), true);
  ASSERT_EQ(BoolFuncUseStatus(ge::GRAPH_SUCCESS), true);

  ASSERT_NE(BoolFuncUseBool(false), true);
  ASSERT_EQ(BoolFuncUseBool(true), true);

  int64_t a;
  ASSERT_NE(BoolFuncUsePointer(nullptr), true);
  ASSERT_EQ(BoolFuncUsePointer(&a), true);

  std::unique_ptr<uint8_t[]> b = std::unique_ptr<uint8_t[]>(new uint8_t[100]);
  ASSERT_NE(BoolFuncUseUniquePtr(nullptr), true);
  ASSERT_EQ(BoolFuncUseUniquePtr(b), true);

  auto c = std::shared_ptr<uint8_t[]>(new uint8_t[100]);
  ASSERT_NE(BoolFuncUseSharedPtr(nullptr), true);
  ASSERT_EQ(BoolFuncUseSharedPtr(c), true);

  ASSERT_NE(BoolFuncUseEOK(EINVAL), true);
  ASSERT_EQ(BoolFuncUseEOK(EOK), true);
}

TEST_F(CheckerUT, ReturnPointerOk) {
  ASSERT_EQ(PointerFuncUseStatus(ge::FAILED), nullptr);
  ASSERT_EQ(PointerFuncUseStatus(ge::GRAPH_FAILED), nullptr);
  ASSERT_NE(PointerFuncUseStatus(ge::GRAPH_SUCCESS), nullptr);

  ASSERT_EQ(PointerFuncUseBool(false), nullptr);
  ASSERT_NE(PointerFuncUseBool(true), nullptr);

  int64_t a;
  ASSERT_EQ(PointerFuncUsePointer(nullptr), nullptr);
  ASSERT_NE(PointerFuncUsePointer(&a), nullptr);

  std::unique_ptr<uint8_t[]> b = std::unique_ptr<uint8_t[]>(new uint8_t[100]);
  ASSERT_EQ(PointerFuncUseUniquePtr(nullptr), nullptr);
  ASSERT_NE(PointerFuncUseUniquePtr(b), nullptr);

  auto c = std::shared_ptr<uint8_t[]>(new uint8_t[100]);
  ASSERT_EQ(PointerFuncUseSharedPtr(nullptr), nullptr);
  ASSERT_NE(PointerFuncUseSharedPtr(c), nullptr);

  ASSERT_EQ(PointerFuncUseEOK(EINVAL), nullptr);
  ASSERT_NE(PointerFuncUseEOK(EOK), nullptr);
}