#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
#-------------------------------------------------------------------
# Purpose:
# Copyright 2020 Huawei Technologies Co., Ltd. All rights reserved.
#-------------------------------------------------------------------

import os
import re
import sys
import logging

"""
    generate stub func body by return type
"""
RETURN_STATEMENTS = {
    'ge::graphStatus':
        '    std::cout << "[ERROR]: stub library libregister cannot be used for execution, please check your "\n'
        '        << "environment variables and compilation options to make sure you use the correct library."\n'
        '        << std::endl;\n'
        '    return ge::GRAPH_FAILED;',
    'Status': '    return SUCCESS;',
    'ge::AscendString': '    return ge::AscendString();',
    'ge::AscendString&': '    static ge::AscendString str;\n'
                         '    return str;',
    'OpDef': '    return OpDef("default");',
    'OpDef&': '    return *this;',
    'ByteBuffer&': '    return buf;',
    'ByteBuffer& OpRunInfo::GetAllTilingData': '    static ByteBuffer buf;\n'
                                               '    return buf;',
    'CTilingDataClassFactory&': '    return *this;',
    'CTilingDataClassFactory& CTilingDataClassFactory::GetInstance': '    static CTilingDataClassFactory instance;\n'
                                                                     '    return instance;',
    'FrameworkRegistry&': '    static FrameworkRegistry instance;\n'
                          '    return instance;',
    'ItemFindStatus': '    return ItemFindStatus::ITEM_NOEXIST;',
    'KernelRegisterV2&': '    return *this;',
    'KernelRegistry&': '    std::shared_ptr<KernelRegistry> g_user_defined_registry = nullptr;\n'
                       '    return *g_user_defined_registry;',
    'OpAICoreConfig&': '    return *this;',
    'OpAICoreDef&': '    return *this;',
    'OpAICoreDef& OpDef::AICore': '    return this->impl_->op_aicore;',
    'OpAttrDef&': '    return *this;',
    'OpAttrDef& OpDef::Attr': '    return this->GetOrCreateAttr(name);',
    'OpAttrDef& OpDef::AddAttr': '    return this->impl_->attrs.back();',
    'OpAttrDef& OpDef::GetOrCreateAttr': '    OpAttrDef attr(name);\n'
                                         '    return this->AddAttr(attr);',
    'OpCompileInfo&': '    return *this;',
    'OpImplRegister&': '    return *this;',
    'OpImplRegisterV2&': '    return *this;',
    'OpImplRegistry&': '    static OpImplRegistry instance;\n'
                       '    return instance;',
    'OpParamDef& OpParamDef::': '    return *this;',
    'OpParamDef& OpAICoreConfig::Input': '    return this->impl_->op_params.Input(name);',
    'OpParamDef& OpAICoreConfig::Output': '    return this->impl_->op_params.Output(name);',
    'OpParamDef& OpDef::Input': '    return this->impl_->op_params.Input(name);',
    'OpParamDef& OpDef::Output': '    return this->impl_->op_params.Output(name);',
    'OpRegistrationData&': '    return *this;',
    'OpRunInfo&': '    return *this;',
    'Option': '    return this->impl_->param_type;',
    'OpImplRegistry::PrivateAttrList&': '    static OpImplRegistry::PrivateAttrList emptyPrivateAttr;\n'
                                        '    return emptyPrivateAttr;',
    'OpImplKernelRegistry::PrivateAttrList&': '    static OpImplKernelRegistry::PrivateAttrList emptyPrivateAttr;\n'
                                              '    return emptyPrivateAttr;',
    'StructSizeInfoBase&': '    return *this;',
    'StructSizeInfoBase& StructSizeInfoBase::GetInstance': '    static StructSizeInfoBase instance;\n'
                                                           '    return instance;',
    'domi::FrameworkType': '    return domi::FrameworkType::FRAMEWORK_RESERVED;',
    'domi::ImplyType': '    return domi::ImplyType::BUILDIN;',
    'gert::OpImplKernelRegistry::TilingKernelFunc&': '    return this->impl_->tiling_func;',
    'std::vector<ge::DataType>&': '    return this->impl_->types;',
    'std::vector<ge::Format>&': '    return this->impl_->formats;',
    'AutoMappingSubgraphIOIndexFunc': '    return nullptr;',
    'optiling::OP_CHECK_FUNC& OpAICoreDef::GetCheckSupport': '    return this->impl_->op_chk_support;',
    'optiling::OP_CHECK_FUNC& OpAICoreDef::GetOpSelectFormat': '    return this->impl_->op_sel_format;',
    'optiling::OP_CHECK_FUNC& OpAICoreDef::GetOpSupportInfo': '    return this->impl_->op_get_support;',
    'optiling::OP_CHECK_FUNC& OpAICoreDef::GetOpSpecInfo': '    return this->impl_->op_get_spec;',
    'optiling::PARAM_GENERALIZE_FUNC& OpAICoreDef::GetParamGeneralize': '    return this->impl_->op_generlize_func;',
    'gert::OpImplKernelRegistry::InferShapeKernelFunc&': '    return this->impl_->infer_shape;',
    'gert::OpImplKernelRegistry::InferShapeRangeKernelFunc&': '    return this->impl_->infer_shape_range;',
    'gert::OpImplKernelRegistry::InferDataTypeKernelFunc& OpDef::GetInferDataType':
        '    return this->impl_->infer_data_type;',
    'OpImplKernelRegistry::OpImplFunctions& OpImplRegistry::CreateOrGetOpImpl': '    return types_to_impl_[op_type];',
    'OpTilingFunc& OpTilingFuncInfo::GetOpTilingFunc': '    return this->tiling_func_;',
    'OpTilingFuncV2& OpTilingFuncInfo::GetOpTilingFuncV2': '    return this->tiling_func_v2_;',
    'OpTilingFuncV3& OpTilingFuncInfo::GetOpTilingFuncV3': '    return this->tiling_func_v3_;',
    'OpParseFuncV3& OpTilingFuncInfo::GetOpParseFuncV3': '    return this->parse_func_v3_;',
    'OpTilingFuncV4& OpTilingFuncInfo::GetOpTilingFuncV4': '    return this->tiling_func_v4_;',
    'OpParseFuncV4& OpTilingFuncInfo::GetOpParseFuncV4': '    return this->parse_func_v4_;',
    'ParseParamFunc': '    return nullptr;',
    'ParseParamByOpFunc OpRegistrationData::GetParseParamByOperatorFn': '    return nullptr;',
    'FusionParseParamFunc OpRegistrationData::GetFusionParseParamFn': '    return nullptr;',
    'FusionParseParamByOpFunc OpRegistrationData::GetFusionParseParamByOpFn': '    return nullptr;',
    'ParseSubgraphFunc OpRegistrationData::GetParseSubgraphPostFn': '    return nullptr;',
    'ParseOpToGraphFunc OpRegistrationData::GetParseOpToGraphFn': '    return nullptr;',
    'OpBankKeyConvertFun& OpBankKeyFuncInfo::GetBankKeyConvertFunc': '    return convert_func_;',
    'OpBankParseFun& OpBankKeyFuncInfo::GetBankKeyParseFunc': '    return parse_func_;',
    'OpBankLoadFun& OpBankKeyFuncInfo::GetBankKeyLoadFunc': '    return load_func_;',
    'Ptr': '    return nullptr;',
    'std::string': '    return "";',
    'std::string&': '    static std::string s;\n'
                    '    return s;',
    'int': '    return 0;',
    'std::vector<std::string>': '    return {};',
    'std::vector<int64_t>': '    return {};',
    'std::vector<int64_t>&': '    static std::vector<int64_t> vec;\n'
                             '    return vec;',
    'std::vector<OpParamDef>& OpAICoreConfig::GetInputs': '    return this->impl_->op_params.GetInputs();',
    'std::vector<OpParamDef>& OpAICoreConfig::GetOutputs': '    return this->impl_->op_params.GetOutputs();',
    'std::vector<OpParamDef>& OpDef::GetInputs': '    return this->impl_->op_params.GetInputs();',
    'std::vector<OpParamDef>& OpDef::GetOutputs': '    return this->impl_->op_params.GetOutputs();',
    'std::vector<OpAttrDef>& OpDef::GetAttrs': '    return this->impl_->attrs;',
    'std::vector<ge::AscendString>&': '    static std::vector<ge::AscendString> ops_list;\n'
                                      '    return ops_list;',
    'std::map': '    return {};',
    'std::map<ge::AscendString, ge::AscendString>& OpAICoreConfig::GetCfgInfo': '    return this->impl_->cfg_info;',
    'std::map<ge::AscendString, OpAICoreConfig>& OpAICoreDef::GetAICoreConfigs':
        '    return this->impl_->aicore_configs;',
    'std::map<OpImplKernelRegistry::OpType, OpImplKernelRegistry::OpImplFunctions>&':
        '    static std::map<OpImplKernelRegistry::OpType, OpImplKernelRegistry::OpImplFunctions> m;\n'
        '    return m;',
    'std::map<ge::AscendString, TuningTilingDefConstructor>& TuningTilingClassFactory::RegisterInfo':
        '    static std::map<ge::AscendString, TuningTilingDefConstructor> instance;\n'
        '    return instance;',
    'std::unordered_map<std::string, OpTilingFunc>& OpTilingRegistryInterf::RegisteredOpInterf':
        '    static std::unordered_map<std::string, OpTilingFunc> interf;\n'
        '    return interf;',
    'std::unordered_map<std::string, OpTilingFuncV2>& OpTilingRegistryInterf_V2::RegisteredOpInterf':
        '    static std::unordered_map<std::string, OpTilingFuncV2> interf;\n'
        '    return interf;',
    'std::unordered_map<std::string, OpTilingFuncInfo>& OpTilingFuncRegistry::RegisteredOpFuncInfo':
        '    static std::unordered_map<std::string, OpTilingFuncInfo> op_func_map;\n'
        '    return op_func_map;',
    'std::unordered_map<ge::AscendString, OpBankKeyFuncInfo>& OpBankKeyFuncRegistry::RegisteredOpFuncInfo':
        '    static std::unordered_map<ge::AscendString, OpBankKeyFuncInfo> op_func_map;\n'
        '    return op_func_map;',
    'std::shared_ptr<TilingDef>': '    return nullptr;',
    'std::shared_ptr<TuningTilingDef>': '    return nullptr;',
    'int32_t': '    return 0;',
    'uint32_t': '    return 0;',
    'int64_t': '    return 0;',
    'uint64_t': '    return 0;',
    'size_t': '    return 0;',
    'float': '    return 0.0f;',
    'bool': '    return false;',
}

"""
    white_list_for_debug, include_dir_key_words is to
    determines which header files to generate cc files from
    when DEBUG on
"""
white_list_for_debug = [
    "op_def.h",
    "op_def_factory.h",
    "op_impl_registry.h",
    "op_tiling_info.h",
    "op_tiling_registry.h",
    "register.h",
    "tilingdata_base.h",
    "tuning_bank_key_registry.h",
    "tuning_tiling_registry.h",
]
include_dir_key_words = ["register"]

"""
    this attr is used for symbol table visible
"""
GE_ATTR = 'GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY'

"""
    max code len per line in hua_wei software programming specifications
"""
MAX_CODE_LEN_PER_LINE = 100

DEBUG = True

logging.basicConfig(stream=sys.stdout, format='[%(asctime)s] [%(lineno)s] %(levelname)s: %(message)s',
                    level=logging.INFO)


def need_generate_func(func_line):
    """
    :param func_line:
    :return:
    """
    if func_line.strip().endswith("default") or func_line.strip().endswith("delete") \
            or func_line.strip().startswith("typedef") or func_line.strip().startswith("using"):
        return False
    return True


def file_endswith_white_list_suffix(file):
    """
    :param file:
    :return:
    """
    if DEBUG:
        for suffix in white_list_for_debug:
            suffix = re.sub(r'^/*', '/', suffix)
            if file.endswith(suffix):
                return True
        return False
    else:
        return True


"""
    belows are patterns used for analyse .h file
"""
# pattern function
pattern_func = re.compile(r"""(^[\s]*)([a-zA-Z~_].*[)](?!.*{).*)(;.*)\n$""", re.VERBOSE | re.MULTILINE | re.DOTALL)
# pattern virtual function
pattern_virtual_func = re.compile(r"""^[ ]*
                                       virtual
                                       [ ]+
                                       (?:const[ ]+)?
                                       [:\w]+
                                       [ &*]+
                                       [:\w]+
                                       \(
                                         [^()]*
                                       \)
                                       [ ]+
                                       (?:const[ ]+)?
                                       =[ ]+0;$""", re.VERBOSE)
# pattern comment
pattern_comment = re.compile(r'^\s*//')
pattern_comment_2_start = re.compile(r'^\s*/[*]')
pattern_comment_2_end = re.compile(r'[*]/\s*$')
# pattern visibility
pattern_visibility = re.compile(r'(FMK_FUNC_HOST_VISIBILITY|FMK_FUNC_DEV_VISIBILITY) *')
# pattern override
pattern_override = re.compile(r' +override\b')
# pattern define
pattern_define = re.compile(r'^\s*#define')
pattern_define_return = re.compile(r'\\\s*$')
# blank line
pattern_blank_line = re.compile(r'^\s*$')
# virtual,explicit,friend,static
pattern_keyword = re.compile(r'(virtual\s+|explicit\s+|friend\s+|static\s+)')
# lead space
pattern_leading_space = re.compile(r'(^[\s]*)[a-zA-Z~_]')
# functions will have patterns such as func ( or func(
# but operator is an exception; the class name is preceded by an operator, and the above mode does not exist
# format like :"operator = ()"
pattern_func_name = re.compile(r'([a-zA-Z0-9~_\-]+\s*|operator?.*)[(]')
# template
pattern_template = re.compile(r'^\s*template')
pattern_template_end = re.compile(r'>\s*$')
# namespace
pattern_namespace = re.compile(r'namespace.*{')
# class : which can handle classA a and {not on the same line, but if found ';' after class,then don't deal with
pattern_class = re.compile(r'^[\s]*(class|struct)\s+(%s\s+)?([a-zA-Z0-9_\-]+<?)(?!.*;)' % GE_ATTR)
# pattern for function body start and end
pattern_start = re.compile('{')
pattern_end = re.compile('}')
# pattern for format func
pat_format_func = re.compile(r"""^(
                                   (?:const[ ]+)?
                                   (?:
                                    [:\w]+
                                    |
                                    std::(?:vector|shared_ptr)<[:\w ]+>
                                    |
                                    std::(?:vector|shared_ptr)<std::vector<[:\w ]+>>
                                    |
                                    std::(?:map|unordered_map|pair)<[:\w]+[, ]+[:\w]+>
                                   )
                                  )
                                  ([ ]+)
                                  ([&*]+)""", re.VERBOSE)
# pattern for parser ret_type & func_name
pat_search_func = re.compile(r"""^(?:const[ ]+)?
                                  (?P<ret_type>
                                   (?:
                                    [:\w]+
                                    |
                                    std::(?:vector|shared_ptr)<[:\w ]+>
                                    |
                                    std::(?:vector|shared_ptr)<std::vector<[:\w ]+>>
                                    |
                                    std::(?:map|unordered_map|pair)<[:\w]+[, ]+[:\w]+>
                                   )
                                   (?:[&*]+)?
                                  )
                                  [ ]+
                                  (?P<class_name>\w+)
                                  ::
                                  \n?
                                  (?P<func_name>\w+|operator=)
                                  [ ]*
                                  \(""", re.VERBOSE)


class H2CC(object):
    def __init__(self, input_file, output_file, shared_includes_content):
        """
        :param input_file:
        :param output_file:
        :param shared_includes_content:
        """
        self.input_file = input_file
        self.output_file = output_file
        self.shared_includes_content = shared_includes_content
        self.line_index = 0
        self.input_fd = open(self.input_file, 'r')
        self.input_content = self.input_fd.readlines()
        self.output_fd = open(self.output_file, 'w')

        # The state may be normal_now(in the middle of {}),class_now,namespace_now
        self.stack = []
        self.stack_class = []
        self.stack_template = []
        # record funcs generated by h2cc func
        self.func_list_exist = []

    def __del__(self):
        if self.output_file.endswith('/stub_register.cc'):
            impl = """
namespace domi {
class FMK_FUNC_HOST_VISIBILITY FMK_FUNC_DEV_VISIBILITY FrameworkRegistryImpl {
 public:
  void AddAutoMappingSubgraphIOIndexFunc(const domi::FrameworkType framework, AutoMappingSubgraphIOIndexFunc fun) {}
  AutoMappingSubgraphIOIndexFunc GetAutoMappingSubgraphIOIndexFunc(const domi::FrameworkType framework) { return nullptr; }
};
}  // namespace domi
"""
            self.output_fd.write(impl)
        self.input_fd.close()
        self.output_fd.close()
        del self.stack
        del self.stack_class
        del self.stack_template
        del self.func_list_exist

    def just_skip(self):
        # skip blank line or comment
        if pattern_blank_line.search(self.input_content[self.line_index]) or pattern_comment.search(
                self.input_content[self.line_index]):  # /n or comment using //
            self.line_index += 1
        if pattern_comment_2_start.search(self.input_content[self.line_index]):  # comment using /*
            while not pattern_comment_2_end.search(self.input_content[self.line_index]):  # */
                self.line_index += 1
            self.line_index += 1
        # skip define
        if pattern_define.search(self.input_content[self.line_index]):
            while pattern_blank_line.search(self.input_content[self.line_index]) or pattern_define_return.search(
                    self.input_content[self.line_index]):
                self.line_index += 1
            self.line_index += 1
        # skip virtual function
        while pattern_virtual_func.search(self.input_content[self.line_index]):
            self.line_index += 1

    def write_inc_content(self):
        for shared_include_content in self.shared_includes_content:
            self.output_fd.write(shared_include_content)

    def h2cc(self):
        """
        :return:
        """
        logging.info("start generate cc_file[%s] from h_file[%s]", self.output_file, self.input_file)
        # write inc content
        self.write_inc_content()
        # core processing cycle, process the input .h file by line
        while self.line_index < len(self.input_content):
            # handle comment and blank line
            self.just_skip()

            # match namespace
            self.handle_namespace()

            # match template
            template_string = self.handle_template()
            # match class
            line = self.input_content[self.line_index]
            # handle visibility keywords
            if pattern_visibility.search(line):
                line = pattern_visibility.sub('', line)
            # handle override keywords
            if pattern_override.search(line):
                line = pattern_override.sub('', line)
            match_class = pattern_class.search(line)
            match_start = pattern_start.search(line)
            handle_class_result = self.handle_class(template_string, line, match_start, match_class)
            if handle_class_result == "continue":
                continue

            # match "}"
            handle_stack_result = self.handle_stack(match_start)
            if handle_stack_result == "continue":
                continue
            # handle func
            handle_func1_result, line, start_i = self.handle_func1(line)
            if handle_func1_result == "continue":
                continue

            # here means func is found
            # delete key word
            line = pattern_keyword.sub('', line)
            logging.info("line[%s]", line)

            # Class member function
            # if friend we will not add class name
            friend_match = re.search('friend ', line)
            if len(self.stack_class) > 0 and not friend_match:
                line, func_name = self.handle_class_member_func(line, template_string)
            # Normal functions
            else:
                line, func_name = self.handle_normal_func(line, template_string)

            need_generate = need_generate_func(line)
            # func body
            line += self.implement_function(line)
            # comment
            line = self.gen_comment(start_i) + line
            # write to out file
            self.write_func_content(line, func_name, need_generate)
            # next loop
            self.line_index += 1

        logging.info('Added %s functions', len(self.func_list_exist))
        logging.info('Successfully converted,please see ' + self.output_file)

    def handle_func1(self, line):
        """
        :param line:
        :return:
        """
        find1 = re.search('[(]', line)
        if not find1:
            self.line_index += 1
            return "continue", line, None
        find2 = re.search('[)]', line)
        start_i = self.line_index
        space_match = pattern_leading_space.search(line)
        # deal with
        # int abc(int a,
        #        int b)
        if find1 and (not find2):
            self.line_index += 1
            line2 = self.input_content[self.line_index]
            if space_match:
                line2 = re.sub('^' + space_match.group(1), '', line2)
            line += line2
            while self.line_index < len(self.input_content):
                if re.search('[)]', line2) \
                    and not re.search(r'std::function<.+?> &input,', line2):
                    break
                self.line_index += 1
                line2 = self.input_content[self.line_index]
                line2 = re.sub('^' + space_match.group(1), '', line2)
                line += line2

        match_start = pattern_start.search(self.input_content[self.line_index])
        match_end = pattern_end.search(self.input_content[self.line_index])
        if match_start:  # like  ) {  or ) {}    int the last line
            if not match_end:
                self.stack.append('normal_now')
            ii = start_i
            while ii <= self.line_index:
                ii += 1
            self.line_index += 1
            return "continue", line, start_i
        logging.info("line[%s]", line)
        # '  int abc();'->'int abc()'
        (line, match) = pattern_func.subn(r'\2\n', line)
        logging.info("line[%s]", line)
        # deal with case of 'return type' and 'func_name' are not in the same line, like: 'int \n abc(int a, int b)'
        if re.search(r'^\s*(inline)?\s*[a-zA-Z0-9_]+\s*$', self.input_content[start_i - 1]):
            line = self.input_content[start_i - 1] + line
        line = line.lstrip()
        if not match:
            self.line_index += 1
            return "continue", line, start_i
        return "pass", line, start_i

    def handle_stack(self, match_start):
        """
        :param match_start:
        :return:
        """
        line = self.input_content[self.line_index]
        match_end = pattern_end.search(line)
        if match_start:
            self.stack.append('normal_now')
        if match_end:
            top_status = self.stack.pop()
            if top_status == 'namespace_now':
                self.output_fd.write(line + '\n')
            elif top_status == 'class_now':
                self.stack_class.pop()
                self.stack_template.pop()
        if match_start or match_end:
            self.line_index += 1
            return "continue"

        if len(self.stack) > 0 and self.stack[-1] == 'normal_now':
            self.line_index += 1
            return "continue"
        return "pass"

    def handle_class(self, template_string, line, match_start, match_class):
        """
        :param template_string:
        :param line:
        :param match_start:
        :param match_class:
        :return:
        """
        if not match_class:  # we face a class
            return "pass"
        self.stack_template.append(template_string)
        self.stack.append('class_now')
        class_name = match_class.group(3)

        # class template specializations: class A<u,Node<u> >
        if '<' in class_name:
            k = line.index('<')
            fit = 1
            for ii in range(k + 1, len(line)):
                if line[ii] == '<':
                    fit += 1
                if line[ii] == '>':
                    fit -= 1
                if fit == 0:
                    break
            class_name += line[k + 1:ii + 1]
        logging.info('class_name[%s]', class_name)
        self.stack_class.append(class_name)
        while not match_start:
            self.line_index += 1
            line = self.input_content[self.line_index]
            match_start = pattern_start.search(line)
        self.line_index += 1
        return "continue"

    def handle_template(self):
        line = self.input_content[self.line_index]
        match_template = pattern_template.search(line)
        template_string = ''
        if match_template:
            match_template_end = pattern_template_end.search(line)
            template_string = line
            while not match_template_end:
                self.line_index += 1
                line = self.input_content[self.line_index]
                template_string += line
                match_template_end = pattern_template_end.search(line)
            self.line_index += 1
        return template_string

    def handle_namespace(self):
        line = self.input_content[self.line_index]
        match_namespace = pattern_namespace.search(line)
        if match_namespace:  # we face namespace
            self.output_fd.write(line + '\n')
            self.stack.append('namespace_now')
            self.line_index += 1

    def handle_normal_func(self, line, template_string):
        template_line = ''
        self.stack_template.append(template_string)
        if self.stack_template[-1] != '':
            template_line = re.sub(r'\s*template', 'template', self.stack_template[-1])
            # change '< class T = a, class U = A(3)>' to '<class T, class U>'
            template_line = re.sub(r'\s*=.*>(\s*)$', r'>\1', template_line)
            template_line = re.sub(r'\s*=.*,', ',', template_line)
            template_line = re.sub(r'\s*=.*', '', template_line)
        line = re.sub(r'\s*=.*,', ',', line)
        line = re.sub(r'\s*=.*\)', ')', line)
        line = template_line + line
        self.stack_template.pop()
        func_name = re.search(r'^.*\)', line, re.MULTILINE | re.DOTALL).group()
        logging.info("line[%s]", line)
        logging.info("func_name[%s]", func_name)
        return line, func_name

    def handle_class_member_func(self, line, template_string):
        template_line = ''
        x = ''
        if template_string != '':
            template_string = re.sub(r'\s*template', 'template', template_string)
            template_string = re.sub(r'\s*=.*>(\s*)$', r'>\1', template_string)
            template_string = re.sub(r'\s*=.*,', ',', template_string)
            template_string = re.sub(r'\s*=.*', '', template_string)
        if self.stack_template[-1] != '':
            if not (re.search(r'<\s*>', stack_template[-1])):
                template_line = re.sub(r'^\s*template', 'template', stack_template[-1])
                if not (re.search(r'<.*>', self.stack_class[-1])):
                    # for x we get like template<class T, typename U> -> <T,U>
                    x = re.sub(r'template\s*<', '<', template_line)  # remove template -> <class T, typename U>
                    x = re.sub(r'\n', '', x)
                    x = re.sub(r'\s*=.*,', ',', x)
                    x = re.sub(r'\s*=.*\>', '>', x)
                    x = x.rstrip()  # remove \n
                    x = re.sub(r'(class|typename)\s+|(<class>|<typename>\s*class)', '',
                               x)  # remove class,typename ->  <T, U>
                    x = re.sub(r'<\s+', '<', x)
                    x = re.sub(r'\s+>', '>', x)
                    x = re.sub(r'\s+,', ',', x)
                    x = re.sub(r',\s+', ', ', x)
        line = re.sub(r'\s*=\s+0', '', line)
        line = re.sub(r'\s*=\s+.*,', ',', line)
        line = re.sub(r'\s*=\s+.*\)', ')', line)
        logging.info("x[%s]\nline[%s]", x, line)
        # if the function is long, void ABC::foo()
        # breaks into two lines void ABC::\n foo()
        rep_fmt = '%s%s::{}%s' % (self.stack_class[-1], x, r'\1(')
        temp_line = pattern_func_name.sub(rep_fmt.format(''), line, count=1)
        if len(temp_line) > MAX_CODE_LEN_PER_LINE:
            line = pattern_func_name.sub(rep_fmt.format('\n'), line, count=1)
        else:
            line = temp_line
        logging.info("line[%s]", line)
        # add template as the above if there is one
        template_line = re.sub(r'\s*=.*>(\s*)$', r'>\1', template_line)
        template_line = re.sub(r'\s*=.*,', ',', template_line)
        template_line = re.sub(r'\s*=.*', '', template_line)
        line = template_line + template_string + line
        func_name = re.search(r'^.*\)', line, re.MULTILINE | re.DOTALL).group()
        line = re.sub(r'\b(KernelInfo)\b', r'KernelRegistry::\1', line)
        line = re.sub(r'\b(KernelFuncs)\b', r'KernelRegistry::\1', line)
        line = re.sub(r'\b(OpImplFunctions)\b', r'OpImplKernelRegistry::\1', line)
        line = re.sub(r'\b(OpType)\b', r'OpImplKernelRegistry::\1', line)
        line = re.sub(r'\b(PrivateAttrList &OpImplKernelRegistry::)', r'OpImplKernelRegistry::\1', line)
        line = re.sub(r'\b(PrivateAttrList &OpImplRegistry::)', r'OpImplRegistry::\1', line)
        logging.info("line[%s]", line)
        logging.info("func_name[%s]", func_name)
        return line, func_name

    def write_func_content(self, content, func_name, need_generate):
        if not (func_name in self.func_list_exist) and need_generate:
            self.output_fd.write(content)
            self.func_list_exist.append(func_name)
            logging.info('add func:[%s]', func_name)

    def gen_comment(self, start_i):
        comment_line = ''
        # Function comments are on top of function declarations, copy them over
        k = start_i - 1  # one line before this func start
        if pattern_template.search(self.input_content[k]):
            k -= 1
        if pattern_comment_2_end.search(self.input_content[k]):
            comment_line = self.input_content[k].lstrip()
            while not pattern_comment_2_start.search(self.input_content[k]):
                k -= 1
                comment_line = self.input_content[k].lstrip() + comment_line
        else:
            for j in range(k, 0, -1):
                c_line = self.input_content[j]
                if pattern_comment.search(c_line):
                    c_line = re.sub(r'\s*//', '//', c_line)
                    comment_line = c_line + comment_line
                else:
                    break
        return comment_line

    @staticmethod
    def get_return_statements(func):
        func = pat_format_func.sub(r'\1\3\2', func)
        m = pat_search_func.search(func)
        if not m:
            return None
        logging.info('ret_type: %s, class_name: %s, func_name: %s', *m.group('ret_type', 'class_name', 'func_name'))
        type_cls_func_name = '%s %s::%s' % m.group('ret_type', 'class_name', 'func_name')
        if type_cls_func_name in RETURN_STATEMENTS:
            logging.info('type_cls_func_name:[%s] matched!', type_cls_func_name)
            return RETURN_STATEMENTS[type_cls_func_name]
        type_cls_name = '%s %s::' % m.group('ret_type', 'class_name')
        if type_cls_name in RETURN_STATEMENTS:
            logging.info('type_cls_name:[%s] matched!', type_cls_name)
            return RETURN_STATEMENTS[type_cls_name]
        type_only = m.group('ret_type')
        if type_only in RETURN_STATEMENTS:
            logging.info('type_only:[%s] matched!', type_only)
            return RETURN_STATEMENTS[type_only]
        return None

    @staticmethod
    def implement_function(func):
        function_def = ''
        if func.strip() == 'OpImplRegister::OpImplRegister(const ge::char_t *op_type)':
            function_def += '    : functions_(OpImplRegistry::GetInstance().CreateOrGetOpImpl(op_type))\n'
        function_def += '{\n'

        return_statements = H2CC.get_return_statements(func)
        if return_statements is not None:
            function_def += return_statements
        else:
            all_items = func.split()
            start = 0
            return_type = all_items[start]
            if return_type == "const":
                start += 1
                return_type = all_items[start]
            if return_type.startswith(('std::map', 'std::set', 'std::vector')):
                return_type = "std::map"
            if return_type.endswith('*') or (
                    len(all_items) > start + 1 and all_items[start + 1].startswith('*')) or return_type.startswith(
                'std::unique_ptr'):
                return_type = "Ptr"
            if len(all_items) > start + 1 and all_items[start + 1].startswith('&'):
                return_type += "&"
            if RETURN_STATEMENTS.__contains__(return_type):
                function_def += RETURN_STATEMENTS[return_type]
            else:
                logging.info("Unhandled func[%s]", func)
                logging.warning("Unhandled return type[%s]", return_type)

        function_def += '\n'
        function_def += '}\n'
        function_def += '\n'
        return function_def


def collect_header_files(path):
    """
    :param path:
    :return:
    """
    header_files = []
    shared_includes_content = []
    for root, dirs, files in os.walk(path):
        files.sort()
        for file in files:
            if file.find("git") >= 0:
                continue
            if not file.endswith('.h'):
                continue
            file_path = os.path.join(root, file)
            file_path = file_path.replace('\\', '/')
            header_files.append(file_path)
            include_str = '#include "{}"\n'.format(file_path[path.rindex('/') + 1:])
            shared_includes_content.append(include_str)
    # for acl error code
    shared_includes_content.append('#include "register/kernel_register_data.h"\n')
    shared_includes_content.append('#include "register/opdef/op_def_impl.h"\n')
    shared_includes_content.append('#include "register/op_impl_register_v2_impl.h"\n')
    shared_includes_content.append('#include <iostream>\n')
    return header_files, shared_includes_content


def generate_stub_file(inc_dir, out_cc_dir):
    """
    :param inc_dir:
    :param out_cc_dir:
    :return:
    """
    target_header_files, shared_includes_content = collect_header_files(inc_dir)
    for header_file in target_header_files:
        if not file_endswith_white_list_suffix(header_file):
            continue
        cc_file = re.sub(r'([^/]+)\.h$', r'stub_\1.cc', header_file)
        h_2_cc = H2CC(header_file, out_cc_dir + cc_file[cc_file.rindex('/') + 1:], shared_includes_content)
        h_2_cc.h2cc()


def gen_code(inc_dir, out_cc_dir):
    """
    :param inc_dir:
    :param out_cc_dir:
    :return:
    """
    if not inc_dir.endswith('/'):
        inc_dir += '/'
    if not out_cc_dir.endswith('/'):
        out_cc_dir += '/'
    for include_dir_key_word in include_dir_key_words:
        generate_stub_file(inc_dir + include_dir_key_word, out_cc_dir)


def main():
    if len(sys.argv) != 3:
        logging.error("script %s must have 2 input parameters!" % sys.argv[0])
        return
    inc_dir = sys.argv[1]
    out_cc_dir = sys.argv[2]
    gen_code(inc_dir, out_cc_dir)


if __name__ == '__main__':
    main()