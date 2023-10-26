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

#include "graph/args_format_desc.h"

#include "common/checker.h"
#include "common/ge_common/debug/ge_log.h"
#include "graph/op_desc.h"
#include "graph/utils/op_desc_utils.h"
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
namespace ge {
constexpr size_t kMaxDimNum = 25UL;
constexpr size_t kMaxWorkspaceNum = 16UL;
constexpr int32_t kDecimalCarry = 10;

using ParseFunc =
    std::function<graphStatus(const OpDescPtr &, const std::string &, const AddrType type, std::vector<ArgDesc> &)>;
using GetArgsSize = std::function<graphStatus(const OpDescPtr &, const ArgDesc &, size_t &)>;
using SerializeFunc = std::function<void(std::stringstream &ss, const std::string &pattern, const ArgDesc &arg_desc)>;

struct PatternHandler {
  ParseFunc parse;
  GetArgsSize getArgsSize;
  SerializeFunc serialize;
  AddrType type;
};

graphStatus DefaultCalcSize(const OpDescPtr &op_desc, const ArgDesc &arg_desc, size_t &size) {
  (void) op_desc;
  (void) arg_desc;
  size += sizeof(uintptr_t);
  return GRAPH_SUCCESS;
}

graphStatus DefaultParser(const OpDescPtr &op_desc, const std::string &pattern_str, const AddrType type,
                          std::vector<ArgDesc> &args_desc) {
  (void) op_desc;
  (void) pattern_str;
  args_desc.push_back({type, -1, false, {0}});
  return GRAPH_SUCCESS;
}

void DefaultSerializer(std::stringstream &ss, const std::string &pattern, const ArgDesc &arg_desc) {
  (void) arg_desc;
  ss << pattern;
}

void FftsTilingSerializer(std::stringstream &ss, const std::string &pattern, const ArgDesc &arg_desc) {
  ss << pattern;
  if (arg_desc.ir_idx == 0) {
    ss << ".non_tail";
  } else {
    ss << ".tail";
  }
}

void ArrayLikeSerializer(std::stringstream &ss, const std::string &pattern, const ArgDesc &arg_desc) {
  ss << pattern;
  if (arg_desc.ir_idx >= 0) {
    ss << std::to_string(arg_desc.ir_idx);
    if (!arg_desc.folded) {
      ss << '*';
    }
  } else {
    ss << '*';
  }
}

graphStatus WorkspaceCalcSize(const OpDescPtr &op_desc, const ArgDesc &arg_desc, size_t &size) {
  (void) op_desc;
  if (arg_desc.ir_idx == -1) {
    size += sizeof(uintptr_t) * kMaxWorkspaceNum;
  } else {
    size += sizeof(uintptr_t);
  }
  return GRAPH_SUCCESS;
}

graphStatus WorkspaceParser(const OpDescPtr &op_desc, const std::string &pattern_str, const AddrType type,
                            std::vector<ArgDesc> &args_desc) {
  (void) op_desc;
  if (pattern_str == "ws*") {
    args_desc.push_back({type, -1, false, {0}});
    return GRAPH_SUCCESS;
  }
  int32_t ir_idx = 0;
  for (size_t i = 2UL; i < pattern_str.size(); ++i) {
    if (isdigit(pattern_str[i])) {
      ir_idx = ir_idx * kDecimalCarry + pattern_str[i] - '0';
    }
  }
  args_desc.push_back({type, ir_idx, false, {0}});
  return GRAPH_SUCCESS;
}

graphStatus InputCalcSize(const OpDescPtr &op_desc, const ArgDesc &arg_desc, size_t &size) {
  const auto &ir_inputs = op_desc->GetIrInputs();
  size_t count = 0UL;
  if (arg_desc.ir_idx >= 0) {
    // 非通配符场景
    GE_ASSERT((static_cast<size_t>(arg_desc.ir_idx) < ir_inputs.size()), "ir_index [%d] is out of range",
              arg_desc.ir_idx);
    if (ir_inputs[arg_desc.ir_idx].second == IrInputType::kIrInputDynamic) {
      if (arg_desc.folded) {
        ++count;  // pointer to addr
      }
      int32_t dyn_num = 0;
      for (auto &iter : op_desc->GetAllInputName()) {
        if (iter.first == ir_inputs[arg_desc.ir_idx].first + std::to_string(dyn_num)) {
          ++dyn_num;
          ++count;  // real input_addr
        }
      }
    } else {
      ++count;
    }
  } else {
    // 通配符场景，非动态输入默认展开, 动态输入按照i0形式折叠
    for (const auto &ir_input : ir_inputs) {
      ++count;
      if (ir_input.second == IrInputType::kIrInputDynamic) {
        int32_t dyn_num = 0;
        for (auto &iter : op_desc->GetAllInputName()) {
          if (iter.first == ir_input.first + std::to_string(dyn_num)) {
            ++count;  // real input addr
            ++dyn_num;
          }
        }
      }
    }
  }
  size += count * sizeof(uintptr_t);

  return GRAPH_SUCCESS;
}

graphStatus InputParser(const OpDescPtr &op_desc, const std::string &pattern_str, const AddrType type,
                        std::vector<ArgDesc> &arg_descs) {
  const auto &ir_inputs = op_desc->GetIrInputs();
  if (pattern_str == "i*") {
    // 为了方便加载时使用，通配符场景解析后默认展开成多个
    for (size_t i = 0UL; i < ir_inputs.size(); ++i) {
      bool folded{false};
      if (ir_inputs[i].second == IrInputType::kIrInputDynamic) {
        folded = true;
      }
      arg_descs.push_back({type, static_cast<int32_t>(i), folded, {0}});
    }
  } else {
    int32_t ir_idx{0};
    bool has_idx{false};
    for (size_t i = 1UL; i < pattern_str.size(); ++i) {
      if (isdigit(pattern_str[i])) {
        ir_idx = ir_idx * kDecimalCarry + pattern_str[i] - '0';
        has_idx = true;
      }
    }
    GE_ASSERT(has_idx, "Arg format [%s] is invalid", pattern_str.c_str());
    GE_ASSERT(static_cast<size_t>(ir_idx) < ir_inputs.size(), "ir index [%d] is invalid.", ir_idx);

    bool folded{false};
    if (ir_inputs[static_cast<size_t>(ir_idx)].second == IrInputType::kIrInputDynamic &&
        pattern_str[pattern_str.length() - 1UL] != '*') {
      folded = true;
    }
    arg_descs.push_back({type, ir_idx, folded, {0}});
  }

  return GRAPH_SUCCESS;
}

graphStatus OutputCalcSize(const OpDescPtr &op_desc, const ArgDesc &arg_desc, size_t &size) {
  const auto &ir_outputs = op_desc->GetIrOutputs();
  size_t count = 0UL;
  if (arg_desc.ir_idx >= 0) {
    // 非通配符场景
    GE_ASSERT((static_cast<size_t>(arg_desc.ir_idx) < ir_outputs.size()), "ir_index [%d] is out of range",
              arg_desc.ir_idx);
    if (ir_outputs[arg_desc.ir_idx].second == IrOutputType::kIrOutputDynamic) {
      if (arg_desc.folded) {
        count++;  // pointer to addr
      }
      int32_t dyn_num = 0;
      for (auto &iter : op_desc->GetAllOutputName()) {
        if (iter.first == ir_outputs[arg_desc.ir_idx].first + std::to_string(dyn_num)) {
          ++count;  // real input_addr
          ++dyn_num;
        }
      }
    } else {
      count++;
    }
  } else {
    // 通配符场景，非动态输入默认展开, 动态输入按照i0形式折叠
    for (const auto &ir_output : ir_outputs) {
      count++;
      if (ir_output.second == IrOutputType::kIrOutputDynamic) {
        int32_t dyn_num = 0;
        for (auto &iter : op_desc->GetAllOutputName()) {
          if (iter.first == ir_output.first + std::to_string(dyn_num)) {
            ++count;  // real input addr
            ++dyn_num;
          }
        }
      }
    }
  }
  size += count * sizeof(uintptr_t);

  return GRAPH_SUCCESS;
}
graphStatus OutputParser(const OpDescPtr &op_desc, const std::string &pattern_str, const AddrType type,
                         std::vector<ArgDesc> &arg_descs) {
  const auto &ir_outputs = op_desc->GetIrOutputs();
  if (pattern_str == "o*") {
    // 为了方便加载时使用，通配符场景解析后默认展开成多个
    for (size_t i = 0UL; i < ir_outputs.size(); ++i) {
      bool folded{false};
      if (ir_outputs[i].second == IrOutputType::kIrOutputDynamic) {
        folded = true;
      }
      arg_descs.push_back({type, static_cast<int32_t>(i), folded, {0}});
    }
  } else {
    int32_t ir_idx{0};
    bool has_idx{false};
    for (size_t i = 1UL; i < pattern_str.size(); ++i) {
      if (isdigit(pattern_str[i])) {
        ir_idx = ir_idx * kDecimalCarry + pattern_str[i] - '0';
        has_idx = true;
      }
    }
    GE_ASSERT(has_idx, "Arg format [%s] is invalid", pattern_str.c_str());
    GE_ASSERT(static_cast<size_t>(ir_idx) < ir_outputs.size(), "ir index [%d] is invalid.", ir_idx);
    bool folded{false};
    if (ir_outputs[static_cast<size_t>(ir_idx)].second == IrOutputType::kIrOutputDynamic &&
        pattern_str[pattern_str.length() - 1UL] != '*') {
      folded = true;
    }
    arg_descs.push_back({type, ir_idx, folded, {0}});
  }
  return GRAPH_SUCCESS;
}

graphStatus InputDescCalcSize(const OpDescPtr &op_desc, const ArgDesc &arg_desc, size_t &size) {
  const auto &ir_inputs = op_desc->GetIrInputs();
  GE_ASSERT((arg_desc.ir_idx >= 0 && static_cast<size_t>(arg_desc.ir_idx) < ir_inputs.size()),
            "ir_index is out of range");
  auto ir_name = ir_inputs[static_cast<size_t>(arg_desc.ir_idx)].first;

  if (arg_desc.folded) {
    size += sizeof(uintptr_t);  // pointer to desc
  }
  size += sizeof(uintptr_t);  // offset to addr
  size_t dyn_num = 0UL;
  for (auto &iter : op_desc->GetAllInputName()) {
    if (iter.first == ir_name + std::to_string(dyn_num)) {
      const auto &input_desc = op_desc->GetInputDesc(iter.second);
      size += sizeof(uintptr_t) * 2UL;  // dims_info + addr
      if (input_desc.GetShape().IsUnknownDimNum()) {
        size += sizeof(uintptr_t) * kMaxDimNum;
      } else if (input_desc.GetShape().IsScalar()) {
        size += sizeof(uintptr_t);
      } else {
        size += sizeof(uintptr_t) * input_desc.GetShape().GetDimNum();
      }
      ++dyn_num;
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus OutputDescCalcSize(const OpDescPtr &op_desc, const ArgDesc &arg_desc, size_t &size) {
  const auto &ir_outputs = op_desc->GetIrOutputs();
  GE_ASSERT((arg_desc.ir_idx >= 0 && static_cast<size_t>(arg_desc.ir_idx) < ir_outputs.size()),
            "ir_index [%d] is out of range", arg_desc.ir_idx);
  auto ir_name = ir_outputs[static_cast<size_t>(arg_desc.ir_idx)].first;

  if (arg_desc.folded) {
    size += sizeof(uintptr_t);  // pointer to desc
  }
  size += sizeof(uintptr_t);  // offset to addr
  size_t dyn_num = 0UL;
  for (auto &iter : op_desc->GetAllOutputName()) {
    if (iter.first == ir_name + std::to_string(dyn_num)) {
      const auto &output_desc = op_desc->GetOutputDesc(iter.second);
      size += sizeof(uintptr_t) * 2UL;  // dims_info + addr
      if (output_desc.GetShape().IsUnknownDimNum()) {
        size += sizeof(uintptr_t) * kMaxDimNum;
      } else if (output_desc.GetShape().IsScalar()) {
        size += sizeof(uintptr_t);
      } else {
        size += sizeof(uintptr_t) * output_desc.GetShape().GetDimNum();
      }
      ++dyn_num;
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus IODescParser(const OpDescPtr &op_desc, const std::string &pattern_str, const AddrType type,
                         std::vector<ArgDesc> &arg_descs) {
  (void) op_desc;
  bool folded{true};
  if (pattern_str[pattern_str.length() - 1] == '*') {
    folded = false;
  }
  int32_t ir_idx{0};
  bool has_idx{false};
  for (size_t i = 6UL; i < pattern_str.size(); ++i) {  // start after i_desc/o_desc
    if (isdigit(pattern_str[i])) {
      ir_idx = ir_idx * kDecimalCarry + pattern_str[i] - '0';
      has_idx = true;
    }
  }
  GE_ASSERT(has_idx, "Dynamic intput/output should have a concrete ir idx.");
  arg_descs.push_back({type, ir_idx, folded, {0}});
  return GRAPH_SUCCESS;
}

struct PatternCmp {
  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return lhs.size() >= rhs.size();
  };
};

static const std::map<std::string, PatternHandler, PatternCmp> kPatternToHandler = {
    {"i", {&InputParser, &InputCalcSize, ArrayLikeSerializer, AddrType::INPUT}},
    {"o", {&OutputParser, &OutputCalcSize, ArrayLikeSerializer, AddrType::OUTPUT}},
    {"ws", {&WorkspaceParser, &WorkspaceCalcSize, ArrayLikeSerializer, AddrType::WORKSPACE}},
    {"t", {&DefaultParser, &DefaultCalcSize, DefaultSerializer, AddrType::TILING}},
    {"i_desc", {&IODescParser, &InputDescCalcSize, ArrayLikeSerializer, AddrType::INPUT_DESC}},
    {"o_desc", {&IODescParser, &OutputDescCalcSize, ArrayLikeSerializer, AddrType::OUTPUT_DESC}},
    {"ffts_addr", {&DefaultParser, &DefaultCalcSize, DefaultSerializer, AddrType::FFTS_ADDR}},
    {"overflow_addr", {&DefaultParser, &DefaultCalcSize, DefaultSerializer, AddrType::OVERFLOW_ADDR}},
    {"t_ffts", {&DefaultParser, &DefaultCalcSize, FftsTilingSerializer, AddrType::TILING_FFTS}},
};

void ArgsFormatDesc::Append(AddrType type, int32_t ir_idx, bool folded) {
  arg_descs_.push_back({type, ir_idx, folded, {0}});
}

std::string ArgsFormatDesc::ToString() const {
  std::stringstream ss;
  for (const auto &arg_desc : arg_descs_) {
    for (const auto &iter : kPatternToHandler) {
      if (iter.second.type == arg_desc.addr_type) {
        ss << '{';
        iter.second.serialize(ss, iter.first, arg_desc);
        ss << '}';
      }
    }
  }
  return ss.str();
}

graphStatus ArgsFormatDesc::GetArgsSize(const OpDescPtr &op_desc, size_t &size) const {
  size_t args_size{0UL};
  for (const auto &arg_desc : arg_descs_) {
    for (const auto &iter : kPatternToHandler) {
      if (iter.second.type == arg_desc.addr_type) {
        GE_ASSERT_SUCCESS(iter.second.getArgsSize(op_desc, arg_desc, args_size));
      }
    }
  }
  size = args_size;
  return GRAPH_SUCCESS;
}

graphStatus ArgsFormatDesc::Parse(const OpDescPtr &op_desc, const std::string &str, std::vector<ArgDesc> &arg_descs) {
  arg_descs.clear();
  size_t start_idx = 0UL;
  while (start_idx < str.size()) {
    GE_ASSERT(str[start_idx] == '{', "SyntaxError: argsformat should be surrounded by '{','}'");
    size_t end_idx = start_idx + 1UL;
    bool parsed{false};
    while (end_idx < str.size()) {
      if (str[end_idx] == '}') {
        std::string pattern_str = str.substr(start_idx + 1, end_idx - start_idx - 1);
        for (const auto &iter : kPatternToHandler) {
          if (strncmp(pattern_str.c_str(), iter.first.c_str(), iter.first.length()) == 0) {
            GE_ASSERT_SUCCESS(iter.second.parse(op_desc, pattern_str, iter.second.type, arg_descs),
                              "args format [%s] parser failed.", pattern_str.c_str());
            parsed = true;
            start_idx = end_idx + 1UL;
            break;
          }
        }
        GE_ASSERT(parsed, "arg format [%s] is unsupported.", pattern_str.c_str());
        break;
      }
      ++end_idx;
    }
    GE_ASSERT(parsed, "SyntaxError: argsformat should be surrounded by '{','}'");
  }
  return GRAPH_SUCCESS;
}
}  // namespace ge