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

#ifndef AIR_CXX_RUNTIME_V2_METADEF_EXE_GRAPH_EXE_GRAPH_ATTRS_H_
#define AIR_CXX_RUNTIME_V2_METADEF_EXE_GRAPH_EXE_GRAPH_ATTRS_H_
namespace gert {
// 打在const节点上，表达const的值，
// todo 后面这个应该慢慢要废弃掉，通过buffer id代替
constexpr char *kConstValue = (char *)"value";

constexpr char *kGraph = (char *)"graph";

// 打在exe node上，代表本node执行的stage（例如DeInit）
constexpr char *kStage = (char *)"stage";

// 打在feed节点上（Data、InnerData），代表这是第几个输入
constexpr char *kFeedIndex = (char *)"index";

// 打在输出desc上，标识本输出不申请独立的内存，从某个node的某个输出index上引用过来使用
constexpr char *kRefFromNode = (char *)"RefFromNode";
constexpr char *kRefFromIndex = (char *)"RefFromIndex";

// 打在exe graph上，保存了本graph涉及的所有的ComputeNodeInfo
constexpr char *kComputeNodeInfo = (char *)"ComputeNodeInfo";

// 打在exe node上，用来标识本node所对应的计算图上的node的index
constexpr char *kComputeNodeIndex = (char *)"ComputeNodeIndex";

// 打在exe graph上，保存了本graph涉及的所有的KernelExtendInfo
constexpr char *kKernelExtendInfo = (char *)"KernelExtendInfo";

// 打在exe node上，用来标识本node所对应的kernel信息的index
constexpr char *kKernelExtendIndex = (char *)"KernelExtendInfoIndex";

// 打在exe graph上，保存了本graph涉及的所有的二进制buffer（字符串、const值等）
constexpr char *kBuffer = (char *)"buffer";

// 打在exe graph上，保存了本graph涉及的ModelDesc信息
constexpr char *kModelDesc = (char *)"ModelDesc";
}
#endif  //AIR_CXX_RUNTIME_V2_METADEF_EXE_GRAPH_EXE_GRAPH_ATTRS_H_
