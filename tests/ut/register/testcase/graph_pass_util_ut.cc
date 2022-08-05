#include <gtest/gtest.h>
#define private public
#include "graph/ge_tensor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/op_desc.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/fusion_common/graph_pass_util.h"

using namespace std;
using namespace ge;

namespace fe {
class GraphPassUtilUT : public testing::Test {
protected:
  void SetUp() {}

  void TearDown() {}
};

bool CheckOriginName(const std::vector<ge::NodePtr> &nodes, std::string pass_name, std::vector<std::string> origin_names) {
  for (auto node : nodes) {
    auto op_desc = node->GetOpDesc();
    std::shared_ptr<std::unordered_map<std::string, std::vector<std::string>>> op_names_maps_tmp =
        std::make_shared<std::unordered_map<std::string, std::vector<std::string>>>();
    op_names_maps_tmp = op_desc->TryGetExtAttr(ge::ATTR_NAME_ORIGIN_OP_NAMES_MAP, op_names_maps_tmp);
    if (op_names_maps_tmp->find(pass_name) == op_names_maps_tmp->cend()) {
      return false;
    }
    auto names_in_vec = (*op_names_maps_tmp)[pass_name];
    if (names_in_vec.size() != origin_names.size()) {
      return false;
    }
    for (const auto &origin_name : origin_names) {
      if (std::find(names_in_vec.cbegin(), names_in_vec.cend(), origin_name) == names_in_vec.cend()) {
        return false;
      }
    }
  }
  return true;
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case1) {
  NodePtr origin_node = nullptr;
  NodePtr fusion_node = nullptr;
  GraphPassUtil::SetOutputDescAttr(0, 0, origin_node, fusion_node);
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case2) {
  OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
  OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "Relu");
  vector<int64_t> dim = {4, 4, 1, 4};
  GeShape shape(dim);
  GeTensorDesc tenosr_desc(shape, ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  tenosr_desc.SetOriginFormat(FORMAT_NCHW);
  tenosr_desc.SetOriginDataType(DT_FLOAT);
  relu1->AddInputDesc(tenosr_desc);
  relu1->AddOutputDesc(tenosr_desc);
  relu2->AddInputDesc(tenosr_desc);
  relu2->AddOutputDesc(tenosr_desc);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr relu1_node = graph->AddNode(relu1);
  NodePtr relu2_node = graph->AddNode(relu2);
  GraphPassUtil::SetOutputDescAttr(1, 0, relu1_node, relu2_node);
  EXPECT_EQ(relu2_node->GetOpDesc()->GetOutputDescPtr(0)->HasAttr(ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME), false);
  GraphPassUtil::SetOutputDescAttr(0, 1, relu1_node, relu2_node);
  EXPECT_EQ(relu2_node->GetOpDesc()->GetOutputDescPtr(0)->HasAttr(ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME), false);
  GraphPassUtil::SetOutputDescAttr(0, 0, relu1_node, relu2_node);
  EXPECT_EQ(relu2_node->GetOpDesc()->GetOutputDescPtr(0)->HasAttr(ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME), true);
  string origin_name;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME, origin_name);
  EXPECT_EQ(origin_name, "relu1");
  string origin_dtype;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_DATA_TYPE, origin_dtype);
  EXPECT_EQ(origin_dtype, "DT_FLOAT");
  string origin_format;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_FORMAT, origin_format);
  EXPECT_EQ(origin_format, "NCHW");
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case3) {
  OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
  OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "Relu");
  vector<int64_t> dim = {4, 4, 1, 4};
  GeShape shape(dim);
  GeTensorDesc tenosr_desc(shape, ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  tenosr_desc.SetOriginFormat(FORMAT_NCHW);
  tenosr_desc.SetOriginDataType(DT_FLOAT);
  relu1->AddInputDesc(tenosr_desc);
  relu1->AddOutputDesc(tenosr_desc);
  AttrUtils::SetStr(relu1->MutableOutputDesc(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME, "origin_relu1");
  AttrUtils::SetStr(relu1->MutableOutputDesc(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_DATA_TYPE, "DT_DOUBLE");
  AttrUtils::SetStr(relu1->MutableOutputDesc(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_FORMAT, "ND");
  relu2->AddInputDesc(tenosr_desc);
  relu2->AddOutputDesc(tenosr_desc);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr relu1_node = graph->AddNode(relu1);
  NodePtr relu2_node = graph->AddNode(relu2);

  GraphPassUtil::SetOutputDescAttr(0, 0, relu1_node, relu2_node);
  EXPECT_EQ(relu2_node->GetOpDesc()->GetOutputDescPtr(0)->HasAttr(ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME), true);
  string origin_name;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME, origin_name);
  EXPECT_EQ(origin_name, "origin_relu1");
  string origin_dtype;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_DATA_TYPE, origin_dtype);
  EXPECT_EQ(origin_dtype, "DT_DOUBLE");
  string origin_format;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_FORMAT, origin_format);
  EXPECT_EQ(origin_format, "ND");
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case4) {
  OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
  OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "Relu");
  vector<int64_t> dim = {4, 4, 1, 4};
  GeShape shape(dim);
  GeTensorDesc tenosr_desc(shape, ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  tenosr_desc.SetOriginFormat(FORMAT_NCHW);
  tenosr_desc.SetOriginDataType(DT_FLOAT);
  relu1->AddInputDesc(tenosr_desc);
  relu1->AddOutputDesc(tenosr_desc);
  vector<string> names = {"ori_rule1"};
  AttrUtils::SetListStr(relu1, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, names);
  AttrUtils::SetStr(relu1->MutableOutputDesc(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_DATA_TYPE, "RESERVED");
  AttrUtils::SetStr(relu1->MutableOutputDesc(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_FORMAT, "RESERVED");
  relu2->AddInputDesc(tenosr_desc);
  relu2->AddOutputDesc(tenosr_desc);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr relu1_node = graph->AddNode(relu1);
  NodePtr relu2_node = graph->AddNode(relu2);

  GraphPassUtil::SetOutputDescAttr(0, 0, relu1_node, relu2_node);
  EXPECT_EQ(relu2_node->GetOpDesc()->GetOutputDescPtr(0)->HasAttr(ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME), true);
  string origin_name;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME, origin_name);
  EXPECT_EQ(origin_name, "ori_rule1");
  string origin_dtype;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_DATA_TYPE, origin_dtype);
  EXPECT_EQ(origin_dtype, "DT_FLOAT");
  string origin_format;
  AttrUtils::GetStr(relu2->GetOutputDescPtr(0), ge::ATTR_NAME_DATA_DUMP_ORIGIN_FORMAT, origin_format);
  EXPECT_EQ(origin_format, "NCHW");
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case5) {
  vector<int64_t> dims = {1,2,3,4};
  std::string origin_data_type_str = "RESERVED";
  GeShape shape(dims);
  GeTensorDescPtr tensor_desc_ptr = std::make_shared<GeTensorDesc>(shape, FORMAT_NCHW, DT_FLOAT);
  tensor_desc_ptr->SetDataType((ge::DataType)24);
  (void)ge::AttrUtils::SetStr(tensor_desc_ptr, ge::ATTR_NAME_DATA_DUMP_ORIGIN_DATA_TYPE, "DT_DOUBLE");
  ge::DataType origin_dtype;
  origin_dtype = GraphPassUtil::GetDataDumpOriginDataType(tensor_desc_ptr);
  EXPECT_EQ(origin_dtype, (ge::DataType)11);
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case6) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::OpDescPtr op = std::make_shared<OpDesc>("test_op", "TestOp");
  auto node = graph->AddNode(op);

  std::map<std::string, ge::NodePtr> inner_map;
  inner_map["test"] = node;
  std::unordered_map<std::string, std::map<std::string, ge::NodePtr>> node_map;
  node_map["test"] = inner_map;

  NodeTypeMapPtr node_type_map = std::make_shared<NodeTypeMap>(node_map);
  GraphPassUtil::AddNodeToNodeTypeMap(node_type_map, "test", node);
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case7) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::OpDescPtr op = std::make_shared<OpDesc>("test_op", "TestOp");
  auto node = graph->AddNode(op);

  std::map<std::string, ge::NodePtr> inner_map;
  inner_map["test"] = node;
  std::unordered_map<std::string, std::map<std::string, ge::NodePtr>> node_map;
  node_map["test"] = inner_map;

  NodeTypeMapPtr node_type_map = std::make_shared<NodeTypeMap>(node_map);
  GraphPassUtil::RemoveNodeFromNodeTypeMap(node_type_map, "test", node);
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case8) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::OpDescPtr op = std::make_shared<OpDesc>("test_op", "TestOp");
  auto node = graph->AddNode(op);

  std::map<std::string, ge::NodePtr> inner_map;
  inner_map["test"] = node;
  std::unordered_map<std::string, std::map<std::string, ge::NodePtr>> node_map;
  node_map["test"] = inner_map;

  NodeTypeMapPtr node_type_map = std::make_shared<NodeTypeMap>(node_map);
  vector<ge::NodePtr> nodes;
  GraphPassUtil::GetNodesFromNodeTypeMap(node_type_map, "test", nodes);
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case9) {
  vector<int64_t> dims = {1,2,3,4};
  std::string origin_data_type_str = "RESERVED";
  GeShape shape(dims);
  GeTensorDescPtr tensor_desc_ptr = std::make_shared<GeTensorDesc>(shape, FORMAT_NCHW, DT_FLOAT);
  tensor_desc_ptr->SetDataType((ge::DataType)24);
  (void)ge::AttrUtils::SetStr(tensor_desc_ptr, ge::ATTR_NAME_DATA_DUMP_ORIGIN_FORMAT, "NCHW");
  ge::Format origin_format;
  origin_format = GraphPassUtil::GetDataDumpOriginFormat(tensor_desc_ptr);
  EXPECT_EQ(origin_format, (ge::Format)0);
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case10) {
  putenv("DUMP_GE_GRAPH=2");
  OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
  vector<int64_t> dim = {4, 4, 1, 4};
  GeShape shape(dim);
  GeTensorDesc tenosr_desc(shape, ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  tenosr_desc.SetOriginFormat(FORMAT_NCHW);
  tenosr_desc.SetOriginDataType(DT_FLOAT);
  relu1->AddInputDesc(tenosr_desc);
  relu1->AddOutputDesc(tenosr_desc);
  vector<string> names = {"ori_rule1"};
  std::shared_ptr<std::unordered_map<std::string, std::vector<std::string>>> op_names_maps_tmp =
      std::make_shared<GraphPassUtil::UnorderedMapping>();
  std::vector<std::string> origin_op_names_vec = {"nodeA, nodeB"};
  op_names_maps_tmp->insert(std::pair<std::string, std::vector<std::string>>("pass_test", origin_op_names_vec));
  (void)relu1->SetExtAttr(ge::ATTR_NAME_ORIGIN_OP_NAMES_MAP, op_names_maps_tmp);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr relu1_node = graph->AddNode(relu1);
  std::vector<ge::NodePtr> original_nodes = {relu1_node};
  std::vector<ge::NodePtr> fus_nodes = {relu1_node};
  std::vector<std::string> origin_op_names = {"relu"};
  GraphPassUtil::RecordPassnameAndOriginalNames(original_nodes, fus_nodes, "passA");
  std::vector<std::string> origin_op_names_to_check = {"nodeA", "nodeB"};
  CheckOriginName(fus_nodes, "passA", origin_op_names_to_check);
}

TEST_F(GraphPassUtilUT, set_output_desc_attr_case11) {
  putenv("DUMP_GE_GRAPH=2");
  OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
  OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "Relu");
  vector<int64_t> dim = {4, 4, 1, 4};
  GeShape shape(dim);
  GeTensorDesc tenosr_desc(shape, ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  tenosr_desc.SetOriginFormat(FORMAT_NCHW);
  tenosr_desc.SetOriginDataType(DT_FLOAT);
  relu1->AddInputDesc(tenosr_desc);
  relu1->AddOutputDesc(tenosr_desc);
  vector<string> names = {"ori_rule1"};
  std::shared_ptr<std::unordered_map<std::string, std::vector<std::string>>> op_names_maps_tmp =
      std::make_shared<GraphPassUtil::UnorderedMapping>();
  std::vector<std::string> origin_op_names_vec = {"nodeA", "nodeB"};
  op_names_maps_tmp->insert(std::pair<std::string, std::vector<std::string>>("pass_test", origin_op_names_vec));
  (void)relu1->SetExtAttr(ge::ATTR_NAME_ORIGIN_OP_NAMES_MAP, op_names_maps_tmp);
  vector<std::string> pass_names = {"pass_test"};
  (void)AttrUtils::SetListStr(relu1, "pass_name", pass_names);

  relu2->AddInputDesc(tenosr_desc);
  relu2->AddOutputDesc(tenosr_desc);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr relu1_node = graph->AddNode(relu1);
  NodePtr relu2_node = graph->AddNode(relu2);
  std::vector<ge::NodePtr> original_nodes = {relu1_node};
  std::vector<ge::NodePtr> fus_nodes = {relu2_node};

  GraphPassUtil::RecordPassnameAndOriginalNames(original_nodes, fus_nodes, "passA");
  std::vector<std::string> origin_op_names_to_check = {"nodeA", "nodeB", "relu1"};
  CheckOriginName(fus_nodes, "passA", origin_op_names_to_check);
}


void CreateGraph(ComputeGraphPtr &graph, std::vector<ge::NodePtr> &original_nodes,
                 std::vector<ge::NodePtr> &fus_nodes) {
  OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
  OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "Relu");
  OpDescPtr relu3 = std::make_shared<OpDesc>("relu3", "Relu");
  OpDescPtr fusion_op = std::make_shared<OpDesc>("fusion", "Fusion");
  vector<int64_t> dim = {4, 4, 1, 4};
  GeShape shape(dim);
  GeTensorDesc tenosr_desc(shape, ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  tenosr_desc.SetOriginFormat(FORMAT_NCHW);
  tenosr_desc.SetOriginDataType(DT_FLOAT);
  relu1->AddInputDesc(tenosr_desc);
  relu1->AddOutputDesc(tenosr_desc);


  relu2->AddInputDesc(tenosr_desc);
  relu2->AddOutputDesc(tenosr_desc);

  relu3->AddInputDesc(tenosr_desc);
  relu3->AddOutputDesc(tenosr_desc);

  fusion_op->AddInputDesc(tenosr_desc);
  fusion_op->AddOutputDesc(tenosr_desc);


  NodePtr relu1_node = graph->AddNode(relu1);
  NodePtr relu2_node = graph->AddNode(relu2);
  NodePtr relu3_node = graph->AddNode(relu3);
  NodePtr fusion_node = graph->AddNode(fusion_op);
  ge::GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), relu3_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(relu3_node->GetOutDataAnchor(0), fusion_node->GetInDataAnchor(0));

  original_nodes = {relu2_node, relu3_node};
  fus_nodes = {fusion_node};
}

TEST_F(GraphPassUtilUT, test_get_back_ward_attr_01) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  std::vector<ge::NodePtr> original_nodes;
  std::vector<ge::NodePtr> fus_nodes;
  CreateGraph(graph, original_nodes, fus_nodes);
  bool backward = false;
  GraphPassUtil::GetBackWardAttr(original_nodes, backward, BackWardInheritMode::kInheritTrue);
  EXPECT_EQ(backward, true);
}

TEST_F(GraphPassUtilUT, test_get_back_ward_attr_02) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  std::vector<ge::NodePtr> original_nodes;
  std::vector<ge::NodePtr> fus_nodes;
  CreateGraph(graph, original_nodes, fus_nodes);
  auto ori_node0 = original_nodes[0];
  auto ori_node1 = original_nodes[1];
  ge::AttrUtils::SetBool(ori_node0->GetOpDesc(), "_backward", true);
  ge::AttrUtils::SetBool(ori_node1->GetOpDesc(), "_backward", true);
  bool backward = false;
  GraphPassUtil::GetBackWardAttr(original_nodes, backward, BackWardInheritMode::kFusedNode);
  EXPECT_EQ(backward, true);
}

TEST_F(GraphPassUtilUT, test_get_back_ward_attr_03) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  std::vector<ge::NodePtr> original_nodes;
  std::vector<ge::NodePtr> fus_nodes;
  CreateGraph(graph, original_nodes, fus_nodes);
  auto ori_node0 = original_nodes[0];
  auto ori_node1 = original_nodes[1];
  ge::AttrUtils::SetBool(ori_node1->GetOpDesc(), "_backward", true);
  bool backward = false;
  GraphPassUtil::GetBackWardAttr(original_nodes, backward, BackWardInheritMode::kInsertNode);
  EXPECT_EQ(backward, true);
}

TEST_F(GraphPassUtilUT, test_get_back_ward_attr_03_1) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  std::vector<ge::NodePtr> original_nodes;
  std::vector<ge::NodePtr> fus_nodes;
  CreateGraph(graph, original_nodes, fus_nodes);
  auto ori_node0 = original_nodes[0];
  auto ori_node1 = original_nodes[1];
  ge::AttrUtils::SetBool(ori_node1->GetOpDesc(), "_backward", true);
  bool backward = false;
  GraphPassUtil::GetBackWardAttr(original_nodes, backward, BackWardInheritMode::kFusedNode);
  EXPECT_EQ(backward, false);
}

TEST_F(GraphPassUtilUT, test_get_back_ward_attr_04) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  std::vector<ge::NodePtr> original_nodes;
  std::vector<ge::NodePtr> fus_nodes;
  CreateGraph(graph, original_nodes, fus_nodes);
  auto ori_node0 = original_nodes[0];
  auto ori_node1 = original_nodes[1];
  ge::AttrUtils::SetBool(ori_node0->GetOpDesc(), "_backward", true);
  ge::AttrUtils::SetBool(ori_node1->GetOpDesc(), "_backward", true);
  bool backward = false;
  GraphPassUtil::GetBackWardAttr(original_nodes, backward, BackWardInheritMode::kDoNotInherit);
  EXPECT_EQ(backward, false);
}

TEST_F(GraphPassUtilUT, test_inherit_attrs_01) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  std::vector<ge::NodePtr> original_nodes;
  std::vector<ge::NodePtr> fus_nodes;
  CreateGraph(graph, original_nodes, fus_nodes);
  auto ori_node0 = original_nodes[0];
  auto ori_node1 = original_nodes[1];
  ge::AttrUtils::SetBool(ori_node0->GetOpDesc(), "_backward", true);
  ge::AttrUtils::SetBool(ori_node1->GetOpDesc(), "_backward", true);
  ge::AttrUtils::SetInt(ori_node1->GetOpDesc(), "_recompute", 2);
  ge::AttrUtils::SetInt(ori_node1->GetOpDesc(), "_optimizer", 3);
  ge::AttrUtils::SetInt(ori_node1->GetOpDesc(), ge::ATTR_NAME_KEEP_DTYPE, 1);
  ge::AttrUtils::SetStr(ori_node1->GetOpDesc(), ge::ATTR_NAME_OP_COMPILE_STRATEGY, "test");

  GraphPassUtil::InheritAttrFromOriNodes(original_nodes, fus_nodes, BackWardInheritMode::kFusedNode);
  auto fus_op = fus_nodes.at(0)->GetOpDesc();

  bool backward = false;
  ge::AttrUtils::GetBool(fus_op, "_backward", backward);
  EXPECT_EQ(backward, true);

  int64_t recompute = 0;
  ge::AttrUtils::GetInt(ori_node1->GetOpDesc(), "_recompute", recompute);
  EXPECT_EQ(recompute, 2);

  int64_t optimizer = 0;
  ge::AttrUtils::GetInt(ori_node1->GetOpDesc(), "_optimizer", optimizer);
  EXPECT_EQ(optimizer, 3);

  int64_t keep_dtype = 0;
  ge::AttrUtils::GetInt(ori_node1->GetOpDesc(), ge::ATTR_NAME_KEEP_DTYPE, keep_dtype);
  EXPECT_EQ(keep_dtype, 1);

  string strategy = "";
  ge::AttrUtils::GetStr(ori_node1->GetOpDesc(), ge::ATTR_NAME_OP_COMPILE_STRATEGY, strategy);
  EXPECT_EQ(strategy, "test");
}


TEST_F(GraphPassUtilUT, test_inherit_attrs_02) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  std::vector<ge::NodePtr> original_nodes;
  std::vector<ge::NodePtr> fus_nodes;
  CreateGraph(graph, original_nodes, fus_nodes);
  auto ori_node0 = original_nodes[0];
  auto ori_node1 = original_nodes[1];
  ge::AttrUtils::SetBool(ori_node1->GetOpDesc(), "_backward", true);

  GraphPassUtil::InheritAttrFromOriNodes(original_nodes, fus_nodes, BackWardInheritMode::kFusedNode);
  auto fus_op = fus_nodes.at(0)->GetOpDesc();

  bool backward = false;
  ge::AttrUtils::GetBool(fus_op, "_backward", backward);
  EXPECT_EQ(backward, false);


  EXPECT_EQ(fus_op->HasAttr("_recompute"), false);
  EXPECT_EQ(fus_op->HasAttr("_optimizer"), false);
  EXPECT_EQ(fus_op->HasAttr(ge::ATTR_NAME_KEEP_DTYPE), false);
  EXPECT_EQ(fus_op->HasAttr(ge::ATTR_NAME_OP_COMPILE_STRATEGY), false);
}
}
