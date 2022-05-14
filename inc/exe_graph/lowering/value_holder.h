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

#ifndef AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_VALUE_HOLDER_H_
#define AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_VALUE_HOLDER_H_
#include <cstdint>
#include <string>
#include <memory>
#include <atomic>

#include "graph/buffer.h"
#include "graph/any_value.h"
#include "graph/compute_graph.h"
#include "graph/node.h"
#include "common/hyper_status.h"
#include "graph_frame.h"

namespace gert {
namespace bg {
class ValueHolder;
using ValueHolderPtr = std::shared_ptr<ValueHolder>;
class ValueHolder {
 public:
  enum ValueHolderType {
    kConst,   // 常量，执行时不变
    kFeed,    // 执行时外部指定
    kOutput,  // 由node产生，包含数据输出与控制输出
    // Add new type definitions here
    kValueHolderTypeEnd
  };

  enum RunStage {
    kInit,
    kMain,
    kExit,

    kRunStageEnd
  };

  using NodeHolder = ge::Node;
  using NodeHolderPtr = ge::NodePtr;
  using GraphHolder = ge::ComputeGraph;
  using GraphHolderPtr = ge::ComputeGraphPtr;
  using ExecuteGraphPtr = ge::ComputeGraphPtr;

  class CurrentComputeNodeGuarder {
   public:
    explicit CurrentComputeNodeGuarder(ge::NodePtr old_node) : old_node_(std::move(old_node)) {}
    ~CurrentComputeNodeGuarder() {
      ValueHolder::SetCurrentComputeNode(old_node_);
    }

   private:
    ge::NodePtr old_node_;
  };

  ValueHolder(const ValueHolder &other) = delete;
  ValueHolder &operator=(const ValueHolder &other) = delete;
  ~ValueHolder();

  bool IsOk() const noexcept;

  int64_t GetId() const noexcept;
  ValueHolderType GetType() const noexcept;
  const NodeHolder *GetNode() const noexcept;
  const GraphHolder *GetGraph() const noexcept;

  int32_t GetOutIndex() const noexcept;
  void SetStage(RunStage stage);
  // ref-from other的含义是，本value指向了other（本value没有独立的内存）
  ge::graphStatus RefFrom(const ValueHolderPtr &other);

  static ValueHolderPtr CreateError(const char *fmt, ...);
  static ValueHolderPtr CreateError(const char *fmt, va_list arg);
  static ValueHolderPtr CreateConst(const void *data, size_t size, bool is_string = false);
  static ValueHolderPtr CreateFeed(int64_t index);

  static ValueHolderPtr CreateSingleDataOutput(const char *node_type, const std::vector<ValueHolderPtr> &inputs);
  static std::vector<ValueHolderPtr> CreateDataOutput(const char *node_type, const std::vector<ValueHolderPtr> &inputs,
                                                      size_t out_count);
  static ValueHolderPtr CreateVoid(const char *node_type, const std::vector<ValueHolderPtr> &inputs);
  static HyperStatus AddDependency(const ValueHolderPtr &src, const ValueHolderPtr &dst);

  static GraphFrame *PushGraphFrame();
  static std::unique_ptr<GraphFrame> PopGraphFrame();
  static GraphFrame *GetCurrentFrame();
  static GraphHolder *GetCurrentGraph();
  static void SetCurrentComputeNode(const ge::NodePtr &node);
  static std::unique_ptr<CurrentComputeNodeGuarder> SetScopedCurrentComputeNode(const ge::NodePtr &node);

  static NodeHolderPtr AddNode(const char *node_type, size_t input_count,
                               size_t output_count, GraphFrame &current_frame);

 private:
  ValueHolder();
  static ValueHolderPtr CreateFromNode(NodeHolderPtr node, int32_t index, ValueHolderType type);
  static std::vector<ValueHolderPtr> CreateFromNode(const NodeHolderPtr &node, size_t out_count);
  static NodeHolderPtr CreateNode(const char *node_type, const std::vector<ValueHolderPtr> &inputs, size_t out_count);

  static HyperStatus MergeToOneGraph(const vector<ValueHolderPtr> &value_holders);

 private:
  static std::atomic<int64_t> id_generator_;
  int64_t id_;
  ValueHolder::ValueHolderType type_;
  GraphHolderPtr graph_;
  ge::NodePtr node_;
  int32_t index_;
  std::unique_ptr<char[]> error_msg_;
};
}  // namespace bg
}  // namespace gert

#endif  //AIR_CXX_RUNTIME_V2_GRAPH_BUILDER_VALUE_HOLDER_H_
