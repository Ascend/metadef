#include "register/graph_optimizer/graph_fusion/fusion_quant_util.h"
#include "register/graph_optimizer/graph_fusion/fusion_quant_util_impl.h"

namespace fe {
Status QuantUtil::BiasOptimizeByEdge(ge::NodePtr &quant_node, BiasOptimizeEdges &param,
                                     std::vector<ge::NodePtr> &fusion_nodes) {
  return QuantUtilImpl::BiasOptimizeByEdge(quant_node, param, fusion_nodes);
}

Status QuantUtil::BiasOptimizeByEdge(BiasOptimizeEdges &param, std::vector<ge::NodePtr> &fusion_nodes) {
  return QuantUtilImpl::BiasOptimizeByEdge(param, fusion_nodes);
}

Status QuantUtil::InsertFixpipeDequantScaleConvert(ge::InDataAnchorPtr deq_scale,
    std::vector<ge::NodePtr> &fusion_nodes) {
  return QuantUtilImpl::InsertFixpipeDequantScaleConvert(deq_scale, fusion_nodes);
}
}  // namespace fe