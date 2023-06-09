/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#ifndef __INC_REGISTER_TUNING_BANK_KEY_REGISTRY_HEADER__
#define __INC_REGISTER_TUNING_BANK_KEY_REGISTRY_HEADER__
#include <type_traits>
#include <nlohmann/json.hpp>
#include <string>
#include "graph/ascend_string.h"
#include "external/register/register_types.h"
#include "exe_graph/runtime/tiling_context.h"
#include "register/tuning_tiling_reflection_utils.h"

#define REGISTER_OP_BANK_KEY_CONVERT_FUN(op, opfunc)                                                                   \
  REGISTER_OP_BANK_KEY_CONVERT_FUN_UNIQ_HELPER(op, (opfunc), __COUNTER__)

#define REGISTER_OP_BANK_KEY_CONVERT_FUN_UNIQ_HELPER(optype, opfunc, counter)                                          \
  REGISTER_OP_BANK_KEY_UNIQ(optype, (opfunc), counter)

#define REGISTER_OP_BANK_KEY_UNIQ(optype, opfunc, counter)                                                             \
  static tuningtiling::OpBankKeyFuncRegistry g_##optype##BankKeyRegistryInterf##counter(#optype, (opfunc))

namespace tuningtiling {
class OpBankKeyDef {
public:
  OpBankKeyDef() = default;
  virtual ~OpBankKeyDef() {
    if (bank_key_ != nullptr) {
      delete[] bank_key_;
      bank_key_ = nullptr;
    }
  }

  virtual void FromJson(const nlohmann::json &j) = 0;
  virtual void ToJson(nlohmann::json &j) = 0;
  virtual uint8_t *GetRawBankKey() = 0;
  virtual size_t GetDataSize() const = 0;

protected:
  uint8_t *bank_key_ = nullptr;
};

#define BEGIN_OP_BANK_KEY_DEF(class_name)                                                                              \
  class class_name : public OpBankKeyDef {                                                                             \
  public:                                                                                                              \
    virtual void FromJson(const nlohmann::json &j) {                                                                   \
      FromJsonImpl(*this, "", j);                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    virtual void ToJson(nlohmann::json &j) {                                                                           \
      DumpObj(*this, "", j);                                                                                           \
    }                                                                                                                  \
                                                                                                                       \
    uint8_t *GetRawBankKey() {                                                                                         \
      if (bank_key_ != nullptr) {                                                                                      \
        return bank_key_;                                                                                              \
      }                                                                                                                \
      size_t data_size = 0;                                                                                            \
      GetStructSize(*this, "", data_size);                                                                             \
      if (data_size <= 0) {                                                                                            \
        return nullptr;                                                                                                \
      }                                                                                                                \
      bank_key_ = new uint8_t[data_size];                                                                              \
      size_t offset = 0U;                                                                                              \
      SaveToBuffer(*this, "", bank_key_, data_size, offset);                                                           \
      return bank_key_;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    size_t GetDataSize() const {                                                                                       \
      size_t data_size = 0;                                                                                            \
      GetStructSize(*this, "", data_size);                                                                             \
      return data_size;                                                                                                \
    }

#define OP_BANK_KEY_FIELD_DEF(data_type, field_name)                                                                   \
  public:                                                                                                              \
    data_type field_name;

#define END_OP_BANK_KEY_DEF                                                                                            \
  }                                                                                                                    \
  ;

template<typename T>
struct GetStructSizeFunctor;

template<
    typename T, typename S,
    detail::enable_if_t<std::is_class<detail::decay_t<T>>::value && (!is_containable<detail::decay_t<T>>())>* = nullptr>
void GetStructSize(const T &obj, const std::string &field_name, S &total_size) {
  (void) field_name;
  ForEachField(obj, GetStructSizeFunctor<S>(total_size));
}

template<typename T, typename S, detail::enable_if_t<!std::is_class<detail::decay_t<T>>::value>* = nullptr>
void GetStructSize(const T &obj, const std::string &field_name, S &total_size) {
  (void) field_name;
  (void) obj;
  total_size += sizeof(detail::decay_t<T>);
}

template<
    typename T, typename S,
    detail::enable_if_t<std::is_class<detail::decay_t<T>>::value && (is_containable<detail::decay_t<T>>())>* = nullptr>
void GetStructSize(const T &obj, const std::string &field_name, S &total_size) {
  (void) field_name;
  for (const auto &t : obj) {
    GetStructSize(t, field_name, total_size);
  }
}

template<typename S>
struct GetStructSizeFunctor {
  explicit GetStructSizeFunctor(S &data_len) : total_size(data_len) {}
  template<typename Name, typename Field>
  void operator()(Name &&name, Field &&field) const {
    GetStructSize(field, name, total_size);
  }
  S &total_size;
};

template<typename T>
struct SaveToBufferFunctor;

template<
    typename T, typename D,
    detail::enable_if_t<std::is_class<detail::decay_t<T>>::value && !is_containable<detail::decay_t<T>>()>* = nullptr>
void SaveToBuffer(const T &obj, const std::string &field_name, D *buff, size_t buf_size, size_t &offset) {
  (void) field_name;
  ForEachField(obj, SaveToBufferFunctor<D>(offset, buff, buf_size));
}

template<typename T, typename D, detail::enable_if_t<!std::is_class<detail::decay_t<T>>::value>* = nullptr>
void SaveToBuffer(const T &obj, const std::string &field_name, D *buff, size_t buf_size, size_t &offset) {
  if (field_name.empty() || (buff == nullptr)) {
    return;
  }
  const size_t data_size = sizeof(T);
  if (offset + data_size > buf_size) {
    return;
  }
  *((detail::decay_t<T> *)(buff + offset)) = obj;
  offset += data_size;
}

template<
    typename T, typename D,
    detail::enable_if_t<std::is_class<detail::decay_t<T>>::value && is_containable<detail::decay_t<T>>()>* = nullptr>
void SaveToBuffer(const T &obj, const std::string &field_name, D *buff, size_t buf_size, size_t &offset) {
  if (field_name.empty()) {
    return;
  }
  size_t data_size = 0U;
  GetStructSize(obj, field_name, data_size);
  if (offset + data_size > buf_size) {
    return;
  }
  for (const auto &v : obj) {
    SaveToBuffer(v, field_name, buff, buf_size, offset);
  }
}

template<typename T>
struct SaveToBufferFunctor {
  SaveToBufferFunctor(size_t &i, T *data, size_t data_len) : offset(i), buff(data), buf_size(data_len) {}
  template<typename Name, typename Field>
  void operator()(Name &&name, Field &&field) const {
    SaveToBuffer(field, name, buff, buf_size, offset);
  }
  size_t &offset;
  T *buff;
  size_t buf_size;
};

using OpBankKeyConstructor = std::shared_ptr<OpBankKeyDef> (*)();

class FMK_FUNC_HOST_VISIBILITY OpBankKeyClassFactory {
public:
  static std::map<ge::AscendString, OpBankKeyConstructor> &RegisterInfo();
  static void RegisterOpBankKey(const ge::AscendString &optype, OpBankKeyConstructor const constructor);
  static std::shared_ptr<OpBankKeyDef> CreateBankKeyInstance(const ge::AscendString &optype);
};

#define REGISTER_OP_BANK_KEY_CLASS(optype, class_name)                                                                 \
  class optype##class_name##Helper {                                                                                   \
  public:                                                                                                              \
    optype##class_name##Helper() {                                                                                     \
      OpBankKeyClassFactory::RegisterOpBankKey(#optype, optype##class_name##Helper::CreateBankKeyInstance);            \
    }                                                                                                                  \
    static std::shared_ptr<OpBankKeyDef> CreateBankKeyInstance() {                                                     \
      return std::make_shared<class_name>();                                                                           \
    }                                                                                                                  \
  };                                                                                                                   \
  optype##class_name##Helper g_op_bank_key_##optype##class_name##Helper;

using OpBankKeyDefPtr = std::shared_ptr<OpBankKeyDef>;
using OpBankKeyFun = std::function<bool(const gert::TilingContext *, OpBankKeyDefPtr &)>;

class FMK_FUNC_HOST_VISIBILITY OpBankKeyFuncRegistry {
public:
  OpBankKeyFuncRegistry(const ge::AscendString &optype, OpBankKeyFun bank_key_func);
  ~OpBankKeyFuncRegistry() = default;
  static std::unordered_map<ge::AscendString, OpBankKeyFun> &RegisteredOpFuncInfo();
};
}  // namespace tuningtiling
#endif
