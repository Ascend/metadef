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

#include "exe_graph/lowering/value_holder_generator.h"
namespace gert {
namespace bg {
std::vector<ValueHolderPtr> CreateErrorValueHolders(size_t count, const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  auto holder = ValueHolder::CreateError(fmt, arg);
  va_end(arg);
  return {count, holder};
}
}  // namespace bg
}  // namespace gert