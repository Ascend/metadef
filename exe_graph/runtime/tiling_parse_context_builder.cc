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
#include "exe_graph/runtime/tiling_parse_context_builder.h"

#include "graph/compute_graph.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/debug/ge_util.h"
#include "graph/def_types.h"
#include "common/checker.h"
#include "graph/debug/ge_util.h"
#include "register/op_impl_space_registry.h"

namespace gert {
namespace {
ge::graphStatus FindImplFuncs(const char * const op_type, const gert::OpImplKernelRegistry::OpImplFunctions *&funcs) {
  const auto space_registry = gert::DefaultOpImplSpaceRegistry::GetInstance().GetDefaultSpaceRegistry();
  if (space_registry == nullptr) {
    GELOGE(ge::GRAPH_FAILED, "Invalid space registry!");
    return ge::GRAPH_FAILED;
  }
  funcs = space_registry->GetOpImpl(op_type);
  if ((funcs == nullptr) || (funcs->tiling == nullptr) || (funcs->tiling_parse == nullptr)) {
    funcs = space_registry->GetOpImpl("DefaultImpl");
    GELOGD("funcs/tiling/tiling_parse is null. op type is %s. Try auto tiling", op_type);
    if ((funcs == nullptr) || (funcs->tiling == nullptr) || (funcs->tiling_parse == nullptr)) {
      GELOGE(ge::GRAPH_FAILED, "auto funcs/tiling/tiling_parse is null. op type is %s.", op_type);
      REPORT_CALL_ERROR("E19999", "auto funcs/tiling/tiling_parse is null. op type is %s.", op_type);
      return ge::GRAPH_FAILED;
    }
  }
  GELOGD("Found rt2 tiling func. op type is %s.", op_type);
  return ge::GRAPH_SUCCESS;
}
}
TilingParseContextBuilder &TilingParseContextBuilder::CompileInfo(void *compile_info) {
  compile_info_ = compile_info;
  return *this;
}

TilingParseContextBuilder &TilingParseContextBuilder::PlatformInfo(void *platform_info) {
  platform_info_ = platform_info;
  return *this;
}

KernelContextHolder TilingParseContextBuilder::Build(const ge::Operator &op) {
  KernelContextHolder holder;
  if (compile_info_ == nullptr) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Compile info is nullptr.");
    return holder;
  }
  if (platform_info_ == nullptr) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Platform info is nullptr.");
    return holder;
  }
  const auto op_desc = ge::OpDescUtils::GetOpDescFromOperator(op);

  const gert::OpImplKernelRegistry::OpImplFunctions *funcs;
  if (FindImplFuncs(op_desc->GetType().c_str(), funcs) != ge::GRAPH_SUCCESS) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Find op impl funcs failed.");
    return holder;
  }
  std::vector<std::pair<void *, gert::Chain::Deleter>> tiling_parse_outputs(1, std::make_pair(nullptr, nullptr));
  tiling_parse_outputs[0].first = funcs->compile_info_creator();
  tiling_parse_outputs[0].second = funcs->compile_info_deleter;
  return gert::KernelRunContextBuilder()
      .Inputs({compile_info_})
      .Inputs({platform_info_})
      .Inputs({const_cast<ge::char_t *>(op_desc->GetType().c_str())})
      .Outputs(tiling_parse_outputs)
      .Build(op_desc);
}
}  // namespace gert
