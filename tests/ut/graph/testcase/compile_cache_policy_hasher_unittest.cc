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
  uint8_t data1[9] = {0U,1U,2U,3U,4U,5U,6U,7U,8U};
  uint8_t data2[9] = {0U,1U,2U,3U,4U,5U,7U,9U,11U};
  uint8_t data3[9] = {0U,1U,2U,3U,4U,5U,7U,9U,11U};

  BinaryHolder holder1 = BinaryHolder();
  holder1.SharedFrom(data1, 1);

  const uint8_t *dataPtr = holder1.GetDataPtr();
  ASSERT_NE(dataPtr, nullptr);
  size_t size1 = holder1.GetDataLen();
  ASSERT_EQ(size1, 1UL);

  BinaryHolder holder2(holder1);
  BinaryHolder holder3 = BinaryHolder();
  holder3.SharedFrom(data2, 9);
  BinaryHolder holder4 = BinaryHolder();
  holder4.SharedFrom(data3, 9);
  BinaryHolder holder5 = BinaryHolder();
  holder5.SharedFrom(nullptr, 0);
  BinaryHolder holder6 = BinaryHolder();
  holder6.SharedFrom(data1, 9);

  bool b1 = (holder1 != holder5);
  ASSERT_EQ(b1, false);
  bool b2 = (holder1 != holder2);
  ASSERT_EQ(b2, false);
  bool b3 = (holder3 != holder4);
  ASSERT_EQ(b3, false);
  bool b4 = (holder3 != holder6);
  ASSERT_EQ(b4, true);
  bool b5 = (holder4 != holder5);
  ASSERT_EQ(b5, false);
}

TEST_F(UtestCompileCachePolicyHasher, GetCacheDescHashWithoutShape) {
  int64_t uid = 100UL;
  CompileCacheDesc::TensorInfoArgs tensor_info_args;
  tensor_info_args.shapes = {{2,3,4}};
  tensor_info_args.origin_shapes = {{2,3,4}};
  tensor_info_args.shape_ranges = {};
  tensor_info_args.formats = {FORMAT_ND};
  tensor_info_args.origin_formats = {FORMAT_ND};
  tensor_info_args.data_types = {DT_FLOAT};
  uint8_t *data = new uint8_t(9);
  BinaryHolder holder = BinaryHolder();
  holder.SharedFrom(data, 1);
  SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc = {holder};
  auto ccd = CompileCacheDesc(uid, tensor_info_args);
  auto seed = CompileCacheHasher::GetCacheDescHashWithoutShape(ccd);
  ASSERT_EQ(seed, 34605809643570136);

  delete data;
  data = nullptr;
}

TEST_F(UtestCompileCachePolicyHasher, GetCacheDescShapeHash) {
  int64_t uid = 100UL;
  CompileCacheDesc::TensorInfoArgs tensor_info_args;
  tensor_info_args.shapes = {{2,3,4}};
  tensor_info_args.origin_shapes = {{2,3,4}};
  tensor_info_args.shape_ranges = {};
  tensor_info_args.formats = {FORMAT_ND};
  tensor_info_args.origin_formats = {FORMAT_ND};
  tensor_info_args.data_types = {DT_FLOAT};
  uint8_t *data = new uint8_t(9);
  BinaryHolder holder = BinaryHolder();
  holder.SharedFrom(data, 1);
  SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc = {holder};
  auto ccd = CompileCacheDesc(uid, tensor_info_args);
  auto seed = CompileCacheHasher::GetCacheDescShapeHash(ccd);
  ASSERT_EQ(seed, 8487203673785339670);

  delete data;
  data = nullptr;
}
}