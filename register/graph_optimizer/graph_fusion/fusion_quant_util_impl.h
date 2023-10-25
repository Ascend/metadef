/**
 * Copyright 2023-2024 Huawei Technologies Co., Ltd
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

#ifndef INC_FUSION_QUANT_UTIL_IMPL_H_
#define INC_FUSION_QUANT_UTIL_IMPL_H_
#include "graph/node.h"
#include "common/ge_common/ge_inner_error_codes.h"
#include "register/graph_optimizer/graph_optimize_register_error_codes.h"
#include "graph/ge_tensor.h"
#include <vector>

namespace fe {
const int32_t NCHW_DIM_N = 0;
const int32_t NCHW_DIM_C = 1;
const int32_t NCHW_DIM_H = 2;
const int32_t NCHW_DIM_W = 3;

const int32_t NC1HWC0_DIM_N = 0;
const int32_t NC1HWC0_DIM_C1 = 1;
const int32_t NC1HWC0_DIM_C0 = 4;
const int32_t NC1HWC0_DIM_H = 2;
const int32_t NC1HWC0_DIM_W = 3;

const int32_t NDC1HWC0_DIM_N = 0;
const int32_t NDC1HWC0_DIM_D = 1;
const int32_t NDC1HWC0_DIM_C1 = 2;
const int32_t NDC1HWC0_DIM_C0 = 5;
const int32_t NDC1HWC0_DIM_H = 3;
const int32_t NDC1HWC0_DIM_W = 4;

const int32_t C1HWNCoC0_DIM_C1 = 0;
const int32_t C1HWNCoC0_DIM_H = 1;
const int32_t C1HWNCoC0_DIM_W = 2;
const int32_t C1HWNCoC0_DIM_N = 3;
const int32_t C1HWNCoC0_DIM_Co = 4;
const int32_t C1HWNCoC0_DIM_C0 = 5;

const int32_t C1DHWNCoC0_DIM_C1 = 0;
const int32_t C1DHWNCoC0_DIM_D = 1;
const int32_t C1DHWNCoC0_DIM_H = 2;
const int32_t C1DHWNCoC0_DIM_W = 3;

const int32_t NHWC_DIM_N = 0;
const int32_t NHWC_DIM_H = 1;
const int32_t NHWC_DIM_W = 2;
const int32_t NHWC_DIM_C = 3;

const int32_t HWCN_DIM_H = 0;
const int32_t HWCN_DIM_W = 1;
const int32_t HWCN_DIM_C = 2;
const int32_t HWCN_DIM_N = 3;

const int32_t CHWN_DIM_C = 0;
const int32_t CHWN_DIM_H = 1;
const int32_t CHWN_DIM_W = 2;
const int32_t CHWN_DIM_N = 3;

const int32_t NDHWC_DIM_N = 0;
const int32_t NDHWC_DIM_D = 1;
const int32_t NDHWC_DIM_H = 2;
const int32_t NDHWC_DIM_W = 3;
const int32_t NDHWC_DIM_C = 4;
const uint32_t DIMENSION_NUM_FIVE = 5;

const int32_t NCDHW_DIM_N = 0;
const int32_t NCDHW_DIM_C = 1;
const int32_t NCDHW_DIM_D = 2;
const int32_t NCDHW_DIM_H = 3;
const int32_t NCDHW_DIM_W = 4;

const int32_t DHWCN_DIM_D = 0;
const int32_t DHWCN_DIM_H = 1;
const int32_t DHWCN_DIM_W = 2;
const int32_t DHWCN_DIM_C = 3;
const int32_t DHWCN_DIM_N = 4;

const int32_t DHWNC_DIM_D = 0;
const int32_t DHWNC_DIM_H = 1;
const int32_t DHWNC_DIM_W = 2;
const int32_t DHWNC_DIM_N = 3;
const int32_t DHWNC_DIM_C = 4;
struct BiasOptimizeEdges {
  ge::InDataAnchorPtr quant_scale;
  ge::InDataAnchorPtr quant_offset;
  ge::InDataAnchorPtr cube_weight;
  ge::InDataAnchorPtr cube_bias;
  ge::InDataAnchorPtr deq_scale;
  bool isValid() {
    return !(cube_weight == nullptr || cube_bias == nullptr || deq_scale == nullptr);
  }
};
using TensorPtr = std::shared_ptr<ge::GeTensor>;
class QuantUtilImpl {
 public:
  static Status BiasOptimizeByEdge(BiasOptimizeEdges &param, std::vector<ge::NodePtr> &fusion_nodes);
  static Status BiasOptimizeByEdge(ge::NodePtr &quant_node, BiasOptimizeEdges &param,
                                   std::vector<ge::NodePtr> &fusion_nodes);
  static Status InsertFixpipeDequantScaleConvert(ge::InDataAnchorPtr deq_scale, std::vector<ge::NodePtr> &fusion_nodes);

 private:
  static Status BiasOptimizeByEdgeCommon(ge::NodePtr &quant_node, BiasOptimizeEdges &param,
                                         std::vector<ge::NodePtr> &fusion_nodes);
  static bool NeedBiasInput(const ge::InDataAnchorPtr &bias);
  static Status GetCoValueByWeight(ge::NodePtr &cube_node, const size_t &idx, int64_t &co);
  static Status PadShapeTo4Dim(const ge::Format &filter_format, const std::vector<int64_t> &filter_dims,
                               std::vector<int64_t> &filter_dims4_d);
  static int32_t GetAxisIndexByFormat(const ge::Format &format, const string &axis);
  static TensorPtr CreateBiasTensor(const int64_t co);
  static ge::NodePtr CreateBiasNode(std::shared_ptr<ge::ComputeGraph> &graph, const ge::GeTensorPtr &bias_ptr,
                                    const std::string &cube_node_name);
  static Status UpdateBiasOutputDesc(const ge::NodePtr &cube_node, const ge::GeShape &shape, const ge::Format &format,
                                     const uint32_t index);
  static Status UpdateCubeInputDesc(const ge::NodePtr &cube_node, const ge::GeShape &shape, const ge::Format &format,
                                    const uint32_t index);
  static Status CreateBiasInput(std::shared_ptr<ge::ComputeGraph> &graph, ge::NodePtr &cube_node, const int64_t &co,
                                const size_t &bias_idx);
  static Status GetWeightConstNode(const ge::InDataAnchorPtr &weight, ge::NodePtr &weight_const_node,
                                   ge::NodePtr &ascend_weight_quant_node);
  static Status GetInputDescByAnchor(const ge::InDataAnchorPtr &in_data_anchor, ge::GeTensorDesc &tensor_desc);
  static void SetAttrsForBiasOptimizerOp(ge::OpDescPtr &op_desc, const ge::NodePtr &cube_node,
                                         const ge::NodePtr &ascend_weight_quant_node);
  static Status SetQuantScaleAndOffset(const ge::NodePtr &quant_node, const BiasOptimizeEdges &param,
                                       ge::OpDescPtr &host_op_desc);
  static Status LinkBiasOptimizeHostOp(const ge::NodePtr &quant_node, const ge::NodePtr &weight_const_node,
                                       const BiasOptimizeEdges &param, ge::NodePtr &host_op_node);
  static Status CreateBiasOptimizeHostCpuOp(std::shared_ptr<ge::ComputeGraph> &graph, const ge::NodePtr &quant_node,
                                            const BiasOptimizeEdges &param, const ge::NodePtr &weight_const_node,
                                            std::vector<ge::NodePtr> &fusion_nodes);
  static ge::OpDescPtr CreateDeqScaleHostOp(const std::string &op_name, const std::string &op_type,
                                            const ge::OpDescPtr &pre_op_desc, size_t index);
  static bool IsNanoSoc();
};
}  // namespace fe
#endif