/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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

#ifndef D_INC_GRAPH_LIST_NODE_H
#define D_INC_GRAPH_LIST_NODE_H

namespace ge {
enum class ListMode { kWorkMode = 0, kFreeMode };

template <class T>
class QuickList;

template <class T>
struct ListElement {
  ListElement<T> *next;
  ListElement<T> *prev;
  QuickList<T> *owner;
  ListMode mode;
  T data;
  ListElement(const T &x) : data(x), next(nullptr), prev(nullptr), owner(nullptr), mode(ListMode::kFreeMode) {}
  bool operator==(const ListElement<T> &r_ListElement) const {
    return data == r_ListElement.data;
  }
  ListElement() : next(nullptr), prev(nullptr), owner(nullptr), mode(ListMode::kFreeMode) {}
};
}  // namespace ge
#endif