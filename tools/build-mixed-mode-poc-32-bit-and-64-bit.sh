#!/bin/bash

# Copyright (c) 2024 by ekxide IO GmbH. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

set -e

COLOR_RESET='\033[0m'
COLOR_GREEN='\033[1;32m'
COLOR_YELLOW='\033[1;33m'

WORKSPACE=$(git rev-parse --show-toplevel)
NUM_JOBS=$(nproc)
CLEAN_BUILD=false
EXPLICIT_ALIGNMENT_FLAG=OFF
MALIGN_DOUBLE_OPTION=""

while (( "$#" )); do
  case "$1" in
    -j|--jobs)
        NUM_JOBS="$2"
        shift 2
        ;;
    "clean")
        CLEAN_BUILD=true
        shift 1
        ;;
    "explicit-alignment")
        EXPLICIT_ALIGNMENT_FLAG=ON
        shift 1
        ;;
    "malign-double")
        MALIGN_DOUBLE_OPTION="-malign-double"
        shift 1
        ;;
    "help")
        echo "Build script for the 32-64 bit mixed mode PoC."
        echo ""
        echo "Options:"
        echo "    -j --jobs             Specify the number of build jobs"
        echo "Args:"
        echo "    clean                 Delete the build/ directory before build-step"
        echo "    malign-double         Use the malign-double compiler flag to align double, long double,"
        echo "                          and long long on double word boundaries"
        echo "    explicit-alignment    Use explicit alignment"
        echo "    help                  Print this help"
        echo ""
        exit 0
        ;;
    *)
        echo "Invalid argument '$1'. Try 'help' for options."
        exit 1
        ;;
  esac
done

cd ${WORKSPACE}

if [ "$CLEAN_BUILD" == true ] && [ -d "${WORKSPACE}/build" ]; then
    echo "${COLOR_GREEN}# Cleaning build directory${COLOR_RESET}"
    rm -rf "${WORKSPACE}/build/*"
fi

echo -e "${COLOR_GREEN}# Building 64 bit iceoryx${COLOR_RESET}"
cmake -S iceoryx_meta \
      -B build/64/iceoryx \
      -DEXAMPLES=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=build/install/64
cmake --build build/64/iceoryx --target install -- -j$NUM_JOBS

echo -e "${COLOR_GREEN}# Building 32 bit iceoryx${COLOR_RESET}"
cmake -S iceoryx_meta \
      -B build/32/iceoryx \
      -DEXAMPLES=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=build/install/32 \
      -DCMAKE_C_FLAGS="-m32 ${MALIGN_DOUBLE_OPTION}" \
      -DCMAKE_CXX_FLAGS="-m32 ${MALIGN_DOUBLE_OPTION}"
cmake --build build/32/iceoryx --target install -- -j$NUM_JOBS

echo -e "${COLOR_GREEN}# Building 64 bit mixed-mode-poc${COLOR_RESET}"
cmake -S iceoryx_examples/mixed_mode_poc \
      -B build/64/example \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=${WORKSPACE}/build/install/64 \
      -DUSE_EXPLICIT_ALIGNMENT==${EXPLICIT_ALIGNMENT_FLAG}
cmake --build build/64/example

echo -e "${COLOR_GREEN}# Building 32 bit mixed-mode-poc${COLOR_RESET}"
cmake -S iceoryx_examples/mixed_mode_poc \
      -B build/32/example \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=${WORKSPACE}/build/install/32 \
      -DCMAKE_C_FLAGS="-m32 ${MALIGN_DOUBLE_OPTION}" \
      -DCMAKE_CXX_FLAGS="-m32 ${MALIGN_DOUBLE_OPTION}" \
      -DUSE_EXPLICIT_ALIGNMENT=${EXPLICIT_ALIGNMENT_FLAG}
cmake --build build/32/example

echo -e "${COLOR_GREEN}# Build finished! The binaries can be found in 'build/64/example' and 'build/32/example'!${COLOR_RESET}"
