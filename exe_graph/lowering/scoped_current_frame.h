/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef METADEF_CXX_EXE_GRAPH_LOWERING_SCOPED_CURRENT_FRAME_H_
#define METADEF_CXX_EXE_GRAPH_LOWERING_SCOPED_CURRENT_FRAME_H_
#include "exe_graph/lowering/graph_frame.h"
#include "value_holder_inner.h"
namespace gert {
namespace bg {
class ScopedCurrentFrame {
 public:
  explicit ScopedCurrentFrame(GraphFrame *frame) {
    backup_graph_frame_ = GetCurrentFrame();
    SetCurrentFrame(frame);
  }
  ~ScopedCurrentFrame() {
    SetCurrentFrame(backup_graph_frame_);
  }

 private:
  GraphFrame *backup_graph_frame_;
};
}  // namespace bg
}  // namespace gert
#endif  // METADEF_CXX_EXE_GRAPH_LOWERING_SCOPED_CURRENT_FRAME_H_
