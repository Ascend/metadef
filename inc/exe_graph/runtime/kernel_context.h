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

#ifndef AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_KERNEL_CONTEXT_H_
#define AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_KERNEL_CONTEXT_H_
#include <type_traits>
#include "kernel_run_context.h"
namespace gert {
class Chain {
 public:
  using Deleter = void(void *);
  template<typename T, typename std::enable_if<(sizeof(T) <= sizeof(void *)), int>::type = 0>
  const T *GetPointer() const {
    return reinterpret_cast<const T *>(&(any_value_.data));
  }
  template<typename T, typename std::enable_if<(sizeof(T) > sizeof(void *)), int>::type = 0>
  const T *GetPointer() const {
    return reinterpret_cast<const T *>(any_value_.data);
  }
  template<typename T, typename std::enable_if<(sizeof(T) <= sizeof(void *)), int>::type = 0>
  T *GetPointer() {
    return reinterpret_cast<T *>(&(any_value_.data));
  }
  template<typename T, typename std::enable_if<(sizeof(T) > sizeof(void *)), int>::type = 0>
  T *GetPointer() {
    return reinterpret_cast<T *>(any_value_.data);
  }
  template<typename T, typename std::enable_if<(sizeof(T) <= sizeof(void *)), int>::type = 0>
  const T &GetValue() const {
    return *reinterpret_cast<const T *>(&(any_value_.data));
  }
  template<typename T, typename std::enable_if<(sizeof(T) <= sizeof(void *)), int>::type = 0>
  T &GetValue() {
    return *reinterpret_cast<T *>(&(any_value_.data));
  }

  void Set(void *data, Chain::Deleter deleter) {
    any_value_.data = data;
    any_value_.deleter = deleter;
  }

  template<typename T>
  void SetWithDefaultDeleter(T *data) {
    Set(data, reinterpret_cast<FreeCallback>(DefaultDeleter<T>));
  }

 private:
  template<typename T>
  static void DefaultDeleter(T *data) {
    delete data;
  }

 private:
  AsyncAnyValue any_value_;
};
static_assert(std::is_standard_layout<Chain>::value, "The class Chain must be a POD");
class KernelContext {
 public:
  size_t GetInputNum() const {
    return context_.input_size;
  }
  size_t GetOutputNum() const {
    return context_.output_size;
  }
  const Chain *GetInput(size_t i) const {
    if (i >= context_.input_size) {
      return nullptr;
    }
    return reinterpret_cast<const Chain *>(context_.values[i]);
  }
  Chain *GetOutput(size_t i) {
    if (i >= context_.output_size) {
      return nullptr;
    }
    return reinterpret_cast<Chain *>(context_.values[context_.input_size + i]);
  }
  Chain *GetOutput2(size_t i) {
    if (i >= context_.output_size) {
      return nullptr;
    }
    return reinterpret_cast<Chain *>(context_.output_start[i]);
  }

  template<typename T>
  const T GetInputValue(size_t i) const {
    auto av = GetInput(i);
    if (av == nullptr) {
      return {};
    }
    return av->GetValue<T>();
  }
  template<typename T>
  const T *GetInputPointer(size_t i) const {
    auto av = GetInput(i);
    if (av == nullptr) {
      return nullptr;
    }
    return av->GetPointer<T>();
  }
  // todo 特化一个模板就可以了
  const char *GetInputStrPointer(size_t i) const {
    auto av = GetInput(i);
    if (av == nullptr) {
      return nullptr;
    }
    return av->GetValue<const char *>();
  }

  const void *GetComputeNodeExtend() const {
    return context_.compute_node_info;
  }

  const void *GetKernelExtend() const {
    return context_.kernel_extend_info;
  }

  template<typename T>
  T *GetOutputPointer(size_t i) {
    auto av = GetOutput(i);
    if (av == nullptr) {
      return nullptr;
    }
    return av->GetPointer<T>();
  }

  KernelRunContext *GetContext() {
    return &context_;
  }
  const KernelRunContext *GetContext() const {
    return &context_;
  }

  static bool IsInlineSize(size_t size) {
    return size <= sizeof(void *);
  }

 private:
  KernelRunContext context_;
};
static_assert(std::is_standard_layout<KernelContext>::value, "The class KernelContext must be a POD");
}  // namespace gert
#endif  //AIR_CXX_RUNTIME_V2_METADEF_RUNTIME_KERNEL_CONTEXT_H_
