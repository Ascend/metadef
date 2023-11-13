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
#include "fast_graph/quick_list.h"

namespace ge {
class UtestQuickList : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestQuickList, TestPushBack) {
  QuickList<int> test2;
  int32_t test_loop = 100;
  ListElement<int32_t> *list[test_loop] = {};
  for (int32_t i = 0; i < test_loop; i++) {
    list[i] = new ListElement<int32_t>;
    list[i]->data = 1;
    test2.push_back(list[i], ListMode::kWorkMode);
  }
  ASSERT_EQ(test2.size(), test_loop);
  test2.clear();
  for (int32_t i = 0; i < test_loop; i++) {
    if (list[i] != nullptr) {
      delete list[i];
    }
  }
}

TEST_F(UtestQuickList, TestInsert) {
  int32_t test_loop = 100;
  ListElement<int32_t> *list[test_loop] = {};
  for (int32_t i = 0; i < test_loop; i++) {
    list[i] = new ListElement<int32_t>;
    list[i]->data = 1;
  }

  // test insert begin()
  QuickList<int32_t> test3;
  for (int32_t i = 0; i < test_loop; i++) {
    test3.insert(test3.begin(), list[i], ListMode::kWorkMode);
  }
  ASSERT_EQ(test3.size(), test_loop);
  test3.clear();

  // test insert end()
  QuickList<int32_t> test5;
  int insert_begin_num = 3;
  for (int32_t i = 0; i < insert_begin_num; i++) {
    test5.insert(test5.begin(), list[i], ListMode::kWorkMode);
  }
  for (int32_t i = insert_begin_num; i < test_loop; i++) {
    test5.insert(test5.end(), list[i], ListMode::kWorkMode);
  }
  ASSERT_EQ(test5.size(), test_loop);
  auto pos = test5.end();
  --pos;
  // check the end node is correct.
  ASSERT_EQ(*(pos), list[test_loop - 1]);
  test5.clear();

  QuickList<int32_t> test6;
  int inner_num = 3;
  for (int32_t i = 0; i < inner_num; i++) {
    test6.insert(test6.begin(), list[i], ListMode::kWorkMode);
  }
  auto inner_pos = test6.begin();
  ++inner_pos;
  for (int32_t i = inner_num; i < test_loop; i++) {
    test6.insert(inner_pos, list[i], ListMode::kWorkMode);
  }
  ASSERT_EQ(test6.size(), test_loop);
  // check the end node is correct.
  auto end_pos = test6.end();
  --end_pos;
  ASSERT_EQ(*(end_pos), list[0]);
  test6.clear();

  for (int32_t i = 0; i < test_loop; i++) {
    if (list[i] != nullptr) {
      delete list[i];
    }
  }
}

TEST_F(UtestQuickList, TestPushFront) {
  QuickList<int> test2;
  int32_t test_loop = 100;
  ListElement<int32_t> *list[test_loop] = {};
  for (int32_t i = 0; i < test_loop; i++) {
    list[i] = new ListElement<int32_t>;
    list[i]->data = 1;
    test2.push_front(list[i], ListMode::kWorkMode);
  }
  ASSERT_EQ(test2.size(), test_loop);
  auto begin_pos = test2.begin();
  ASSERT_EQ(*begin_pos, list[test_loop - 1]);
  auto end_pos = test2.end();
  --end_pos;
  ASSERT_EQ(*end_pos, list[0]);
  test2.clear();

  for (int32_t i = 0; i < test_loop; i++) {
    if (list[i] != nullptr) {
      delete list[i];
    }
  }
}

TEST_F(UtestQuickList, TestMove) {
  int32_t test_loop = 100;
  int dst_relative_pose = 2;
  {
    QuickList<int> test1;
    ListElement<int32_t> *list[test_loop] = {};
    for (int32_t i = 0; i < test_loop; i++) {
      list[i] = new ListElement<int32_t>;
      list[i]->data = 1;
      test1.push_back(list[i], ListMode::kWorkMode);
    }
    test1.move(list[test_loop - 1], list[test_loop - dst_relative_pose], true);
    auto pos = test1.end();
    --pos;
    // check the end node is correct.
    ASSERT_EQ(*(pos), list[test_loop - dst_relative_pose]);
    test1.clear();
    for (int32_t i = 0; i < test_loop; i++) {
      if (list[i] != nullptr) {
        delete list[i];
      }
    }
  }

  {
    QuickList<int> test1;
    ListElement<int32_t> *list[test_loop] = {};
    for (int32_t i = 0; i < test_loop; i++) {
      list[i] = new ListElement<int32_t>;
      list[i]->data = 1;
      test1.push_back(list[i], ListMode::kWorkMode);
    }
    test1.move(list[test_loop - 1], list[test_loop - dst_relative_pose], false);
    auto pos = test1.end();
    --pos;
    // check the end node is correct.
    ASSERT_EQ(*(pos), list[test_loop - 1]);
    test1.clear();
    for (int32_t i = 0; i < test_loop; i++) {
      if (list[i] != nullptr) {
        delete list[i];
      }
    }
  }

  {
    QuickList<int> test1;
    ListElement<int32_t> *list[test_loop] = {};
    for (int32_t i = 0; i < test_loop; i++) {
      list[i] = new ListElement<int32_t>;
      list[i]->data = 1;
      test1.push_back(list[i], ListMode::kWorkMode);
    }
    test1.move(list[0], list[test_loop - 1], false);
    auto pos = test1.end();
    --pos;
    // check the end node is correct.
    ASSERT_EQ(*(pos), list[0]);
    test1.clear();
    for (int32_t i = 0; i < test_loop; i++) {
      if (list[i] != nullptr) {
        delete list[i];
      }
    }
  }

  {
    QuickList<int> test1;
    ListElement<int32_t> *list[test_loop] = {};
    for (int32_t i = 0; i < test_loop; i++) {
      list[i] = new ListElement<int32_t>;
      list[i]->data = 1;
      test1.push_back(list[i], ListMode::kWorkMode);
    }
    test1.move(list[0], list[test_loop - 1], true);
    auto pos = test1.end();
    --pos;
    // check the end node is correct.
    ASSERT_EQ(*(pos), list[test_loop - 1]);
    test1.clear();
    for (int32_t i = 0; i < test_loop; i++) {
      if (list[i] != nullptr) {
        delete list[i];
      }
    }
  }
}

TEST_F(UtestQuickList, TestErase) {
  QuickList<int> test2;
  int real_loop = 100;
  int32_t test_loop = real_loop;
  ListElement<int32_t> *list[test_loop] = {};
  for (int32_t i = 0; i < real_loop; i++) {
    list[i] = new ListElement<int32_t>;
    list[i]->data = 1;
    test2.push_back(list[i], ListMode::kWorkMode);
  }
  ASSERT_EQ(test2.size(), test_loop);
  test2.erase(list[test_loop - 1]);
  test_loop--;

  auto iter = test2.begin();
  test2.erase(iter);
  test_loop--;
  ASSERT_EQ(test2.size(), test_loop);

  ASSERT_EQ(*(test2.begin()), list[1]);
  auto end_iter = test2.end();
  --end_iter;
  ASSERT_EQ(*(end_iter), list[test_loop]);
  test2.clear();
  for (int32_t i = 0; i < real_loop; i++) {
    if (list[i] != nullptr) {
      delete list[i];
    }
  }
}

}  // namespace ge