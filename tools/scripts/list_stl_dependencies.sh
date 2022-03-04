#!/bin/bash

# Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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


#=====================================================
# usage:
#   ./list_stl_dependencies.sh
#=====================================================

COMPONENTS=(iceoryx_hoofs iceoryx_posh)
SOURCE_DIR=(source include)
WORKSPACE=$(git rev-parse --show-toplevel)

for COMPONENT in ${COMPONENTS[@]}; do
    for DIR in ${SOURCE_DIR[@]}; do
        GREP_PATH="${GREP_PATH} ${WORKSPACE}/${COMPONENT}/$DIR"
    done
done

echo
echo "usage of system header includes (excluding comments)"
gcc -w -fpreprocessed -dD -E $(find $GREP_PATH -type f -iname *.cpp -o -iname *.hpp -o -iname *.inl) 2> /dev/null\
 | grep -e "#include <" \
 | sort \
 | uniq

# todo: Check against header whitelist here

echo
echo "usage of std components (excluding comments)"
gcc -w -fpreprocessed -dD -E $(find $GREP_PATH -type f -iname *.cpp -o -iname *.hpp -o -iname *.inl) 2> /dev/null\
 | grep -HIne "std::" \
 | sed -n  "s/\([^:]*\:[0-9]*\)\:.*\(std::[a-zA-Z_]*\).*/\ \ \1  \2/p" \
 | sort \
 | uniq



exit 1

echo
echo "files with stl dependency (including comments)"
grep -RIne "std::" $GREP_PATH | sed -n  "s/\([^:]*\)\:[0-9]*\:.*std::[a-zA-Z_]*.*/\1/p" | xargs -I{} basename {} | sort | uniq

echo
echo "using namespace with std component (including comments)"
grep -RIne ".*using[ ]*namespace[ ]*std" $GREP_PATH | sed -n "s/\(.*\)/\ \ \1/p"

echo
echo "usage of std components (including comments)""
grep -RIne "std::" $GREP_PATH | sed -n  "s/.*\(std::[a-zA-Z_]*\).*/\ \ \1/p" | sort | uniq


