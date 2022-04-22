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
#ifndef IOX_HOOFS_LOG_NG_LOGSTREAM_HPP
#define IOX_HOOFS_LOG_NG_LOGSTREAM_HPP

#include "iceoryx_hoofs/log/ng/logger.hpp"

#include <string>

namespace iox
{
namespace log
{
namespace ng
{
class LogStream
{
  public:
    LogStream(const char* file, const int line, const char* function, LogLevel logLevel) noexcept
    {
        Logger::get().setupNewLogMessage(file, line, function, logLevel);
    }

    virtual ~LogStream() noexcept
    {
        flush();
    }

    void flush() noexcept
    {
        if (!m_flushed)
        {
            Logger::get().flush();
            m_flushed = true;
        }
    }

    LogStream& operator<<(const char* cstr) noexcept
    {
        Logger::get().putString(cstr);
        m_flushed = false;
        return *this;
    }

    LogStream& operator<<(const std::string& str) noexcept
    {
        Logger::get().putString(str.c_str());
        m_flushed = false;
        return *this;
    }

    template <typename T, typename std::enable_if<std::is_signed<T>::value, int>::type = 0>
    LogStream& operator<<(const T val) noexcept
    {
        Logger::get().putI64(val);
        m_flushed = false;
        return *this;
    }

    template <typename T, typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
    LogStream& operator<<(const T val) noexcept
    {
        Logger::get().putU64(val);
        m_flushed = false;
        return *this;
    }

  private:
    bool m_flushed{false};
};


// clang-format off

// TODO cmake target iceoryx_hoofs_log with
// - void initLogger(LogLevel level, bool useColors, bool printDate, bool printLogLevel, bool printFile, bool printFunction); // environment variables could be used to get the values for the parameters, e.g. initLogger(logLevelFromEnv(), useColorsFromEnvOr(true), ...)
// - void log(const char* file, const char* line, const char* namespace, const char* class, uint8_t level, char* message);
// - std::span<char, size_t> getLogBuffer();
// cmake option to provide custom lib to link against
// alternatively, register callbacks for this functions at runtime and do nothing by default
// -> this is probably better since it can easily be used to suppress the logger output for tests

// TODO check whether expression templates could be used to defer serialization of integers, etc and get length of all elements to log ... alternatively, store all elements in a variant ... or a function/function_ref which captures the data ... this might not work or be expensive for stuff like std::string

// clang-format on


// TODO use environment variable to set file or function for custom filter
// ... environment variables for this should be read in initLogger
inline bool custom(const char* file, const char* function)
{
    static_cast<void>(file);
    static_cast<void>(function);
    return false;
}

// TODO move this to logging.hpp which shall be included for logging purposes

// TODO how shall the filter work?
// - (GLOBAL_LOG_ALL || level <= globalLogLevel) && custom
// - GLOBAL_LOG_ALL || (level <= globalLogLevel && custom) <- this might be the best
// - GLOBAL_LOG_ALL || level <= globalLogLevel  || custom  <- or this if we pass the log level to the custom filter

#define IOX_LOG_INTERNAL(level)                                                                                        \
    if (level <= iox::log::ng::Logger::MINIMAL_LOG_LEVEL                                                               \
        && (iox::log::ng::Logger::GLOBAL_LOG_ALL                                                                       \
            || level <= iox::log::ng::Logger::globalLogLevel.load(std::memory_order_relaxed)                           \
            || iox::log::ng::custom(__FILE__, __PRETTY_FUNCTION__)))                                                   \
    iox::log::ng::LogStream(__FILE__, __LINE__, __PRETTY_FUNCTION__, level)

// use this
#define IOX_LOG(level) IOX_LOG_INTERNAL(iox::log::ng::LogLevel::level)

// not this, because there is no iox prefix and it will easily clash with other macros
#define LogFatal() IOX_LOG(FATAL)
#define LogError() IOX_LOG(ERROR)
#define LogWarn() IOX_LOG(WARN)
#define LogInfo() IOX_LOG(INFO)
#define LogDebug() IOX_LOG(DEBUG)
#define LogTrace() IOX_LOG(TRACE)
#define LogVerbose() IOX_LOG(TRACE)

} // namespace ng
} // namespace log
} // namespace iox

#endif // IOX_HOOFS_LOG_NG_LOGSTREAM_HPP
