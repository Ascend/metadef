#!/bin/bash
# Copyright 2023 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

if [ x$1 = x ]; then
  echo "usage: $0 <dst-path>"
  exit 1
fi

set -e
set -u

local_dir=$(realpath $1)

newest_dir=$(curl -s https://ascend-repo.obs.cn-east-2.myhuaweicloud.com/CANN_daily_y2b/common/libs.txt)
newest_dir=$(echo ${newest_dir%*/})
filename=ai_cann_x86.tar.gz
newest_file=${newest_dir}/${filename}
echo "newest file URL: ${newest_file}"

local_newest_dir=$(echo "${newest_dir}" | awk -F "/" '{print $NF}')
local_newest_path="${local_dir}/$(echo "${newest_dir}" | awk -F "/" '{print $NF}')"
echo "local newest dir ${local_newest_path}"

if [ -d ${local_newest_path} ]; then
    echo "The newest package already exists,no need to update"
    echo "Newest package: ${local_newest_path}"
    exit 0
fi

echo "Download the newest file from ${newest_file} to ${local_newest_path}/${filename}..."
mkdir -p ${local_newest_path} && cd ${local_newest_path}
curl -o ${filename} ${newest_file}

echo "untar..."
tar -xf ${filename}
mv ai_cann_x86/*opensdk* ./
rm -rf ai_cann_x86
ls | grep opensdk | xargs tar -xf

cd ${local_dir}
rm -rf latest
ln -s ${local_newest_dir} latest


echo "updated successfully!"
echo "when you build in metadef: set cmake option -DASCEND_OPENSDK_DIR=${local_dir}/latest/opensdk/opensdk/"
echo "When you build in air: set env variable ASCEND_CUSTOM_PATH=${local_dir}/latest"