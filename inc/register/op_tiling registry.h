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

#ifndef INC_REGISTER_OP_TILING_REGISTRY_H_
#define INC_REGISTER_OP_TILING_REGISTRY_H_

#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>
#include "external/register/register_types.h"
#include "external/graph/tensor.h"

#define REGISTER_OP_TILING_FUNC(optype, opfunc)                     \
    REGISTER_OP_TILING_FUNC_UNIQ_HELPER(optype, opfunc, __COUNTER__)

#define REGISTER_OP_TILING_FUNC_UNIQ_HELPER(optype, opfunc, counter)    \
    REGISTER_OP_TILING_FUNC_UNIQ(optype, opfunc, counter)

#define REGISTER_OP_TILING_FUNC_UNIQ(optype, opfunc, counter)    \
    static OpTilingInterf g_##optype##TilingInterf##counter(#optype, opfunc)

#define REGISTER_OP_TILING_FUNC_NEW(optype, opfunc)                     \
    REGISTER_OP_TILING_FUNC_UNIQ_HELPER_NEW(optype, opfunc, __COUNTER__)

#define REGISTER_OP_TILING_FUNC_UNIQ_HELPER_NEW(optype, opfunc, counter)    \
    REGISTER_OP_TILING_FUNC_UNIQ_NEW(optype, opfunc, counter)

#define REGISTER_OP_TILING_FUNC_UNIQ_NEW(optype, opfunc, counter)    \
    static OpTilingRegistryInterf g_##optype##TilingRegistryInterf##counter(#optype, opfunc)

namespace optiling {

enum TensorArgType {
    TA_NONE,
    TA_SINGLE,
    TA_LIST,
};


using ByteBuffer = std::stringstream;

struct TeOpTensor {
    std::vector<int64_t> shape;
    std::vector<int64_t> ori_shape;
    std::string format;
    std::string ori_format;
    std::string dtype;
    std::map<std::string, std::string> attrs;
};


struct TeOpTensorArg {
    TensorArgType arg_type;
    std::vector<TeOpTensor> tensor;
};

struct OpRunInfo {
    uint32_t block_dim;
    std::vector<int64_t> workspaces;
    ByteBuffer tiling_data;
    bool clear_atomic;
};


using TeOpAttrArgs = std::vector<std::string>;
using TeConstTensorData = std::tuple<const uint8_t*, size_t, ge::Tensor>;

struct TeOpParas {
    std::vector<TeOpTensorArg> inputs;
    std::vector<TeOpTensorArg> outputs;
    std::map<std::string, TeConstTensorData> const_inputs;
    TeOpAttrArgs attrs;
    std::string op_type;
};


using OpTilingFunc = std::function<bool(const std::string&, const TeOpParas&,
                                        const nlohmann::json& , OpRunInfo&)>;

using OpTilingFuncPtr = bool(*)(const std::string&, const TeOpParas&, const nlohmann::json& , OpRunInfo&);

class FMK_FUNC_HOST_VISIBILITY OpTilingInterf
{
public:
    OpTilingInterf(std::string op_type, OpTilingFunc func);
    ~OpTilingInterf() = default;
    static std::map<std::string, OpTilingFunc> &RegisteredOpInterf();
    static std::string OpTilingUuid;
};

struct OpCompileInfo {
    std::string str;
    std::string key;
};

using OpTilingFuncNew = std::function<bool(const TeOpParas&, const OpCompileInfo& , OpRunInfo&)>;

using OpTilingFuncPtrNew = bool(*)(const TeOpParas&, const OpCompileInfo& , OpRunInfo&);

class FMK_FUNC_HOST_VISIBILITY OpTilingRegistryInterf
{
public:
    OpTilingRegistryInterf(std::string op_type, OpTilingFuncNew func);
    ~OpTilingRegistryInterf() = default;
    static std::map<std::string, OpTilingFuncNew> &RegisteredOpInterfNew();
};

template <class T>
ByteBuffer& ByteBufferPut(ByteBuffer &buf, const T &value)
{
    buf.write(reinterpret_cast<const char*>(&value), sizeof(value));
    buf.flush();
    return buf;
}

template <class T>
ByteBuffer& ByteBufferGet(ByteBuffer &buf, T &value)
{
    buf.read(reinterpret_cast<char*>(&value), sizeof(value));
    return buf;
}

inline size_t ByteBufferGetAll(ByteBuffer &buf, char *dest, size_t dest_len)
{
    size_t nread = 0;
    size_t rn = 0;
    do {
        rn = buf.readsome(dest + nread, dest_len - nread);
        nread += rn;
    } while (rn > 0 && dest_len > nread);

    return nread;
}
}

#endif // INC_REGISTER_OP_TILING_REGISTRY_H_
