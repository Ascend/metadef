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

#include "exe_graph/lowering/value_holder.h"

#include <stack>

#include <securec.h>
#include "graph/utils/graph_utils.h"
#include "graph/debug/ge_log.h"
#include "graph/utils/mem_utils.h"
#include "framework/common/util.h"
#include "common/checker.h"

#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/context_extend.h"
namespace gert {
namespace bg {
namespace {
thread_local std::stack<std::unique_ptr<GraphFrame>> graph_frames;
ge::OpDescPtr CreateOpDesc(int64_t node_index, const char *node_type, size_t in_count, size_t out_count) {
  auto op_desc = ge::MakeShared<ge::OpDesc>("node" + std::to_string(node_index), node_type);
  GE_ASSERT_NOTNULL(op_desc);
  for (size_t i = 0; i < in_count; ++i) {
    if (op_desc->AddInputDesc(ge::GeTensorDesc()) != ge::GRAPH_SUCCESS) {
      GE_LOGE("Failed to create OpDesc for node %s index %ld, io-count %zu/%zu, add input desc %zu failed ", node_type,
              node_index, in_count, out_count, i);
      return nullptr;
    }
  }
  for (size_t i = 0; i < out_count; ++i) {
    if (op_desc->AddOutputDesc(ge::GeTensorDesc()) != ge::GRAPH_SUCCESS) {
      GE_LOGE("Failed to create OpDesc for node %s index %ld, io-count %zu/%zu, add output desc %zu failed ", node_type,
              node_index, in_count, out_count, i);
      return nullptr;
    }
  }
  return op_desc;
}
ge::graphStatus AddDataEdge(const ge::NodePtr &src, int src_index, const ge::NodePtr &dst, int dst_index) {
  auto ret = ge::GraphUtils::AddEdge(src->GetOutDataAnchor(src_index), dst->GetInDataAnchor(dst_index));
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGE(ret, "Failed to connect edge from %s:%d to %s:%d, error code %u", src->GetName().c_str(), src_index,
           dst->GetName().c_str(), dst_index, ret);
  }
  return ret;
}
ge::graphStatus AddControlEdge(const ge::NodePtr &src, const ge::NodePtr &dst) {
  auto ret = ge::GraphUtils::AddEdge(src->GetOutControlAnchor(), dst->GetInControlAnchor());
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGE(ret, "Failed to connect control edge from %s to %s, error code %u", src->GetName().c_str(),
           dst->GetName().c_str(), ret);
  }
  return ret;
}
std::unique_ptr<uint8_t[]> CreateKernelExtendInfo(const char *name, const char *type, BufferPool &buffer_pool,
                                                  size_t &total_size) {
  total_size = sizeof(KernelExtendInfo);
  auto holder = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[total_size]);
  GE_ASSERT_NOTNULL(holder);

  auto name_id = buffer_pool.AddStr(name);
  auto type_id = buffer_pool.AddStr(type);

  auto extend_info = reinterpret_cast<KernelExtendInfo *>(holder.get());
  extend_info->SetKernelName(reinterpret_cast<const char *>(name_id));
  extend_info->SetKernelType(reinterpret_cast<const char *>(type_id));

  return holder;
}
}  // namespace
std::atomic<int64_t> ValueHolder::id_generator_{0};
ValueHolder::~ValueHolder() = default;

ValueHolder::ValueHolder() : id_(id_generator_++), type_(ValueHolderType::kValueHolderTypeEnd), index_(0) {}

bool ValueHolder::IsOk() const noexcept {
  return error_msg_ == nullptr;
}
ValueHolder::ValueHolderType ValueHolder::GetType() const noexcept {
  return type_;
}
const ge::Node *ValueHolder::GetNode() const noexcept {
  return node_.get();
}
int32_t ValueHolder::GetOutIndex() const noexcept {
  return index_;
}
int64_t ValueHolder::GetId() const noexcept {
  return id_;
}
const ValueHolder::GraphHolder *ValueHolder::GetGraph() const noexcept {
  return graph_.get();
}
ValueHolderPtr ValueHolder::CreateError(const char *fmt, va_list arg) {
  auto value_holder = std::shared_ptr<ValueHolder>(new (std::nothrow) ValueHolder());
  GE_ASSERT_NOTNULL(value_holder);
  value_holder->error_msg_ = std::unique_ptr<char[]>(CreateMessage(fmt, arg));
  return value_holder;
}
ValueHolderPtr ValueHolder::CreateError(const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  auto holder = CreateError(fmt, arg);
  va_end(arg);
  return holder;
}
ValueHolder::NodeHolderPtr ValueHolder::AddNode(const GraphHolderPtr &graph, const char *node_type, size_t input_count,
                                                size_t output_count) {
  auto node = graph->AddNode(CreateOpDesc(id_generator_++, node_type, input_count, output_count));
  GE_ASSERT_NOTNULL(node);

  auto frame = GetCurrentFrame();
  GE_ASSERT_NOTNULL(frame);

  // add compute node info index
  size_t index;
  if (frame->GetCurrentNodeIndex(index)) {
    if (!ge::AttrUtils::SetInt(node->GetOpDesc(), kComputeNodeIndex, static_cast<int64_t>(index))) {
      GE_LOGE("Failed to add node %s, add ComputeNodeIndex failed", node_type);
      return nullptr;
    }
  }

  // add kernel extend info index
  size_t extend_info_size;
  auto holder = CreateKernelExtendInfo(node->GetName().c_str(), node->GetType().c_str(), frame->GetBufferPool(),
                                       extend_info_size);
  GE_ASSERT_NOTNULL(holder);

  auto buf_id = frame->GetKernelExtendInfos().AddBuf(holder.get(), extend_info_size);
  if (!ge::AttrUtils::SetInt(node->GetOpDesc(), kKernelExtendIndex, static_cast<int64_t>(buf_id))) {
    GE_LOGE("Failed to add node %s, add KernelExtendIndex failed", node_type);
    return nullptr;
  }

  return node;
}
ValueHolder::NodeHolderPtr ValueHolder::CreateNode(const char *node_type, const std::vector<ValueHolderPtr> &inputs,
                                                   size_t out_count) {
  auto frame = GetCurrentFrame();
  if (frame == nullptr) {
    GE_LOGE("The current frame does not exists, "
            "the function ValueHolder::PushGraphFrame should be called before construct the graph");
    return nullptr;
  }
  const auto &graph = frame->GetExeGraph();
  GE_ASSERT_NOTNULL(graph);

  auto node = ValueHolder::AddNode(graph, node_type, inputs.size(), out_count);
  GE_ASSERT_NOTNULL(node);

  /*
   * todo 检查是否有子图向父图连接的场景，这种场景需要报错
   *      父图向子图连接的场景，为父图节点创建一个InnerData
   */
  for (size_t i = 0; i < inputs.size(); ++i) {
    GE_ASSERT_NOTNULL(inputs[i]);
    GE_ASSERT_NOTNULL(inputs[i]->node_);
    GE_ASSERT_SUCCESS(AddDataEdge(inputs[i]->node_, inputs[i]->index_, node, static_cast<int32_t>(i)));
  }
  return node;
}
ValueHolderPtr ValueHolder::CreateFromNode(ge::NodePtr node, int32_t index, ValueHolderType type) {
  auto holder = std::shared_ptr<ValueHolder>(new (std::nothrow) ValueHolder());
  GE_ASSERT_NOTNULL(holder);

  holder->type_ = type;
  holder->graph_ = node->GetOwnerComputeGraph();
  holder->node_ = std::move(node);
  holder->index_ = index;
  return holder;
}
std::vector<ValueHolderPtr> ValueHolder::CreateFromNode(const NodeHolderPtr &node, size_t out_count) {
  std::vector<ValueHolderPtr> holders;
  for (size_t i = 0; i < out_count; ++i) {
    holders.emplace_back(CreateFromNode(node, static_cast<int32_t>(i), ValueHolderType::kOutput));
  }

  return holders;
}

std::vector<ValueHolderPtr> ValueHolder::CreateDataOutput(const char *node_type,
                                                          const std::vector<ValueHolderPtr> &inputs, size_t out_count) {
  auto node = CreateNode(node_type, inputs, out_count);
  if (node == nullptr) {
    return {out_count, nullptr};
  }
  return CreateFromNode(node, out_count);
}
ValueHolderPtr ValueHolder::CreateVoid(const char *node_type, const vector<ValueHolderPtr> &inputs) {
  auto node = CreateNode(node_type, inputs, 0);
  GE_ASSERT_NOTNULL(node);
  return CreateFromNode(node, -1, kOutput);
}
/**
 * @param data const数据
 * @param size const数据的长度
 * @param is_string 此const是否是个字符串, todo: 当前对string支持的不好
 * @return
 */
ValueHolderPtr ValueHolder::CreateConst(const void *data, size_t size, bool is_string) {
  auto node = ValueHolder::CreateNode("Const", {}, 1);
  GE_ASSERT_NOTNULL(node);
  GE_ASSERT_SUCCESS(node->GetOpDesc()->SetAttr("is_string", ge::AnyValue::CreateFrom(is_string)));
  GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(node->GetOpDesc(), kConstValue,
                                                 ge::Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(data), size)));
  return CreateFromNode(node, 0, kConst);
}
ValueHolderPtr ValueHolder::CreateFeed(int64_t index) {
  auto node = ValueHolder::CreateNode("Data", {}, 1);
  GE_ASSERT_NOTNULL(node);
  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(node->GetOpDesc(), kFeedIndex, index));
  return CreateFromNode(node, 0, kFeed);
}

ValueHolderPtr ValueHolder::CreateSingleDataOutput(const char *node_type, const vector<ValueHolderPtr> &inputs) {
  auto holders = CreateDataOutput(node_type, inputs, 1);
  if (holders.empty()) {
    return nullptr;
  }
  return holders[0];
}
HyperStatus ValueHolder::AddDependency(const ValueHolderPtr &src, const ValueHolderPtr &dst) {
  if (src == nullptr || src->GetNode() == nullptr) {
    return HyperStatus::ErrorStatus("Failed to add control ege, because the src does not have a node.");
  }
  if (dst == nullptr || dst->GetNode() == nullptr) {
    return HyperStatus::ErrorStatus("Failed to add control ege, because the dst does not have a node.");
  }
  // todo 检查是否在一张图上
  if (ge::GraphUtils::AddEdge(src->GetNode()->GetOutControlAnchor(), dst->GetNode()->GetInControlAnchor()) !=
      ge::GRAPH_SUCCESS) {
    return HyperStatus::ErrorStatus("Failed to add control edge from %s to %s", src->GetNode()->GetName().c_str(),
                                    dst->GetNode()->GetName().c_str());
  }
  return HyperStatus::Success();
}
void ValueHolder::SetStage(ValueHolder::RunStage stage) {
  ge::AttrUtils::SetInt(node_->GetOpDesc(), kStage, stage);
}
GraphFrame *ValueHolder::PushGraphFrame() {
  auto graph = ge::MakeShared<ge::ComputeGraph>("");
  GE_ASSERT_NOTNULL(graph);
  auto frame = new (std::nothrow) GraphFrame(graph);
  GE_ASSERT_NOTNULL(frame);
  graph_frames.emplace(frame);
  return graph_frames.top().get();
}
std::unique_ptr<GraphFrame> ValueHolder::PopGraphFrame() {
  if (graph_frames.empty()) {
    return nullptr;
  }
  auto ret = std::move(graph_frames.top());
  graph_frames.pop();
  return ret;
}
GraphFrame *ValueHolder::GetCurrentFrame() {
  if (graph_frames.empty()) {
    return nullptr;
  }
  return graph_frames.top().get();
}
void ValueHolder::SetCurrentComputeNode(const ge::NodePtr &node) {
  auto frame = GetCurrentFrame();
  if (frame == nullptr) {
    GELOGW("Ignore to add current compute node, the current frame is nullptr");
    return;
  }
  frame->SetCurrentComputeNode(node);
}
std::unique_ptr<ValueHolder::CurrentComputeNodeGuarder> ValueHolder::SetScopedCurrentComputeNode(  //
    const ge::NodePtr &node) {
  auto frame = GetCurrentFrame();
  GE_ASSERT_NOTNULL(frame);

  auto guarder = std::unique_ptr<CurrentComputeNodeGuarder>(
      new (std::nothrow) CurrentComputeNodeGuarder(frame->GetCurrentComputeNode()));
  GE_ASSERT_NOTNULL(guarder);
  frame->SetCurrentComputeNode(node);
  return guarder;
}
ValueHolder::GraphHolder *ValueHolder::GetCurrentGraph() {
  auto frame = GetCurrentFrame();
  GE_ASSERT_NOTNULL(frame);
  return frame->GetExeGraph().get();
}
ge::graphStatus ValueHolder::RefFrom(const ValueHolderPtr &other) {
  GE_ASSERT_NOTNULL(node_);
  GE_ASSERT_NOTNULL(other);
  GE_ASSERT_NOTNULL(other->node_);

  if (index_ < 0 || other->index_ < 0) {
    GELOGE(ge::PARAM_INVALID, "Invalid index to ref %d -> %d", index_, other->index_);
    return ge::PARAM_INVALID;
  }

  auto td = node_->GetOpDesc()->MutableOutputDesc(index_);
  GE_ASSERT_NOTNULL(td);

  GE_ASSERT_TRUE(ge::AttrUtils::SetStr(td, kRefFromNode, other->GetNode()->GetName()));
  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(td, kRefFromIndex, other->index_));
  return ge::GRAPH_SUCCESS;
}

ValueHolder::GraphBuilder &ValueHolder::GraphBuilder::SetOutputs(vector<ValueHolderPtr> outputs) {
  outputs_ = std::move(outputs);
  return *this;
}
ValueHolder::GraphBuilder &ValueHolder::GraphBuilder::SetTargets(std::vector<ValueHolderPtr> targets) {
  targets_ = std::move(targets);
  return *this;
}
ge::NodePtr GetOrCreateNetOutput(const ge::ComputeGraphPtr &graph, size_t output_num) {
  auto netoutput = graph->FindFirstNodeMatchType("NetOutput");
  if (netoutput != nullptr) {
    return netoutput;
  }
  auto op_desc = std::make_shared<ge::OpDesc>("NetOutput", "NetOutput");
  GE_ASSERT_NOTNULL(op_desc);

  for (size_t i = 0; i < output_num; ++i) {
    GE_ASSERT_SUCCESS(op_desc->AddInputDesc(ge::GeTensorDesc()));
  }
  return graph->AddNode(op_desc);
}
ValueHolder::ExecuteGraphPtr ValueHolder::GraphBuilder::BuildExecuteGraph() {
  auto frame = ValueHolder::PopGraphFrame();
  GE_ASSERT_NOTNULL(frame);

  std::vector<ValueHolderPtr> all_outputs;
  all_outputs.insert(all_outputs.end(), outputs_.begin(), outputs_.end());
  all_outputs.insert(all_outputs.end(), targets_.begin(), targets_.end());
  if (all_outputs.empty()) {
    GE_LOGE("Failed to build execute graph, no outputs specified");
    return nullptr;
  }
  GE_ASSERT_SUCCESS(AppendGraphLevelData(frame.get()));

  auto &graph = frame->GetExeGraph();
  auto netoutput = GetOrCreateNetOutput(graph, outputs_.size());
  GE_ASSERT_NOTNULL(netoutput);
  for (size_t i = 0; i < outputs_.size(); ++i) {
    GE_ASSERT_NOTNULL(outputs_[i]);
    GE_ASSERT_NOTNULL(outputs_[i]->node_);
    GE_ASSERT_SUCCESS(AddDataEdge(outputs_[i]->node_, outputs_[i]->GetOutIndex(), netoutput, static_cast<int32_t>(i)));
  }
  for (const auto &target_node : targets_) {
    GE_ASSERT_NOTNULL(target_node);
    GE_ASSERT_NOTNULL(target_node->node_);
    GE_ASSERT_SUCCESS(AddControlEdge(target_node->node_, netoutput));
  }
  return graph;
}
ge::graphStatus ValueHolder::GraphBuilder::AppendGraphLevelData(const GraphFrame *frame) {
  if (!frame->IsRootFrame()) {
    GELOGE(ge::PARAM_INVALID, "Failed to append graph level data, current exe_graph is not the root graph");
    return ge::GRAPH_FAILED;
  }
  auto &graph = frame->GetExeGraph();
  GE_ASSERT_NOTNULL(graph);
  size_t buffer_size;

  // buffer pool
  auto buffer = frame->GetBufferPool().Serialize(buffer_size);
  GE_ASSERT_NOTNULL(buffer);
  GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(graph, kBuffer, ge::Buffer::CopyFrom(buffer.get(), buffer_size)));

  // compute node info
  buffer = frame->GetAllComputeNodeInfos().Serialize(buffer_size);
  GE_ASSERT_NOTNULL(buffer);
  GE_ASSERT_TRUE(
      ge::AttrUtils::SetZeroCopyBytes(graph, kComputeNodeInfo, ge::Buffer::CopyFrom(buffer.get(), buffer_size)));

  // kernel extend info
  buffer = frame->GetKernelExtendInfos().Serialize(buffer_size);
  GE_ASSERT_NOTNULL(buffer);
  GE_ASSERT_TRUE(
      ge::AttrUtils::SetZeroCopyBytes(graph, kKernelExtendInfo, ge::Buffer::CopyFrom(buffer.get(), buffer_size)));

  // model_desc
  buffer = frame->GetKernelModelDesc().Serialize(buffer_size);
  GE_CHECK_NOTNULL(buffer);
  GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(graph, kModelDesc, ge::Buffer::CopyFrom(buffer.get(), buffer_size)));

  return ge::GRAPH_SUCCESS;
}
}  // namespace bg
}  // namespace gert