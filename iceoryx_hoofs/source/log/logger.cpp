
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

#include "iceoryx_hoofs/log/logger.hpp"
#include "iceoryx_hoofs/log/logging.hpp"

namespace iox
{
namespace log
{
LogLevel logLevelFromEnvOr(const LogLevel logLevel)
{
    if (const auto* logLevelString = std::getenv("IOX_LOG_LEVEL"))
    {
        if (equalStrings(logLevelString, "off"))
        {
            return LogLevel::OFF;
        }
        else if (equalStrings(logLevelString, "fatal"))
        {
            return LogLevel::FATAL;
        }
        else if (equalStrings(logLevelString, "error"))
        {
            return LogLevel::ERROR;
        }
        else if (equalStrings(logLevelString, "warn"))
        {
            return LogLevel::WARN;
        }
        else if (equalStrings(logLevelString, "info"))
        {
            return LogLevel::INFO;
        }
        else if (equalStrings(logLevelString, "debug"))
        {
            return LogLevel::DEBUG;
        }
        else if (equalStrings(logLevelString, "trace"))
        {
            return LogLevel::TRACE;
        }

        IOX_LOG(WARN) << "Invalide value for 'IOX_LOG_LEVEL' environment variable: '" << logLevelString << "'";
    }
    return logLevel;
}
} // namespace log
} // namespace iox
