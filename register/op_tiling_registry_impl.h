#ifndef __OP_TILING_REGISTRY_IMPL_H__
#define __OP_TILING_REGISTRY_IMPL_H__

#include "external/graph/tensor.h"
#include "register/op_tiling_registry.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace optiling {
namespace utils {
using ByteBuffer = std::stringstream;

class OpRunInfoImpl {
 public:
  OpRunInfoImpl() = default;
  ~OpRunInfoImpl() = default;

  OpRunInfoImpl &operator=(const OpRunInfoImpl &runinfo);

  OpRunInfoImpl::OpRunInfoImpl(uint32_t block_dim, bool clear_atomic, uint32_t tiling_key)
    : block_dim(block_dim), clear_atomic(clear_atomic), tiling_key(tiling_key) {}

  void SetBlockDim(uint32_t block_dim);
  uint32_t GetBlockDim();

  void AddWorkspace(int64_t workspace);
  size_t GetWorkspaceNum();
  ge::graphStatus GetWorkspace(size_t idx, int64_t &workspace);
  ge::graphStatus GetAllWorkspaces(std::vector<int64_t> &workspace);

  void AddTilingData(const char *value, size_t size);
  ByteBuffer &GetAllTilingData();
  void SetAllTilingData(ByteBuffer &value);

  void SetClearAtomic(bool clear_atomic);
  bool GetClearAtomic() const;

  void SetTilingKey(uint32_t tiling_key);
  uint32_t GetTilingKey() const;

  uint32_t block_dim;
  bool clear_atomic;
  uint32_t tiling_key;
  ByteBuffer tiling_data;
  std::vector<int64_t> workspaces;
};

class OpCompileInfoImpl {
 public:
  OpCompileInfoImpl() = default;
  ~OpCompileInfoImpl() = default;
  OpCompileInfoImpl(const std::string &key, const std::string &value);

  void SetKey(const std::string &key);
  const std::string &GetKey() const;

  void SetValue(const std::string &value);
  const std::string &GetValue() const;

  std::string str;
  std::string key;
};
}  // namespace utils
}  // namespace optiling

#endif