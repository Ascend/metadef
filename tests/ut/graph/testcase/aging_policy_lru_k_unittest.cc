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
#include "graph/cache_policy/aging_policy_lru_k.h"
#include "cache_desc_stub/runtime_cache_desc.h"
#include "graph/cache_policy/cache_state.h"

namespace ge {
namespace {
CacheDescPtr CreateRuntimeCacheDesc(const std::vector<gert::Shape> &shapes) {
  auto cache_desc = std::make_shared<RuntimeCacheDesc>();
  cache_desc->SetShapes(shapes);
  return cache_desc;
}
void InsertCCStatTypeImpl(CCStatType &hash_2_cache_infos, const time_t time_stamp, const CacheItemId item_id,
                          const std::vector<gert::Shape> &shapes) {
  auto cache_desc = CreateRuntimeCacheDesc(shapes);
  auto hash_key = cache_desc->GetCacheDescHash();
  CacheInfo cache_info{time_stamp, item_id, cache_desc};
  hash_2_cache_infos[hash_key] = {cache_info};
}
void InsertCCStatType(CCStatType &hash_2_cache_infos, uint16_t depth) {
  for (uint16_t i = 0; i < depth; ++i) {
    int64_t dim_0 = i;
    gert::Shape s{dim_0, 256, 256};
    InsertCCStatTypeImpl(hash_2_cache_infos, i, i, {s});
  }
}
void DeleteAgingItem(CCStatType &hash_2_cache_infos, const CacheItemId delete_id) {
  for (auto &hash_and_cache_infos : hash_2_cache_infos) {
    auto &cache_infos = hash_and_cache_infos.second;
    for (auto iter = cache_infos.begin(); iter != cache_infos.end();) {
      if (iter->GetItemId() == delete_id) {
        iter = cache_infos.erase(iter);
      } else {
        ++iter;
      }
    }
  }
}
void DeleteAgingItems(CCStatType &hash_2_cache_infos, const std::vector<CacheItemId> &delete_ids) {
  for (const auto &delete_id : delete_ids) {
    DeleteAgingItem(hash_2_cache_infos, delete_id);
  }
}
}
class AgingPolicyLruKUT : public testing::Test {};

TEST_F(AgingPolicyLruKUT, IsReadyToAddCache_ReturnFalse_CacheDescNotAppear2Times) {
  gert::Shape s1{256, 256};
  const std::vector<gert::Shape> shapes1{s1};
  auto cache_desc = CreateRuntimeCacheDesc(shapes1);
  auto hash = cache_desc->GetCacheDescHash();

  AgingPolicyLruK ag;
  EXPECT_EQ(ag.IsReadyToAddCache(hash, cache_desc), false);
}

TEST_F(AgingPolicyLruKUT, IsReadyToAddCache_ReturnTrue_CacheDescAppear2Times) {
  gert::Shape s1{256, 256};
  const std::vector<gert::Shape> shapes1{s1};
  auto cache_desc = CreateRuntimeCacheDesc(shapes1);
  auto hash = cache_desc->GetCacheDescHash();

  AgingPolicyLruK ag;
  EXPECT_EQ(ag.IsReadyToAddCache(hash, cache_desc), false);
  EXPECT_EQ(ag.IsReadyToAddCache(hash, cache_desc), true);
}

TEST_F(AgingPolicyLruKUT, IsReadyToAddCache_ReturnFalse_CacheDescNotMatched) {
  gert::Shape s1{256, 256};
  gert::Shape s2{1, 256, 256};
  const std::vector<gert::Shape> shapes1{s1};
  const std::vector<gert::Shape> shapes2{s2};
  auto cache_desc1 = CreateRuntimeCacheDesc(shapes1);
  auto cache_desc2 = CreateRuntimeCacheDesc(shapes2);
  auto hash2 = cache_desc2->GetCacheDescHash();

  AgingPolicyLruK ag;
  EXPECT_EQ(ag.IsReadyToAddCache(hash2, cache_desc1), false);
  EXPECT_EQ(ag.IsReadyToAddCache(hash2, cache_desc2), false);
}

TEST_F(AgingPolicyLruKUT, IsReadyToAddCache_ReturnTrue_HashCollisionButCacheDescMatched) {
  gert::Shape s1{256, 256};
  gert::Shape s2{1, 256, 256};
  const std::vector<gert::Shape> shapes1{s1};
  const std::vector<gert::Shape> shapes2{s2};
  auto cache_desc1 = CreateRuntimeCacheDesc(shapes1);
  auto cache_desc2 = CreateRuntimeCacheDesc(shapes2);
  auto hash2 = cache_desc2->GetCacheDescHash();

  AgingPolicyLruK ag;
  EXPECT_EQ(ag.IsReadyToAddCache(hash2, cache_desc1), false);
  EXPECT_EQ(ag.IsReadyToAddCache(hash2, cache_desc2), false);
  EXPECT_EQ(ag.IsReadyToAddCache(hash2, cache_desc2), true);
}

TEST_F(AgingPolicyLruKUT, DoAging_NoAgingId_CacheQueueNotReachDepth) {
  CCStatType hash_2_cache_infos;
  uint16_t depth = 20;
  AgingPolicyLruK ag(depth);

  auto delete_ids = ag.DoAging(hash_2_cache_infos);
  EXPECT_EQ(delete_ids.size(), 0);

  InsertCCStatType(hash_2_cache_infos, depth);
  delete_ids = ag.DoAging(hash_2_cache_infos);
  EXPECT_EQ(delete_ids.size(), 0);
}

TEST_F(AgingPolicyLruKUT, DoAging_GetAgingIds_CacheQueueOverDepth) {
  CCStatType hash_2_cache_infos;
  AgingPolicyLruK ag(20);
  auto delete_ids = ag.DoAging(hash_2_cache_infos);
  EXPECT_EQ(delete_ids.size(), 0);

  uint16_t depth = 21;
  InsertCCStatType(hash_2_cache_infos, depth);
  delete_ids = ag.DoAging(hash_2_cache_infos);
  EXPECT_EQ(delete_ids.size(), 1);
  EXPECT_EQ(delete_ids[0], 0);

  depth = 25;
  InsertCCStatType(hash_2_cache_infos, depth);
  delete_ids = ag.DoAging(hash_2_cache_infos);
  EXPECT_EQ(delete_ids.size(), 1);
  EXPECT_EQ(delete_ids[0], 0);
}
TEST_F(AgingPolicyLruKUT, DoAging_Aging5Times_CacheQueueDepthIs25) {
  CCStatType hash_2_cache_infos;
  AgingPolicyLruK ag(20);
  auto delete_ids = ag.DoAging(hash_2_cache_infos);
  EXPECT_EQ(delete_ids.size(), 0);

  int16_t depth = 25;
  InsertCCStatType(hash_2_cache_infos, depth);

  for (size_t i = 0U; i < depth; ++i) {
    delete_ids = ag.DoAging(hash_2_cache_infos);
    if (i < 5U) {
      EXPECT_EQ(delete_ids.size(), 1);
      EXPECT_EQ(delete_ids[0], i);
    } else {
      EXPECT_EQ(delete_ids.size(), 0);
    }
    DeleteAgingItems(hash_2_cache_infos, delete_ids);
  }
}
}  // namespace ge