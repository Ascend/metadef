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

set -e
set -u

prefix=""
NO_CA_CHECK=""

usage() {
  echo "Usage:"
  echo "    bash update_newest_packages.sh [-k] [-d <path>]"
  echo "Description:"
  echo "    -k, Allow insecure server connections when using SSL."
  echo "    -d <dir_name>, Extract files into dir_name."
}

checkopts(){
  while getopts 'd:k' opt; do
    case "${opt}" in
      k)
        NO_CA_CHECK="-k"
        ;;
      d)
        prefix="$OPTARG"
        ;;
      *)
        echo "Undefined option: ${opt}"
        usage
        exit 1
    esac
  done
}

checkopts "$@"

if [[ -z "$prefix" ]]
then
  echo "Invalid path: Please check \${prefix}"
  usage
  exit 1
fi

local_dir=$(realpath ${prefix})

newest_dir=$(curl -s ${NO_CA_CHECK} https://ascend-repo.obs.cn-east-2.myhuaweicloud.com/CANN_daily_y2b/common/libs.txt)
newest_dir=$(echo ${newest_dir%*/})
echo "${newest_dir%*/}"
filename=ai_cann_x86.tar.gz
newest_file=${newest_dir}/${filename}
echo "Newest file URL: ${newest_file}"

local_newest_dir=$(echo "${newest_dir}" | awk -F "/" '{print $NF}')
local_newest_path="${local_dir}/$(echo "${newest_dir}" | awk -F "/" '{print $NF}')"
echo "Local newest dir: ${local_newest_path}"

if [ -d ${local_newest_path} ]; then
    echo "The newest package already exists, no need to update"
    echo "Newest package: ${local_newest_path}"
    exit 0
fi

echo "Download the newest file from ${newest_file} to ${local_newest_path}/${filename}..."
mkdir -p ${local_newest_path} && cd ${local_newest_path}
curl ${NO_CA_CHECK} -o ${filename} ${newest_file}

echo "Extracting..."
tar -xf ${filename}
mv ai_cann_x86/*opensdk* ./
rm -rf ai_cann_x86
ls | grep opensdk | xargs tar -xf

cd ${local_dir}
rm -rf latest
ln -s ${local_newest_dir} latest


echo "Updated successfully!"
echo "When you build in metadef: set cmake option -DASCEND_OPENSDK_DIR=${local_dir}/latest/opensdk/opensdk/"
echo "When you build in air: set env variable ASCEND_CUSTOM_PATH=${local_dir}/latest"