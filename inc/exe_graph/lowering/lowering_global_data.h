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

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_LOWERING_GLOBAL_DATA_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_LOWERING_GLOBAL_DATA_H_
#include <map>
#include "value_holder.h"
#include "proto/task.pb.h"
namespace gert {
class LoweringGlobalData {
 public:
  struct NodeCompileResult {
    const std::vector<domi::TaskDef> &GetTaskDefs() const {
      return task_defs;
    }
    std::vector<domi::TaskDef> task_defs;
  };
  const bg::ValueHolderPtr &GetStream() const;
  LoweringGlobalData &SetStream(bg::ValueHolderPtr &&stream);
  const NodeCompileResult *FindCompiledResult(const ge::NodePtr &node) const;
  LoweringGlobalData &AddCompiledResult(const ge::NodePtr &node, NodeCompileResult compile_result);
 private:
  bg::ValueHolderPtr stream_;
  std::map<int64_t, NodeCompileResult> node_ids_to_compile_result_holders_;
};
}

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_LOWERING_GLOBAL_DATA_H_
