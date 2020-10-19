#!/bin/bash
# Copyright 2019-2020 Huawei Technologies Co., Ltd
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
BASEPATH=$(cd "$(dirname $0)"; pwd)
OUTPUT_PATH="${BASEPATH}/output"
export BUILD_PATH="${BASEPATH}/build/"

# print usage message
usage()
{
  echo "Usage:"
  echo "sh build.sh [-j[n]] [-h] [-v] [-s] [-t] [-u] [-c]"
  echo ""
  echo "Options:"
  echo "    -h Print usage"
  echo "    -u Only compile ut, not execute"
  echo "    -s Build st"
  echo "    -j[n] Set the number of threads used for building Metadef, default is 8"
  echo "    -t Build and execute ut"
  echo "    -c Build ut with coverage tag"
  echo "    -v Display build command"
  echo "to be continued ..."
}

# parse and set options
checkopts()
{
  VERBOSE=""
  THREAD_NUM=8
  # ENABLE_METADEF_UT_ONLY_COMPILE="off"
  ENABLE_GE_UT="off"
  ENABLE_GE_ST="off"
  ENABLE_GE_COV="off"
  GE_ONLY="on"
  # Process the options
  while getopts 'ustchj:v' opt
  do
    OPTARG=$(echo ${OPTARG} | tr '[A-Z]' '[a-z]')
    case "${opt}" in
      u)
        # ENABLE_GE_UT_ONLY_COMPILE="on"
        ENABLE_GE_UT="on"
        GE_ONLY="off"
        ;;
      s)
        ENABLE_GE_ST="on"
        ;;
      t)
	      ENABLE_GE_UT="on"
	      GE_ONLY="off"
	      ;;
      c)
        ENABLE_GE_COV="on"
        GE_ONLY="off"
        ;;
      h)
        usage
        exit 0
        ;;
      j)
        THREAD_NUM=$OPTARG
        ;;
      v)
        VERBOSE="VERBOSE=1"
        ;;
      *)
        echo "Undefined option: ${opt}"
        usage
        exit 1
    esac
  done
}
checkopts "$@"

mk_dir() {
    local create_dir="$1"  # the target to make

    mkdir -pv "${create_dir}"
    echo "created ${create_dir}"
}

# Meatdef build start
echo "---------------- Metadef build start ----------------"

# create build path
build_metadef()
{
  echo "create build directory and build Metadef";
  mk_dir "${BUILD_PATH}/metadef"
  cd "${BUILD_PATH}/metadef"
  CMAKE_ARGS="-DBUILD_PATH=$BUILD_PATH -DGE_ONLY=$GE_ONLY"

  if [[ "X$ENABLE_GE_COV" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GE_COV=ON"
  fi

  if [[ "X$ENABLE_GE_UT" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GE_UT=ON"
  fi


  if [[ "X$ENABLE_GE_ST" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GE_ST=ON"
  fi

  CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_OPEN_SRC=True"
  echo "${CMAKE_ARGS}"
  cmake ${CMAKE_ARGS} ../..
  make ${VERBOSE} -j${THREAD_NUM}
  echo "Metadef build success!"
}
g++ -v
build_metadef
echo "---------------- Metadef build finished ----------------"
mk_dir ${OUTPUT_PATH}
find "${BUILD_PATH}/metadef" -name "*.so*" -exec cp -f {} "${OUTPUT_PATH}"
rm -rf "${OUTPUT_PATH}/"libproto*
rm -f ${OUTPUT_PATH}/libgmock*.so
rm -f ${OUTPUT_PATH}/libgtest*.so
rm -f ${OUTPUT_PATH}/lib*_stub.so

chmod -R 750 ${OUTPUT_PATH}
find ${OUTPUT_PATH} -name "*.so*" -print0 | xargs -0 chmod 500

echo "---------------- Metadef output generated ----------------"

# generate output package in tar form, including ut/st libraries/executables
generate_package()
{
  cd "${BASEPATH}"

  METADEF_PATH="metadef/lib64"

  METADEF_LIB=("libregister.so" "libgraph.so")

  rm -rf ${OUTPUT_PATH:?}/${METADEF_PATH}/

  find output/ -name metadef_lib.tar -exec rm {} \;

  mk_dir "${OUTPUT_PATH}/${METADEF_PATH}/

  cd "${OUTPUT_PATH}"
  for lib in "${METADEF_LIB[@]}";
  do
    find output/ -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${METADEF_PATH}/ \;
  done

  tar -cf metadef_lib.tar metadef
}

if [[ "X$ENABLE_GE_UT" = "Xoff" ]]; then
  generate_package
fi
echo "---------------- Metadef package archive generated ----------------"
