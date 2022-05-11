// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_HOOFS_LOG_LOGGER_HPP
#define IOX_HOOFS_LOG_LOGGER_HPP

#include "iceoryx_hoofs/log/platform/logger.hpp"

namespace iox
{
namespace log
{
using LogLevel = platform::LogLevel;
using platform::asStringLiteral;

using Logger = platform::Logger;

inline Logger& getLogger()
{
    return Logger::get<platform::DefaultLogger>();
}

inline Logger* setActiveLogger(Logger* newLogger)
{
    return Logger::activeLogger<platform::DefaultLogger>(newLogger);
}

LogLevel logLevelFromEnvOr(const LogLevel logLevel);

inline void initLogger(const LogLevel logLevel = logLevelFromEnvOr(LogLevel::INFO))
{
    getLogger().initLogger(logLevel);
}

template <uint32_t N>
inline constexpr bool equalStrings(const char* lhs, const char (&rhs)[N])
{
    return strncmp(lhs, rhs, N) == 0;
}

} // namespace log
} // namespace iox

#endif // IOX_HOOFS_LOG_LOGGER_HPP
