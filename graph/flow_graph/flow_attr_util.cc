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

#include "flow_graph/flow_attr_util.h"
#include "common/checker.h"
#include "debug/ge_util.h"
#include "flow_graph/data_flow_attr_define.h"

namespace ge {
namespace dflow {
const std::map<DataFlowAttrType, FlowAttrUtil::SetAttrFunc> FlowAttrUtil::set_attr_funcs_ = {
    {DataFlowAttrType::COUNT_BATCH, &FlowAttrUtil::SetCountBatchAttr},
    {DataFlowAttrType::TIME_BATCH, &FlowAttrUtil::SetTimeBatchAttr},
};


bool FlowAttrUtil::CheckAttrsIsSupport(const std::vector<DataFlowInputAttr> &attrs) {
  bool count_batch = false;
  bool time_batch = false;
  if (attrs.empty()) {
    return true;
  }

  for (size_t i = 0; i < attrs.size(); ++i) {
    if (attrs[i].attr_type == DataFlowAttrType::COUNT_BATCH) {
      count_batch = true;
      if (time_batch) {
        GELOGE(ge::FAILED, "[Check]COUNT_BATCH attr and TIME_BATCH attr cannot be config at the same time.");
        return false;
      }
    } else if (attrs[i].attr_type == DataFlowAttrType::TIME_BATCH) {
      time_batch = true;
      if (count_batch) {
        GELOGE(ge::FAILED, "[Check]COUNT_BATCH attr and TIME_BATCH attr cannot be config at the same time.");
        return false;
      }
    } else {
      if (set_attr_funcs_.find(attrs[i].attr_type) == set_attr_funcs_.cend()) {
        GELOGE(ge::FAILED, "[Check]Attr type(%u) is not supported.", static_cast<uint32_t>(attrs[i].attr_type));
        return false;
      }
    }
  }

  return true;
}

graphStatus FlowAttrUtil::SetCountBatchAttr(const void *attr_value, GeTensorDescPtr &tensor_desc) {
  GE_ASSERT_NOTNULL(attr_value);
  const CountBatch *count_batch = static_cast<const CountBatch *>(attr_value);
  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_COUNT_BATCH_BATCH_SIZE, count_batch->batch_size));

  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_COUNT_BATCH_SLIDE_STRIDE, count_batch->slide_stride));

  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_COUNT_BATCH_TIMEOUT, count_batch->timeout));

  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_COUNT_BATCH_BATCH_DIM, count_batch->batch_dim));

  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_COUNT_BATCH_FLAG, count_batch->flag));

  GE_ASSERT_TRUE(ge::AttrUtils::SetBool(tensor_desc, ATTR_NAME_COUNT_BATCH_PADDING, count_batch->padding));

  GE_ASSERT_TRUE(ge::AttrUtils::SetBool(tensor_desc, ATTR_NAME_COUNT_BATCH_DROP_REMAINDER,
                                        count_batch->drop_remainder));

  GELOGI("set count batch attr: batch_size(%ld), slide_stride(%ld), timeout(%ld), "
         "batch_dim(%ld), flag(%d), padding(%d), drop_remainder(%d)", count_batch->batch_size,
         count_batch->slide_stride, count_batch->timeout, count_batch->batch_dim, count_batch->flag,
         count_batch->padding, count_batch->drop_remainder);
  return ge::GRAPH_SUCCESS;
}

graphStatus FlowAttrUtil::SetTimeBatchAttr(const void *attr_value, GeTensorDescPtr &tensor_desc) {
  GE_ASSERT_NOTNULL(attr_value);
  const TimeBatch *time_batch = static_cast<const TimeBatch *>(attr_value);
  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_TIME_BATCH_TIME_WINDOW, time_batch->time_window));

  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_TIME_BATCH_TIME_INTERVAL, time_batch->time_interval));

  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_TIME_BATCH_TIMEOUT, time_batch->timeout));

  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_TIME_BATCH_BATCH_DIM, time_batch->batch_dim));

  GE_ASSERT_TRUE(ge::AttrUtils::SetInt(tensor_desc, ATTR_NAME_TIME_BATCH_FLAG, time_batch->flag));

  GE_ASSERT_TRUE(ge::AttrUtils::SetBool(tensor_desc, ATTR_NAME_TIME_BATCH_PADDING, time_batch->padding));

  GE_ASSERT_TRUE(ge::AttrUtils::SetBool(tensor_desc, ATTR_NAME_TIME_BATCH_DROP_REMAINDER, time_batch->drop_remainder));

  GELOGI("set time batch attr: time_window(%ld), time_interval(%ld), timeout(%ld), "
         "batch_dim(%ld), flag(%d), padding(%d), drop_remainder(%d)", time_batch->time_window,
         time_batch->time_interval, time_batch->timeout, time_batch->batch_dim, time_batch->flag,
         time_batch->padding, time_batch->drop_remainder);
  return ge::GRAPH_SUCCESS;
}

graphStatus FlowAttrUtil::SetAttrsToTensorDesc(const std::vector<DataFlowInputAttr> &attrs,
                                               GeTensorDescPtr &tensor_desc) {
  GE_ASSERT_TRUE(CheckAttrsIsSupport(attrs));
  for (auto &attr : attrs) {
    auto attr_type = attr.attr_type;
    const auto iter = set_attr_funcs_.find(attr_type);
    GE_ASSERT_TRUE(iter != set_attr_funcs_.cend(), "Data flow input attr type(%u) does not has process function..",
                   static_cast<uint32_t>(attr_type));
    GE_ASSERT_SUCCESS(iter->second(attr.attr_value, tensor_desc));
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace dflow
}  // namespace ge