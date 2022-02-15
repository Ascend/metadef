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
#define protected public
#define private public
#include "graph/compile_cache_policy/compile_cache_hasher.h"
namespace ge {
class UtestCompileCachePolicyHasher : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestCompileCachePolicyHasher, TestBinaryHolderOnly) {
  uint8_t data1[8] = {0U,1U,2U,3U,4U,5U,6U,7U};
  uint8_t data2[8] = {0U,1U,2U,3U,4U,5U,6U,7U};
  uint8_t data3[9] = {0U,1U,2U,3U,4U,5U,7U,9U,11U};
  uint8_t data4[8] = {1U,1U,2U,3U,4U,5U,6U,7U};

  BinaryHolder holder1 = BinaryHolder();
  holder1.SharedFrom(data1, sizeof(data1));

  const uint8_t *dataPtr = holder1.GetDataPtr();
  ASSERT_NE(dataPtr, nullptr);
  size_t size1 = holder1.GetDataLen();
  ASSERT_EQ(size1, sizeof(data1));

  // same
  BinaryHolder holder2(holder1);
  ASSERT_EQ((holder1 != holder2), false);
  BinaryHolder holder3 = BinaryHolder();
  holder3 = holder1;
  ASSERT_EQ((holder1 != holder3), false);
  BinaryHolder holder4 = BinaryHolder();
  holder4.SharedFrom(data2, sizeof(data2));
  ASSERT_EQ((holder1 != holder4), false);

  // not equal
  BinaryHolder holder5 = BinaryHolder();
  holder5.SharedFrom(nullptr, sizeof(data1));
  ASSERT_EQ((holder1 != holder5), true);
  BinaryHolder holder6 = BinaryHolder();
  holder6.SharedFrom(data3, sizeof(data3));
  ASSERT_EQ((holder1 != holder6), true);
  BinaryHolder holder7 = BinaryHolder();
  holder7.SharedFrom(data4, sizeof(data4));
  ASSERT_EQ((holder1 != holder7), true);

  // same nullptr
  BinaryHolder holder8 = BinaryHolder();
  BinaryHolder holder9 = BinaryHolder();
  ASSERT_EQ((holder8 != holder9), false);
}

TEST_F(UtestCompileCachePolicyHasher, TestSameacheFail) {
  CompileCacheDesc cache_desc1;
  CompileCacheDesc cache_desc2;
  cache_desc1.SetOpType("test1");
  cache_desc2.SetOpType("test2");
  bool is_same = CompileCacheDesc::IsSameCompileDesc(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);
  is_same = CompileCacheDesc::IsMatchedCompileDesc(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);
  cache_desc2.SetOpType("test1");

  uint8_t val1 = 0;
  BinaryHolder holder1 = BinaryHolder();
  holder1.SharedFrom(&val1, sizeof(val1));
  cache_desc1.other_desc_.emplace_back(holder1);
  is_same = CompileCacheDesc::CheckWithoutTensorInfo(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);

  uint8_t val2 = 2;
  BinaryHolder holder2 = BinaryHolder();
  holder2.SharedFrom(&val2, sizeof(val2));
  cache_desc2.other_desc_.emplace_back(holder2);
  is_same = CompileCacheDesc::CheckWithoutTensorInfo(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);
  cache_desc2.other_desc_.clear();
  cache_desc2.other_desc_.emplace_back(holder1);

  TensorInfoArgs tensor_info1(ge::FORMAT_ND, ge::FORMAT_ND, ge::DT_FLOAT16);
  cache_desc1.AddTensorInfo(tensor_info1);
  TensorInfoArgs tensor_info2(ge::FORMAT_NCHW, ge::FORMAT_ND, ge::DT_FLOAT16);
  cache_desc2.AddTensorInfo(tensor_info2);
  is_same = CompileCacheDesc::IsMatchedCompileDesc(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);
  cache_desc2.tensor_info_args_vec_[0].format_ = FORMAT_ND;

  cache_desc1.tensor_info_args_vec_[0].shape_.emplace_back(-1);
  std::pair<int64_t, int64_t> ranges{10,100};
  cache_desc1.tensor_info_args_vec_[0].shape_range_.emplace_back(ranges);
  cache_desc2.tensor_info_args_vec_[0].shape_.emplace_back(5);
  is_same = CompileCacheDesc::IsMatchedCompileDesc(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);
  cache_desc2.tensor_info_args_vec_[0].shape_[0] = 101;
  is_same = CompileCacheDesc::IsMatchedCompileDesc(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);

  cache_desc1.tensor_info_args_vec_[0].shape_[0] = 1;
  is_same = CompileCacheDesc::IsMatchedCompileDesc(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);

  cache_desc1.tensor_info_args_vec_[0].shape_[0] = -2;
  is_same = CompileCacheDesc::IsMatchedCompileDesc(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, true);

  is_same = CompileCacheDesc::IsSameCompileDesc(cache_desc1, cache_desc2);
  ASSERT_EQ(is_same, false);
}
}