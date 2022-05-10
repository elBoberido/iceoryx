// Copyright (c) 2019, 2021 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef IOX_HOOFS_LOG_NG_LOGGING_HPP
#define IOX_HOOFS_LOG_NG_LOGGING_HPP

#include "iceoryx_hoofs/log/ng/logstream.hpp"
namespace iox
{
namespace log
{
namespace ng
{
// TODO how shall the filter work?
// - (ignoreActiveLogLevel || level <= activeLogLevel) && custom
// - ignoreActiveLogLevel || (level <= activeLogLevel && custom) <- this might be the best
// - ignoreActiveLogLevel || level <= activeLogLevel  || custom  <- or this if we pass the log level to the custom
// filter

#define IOX_LOG_INTERNAL(file, line, function, level)                                                                  \
    if ((level) <= iox::log::ng::Logger::minimalLogLevel()                                                             \
        && (iox::log::ng::Logger::ignoreActiveLogLevel() || (level) <= iox::log::ng::Logger::activeLogLevel()          \
            || iox::log::ng::custom(file, function)))                                                                  \
    iox::log::ng::LogStream(file, line, function, level).self()

// use this
#define IOX_LOG(level) IOX_LOG_INTERNAL(__FILE__, __LINE__, __FUNCTION__, iox::log::ng::LogLevel::level)

} // namespace ng
} // namespace log
} // namespace iox

#endif // IOX_HOOFS_LOG_NG_LOGGING_HPP
