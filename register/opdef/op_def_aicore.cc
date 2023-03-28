/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
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

#include "register/op_def.h"
#include "op_def_impl.h"
#include "framework/common/debug/ge_log.h"

namespace ops {
OpAICoreConfig::OpAICoreConfig() : impl_(new(std::nothrow) OpAICoreConfigImpl) {}

OpAICoreConfig::OpAICoreConfig(const OpAICoreConfig &aicore_config) : impl_(new(std::nothrow) OpAICoreConfigImpl) {
  this->impl_->op_params = aicore_config.impl_->op_params;
  this->impl_->cfg_keys = aicore_config.impl_->cfg_keys;
  this->impl_->cfg_info = aicore_config.impl_->cfg_info;
}

OpAICoreConfig::~OpAICoreConfig() = default;

OpAICoreConfig &OpAICoreConfig::operator=(const OpAICoreConfig &aicore_config) {
  if (this != &aicore_config) {
    *this->impl_ = *aicore_config.impl_;
  }
  return *this;
}

OpParamDef &OpAICoreConfig::Input(const char *name) {
  return this->impl_->op_params.Input(name);
}

OpParamDef &OpAICoreConfig::Output(const char *name) {
  return this->impl_->op_params.Output(name);
}

OpAICoreConfig &OpAICoreConfig::AsyncFlag(bool flag) {
  this->AddCfgItem("async.flag", flag ? "true" : "false");
  return *this;
}

OpAICoreConfig &OpAICoreConfig::DynamicCompileStaticFlag(bool flag) {
  this->AddCfgItem("dynamicCompileStatic.flag", flag ? "true" : "false");
  return *this;
}

OpAICoreConfig &OpAICoreConfig::DynamicFormatFlag(bool flag) {
  this->AddCfgItem("dynamicFormat.flag", flag ? "true" : "false");
  return *this;
}

OpAICoreConfig &OpAICoreConfig::DynamicRankSupportFlag(bool flag) {
  this->AddCfgItem("dynamicRankSupport.flag", flag ? "true" : "false");
  return *this;
}

OpAICoreConfig &OpAICoreConfig::DynamicShapeSupportFlag(bool flag) {
  this->AddCfgItem("dynamicShapeSupport.flag", flag ? "true" : "false");
  return *this;
}

OpAICoreConfig &OpAICoreConfig::HeavyOpFlag(bool flag) {
  this->AddCfgItem("heavyOp.flag", flag ? "true" : "false");
  return *this;
}

OpAICoreConfig &OpAICoreConfig::NeedCheckSupportFlag(bool flag) {
  this->AddCfgItem("needCheckSupport.flag", flag ? "true" : "false");
  return *this;
}

OpAICoreConfig &OpAICoreConfig::OpPattern(const char *pattern) {
  this->AddCfgItem("op.pattern", pattern);
  return *this;
}

OpAICoreConfig &OpAICoreConfig::PrecisionReduceFlag(bool flag) {
  this->AddCfgItem("precision_reduce.flag", flag ? "true" : "false");
  return *this;
}

OpAICoreConfig &OpAICoreConfig::RangeLimitValue(const char *value) {
  this->AddCfgItem("rangeLimit.value", value);
  return *this;
}

OpAICoreConfig &OpAICoreConfig::SlicePatternValue(const char *value) {
  this->AddCfgItem("slicePattern.value", value);
  return *this;
}

std::vector<OpParamDef> &OpAICoreConfig::GetInputs(void) {
  return this->impl_->op_params.GetInputs();
}
std::vector<OpParamDef> &OpAICoreConfig::GetOutputs(void) {
  return this->impl_->op_params.GetOutputs();
}
void OpAICoreConfig::AddCfgItem(const char *key, const char *value) {
  auto it = this->impl_->cfg_info.find(key);
  if (it == this->impl_->cfg_info.cend()) {
    this->impl_->cfg_keys.emplace_back(key);
  } else {
    this->impl_->cfg_info.erase(key);
  }
  this->impl_->cfg_info.emplace(key, value);
}

std::vector<ge::AscendString> &OpAICoreConfig::GetCfgKeys(void) {
  return this->impl_->cfg_keys;
}

std::map<ge::AscendString, ge::AscendString> &OpAICoreConfig::GetCfgInfo(void) {
  return this->impl_->cfg_info;
}

ge::AscendString &OpAICoreConfig::GetConfigValue(const char *key) {
  return this->impl_->cfg_info[key];
}

OpAICoreDef::OpAICoreDef() : impl_(new(std::nothrow) OpAICoreDefImpl) {}

OpAICoreDef::OpAICoreDef(const OpAICoreDef &aicore_def) : impl_(new(std::nothrow) OpAICoreDefImpl) {
  this->impl_->tiling_func = aicore_def.impl_->tiling_func;
  this->impl_->tiling_parse = aicore_def.impl_->tiling_parse;
  this->impl_->ci_creator = aicore_def.impl_->ci_creator;
  this->impl_->ci_deleter = aicore_def.impl_->ci_deleter;
  this->impl_->op_chk_support = aicore_def.impl_->op_chk_support;
  this->impl_->op_sel_format = aicore_def.impl_->op_sel_format;
  this->impl_->op_get_support = aicore_def.impl_->op_get_support;
  this->impl_->op_get_spec = aicore_def.impl_->op_get_spec;
  this->impl_->op_generlize_func = aicore_def.impl_->op_generlize_func;
  this->impl_->aicore_configs = aicore_def.impl_->aicore_configs;
}

OpAICoreDef::~OpAICoreDef() = default;

OpAICoreDef &OpAICoreDef::operator=(const OpAICoreDef &aicore_def) {
  if (this != &aicore_def) {
    *this->impl_ = *aicore_def.impl_;
  }
  return *this;
}

OpAICoreDef &OpAICoreDef::SetTiling(gert::OpImplKernelRegistry::TilingKernelFunc func) {
  this->impl_->tiling_func = func;
  return *this;
}

OpAICoreDef &OpAICoreDef::SetTilingParse(gert::OpImplRegister::TilingParseFunc func) {
  this->impl_->tiling_parse = func;
  return *this;
}

OpAICoreDef &OpAICoreDef::SetCompileInfoCreator(gert::OpImplKernelRegistry::CompileInfoCreatorFunc func) {
  this->impl_->ci_creator = func;
  return *this;
}

OpAICoreDef &OpAICoreDef::SetCompileInfoDeleter(gert::OpImplKernelRegistry::CompileInfoDeleterFunc func) {
  this->impl_->ci_deleter = func;
  return *this;
}

OpAICoreDef &OpAICoreDef::SetCheckSupport(optiling::OP_CHECK_FUNC func) {
  this->impl_->op_chk_support = func;
  return *this;
}

OpAICoreDef &OpAICoreDef::SetOpSelectFormat(optiling::OP_CHECK_FUNC func) {
  this->impl_->op_sel_format = func;
  return *this;
}

OpAICoreDef &OpAICoreDef::SetOpSupportInfo(optiling::OP_CHECK_FUNC func) {
  this->impl_->op_get_support = func;
  return *this;
}

OpAICoreDef &OpAICoreDef::SetOpSpecInfo(optiling::OP_CHECK_FUNC func) {
  this->impl_->op_get_spec = func;
  return *this;
}

OpAICoreDef &OpAICoreDef::SetParamGeneralize(optiling::PARAM_GENERALIZE_FUNC func) {
  this->impl_->op_generlize_func = func;
  return *this;
}

void OpAICoreDef::AddConfig(const char *soc, OpAICoreConfig &aicore_config) {
  this->impl_->aicore_configs.emplace(ge::AscendString(soc), aicore_config);
}

std::map<ge::AscendString, OpAICoreConfig> &OpAICoreDef::GetAICoreConfigs(void) {
  return this->impl_->aicore_configs;
}

void OpAICoreDef::OpCheckPost(const char *op_type) {
  GELOGD("do opcheck post, op_type:%s.", op_type);
  optiling::OpCheckFuncHelper(FUNC_CHECK_SUPPORTED, op_type, this->impl_->op_chk_support);
  optiling::OpCheckFuncHelper(FUNC_OP_SELECT_FORMAT, op_type, this->impl_->op_sel_format);
  optiling::OpCheckFuncHelper(FUNC_GET_OP_SUPPORT_INFO, op_type, this->impl_->op_get_support);
  optiling::OpCheckFuncHelper(FUNC_GET_SPECIFIC_INFO, op_type, this->impl_->op_get_spec);
  optiling::OpCheckFuncHelper(op_type, this->impl_->op_generlize_func);
}

gert::OpImplKernelRegistry::TilingKernelFunc &OpAICoreDef::GetTiling(void) {
  return this->impl_->tiling_func;
}
gert::OpImplRegister::TilingParseFunc &OpAICoreDef::GetTilingParse(void) {
  return this->impl_->tiling_parse;
}
gert::OpImplKernelRegistry::CompileInfoCreatorFunc &OpAICoreDef::GetCompileInfoCreator(void) {
  return this->impl_->ci_creator;
}
gert::OpImplKernelRegistry::CompileInfoDeleterFunc &OpAICoreDef::GetCompileInfoDeleter(void) {
  return this->impl_->ci_deleter;
}
optiling::OP_CHECK_FUNC &OpAICoreDef::GetCheckSupport(void) {
  return this->impl_->op_chk_support;
}
optiling::OP_CHECK_FUNC &OpAICoreDef::GetOpSelectFormat(void) {
  return this->impl_->op_sel_format;
}
optiling::OP_CHECK_FUNC &OpAICoreDef::GetOpSupportInfo(void) {
  return this->impl_->op_get_support;
}
optiling::OP_CHECK_FUNC &OpAICoreDef::GetOpSpecInfo(void) {
  return this->impl_->op_get_spec;
}
optiling::PARAM_GENERALIZE_FUNC &OpAICoreDef::GetParamGeneralize(void) {
  return this->impl_->op_generlize_func;
}
void OpAICoreDef::Log(const char *op_type, const char *info) {
  GELOGD("%s, op_type:%s.", info, op_type);
}
}  // namespace ops