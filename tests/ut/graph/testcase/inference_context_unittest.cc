/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include <gtest/gtest.h>
#include "graph/inference_context.h"
#include "graph/resource_context_mgr.h"
#include "graph/node.h"
#include "graph_builder_utils.h"

namespace ge {
namespace {
struct TestResourceContext : ResourceContext {
  std::vector<GeShape> shapes;
  std::string resource_type;
};
}
class TestInferenceConext : public testing::Test {
 protected:
 ComputeGraphPtr graph_;
  void SetUp() {
    ut::GraphBuilder builder = ut::GraphBuilder("graph");
    builder.AddNode("TensorArrayWrite", "TensorArrayWrite", 1, 1);
    builder.AddNode("TensorArrayRead", "TensorArrayRead", 1, 1);
    graph_ = builder.GetGraph();
  }

  void TearDown() {}
};

TEST_F(TestInferenceConext, TestSetAndGetResourceContext) {
  ResourceContextMgr resource_context_mgr;
  InferenceContextPtr write_inference_context = std::shared_ptr<InferenceContext>(InferenceContext::Create(&resource_context_mgr));
  InferenceContextPtr read_inference_context = std::shared_ptr<InferenceContext>(InferenceContext::Create(&resource_context_mgr));

  // simulate write op
  const char* resource_key = "123";
  std::vector<GeShape> resource_shapes = {GeShape({1,1,2,3})};
  TestResourceContext *resource_context = new TestResourceContext();
  resource_context->shapes = resource_shapes;
  resource_context->resource_type = "normal";
  write_inference_context->SetResourceContext(AscendString(resource_key), resource_context);

  // simulate read op
  TestResourceContext *test_reousce_context =
      dynamic_cast<TestResourceContext *>(read_inference_context->GetResourceContext(resource_key));

  // check result
  auto ret_shape = test_reousce_context->shapes.at(0);
  auto ret_type = test_reousce_context->resource_type;
  ASSERT_EQ(ret_shape.GetDims(), resource_context->shapes.at(0).GetDims());
  ASSERT_EQ(ret_type, resource_context->resource_type);
}

TEST_F(TestInferenceConext, TestRegisterAndGetReiledOnResource) {
  InferenceContextPtr read_inference_context = std::shared_ptr<InferenceContext>(InferenceContext::Create());

  // simulate read_op register relied resource
  const char* resource_key = "456";
  read_inference_context->RegisterReliedOnResourceKey(AscendString(resource_key));

  auto reiled_keys = read_inference_context->GetReliedOnResourceKeys();
  // check result
  ASSERT_EQ(reiled_keys.empty(), false);
  ASSERT_EQ(*reiled_keys.begin(), resource_key);
}

TEST_F(TestInferenceConext, TestAddChangeResourceAndGet) {
  InferenceContextPtr write_inference_context = std::shared_ptr<InferenceContext>(InferenceContext::Create());

  // simulate write node add changed resource
  const char* resource_key = "789";
  write_inference_context->AddChangedResourceKey(AscendString(resource_key));

  auto changed_keys = write_inference_context->GetChangedResourceKeys();
  // check result
  ASSERT_EQ(changed_keys.empty(), false);
  ASSERT_EQ(*(changed_keys.begin()), resource_key);

  // clear changed_key
  write_inference_context->ClearChangedResourceKeys();
  changed_keys = write_inference_context->GetChangedResourceKeys();
  // check result
  ASSERT_EQ(changed_keys.empty(), true);
}
} // namespace ge
