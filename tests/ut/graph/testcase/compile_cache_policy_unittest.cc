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
#include "graph/compile_cache_policy/compile_cache_policy.h"
#include "graph/compile_cache_policy/compile_cache_state.h"

namespace ge {
class UtestCompileCachePolicy : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestCompileCachePolicy, CreateCCPSuccess_1) {
  auto ccp = ge::CompileCachePolicy::Create(ge::MatchPolicyType::MATCH_POLICY_EXACT_ONLY, ge::AgingPolicyType::AGING_POLICY_LRU);
  ASSERT_NE(ccp, nullptr);
}

TEST_F(UtestCompileCachePolicy, CreateCCPSuccess_2) {
  auto mp_ptr = PolicyManager::GetInstance().GetMatchPolicy(ge::MatchPolicyType::MATCH_POLICY_EXACT_ONLY);
  auto ap_ptr = PolicyManager::GetInstance().GetAgingPolicy(ge::AgingPolicyType::AGING_POLICY_LRU);
  auto ccp = ge::CompileCachePolicy::Create(mp_ptr, ap_ptr);
  ASSERT_NE(ccp, nullptr);
}

TEST_F(UtestCompileCachePolicy, CreateCCPFailed_1) {
  auto mp_ptr = PolicyManager::GetInstance().GetMatchPolicy(ge::MatchPolicyType::MATCH_POLICY_EXACT_ONLY);
  auto ap_ptr = nullptr;
  auto ccp = ge::CompileCachePolicy::Create(mp_ptr, ap_ptr);
  ASSERT_EQ(ccp, nullptr);
}

TEST_F(UtestCompileCachePolicy, CreateCCPFailed_2) {
  auto mp_ptr = nullptr;
  auto ap_ptr = nullptr;
  auto ccp = ge::CompileCachePolicy::Create(mp_ptr, ap_ptr);
  ASSERT_EQ(ccp, nullptr);
}

TEST_F(UtestCompileCachePolicy, AddCacheSuccess_1) {
  int64_t uid = 100UL;
  CompileCacheDesc::TensorInfoArgs tensor_info_args;
  tensor_info_args.shapes = {{2,3,4}};
  tensor_info_args.origin_shapes = {{2,3,4}};
  tensor_info_args.shape_ranges = {};
  tensor_info_args.formats = {FORMAT_ND};
  tensor_info_args.origin_formats = {FORMAT_ND};
  tensor_info_args.data_types = {DT_FLOAT};
  uint8_t value = 9;
  uint8_t *data = &value;
  BinaryHolder holder = BinaryHolder();
  holder.SharedFrom(data, 1);
  SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc = {holder};
  auto ccd = CompileCacheDesc(uid, tensor_info_args);

  auto ccp = ge::CompileCachePolicy::Create(ge::MatchPolicyType::MATCH_POLICY_EXACT_ONLY, ge::AgingPolicyType::AGING_POLICY_LRU);
  CacheItem cache_item = ccp->AddCache(ccd);
  ASSERT_NE(cache_item, -1);
}

TEST_F(UtestCompileCachePolicy, AddCacheSuccess_2) {
  int64_t uid = 100UL;
  CompileCacheDesc::TensorInfoArgs tensor_info_args;
  tensor_info_args.shapes = {{2,3,4}};
  tensor_info_args.origin_shapes = {{2,3,4}};
  tensor_info_args.shape_ranges = {};
  tensor_info_args.formats = {FORMAT_ND};
  tensor_info_args.origin_formats = {FORMAT_ND};
  tensor_info_args.data_types = {DT_FLOAT};
  uint8_t value = 9;
  uint8_t *data = &value;
  BinaryHolder holder = BinaryHolder();
  holder.SharedFrom(data, 1);
  SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc = {holder};
  CompileCacheDesc ccd1 = CompileCacheDesc(uid, tensor_info_args);

  auto ccp = ge::CompileCachePolicy::Create(ge::MatchPolicyType::MATCH_POLICY_EXACT_ONLY, ge::AgingPolicyType::AGING_POLICY_LRU);
  CacheItem cache_item = ccp->AddCache(ccd1);
  ASSERT_NE(cache_item, -1);

  uid++;
  CompileCacheDesc ccd2 = CompileCacheDesc(uid, tensor_info_args);
  cache_item = ccp->AddCache(ccd2);
  ASSERT_NE(cache_item, -1);

  bool is_same = CompileCacheDesc::IsSameCompileDesc(ccd1, ccd2);
  ASSERT_NE(is_same, true);
}

TEST_F(UtestCompileCachePolicy, CacheHashKey_1) {
  CacheHashKey seed = 234;
  SmallVector<CacheHashKey, 3> values = {{2,3,4}};

  CacheHashKey result = HashUtils::HashCombine(seed, values);

  // Hash result of HashUtils::HashCombine
  // Same with another UT GetCacheDescShapeHash
  ASSERT_EQ(result, 11093890198896ULL);
}

TEST_F(UtestCompileCachePolicy, FindCacheSuccess_1) {
  int64_t uid = 100UL;
  CompileCacheDesc::TensorInfoArgs tensor_info_args;
  tensor_info_args.shapes = {{2,3,4}};
  tensor_info_args.origin_shapes = {{2,3,4}};
  tensor_info_args.shape_ranges = {};
  tensor_info_args.formats = {FORMAT_ND};
  tensor_info_args.origin_formats = {FORMAT_ND};
  tensor_info_args.data_types = {DT_FLOAT};
  uint8_t value = 9;
  uint8_t *data = &value;
  BinaryHolder holder = BinaryHolder();
  holder.SharedFrom(data, 1);
  SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc = {holder};
  auto ccd = CompileCacheDesc(uid, tensor_info_args);
  auto ccp = ge::CompileCachePolicy::Create(ge::MatchPolicyType::MATCH_POLICY_EXACT_ONLY, ge::AgingPolicyType::AGING_POLICY_LRU);
  CacheItem cache_item = ccp->AddCache(ccd);

  CacheItem cache_item_find = ccp->FindCache(ccd);
  ASSERT_EQ(cache_item, cache_item_find);
}

TEST_F(UtestCompileCachePolicy, DeleteCacheSuccess_1) {
  int64_t uid = 100UL;
  CompileCacheDesc::TensorInfoArgs tensor_info_args;
  tensor_info_args.shapes = {{2,3,4}};
  tensor_info_args.origin_shapes = {{2,3,4}};
  tensor_info_args.shape_ranges = {};
  tensor_info_args.formats = {FORMAT_ND};
  tensor_info_args.origin_formats = {FORMAT_ND};
  tensor_info_args.data_types = {DT_FLOAT};
  uint8_t value = 9;
  uint8_t *data = &value;
  BinaryHolder holder = BinaryHolder();
  holder.SharedFrom(data, 1);
  SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc = {holder};
  auto ccd = CompileCacheDesc(uid, tensor_info_args);

  auto ccp = ge::CompileCachePolicy::Create(ge::MatchPolicyType::MATCH_POLICY_EXACT_ONLY, ge::AgingPolicyType::AGING_POLICY_LRU);
  CacheItem cache_item = ccp->AddCache(ccd);

  auto lamb = [&](CacheInfo info) -> bool {
    if (info.GetItem() > 0) {
      return true;
    } else {
      return false;
    }
  };
  std::vector<CacheItem> del_item = ccp->DeleteCache(lamb);
  ASSERT_EQ(cache_item, del_item[0]);
}

TEST_F(UtestCompileCachePolicy, AgingCacheSuccess_1) {
  int64_t uid = 100UL;
  CompileCacheDesc::TensorInfoArgs tensor_info_args;
  tensor_info_args.shapes = {{2,3,4}};
  tensor_info_args.origin_shapes = {{2,3,4}};
  tensor_info_args.shape_ranges = {};
  tensor_info_args.formats = {FORMAT_ND};
  tensor_info_args.origin_formats = {FORMAT_ND};
  tensor_info_args.data_types = {DT_FLOAT};
  uint8_t value = 9;
  uint8_t *data = &value;
  BinaryHolder holder = BinaryHolder();
  holder.SharedFrom(data, 1);
  SmallVector<BinaryHolder, kDefaultMaxInputNum> other_desc = {holder};
  auto ccd = CompileCacheDesc(uid, tensor_info_args);

  auto ccp = ge::CompileCachePolicy::Create(ge::MatchPolicyType::MATCH_POLICY_EXACT_ONLY, ge::AgingPolicyType::AGING_POLICY_LRU);
  CacheItem cache_item = ccp->AddCache(ccd);

  std::vector<CacheItem> del_item = ccp->DoAging();
  ASSERT_EQ(cache_item, del_item[0]);
}

}