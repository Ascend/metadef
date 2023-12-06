#include "register/graph_optimizer/graph_fusion/fusion_quant_util_impl.h"
#include "common/ge_common/string_util.h"
#include "common/ge_common/util.h"
#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_log.h"
#include "graph/ge_local_context.h"
#include "graph/node.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/type_utils.h"
#include "platform/platform_info.h"
#include "register/tensor_assign.h"

#include <ostream>
#include <string>

namespace fe {
const size_t kAicVersionSize = 3;
const std::string X1INPUTNAME = "x1";
const std::string ATTR_OFFSET = "offset";
const std::string ATTR_OUTDTYPE = "dequantouttype";
const std::string BIAS_OPTIMIZATION_BIAS = "cube_optimization_bias";
const std::string BIAS_OPTIMIZATION_FILTER = "cube_optimization_filter";
const std::string BIAS_OPTIMIZATION_DEQUANT_SCALE = "dequant_scale";
const std::string BIAS_OPTIMIZATION_OUTPUT = "cube_quant_roll_back_output";
const size_t BIAS_OPT_OP_OFFSET_IDX = 3;
const size_t BIAS_OPT_OP_SCALE_IDX = 4;
const size_t IDX_2 = 2;
constexpr char const *kAttrSingleOp = "_is_single_op";
const bool ATTRTRUE = true;
constexpr const char *kSocVersionAscend035 = "Ascend035";
const std::string SOC_VERSION = "ge.socVersion";
const std::unordered_set<std::string> kNanoSocVersionSet = {kSocVersionAscend035};
// maps aic version to ISA arch VERSION
const std::map<std::string, std::string> kAicIsaArchVersionMap{{"100", "v100"}, {"200", "v200"}, {"202", "v200"},
                                                               {"210", "v200"}, {"220", "v220"}, {"300", "v300"},
                                                               {"310", "v300"}, {"350", "v350"}};

const std::set<std::string> kNeedAddBiasWithWeightNd = {"FFN"};

const std::map<ge::Format, std::map<std::string, int32_t>> AXIS_INDEX_OF_FORMAT = {
    {ge::Format::FORMAT_NCHW, {{"N", NCHW_DIM_N}, {"C", NCHW_DIM_C}, {"H", NCHW_DIM_H}, {"W", NCHW_DIM_W}}},
    {ge::Format::FORMAT_HWCN, {{"N", HWCN_DIM_N}, {"C", HWCN_DIM_C}, {"H", HWCN_DIM_H}, {"W", HWCN_DIM_W}}},
    {ge::Format::FORMAT_NHWC, {{"N", NHWC_DIM_N}, {"C", NHWC_DIM_C}, {"H", NHWC_DIM_H}, {"W", NHWC_DIM_W}}},
    {ge::Format::FORMAT_CHWN, {{"N", CHWN_DIM_N}, {"C", CHWN_DIM_C}, {"H", CHWN_DIM_H}, {"W", CHWN_DIM_W}}},
    {ge::Format::FORMAT_NDHWC,
     {{"N", NDHWC_DIM_N}, {"C", NDHWC_DIM_C}, {"H", NDHWC_DIM_H}, {"W", NDHWC_DIM_W}, {"D", NDHWC_DIM_D}}},
    {ge::Format::FORMAT_NCDHW,
     {{"N", NCDHW_DIM_N}, {"C", NCDHW_DIM_C}, {"H", NCDHW_DIM_H}, {"W", NCDHW_DIM_W}, {"D", NCDHW_DIM_D}}},
    {ge::Format::FORMAT_DHWCN,
     {{"N", DHWCN_DIM_N}, {"C", DHWCN_DIM_C}, {"H", DHWCN_DIM_H}, {"W", DHWCN_DIM_W}, {"D", DHWCN_DIM_D}}},
    {ge::Format::FORMAT_DHWNC,
     {{"N", DHWNC_DIM_N}, {"C", DHWNC_DIM_C}, {"H", DHWNC_DIM_H}, {"W", DHWNC_DIM_W}, {"D", DHWNC_DIM_D}}}};

const std::set<std::string> kRootGraphData = {"Data", "RefData"};

uint64_t GetHostCpuAtomicId() {
  static std::atomic<uint64_t> global_trans_atomic_id(0);
  return global_trans_atomic_id.fetch_add(1, std::memory_order_relaxed);
}

uint64_t GetBiasNodeAtomicId() {
  static std::atomic<uint64_t> global_bias_node_atomic_id(0);
  return global_bias_node_atomic_id.fetch_add(1, std::memory_order_relaxed);
}

void GetIsaArchVersionStr(std::string &isa_version) {
  PlatFormInfos platform_infos;
  OptionalInfos optional_infos;
  if (PlatformInfoManager::Instance().GetPlatformInfoWithOutSocVersion(platform_infos, optional_infos) != SUCCESS) {
    GELOGE(ge::FAILED, "Get platform info failed.");
    return;
  }

  // short soc version
  std::string short_soc_version;
  if (!platform_infos.GetPlatformRes("version", "Short_SoC_version", short_soc_version) || short_soc_version.empty()) {
    GELOGE(ge::FAILED, "Get short soc version failed.");
    return;
  }
  GELOGD("Short soc version is [%s].", short_soc_version.c_str());

  // aic version, ISAArchVersion
  std::string aic_version_str;
  if (!platform_infos.GetPlatformRes("version", "AIC_version", aic_version_str) || aic_version_str.empty()) {
    GELOGE(ge::FAILED, "Aic version of [%s] is empty.", short_soc_version.c_str());
    return;
  }
  GELOGD("Aic version of [%s] is [%s].", short_soc_version.c_str(), aic_version_str.c_str());
  std::vector<string> aic_version_vec = ge::StringUtils::Split(aic_version_str, '-');
  if (aic_version_vec.size() < kAicVersionSize) {
    GELOGE(ge::FAILED, "The aic version[%s] is invalid.", aic_version_str.c_str());
    return;
  }
  auto iter = kAicIsaArchVersionMap.find(aic_version_vec[2]);
  if (iter != kAicIsaArchVersionMap.end()) {
    isa_version = iter->second;
  }
}

bool QuantUtilImpl::NeedBiasInput(const ge::InDataAnchorPtr &bias) {
  auto peer_anchor = bias->GetPeerOutAnchor();
  if (peer_anchor == nullptr) {
    return true;
  }
  auto bias_node = peer_anchor->GetOwnerNode();
  return bias_node == nullptr;
}

Status QuantUtilImpl::PadShapeTo4Dim(const ge::Format &filter_format, const std::vector<int64_t> &filter_dims,
                                     std::vector<int64_t> &filter_dims4_d) {
  size_t size_of_filter = filter_dims.size();
  GELOGD("sizeOfFilter is %zu", size_of_filter);
  for (size_t i = 0; i <= BIAS_OPT_OP_OFFSET_IDX; i++) {
    if (i < size_of_filter) {
      GELOGD("dim [%zu] is %ld", i, filter_dims.at(i));
      filter_dims4_d.emplace_back(filter_dims.at(i));
    } else {
      if (filter_format == ge::Format::FORMAT_NCHW) {
        filter_dims4_d.emplace_back(1);
      } else if (filter_format == ge::Format::FORMAT_HWCN) {
        filter_dims4_d.insert(filter_dims4_d.cbegin(), 1);
      } else if (filter_format == ge::Format::FORMAT_NHWC) {
        filter_dims4_d.insert(filter_dims4_d.cbegin() + 1, 1);
      } else if (filter_format == ge::Format::FORMAT_ND) {
        filter_dims4_d.emplace_back(0);
      } else {
        GELOGE(ge::FAILED, "[GraphOpt][Quant][PadShpTo4Dim] format %s can not pad shape.",
               ge::TypeUtils::FormatToSerialString(filter_format).c_str());
        return FAILED;
      }
    }
  }

  if (!filter_dims4_d.empty() && filter_dims4_d.size() >= BIAS_OPT_OP_OFFSET_IDX) {
    GELOGD("Quant bias optimize, weight shape is %s:[%ld %ld %ld %ld].",
           ge::TypeUtils::FormatToSerialString(filter_format).c_str(), filter_dims4_d[NCHW_DIM_N],
           filter_dims4_d[NCHW_DIM_C], filter_dims4_d[NCHW_DIM_H], filter_dims4_d[NCHW_DIM_W]);
  }
  return SUCCESS;
}

int32_t QuantUtilImpl::GetAxisIndexByFormat(const ge::Format &format, const string &axis) {
  auto iter = AXIS_INDEX_OF_FORMAT.find(format);
  if (iter != AXIS_INDEX_OF_FORMAT.end()) {
    auto iter2 = iter->second.find(axis);
    if (iter2 != iter->second.end()) {
      return iter2->second;
    } else {
      GELOGW("Do not support this axis %s", axis.c_str());
      return -1;
    }
  } else {
    GELOGW("Do not support this format %s", ge::TypeUtils::FormatToSerialString(format).c_str());
    return -1;
  }
}

inline Status CheckInt64MulOverflow(int64_t m, int64_t n) {
  if (m > 0) {
    if (n > 0) {
      if (m > (static_cast<int64_t>(INT64_MAX) / n)) {
        return FAILED;
      }
    } else {
      if (n < (static_cast<int64_t>(INT64_MIN) / m)) {
        return FAILED;
      }
    }
  } else {
    if (n > 0) {
      if (m < (static_cast<int64_t>(INT64_MIN) / n)) {
        return FAILED;
      }
    } else {
      if ((m != 0) && (n < (static_cast<int64_t>(INT64_MAX) / m))) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status QuantUtilImpl::GetCoValueByWeight(ge::NodePtr &cube_node, const size_t &idx, std::vector<int64_t> &bias_shape) {
  std::vector<int64_t> filter_dims4_d;
  ge::Format filter_format =
      static_cast<ge::Format>(ge::GetPrimaryFormat(cube_node->GetOpDesc()->MutableInputDesc(idx)->GetFormat()));
  auto filter_shape = cube_node->GetOpDesc()->MutableInputDesc(idx)->MutableShape();

  if (filter_format == ge::FORMAT_ND && kNeedAddBiasWithWeightNd.count(cube_node->GetType()) != 0) {
    auto filter_dims = filter_shape.GetDims();
    if (filter_dims.size() == 2) {  // current only support 2D weight
      bias_shape.emplace_back(filter_dims[1]);
    }

    if (filter_dims.size() == kAicVersionSize) {
      bias_shape.emplace_back(filter_dims[0]);
      bias_shape.emplace_back(filter_dims[IDX_2]);
    }
    return SUCCESS;
  }
  if (filter_format != ge::FORMAT_ND) {
    int64_t groups = 1;
    (void) ge::AttrUtils::GetInt(cube_node->GetOpDesc(), "groups", groups);
    (void) PadShapeTo4Dim(filter_format, filter_shape.GetDims(), filter_dims4_d);
    if (filter_dims4_d.empty()) {
      GELOGE(ge::FAILED, "[GraphOpt][AvgPolQntPcsFus][GetCoVal] Node[%s] filter_dims4_d is empty.",
             cube_node->GetName().c_str());
      return FAILED;
    }
    int64_t index_co = GetAxisIndexByFormat(filter_format, "C");
    if (index_co < 0) {
      GELOGE(ge::FAILED, "[GraphOpt][AvgPolQntPcsFus][GetCoVal] Node[%s] index_co is negative, Check filter_format.",
             cube_node->GetName().c_str());
      return FAILED;
    }
    if (index_co >= static_cast<int64_t>(filter_dims4_d.size())) {
      GELOGE(ge::FAILED,
             "[GraphOpt][AvgPolQntPcsFus][GetCoVal] Node[%s] index_co is larger than the size of filter dims.",
             cube_node->GetName().c_str());
      return FAILED;
    }
    if (CheckInt64MulOverflow(filter_dims4_d[index_co], groups)) {
      return FAILED;
    }
    bias_shape.emplace_back(filter_dims4_d[index_co] * groups);
  }
  return SUCCESS;
}

TensorPtr QuantUtilImpl::CreateBiasTensor(const std::vector<int64_t> &shape) {
  int64_t size = 1;
  for (auto dim : shape) {
    size *= dim;
  }
  std::unique_ptr<int32_t[]> bias_data_temp(new (std::nothrow) int32_t[size]());
  for (int64_t i = 0; i < size; i++) {
    bias_data_temp[i] = 0;
  }

  ge::GeTensorDesc tmp_desc;
  ge::GeTensorPtr bias_ptr = nullptr;
  GE_MAKE_SHARED(bias_ptr = std::make_shared<ge::GeTensor>(tmp_desc, reinterpret_cast<uint8_t *>(bias_data_temp.get()),
                                                           size * sizeof(int32_t)),
                 return nullptr);

  ge::GeShape bias_shape(shape);
  bias_ptr->MutableTensorDesc().SetShape(bias_shape);
  bias_ptr->MutableTensorDesc().SetDataType(ge::DT_INT32);
  Status ret = bias_ptr->SetData(reinterpret_cast<uint8_t *>(bias_data_temp.get()), size * sizeof(int32_t));
  if (ret != SUCCESS) {
    GELOGW("set bias data failed!");
    return nullptr;
  }
  return bias_ptr;
}

ge::NodePtr QuantUtilImpl::CreateBiasNode(std::shared_ptr<ge::ComputeGraph> &graph, const ge::GeTensorPtr &bias_ptr,
                                          const std::string &cube_node_name) {
  ge::OpDescPtr const_opdesc = ge::OpDescUtils::CreateConstOp(bias_ptr);
  if (const_opdesc == nullptr) {
    GELOGE(ge::FAILED, "const_opdesc nullptr");
    return nullptr;
  }
  std::ostringstream oss;
  oss << cube_node_name << "_quant_bias" << GetBiasNodeAtomicId();
  oss.flush();
  const_opdesc->SetName(oss.str());
  ge::NodePtr const_node = graph->AddNode(const_opdesc);
  return const_node;
}

Status QuantUtilImpl::UpdateBiasOutputDesc(const ge::NodePtr &cube_node, const ge::GeShape &shape,
                                           const ge::Format &format, const uint32_t index) {
  ge::NodePtr bias_node = cube_node->GetInDataAnchor(index)->GetPeerOutAnchor()->GetOwnerNode();
  ge::OpDescPtr bias_op_desc = bias_node->GetOpDesc();
  // only has one output, index 0
  ge::GeTensorDesc bias_output_desc = bias_op_desc->GetOutputDesc(0);
  bias_output_desc.SetShape(shape);
  bias_output_desc.SetOriginFormat(format);
  bias_output_desc.SetOriginShape(shape);
  bias_output_desc.SetOriginDataType(ge::DT_INT32);
  bias_output_desc.SetDataType(ge::DT_INT32);
  if (bias_op_desc->UpdateOutputDesc(0, bias_output_desc) != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "bias_op_desc faild");
    return FAILED;
  }
  return SUCCESS;
}

Status QuantUtilImpl::UpdateCubeInputDesc(const ge::NodePtr &cube_node, const ge::GeShape &shape,
                                          const ge::Format &format, const uint32_t index) {
  ge::GeTensorDesc bias_desc = cube_node->GetOpDesc()->GetInputDesc(index);
  bias_desc.SetShape(shape);
  bias_desc.SetOriginFormat(format);
  bias_desc.SetOriginShape(shape);
  bias_desc.SetOriginDataType(ge::DT_INT32);
  bias_desc.SetDataType(ge::DT_INT32);
  if (cube_node->GetOpDesc()->UpdateInputDesc(index, bias_desc) != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "cube_node UpdateInputDesc");
    return FAILED;
  }
  return SUCCESS;
}

Status QuantUtilImpl::CreateBiasInput(std::shared_ptr<ge::ComputeGraph> &graph, ge::NodePtr &cube_node,
                                      const std::vector<int64_t> &shape, const size_t &bias_idx) {
  GELOGD("Node[name: %s, type: %s] has no bias, create bias and set data", cube_node->GetName().c_str(),
         cube_node->GetType().c_str());

  ge::GeTensorPtr bias_ptr = CreateBiasTensor(shape);
  if (bias_ptr == nullptr) {
    return FAILED;
  }
  ge::NodePtr const_node = CreateBiasNode(graph, bias_ptr, cube_node->GetName());
  if (const_node == nullptr) {
    GELOGE(ge::FAILED, "[GraphOpt][BiasQuantPass][CreateBiasInput] Fail to add const node.");
    return FAILED;
  }

  if (cube_node->AddLinkFrom(bias_idx, const_node) != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "[GraphOpt][BiasQuantPass][CreateBiasInput] Fail to link const node with node[%s, %s].",
           cube_node->GetName().c_str(), cube_node->GetType().c_str());
    return FAILED;
  }

  ge::GeShape bias_shape(shape);
  auto bias_input_desc = cube_node->GetOpDesc()->GetInputDesc(bias_idx);
  ge::Format input_desc0_origin_format = bias_input_desc.GetOriginFormat();
  if (UpdateBiasOutputDesc(cube_node, bias_shape, input_desc0_origin_format, bias_idx) != SUCCESS) {
    return FAILED;
  }
  if (UpdateCubeInputDesc(cube_node, bias_shape, input_desc0_origin_format, bias_idx) != SUCCESS) {

    return FAILED;
  }
  return SUCCESS;
}

// 基于weight input anchor 获取weight的input node
Status QuantUtilImpl::GetWeightConstNode(const ge::InDataAnchorPtr &weight, ge::NodePtr &weight_const_node,
                                         ge::NodePtr &ascend_weight_quant_node) {
  auto peer_out_anchor_of_weight = weight->GetPeerOutAnchor();
  if (peer_out_anchor_of_weight == nullptr) {
    GELOGE(ge::FAILED, "error 1");
    return FAILED;
  }

  auto weight_input_node = peer_out_anchor_of_weight->GetOwnerNode();
  if (weight_input_node == nullptr) {
    GELOGE(ge::FAILED, "error 2");
    return FAILED;
  }

  auto weight_input_node_first_input_anchor = weight_input_node->GetInDataAnchor(0);
  // if dynamic batch or dynamic shape, cube_weight_input_node will be a Data node
  if (weight_input_node_first_input_anchor == nullptr ||
      kRootGraphData.count(weight_input_node->GetOpDesc()->GetType()) != 0) {
    ascend_weight_quant_node = nullptr;
    weight_const_node = weight_input_node;
  } else {
    auto weight_const_out_anchor = weight_input_node_first_input_anchor->GetPeerOutAnchor();
    if (weight_const_out_anchor == nullptr) {
      GELOGE(ge::FAILED, "error 3");
      return FAILED;
    }
    weight_const_node = weight_const_out_anchor->GetOwnerNode();
    if (weight_const_node == nullptr) {
      GELOGE(ge::FAILED, "error 4");
      return FAILED;
    }
    ascend_weight_quant_node = weight_input_node;
  }
  return SUCCESS;
}

Status QuantUtilImpl::GetInputDescByAnchor(const ge::InDataAnchorPtr &in_data_anchor, ge::GeTensorDesc &tensor_desc) {
  auto owner_node = in_data_anchor->GetOwnerNode();
  size_t anchor_idx = in_data_anchor->GetIdx();
  if (owner_node == nullptr || anchor_idx >= owner_node->GetOpDesc()->GetAllInputsSize()) {
    return FAILED;
  }

  tensor_desc = owner_node->GetOpDesc()->GetInputDesc(anchor_idx);
  return SUCCESS;
}

void QuantUtilImpl::SetAttrsForBiasOptimizerOp(ge::OpDescPtr &op_desc, const ge::NodePtr &cube_node,
                                               const ge::NodePtr &ascend_weight_quant_node) {
  (void) ge::AttrUtils::SetBool(op_desc, "quant_cin_cout_reverse", false);
  int64_t groups = 1;
  (void) ge::AttrUtils::GetInt(cube_node->GetOpDesc(), "groups", groups);
  (void) ge::AttrUtils::SetInt(op_desc, "groups", groups);
  (void) ge::AttrUtils::SetBool(op_desc, "_is_come_from_const_op", true);
  (void) ge::AttrUtils::SetStr(op_desc, "cube_op_type", cube_node->GetType());
  std::string soc_version = "v100";
  GetIsaArchVersionStr(soc_version);
  (void) ge::AttrUtils::SetStr(op_desc, "soc_version", soc_version);
  int dst_type = ge::DT_INT8;
  if (ascend_weight_quant_node != nullptr) {
    (void) ge::AttrUtils::GetInt(ascend_weight_quant_node->GetOpDesc(), "dst_type", dst_type);
  }
  (void) ge::AttrUtils::SetInt(op_desc, "dst_type", dst_type);
}

Status QuantUtilImpl::SetQuantScaleAndOffset(const ge::NodePtr &quant_node, const BiasOptimizeEdges &param,
                                             ge::OpDescPtr &host_op_desc) {
  if (quant_node != nullptr) {
    // get scale and offset from quant node attr
    float_t scale_a = 0.0;
    (void) ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), "scale", scale_a);
    (void) ge::AttrUtils::SetFloat(host_op_desc, "scale", scale_a);

    float_t offset = 0.0;
    (void) ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), "offset", offset);
    (void) ge::AttrUtils::SetFloat(host_op_desc, "offset", offset);
    return SUCCESS;
  }
  if (param.quant_offset == nullptr || param.quant_scale == nullptr) {
    GELOGE(ge::FAILED, , "Invalid param! Quant_offset anchor and quant_scale anchor should not be nullptr,\
      please check in detail.");
    return FAILED;
  }
  ge::GeTensorDesc quant_offset_tensor;
  if (GetInputDescByAnchor(param.quant_offset, quant_offset_tensor) != SUCCESS) {
    return FAILED;
  }
  host_op_desc->AddInputDesc("offset", quant_offset_tensor);

  ge::GeTensorDesc quant_scale_tensor;
  if (GetInputDescByAnchor(param.quant_scale, quant_scale_tensor) != SUCCESS) {
    return FAILED;
  }
  host_op_desc->AddInputDesc("scale", quant_scale_tensor);

  return SUCCESS;
}

// 改图，区分有没有quant_offset 和 quant_scale输入的场景
Status QuantUtilImpl::LinkBiasOptimizeHostOp(const ge::NodePtr &quant_node, const ge::NodePtr &weight_const_node,
                                             const BiasOptimizeEdges &param, ge::NodePtr &host_op_node) {
  // input index  bias:dequant_scale:weight:quant_offset:quant_scale
  // bias need delete ori link
  auto bias_peer_out_anchor = param.cube_bias->GetPeerOutAnchor();
  if (bias_peer_out_anchor == nullptr) {
    return FAILED;
  }
  if (ge::GraphUtils::RemoveEdge(bias_peer_out_anchor, param.cube_bias) != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "[GraphOpt][CreateHostOp][LinkHostOpEdge] Remove Edge between bias output\
      and cube input anchor failed.");
    return FAILED;
  }

  if (ge::GraphUtils::AddEdge(bias_peer_out_anchor, host_op_node->GetInDataAnchor(0)) != SUCCESS) {
    GELOGE(ge::FAILED, "add edge between bias peer out anchor and host op failed");
    return FAILED;
  }

  // weight
  auto weight_out_anchor = weight_const_node->GetOutDataAnchor(0);
  if (ge::GraphUtils::AddEdge(weight_out_anchor, host_op_node->GetInDataAnchor(1)) != SUCCESS) {
    GELOGE(ge::FAILED, "add edge between weight peer out anchor and host op failed");
    return FAILED;
  }

  // dequant_scale
  auto deq_scale_peer_out_anchor = param.deq_scale->GetPeerOutAnchor();
  if (deq_scale_peer_out_anchor == nullptr) {
    return FAILED;
  }
  if (ge::GraphUtils::AddEdge(deq_scale_peer_out_anchor, host_op_node->GetInDataAnchor(IDX_2)) != SUCCESS) {
    GELOGE(ge::FAILED, "add edge between dequant_scale peer out anchor and host op failed");
    return FAILED;
  }

  // quant_offset
  if (quant_node == nullptr && param.quant_offset != nullptr) {
    auto quant_offset_peer_out_anchor = param.quant_offset->GetPeerOutAnchor();
    if (quant_offset_peer_out_anchor == nullptr ||
        host_op_node->GetAllInDataAnchorsSize() < BIAS_OPT_OP_SCALE_IDX + 1) {
      GELOGE(ge::FAILED, "Quant_offset_peer_out_anchor is nullptr or invalid anchor size %u",
             host_op_node->GetAllInDataAnchorsSize());
      return FAILED;
    }
    if (ge::GraphUtils::AddEdge(quant_offset_peer_out_anchor, host_op_node->GetInDataAnchor(BIAS_OPT_OP_OFFSET_IDX)) !=
        SUCCESS) {
      GELOGE(ge::FAILED, "add edge between quant_offset peer out anchor and host op failed");
      return FAILED;
    }
  }
  // quant_scale
  if (quant_node == nullptr && param.quant_scale != nullptr) {
    auto quant_scale_peer_out_anchor = param.quant_scale->GetPeerOutAnchor();
    if (quant_scale_peer_out_anchor == nullptr) {
      return FAILED;
    }
    if (ge::GraphUtils::AddEdge(quant_scale_peer_out_anchor, host_op_node->GetInDataAnchor(BIAS_OPT_OP_SCALE_IDX)) !=
        SUCCESS) {
      GELOGE(ge::FAILED, "add edge between quant_scale peer out anchor and host op failed");
      return FAILED;
    }
  }

  // output
  auto quant_host_cpu_output_anchor = host_op_node->GetOutDataAnchor(0);
  if (ge::GraphUtils::AddEdge(quant_host_cpu_output_anchor, param.cube_bias) != SUCCESS) {
    GELOGE(ge::FAILED, "add edge between quant_scale peer out anchor and host op failed");
    return FAILED;
  }

  return SUCCESS;
}

Status QuantUtilImpl::CreateBiasOptimizeHostCpuOp(std::shared_ptr<ge::ComputeGraph> &graph,
                                                  const ge::NodePtr &quant_node, const BiasOptimizeEdges &param,
                                                  const ge::NodePtr &weight_const_node,
                                                  std::vector<ge::NodePtr> &fusion_nodes) {
  // create host cpu op desc
  std::ostringstream oss;
  oss << "QuantBiasOptimization" << GetHostCpuAtomicId();
  oss.flush();

  auto bias_optimizer_op_desc = std::make_shared<ge::OpDesc>(oss.str().c_str(), "QuantBiasOptimization");
  if (bias_optimizer_op_desc == nullptr) {
    GELOGE(ge::FAILED, "error 1");
    return FAILED;
  }
  // construct bias and deq_scale tensor
  ge::GeTensorDesc bias_tensor_desc;
  if (GetInputDescByAnchor(param.cube_bias, bias_tensor_desc) != SUCCESS) {
    return FAILED;
  }
  bias_optimizer_op_desc->AddInputDesc(BIAS_OPTIMIZATION_BIAS, bias_tensor_desc);
  ge::GeTensorDesc deq_scale_tensor_desc;
  if (GetInputDescByAnchor(param.deq_scale, deq_scale_tensor_desc) != SUCCESS) {
    return FAILED;
  }
  // get weight tensor desc
  ge::GeTensorDesc weight_tensor_desc = weight_const_node->GetOpDesc()->GetOutputDesc(0);
  bias_optimizer_op_desc->AddInputDesc(BIAS_OPTIMIZATION_FILTER, weight_tensor_desc);
  bias_optimizer_op_desc->AddInputDesc(BIAS_OPTIMIZATION_DEQUANT_SCALE, deq_scale_tensor_desc);

  // get offset and scale form quant attr or input
  if (SetQuantScaleAndOffset(quant_node, param, bias_optimizer_op_desc) != SUCCESS) {
    GELOGE(ge::FAILED, "error SetQuantScaleAndOffset");
    return FAILED;
  }
  // modify host cpu op input desc
  for (uint32_t i = 0; i < bias_optimizer_op_desc->GetAllInputsSize(); ++i) {
    auto tensor_desc_ptr = bias_optimizer_op_desc->MutableInputDesc(i);
    if (tensor_desc_ptr == nullptr) {
      GELOGI("Tensor_desc_ptr is null.");
      continue;
    }
    /* Keep the original data type and format the same as the current ones */
    tensor_desc_ptr->SetOriginDataType(tensor_desc_ptr->GetDataType());
    tensor_desc_ptr->SetOriginFormat(static_cast<ge::Format>(ge::GetPrimaryFormat(tensor_desc_ptr->GetFormat())));
    tensor_desc_ptr->SetOriginShape(tensor_desc_ptr->GetShape());
  }
  // add output desc
  bias_optimizer_op_desc->AddOutputDesc(BIAS_OPTIMIZATION_OUTPUT, bias_tensor_desc);
  bias_optimizer_op_desc->MutableOutputDesc(0)->SetOriginFormat(
      static_cast<ge::Format>(ge::GetPrimaryFormat(bias_tensor_desc.GetFormat())));
  bias_optimizer_op_desc->MutableOutputDesc(0)->SetOriginDataType(bias_tensor_desc.GetDataType());
  bias_optimizer_op_desc->MutableOutputDesc(0)->SetOriginShape(bias_tensor_desc.GetShape());

  SetAttrsForBiasOptimizerOp(bias_optimizer_op_desc, param.deq_scale->GetOwnerNode(), weight_const_node);
  // create host op node
  auto bias_optimizer_node = graph->AddNode(bias_optimizer_op_desc);
  if (bias_optimizer_node == nullptr) {
    return FAILED;
  }
  fusion_nodes.emplace_back(bias_optimizer_node);

  // modify host op edge
  if (LinkBiasOptimizeHostOp(quant_node, weight_const_node, param, bias_optimizer_node)) {
    return FAILED;
  }

  return SUCCESS;
}
Status QuantUtilImpl::BiasOptimizeByEdgeCommon(ge::NodePtr &quant_node, BiasOptimizeEdges &param,
                                               std::vector<ge::NodePtr> &fusion_nodes) {
  if (!param.isValid()) {
    GELOGE(ge::FAILED, "param check failed, input param is invalid");
    return FAILED;
  }
  auto cube_node = param.cube_weight->GetOwnerNode();
  auto graph = cube_node->GetOwnerComputeGraph();
  if (NeedBiasInput(param.cube_bias)) {
    GELOGD("start create bias node for node %s", cube_node->GetNamePtr());
    std::vector<int64_t> bias_shape;
    if (GetCoValueByWeight(cube_node, param.cube_weight->GetIdx(), bias_shape) != SUCCESS) {
      GELOGE(ge::FAILED, "[GraphOpt][AvgPolQntPcsFus][BiasOpti] Get node[%s] co value.", cube_node->GetName().c_str());
      return FAILED;
    }
    GELOGD("start create bias input for node %s", cube_node->GetNamePtr());
    if (CreateBiasInput(graph, cube_node, bias_shape, param.cube_bias->GetIdx()) != SUCCESS) {
      GELOGE(ge::FAILED, "[GraphOpt][CreateBiasInput][BiasOpti] Get node[%s] co value.", cube_node->GetName().c_str());
      return FAILED;
    }
  }
  ge::NodePtr weight_const_node = nullptr;
  ge::NodePtr ascend_weight_quant_node = nullptr;
  if (GetWeightConstNode(param.cube_weight, weight_const_node, ascend_weight_quant_node) != SUCCESS) {
    GELOGE(ge::FAILED,
           "[OriginGraphOptimize][GraphFusion][QuantOpt] Get weight const from node[%s, %s] failed, please check graph",
           cube_node->GetName().c_str(), cube_node->GetType().c_str());
  }
  Status ret = CreateBiasOptimizeHostCpuOp(graph, quant_node, param, weight_const_node, fusion_nodes);
  if (ret != SUCCESS) {
    GELOGE(ge::FAILED, "[OriginGraphOptimize][GraphFusion][QuantOpt] Create host op failed.");
    return FAILED;
  }
  return SUCCESS;
}

Status QuantUtilImpl::BiasOptimizeByEdge(ge::NodePtr &quant_node, BiasOptimizeEdges &param,
                                         std::vector<ge::NodePtr> &fusion_nodes) {
  if (quant_node == nullptr) {
    GELOGE(ge::FAILED,
           "[OriginGraphOptimize][GraphFusion][QuantOpt] Invalid parameter, quant node should not be nullptr!");
    return FAILED;
  }
  return BiasOptimizeByEdgeCommon(quant_node, param, fusion_nodes);
}

Status QuantUtilImpl::BiasOptimizeByEdge(BiasOptimizeEdges &param, std::vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr quant_node = nullptr;
  return BiasOptimizeByEdgeCommon(quant_node, param, fusion_nodes);
}

bool QuantUtilImpl::IsNanoSoc() {
  std::string soc_version_str;
  if (ge::GetThreadLocalContext().GetOption(SOC_VERSION, soc_version_str) != ge::GRAPH_SUCCESS) {
    GELOGD("get option %s failed.", SOC_VERSION.c_str());
    return false;
  }
  GELOGD("Option %s is %s.", SOC_VERSION.c_str(), soc_version_str.c_str());
  return kNanoSocVersionSet.count(soc_version_str) != 0;
}

ge::OpDescPtr QuantUtilImpl::CreateDeqScaleHostOp(const std::string &op_name, const std::string &op_type,
                                                  const ge::OpDescPtr &cube_node, size_t index) {
  GELOGD("Begin to create SetQuantScale Host op[%s, %s].", op_name.c_str(), op_type.c_str());
  // create set quant scale host op
  ge::OpDescPtr op_desc = nullptr;
  GE_MAKE_SHARED(op_desc = std::make_shared<ge::OpDesc>(op_name, op_type), return nullptr);

  ge::ConstGeTensorDescPtr prenode_inputdesc = cube_node->GetInputDescPtr(index);
  ge::ConstGeTensorDescPtr prenode_outputdesc = cube_node->GetOutputDescPtr(0);
  if (prenode_inputdesc == nullptr || prenode_outputdesc == nullptr) {
    return nullptr;
  }

  ge::GeTensorDesc out_tensor_desc = prenode_inputdesc->Clone();
  out_tensor_desc.SetDataType(ge::DT_UINT64);
  out_tensor_desc.SetOriginDataType(ge::DT_UINT64);
  op_desc->AddInputDesc(X1INPUTNAME, *(prenode_inputdesc));
  op_desc->AddOutputDesc("y", out_tensor_desc);

  // set attr
  float offset = 0.0;
  if (ge::AttrUtils::GetFloat(cube_node, ATTR_OFFSET, offset)) {
    (void) ge::AttrUtils::SetFloat(op_desc, ATTR_OFFSET, offset);
    GELOGD("Set offset value [%f] for op[%s]", offset, op_name.c_str());
  }
  (void) ge::AttrUtils::SetInt(op_desc, ATTR_OUTDTYPE, static_cast<int64_t>(prenode_outputdesc->GetDataType()));
  (void) ge::AttrUtils::SetBool(op_desc, kAttrSingleOp, ATTRTRUE);
  GELOGD("Host op[%s, %s] has been created.", op_desc->GetName().c_str(), op_desc->GetType().c_str());
  return op_desc;
}

Status QuantUtilImpl::InsertFixpipeDequantScaleConvert(ge::InDataAnchorPtr deq_scale,
                                                       std::vector<ge::NodePtr> &fusion_nodes) {
  if (deq_scale == nullptr) {
    return FAILED;
  }
  // get post_fuze_node
  ge::NodePtr post_fuze_node = deq_scale->GetOwnerNode();
  GE_CHECK_NOTNULL(post_fuze_node);
  // get pre_fuze_node
  ge::NodePtr pre_fuze_node = deq_scale->GetPeerOutAnchor()->GetOwnerNode();
  GE_CHECK_NOTNULL(pre_fuze_node);
  // get graph
  auto compute_graph = post_fuze_node->GetOwnerComputeGraph();
  // get deq_scale input index
  auto deq_scale_index = deq_scale->GetIdx();
  // set op_desc
  std::string new_op_type = QuantUtilImpl::IsNanoSoc() ? "SetQuantScale" : "SetM1Dequant";
  std::string new_op_name = post_fuze_node->GetName() + new_op_type + std::to_string(GetHostCpuAtomicId());
  ge::OpDescPtr new_op_desc =
      QuantUtilImpl::CreateDeqScaleHostOp(new_op_name, new_op_type, post_fuze_node->GetOpDesc(), deq_scale_index);
  GE_CHECK_NOTNULL(new_op_desc);
  ge::NodePtr new_node = compute_graph->AddNode(new_op_desc);
  GE_CHECK_NOTNULL(new_node);
  // edit edge
  fusion_nodes.push_back(new_node);
  ge::GraphUtils::RemoveEdge(deq_scale, deq_scale->GetPeerOutAnchor());
  ge::GraphUtils::AddEdge(pre_fuze_node->GetOutDataAnchor(0), new_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(new_node->GetOutDataAnchor(0), post_fuze_node->GetInDataAnchor(deq_scale_index));
  return SUCCESS;
}
}  // namespace fe