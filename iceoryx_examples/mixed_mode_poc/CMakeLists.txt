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

cmake_minimum_required(VERSION 3.16)
project(example_mixed_mode_poc)

find_package(iceoryx_platform REQUIRED)
find_package(iceoryx_hoofs CONFIG REQUIRED)

include(IceoryxPackageHelper)
include(IceoryxPlatform)
include(IceoryxPlatformSettings)

iox_add_executable(
    TARGET      mixed-mode-poc-leader
    FILES       mixed_mode_poc_leader.cpp
    LIBS        iceoryx_hoofs::iceoryx_hoofs
)

iox_add_executable(
    TARGET      mixed-mode-poc-follower
    FILES       mixed_mode_poc_follower.cpp
    LIBS        iceoryx_hoofs::iceoryx_hoofs
)

if(USE_EXPLICIT_ALIGNMENT)
    target_compile_definitions(mixed-mode-poc-follower PUBLIC USE_EXPLICIT_ALIGNMENT)
endif()
