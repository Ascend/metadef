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

#include "exe_graph/lowering/tlv.h"
#include "securec.h"
#include "framework/common/debug/ge_log.h"
#include "graph/utils/math_util.h"
namespace gert {
namespace bg {
namespace {
struct TlvHead {
  TLV::Type type;
  size_t len;
};
struct TlvBuffHead {
  uint64_t version;
  uint64_t length;
};
}

std::unique_ptr<uint8_t[]> TLV::Serialize() const {
  uint64_t total_len = sizeof(TlvBuffHead);
  for (const auto &data : tlv_data_) {
    if (ge::AddOverflow(total_len, data.size(), total_len)) {
      return nullptr;
    }
  }
  auto buff = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_len]);
  if (buff == nullptr) {
    return nullptr;
  }

  auto head = reinterpret_cast<TlvBuffHead *>(buff.get());
  head->version = 1;
  head->length = total_len;
  size_t current_pos = sizeof(TlvBuffHead);
  for (const auto &data : tlv_data_) {
    auto ret = memcpy_s(buff.get() + current_pos, total_len - current_pos, data.data(), data.size());
    if (ret != EOK) {
      return nullptr;
    }
    current_pos += data.size();
  }
  return buff;
}
void TLV::AppendTlv(TLV::Type type, size_t len, const void *value) {
  if (value == nullptr) {
    GELOGE(ge::FAILED, "Failed to append to buffer, nullptr");
    return;
  }
  size_t total_len;
  if (ge::AddOverflow(len, sizeof(TlvHead), total_len)) {
    return;
  }
  std::vector<uint8_t> data(total_len);
  auto *head = reinterpret_cast<TlvHead *>(data.data());
  head->type = type;
  head->len = len;
  auto ret = memcpy_s(data.data() + sizeof(*head), data.size() - sizeof(*head), value, len);
  if (ret != EOK) {
    GELOGE(ge::FAILED, "Failed to copy length to buffer");
    return;
  }

  tlv_data_.emplace_back(std::move(data));
}
void TLV::AppendTlv(TLV::Type type, std::initializer_list<std::pair<size_t, const void *>> len_values) {
  size_t total_len = sizeof(TlvHead);
  for (const auto &len_value : len_values) {
    if (ge::AddOverflow(len_value.first, total_len, total_len)) {
      GELOGE(ge::FAILED, "Failed to Add tlv, total length overflow");
      return;
    }
  }

  std::vector<uint8_t> data(total_len);
  auto *head = reinterpret_cast<TlvHead *>(data.data());
  head->type = type;
  head->len = total_len - sizeof(TlvHead);
  size_t current_pos = sizeof(*head);
  for (const auto &len_value : len_values) {
    auto ret = memcpy_s(data.data() + current_pos, data.size() - current_pos, len_value.second, len_value.first);
    if (ret != EOK) {
      GELOGE(ge::FAILED, "Failed to copy length to buffer");
      return;
    }
    current_pos += len_value.first;
  }

  tlv_data_.emplace_back(std::move(data));
}

template<typename T>
const T *TLV::Get(size_t index, TLV::Type t, size_t *len) const {
  if (index >= tlv_data_.size()) {
    return nullptr;
  }
  auto tlv = reinterpret_cast<const TlvHead *>(tlv_data_[index].data());
  if (tlv->type != t) {
    return nullptr;
  }
  if (len != nullptr) {
    *len = tlv->len;
  }
  return reinterpret_cast<const T *>(tlv_data_[index].data() + sizeof(*tlv));
}
TLV TLV::DeserializeFrom(const void *bytes) {
  if (bytes == nullptr) {
    return {};
  }
  auto buff_head = reinterpret_cast<const TlvBuffHead *>(bytes);

  TLV tlv;
  size_t current_pos = sizeof(*buff_head);
  while (current_pos < buff_head->length) {
    auto head = reinterpret_cast<const TlvHead *>(reinterpret_cast<const uint8_t *>(bytes) + current_pos);
    if (ge::AddOverflow(sizeof(*head), current_pos, current_pos)) {
      GELOGE(ge::FAILED, "Failed to deserialize tlv, current pos overflow");
      return {};
    }
    tlv.AppendTlv(head->type, head->len, reinterpret_cast<const uint8_t *>(bytes) + current_pos);
    if (ge::AddOverflow(head->len, current_pos, current_pos)) {
      GELOGE(ge::FAILED, "Failed to deserialize tlv, current pos overflow");
      return {};
    }
  }

  return tlv;
}
size_t TLV::GetBuffLength(const void *buff) {
  if (buff == nullptr) {
    return 0;
  }
  return reinterpret_cast<const TlvBuffHead *>(buff)->length;
}
TLV &TLV::AppendBytes(const void *bytes, size_t len) {
  AppendTlv(kTypeBytes, len, bytes);
  return *this;
}
TLV &TLV::AppendInt(int64_t i) {
  AppendTlv(Type::kTypeInt64, sizeof(i), &i);
  return *this;
}
TLV &TLV::AppendString(const char *str) {
  AppendTlv(kTypeString, strlen(str) + 1, str);
  return *this;
}
const int64_t *TLV::GetInt(size_t index) const {
  return Get<int64_t>(index, kTypeInt64, nullptr);
}
const char *TLV::GetString(size_t index) const {
  return Get<char>(index, kTypeString, nullptr);
}
const void *TLV::GetBytes(size_t index, size_t &bytes_len) const {
  return Get<void>(index, kTypeBytes, &bytes_len);
}
TLV &TLV::AppendListInt(int64_t *i, size_t count) {
  size_t data_len;
  if (ge::MulOverflow(sizeof(int64_t), count, data_len)) {
    GELOGW("Failed to add list int, mul overflow found");
    return *this;
  }
  AppendTlv(kTypeListInt64, {{sizeof(count), &count}, {data_len, i}});
  return *this;
}
const int64_t *TLV::GetListInt(size_t index, size_t &count) const {
  auto head = Get<size_t>(index, kTypeListInt64, nullptr);
  count = *head;
  return reinterpret_cast<const int64_t *>(head + 1);
}
TLV &TLV::AppendFloat(const float d) {
  AppendTlv(kTypeFloat, sizeof(d), &d);
  return *this;
}
const float *TLV::GetFloat(size_t index) const {
  return Get<float>(index, kTypeFloat, nullptr);
}
size_t TLV::GetSize() const {
  return tlv_data_.size();
}
}  // namespace bg
}  // namespace gert