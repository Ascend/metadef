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

#ifndef AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_DEV_MEM_VALUE_HOLDER_H_
#define AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_DEV_MEM_VALUE_HOLDER_H_

#include "value_holder.h"

namespace gert {
namespace bg {
constexpr int64_t kMainStream = 0;
class DevMemValueHolder;
using DevMemValueHolderPtr = std::shared_ptr<DevMemValueHolder>;
/**
 * Value holder with stream
 *
 */
class DevMemValueHolder : public ValueHolder {
 public:
  explicit DevMemValueHolder(const int64_t logic_stream_id) : logic_stream_id_(logic_stream_id){};

  DevMemValueHolder() = delete;
  DevMemValueHolder(const DevMemValueHolder &other) = delete;
  DevMemValueHolder &operator=(const DevMemValueHolder &other) = delete;
  ~DevMemValueHolder() = default;

  ValueHolderPtr CreateMateFromNode(NodeHolderPtr node, int32_t index, ValueHolderType type) override;

  static DevMemValueHolderPtr CreateSingleDataOutput(const ge::char_t *node_type,
                                                     const std::vector<ValueHolderPtr> &inputs,
                                                     int64_t logic_stream_id);
  static std::vector<DevMemValueHolderPtr> CreateDataOutput(const char *node_type,
                                                            const std::vector<ValueHolderPtr> &inputs,
                                                            size_t out_count, int64_t logic_stream_id);
  static DevMemValueHolderPtr CreateConst(const void *data, size_t size, int64_t logic_stream_id,
                                          bool is_string = false);

  static DevMemValueHolderPtr CreateError(int64_t logic_stream_id, const char *fmt, va_list arg);
  static DevMemValueHolderPtr CreateError(int64_t logic_stream_id, const char *fmt, ...);

  int64_t GetLogicStream() const;

 private:
  int64_t logic_stream_id_{kMainStream};
};
}  // namespace bg
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_DEV_MEM_VALUE_HOLDER_H_