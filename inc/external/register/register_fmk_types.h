/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef INC_EXTERNAL_REGISTER_REGISTER_FMK_TYPES_H_
#define INC_EXTERNAL_REGISTER_REGISTER_FMK_TYPES_H_

#include <string>

namespace domi {
///
/// @ingroup domi_omg
/// @brief  AI framework types
///
enum FrameworkType {
  CAFFE = 0,
  MINDSPORE = 1,
  TENSORFLOW = 3,
  ANDROID_NN,
  ONNX,
  FRAMEWORK_RESERVED,
};

///
/// @ingroup domi_omg
/// @brief  AI framework type to string
///
const std::map<std::string, std::string> kFwkTypeToStr = {
        {"0", "Caffe"},
        {"1", "MindSpore"},
        {"3", "TensorFlow"},
        {"4", "Android_NN"},
        {"5", "Onnx"}
};
}  // namespace domi

#endif  // INC_EXTERNAL_REGISTER_REGISTER_FMK_TYPES_H_
