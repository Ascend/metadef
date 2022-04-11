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

#ifndef AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_TLV_H_
#define AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_TLV_H_
#include <memory>
#include <vector>
namespace gert {
namespace bg {
class TLV {
 public:
  enum Type {
    kTypeBytes,
    kTypeInt64,
    kTypeListInt64,
    kTypeString,
    kTypeFloat,

    kTypeEnd
  };

  std::unique_ptr<uint8_t[]> Serialize() const;
  static TLV DeserializeFrom(const void *bytes);
  static size_t GetBuffLength(const void *buff);

  size_t GetSize() const;

  TLV &AppendBytes(const void *bytes, size_t len);
  const void *GetBytes(size_t index, size_t &bytes_len) const;

  TLV &AppendInt(int64_t i);
  const int64_t *GetInt(size_t index) const;

  TLV &AppendListInt(int64_t *i, size_t count);
  const int64_t *GetListInt(size_t index, size_t &count) const;

  TLV &AppendString(const char *str);
  const char *GetString(size_t index) const;

  TLV &AppendFloat(const float d);
  const float *GetFloat(size_t index) const;

 private:
  void AppendTlv(Type type, size_t len, const void *value);
  void AppendTlv(Type type, std::initializer_list<std::pair<size_t, const void *>> values);

  template<typename T>
  const T *Get(size_t index, TLV::Type t, size_t *len) const;

  private:
  std::vector<std::vector<uint8_t>> tlv_data_;
};
}
}

#endif  //AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_TLV_H_
