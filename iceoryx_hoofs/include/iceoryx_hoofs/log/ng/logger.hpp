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

#ifndef IOX_HOOFS_LOG_NG_LOGGER_HPP
#define IOX_HOOFS_LOG_NG_LOGGER_HPP

#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

namespace iox
{
namespace log
{
namespace ng
{
enum class LogLevel : uint8_t
{
    OFF = 0,
    FATAL,
    ERROR,
    WARN,
    INFO,
    DEBUG,
    TRACE // this could be used instead of commenting the code; with MINIMAL_LOG_LEVEL set to DEBUG, the compiler would
          // optimize this out and there wouldn't be a performance hit
};

constexpr const char* LOG_LEVEL_COLOR[] = {
    "",                 // nothing
    "\033[0;1;97;41m",  // bold bright white on red
    "\033[0;1;31;103m", // bold red on light yellow
    "\033[0;1;93m",     // bold bright yellow
    "\033[0;1;92m",     // bold bright green
    "\033[0;1;96m",     // bold bright cyan
    "\033[0;1;36m",     // bold cyan
};

constexpr const char* LOG_LEVEL_TEXT[] = {
    "[ Off ]", // nothing
    "[Fatal]", // bold bright white on red
    "[Error]", // bold red on light yellow
    "[Warn ]", // bold bright yellow
    "[Info ]", // bold bright green
    "[Debug]", // bold bright cyan
    "[Trace]", // bold cyan
};

class Logger
{
  private:
    template <uint32_t N>
    static constexpr uint32_t stringLength(const char (&)[N])
    {
        return N;
    }

  public:
    static Logger& get()
    {
        static Logger logger;
        return logger;
    }

    // TODO split this into `setupNewLog(file, line, function, logLevel, timestamp)`
    //      and `putString(const char*)`, `putU64(const uint64_t)`...
    virtual void log(const char* file,
                     const int line,
                     const char* function,
                     LogLevel logLevel,
                     timespec timestamp,
                     std::string message)
    {
        std::time_t time = timestamp.tv_sec;

        // TODO: std::localtime is thread-unsafe -> replace with localtime_r or strftime
        auto* timeInfo = std::localtime(&time);

        constexpr const char TIME_FORMAT[]{"2002-02-20 11:11:11"};
        constexpr uint32_t ZERO_TERMINATION{1U};
        constexpr uint32_t YEAR_1000000_PROBLEM{2U}; // in case iceoryx is still in use, please change to 3
        constexpr auto TIMESTAMP_BUFFER_SIZE{Logger::stringLength(TIME_FORMAT) + YEAR_1000000_PROBLEM
                                             + ZERO_TERMINATION};
        char timestampString[TIMESTAMP_BUFFER_SIZE];
        strftime(timestampString,
                 TIMESTAMP_BUFFER_SIZE - 1,
                 "%Y-%m-%d %H:%M:%S",
                 timeInfo); // TODO check whether the -1 is required


        constexpr auto MILLISECS_PER_SECOND{1000};
        auto milliseconds = timestamp.tv_nsec % MILLISECS_PER_SECOND;

        // TODO do we also want to always log the iceoryx version and commit sha? Maybe do that only in
        // `initLogger` with LogDebug
        // buffer << "\033[0;90m " << file << ':' << line << " ‘" << function << "’";
        static_cast<void>(file);
        static_cast<void>(line);
        static_cast<void>(function);

        // TODO check whether snprintf_s would be the better solution
        // TODO double check whether snprintf is correctly used
        auto retVal = snprintf(m_buffer,
                               NULL_TERMINATED_BUFFER_SIZE,
                               "\033[0;90m%s.%03ld %s%s\033[m: %s",
                               timestampString,
                               milliseconds,
                               LOG_LEVEL_COLOR[static_cast<uint8_t>(logLevel)],
                               LOG_LEVEL_TEXT[static_cast<uint8_t>(logLevel)],
                               message.c_str());
        if (retVal >= 0)
        {
            m_bufferWriteIndex = static_cast<uint32_t>(retVal);
        }
        else
        {
            // TODO an error occurred; what to do next?
        }
    }

    virtual void flush()
    {
        std::puts(m_buffer);
        m_buffer[0] = 0;
        m_bufferWriteIndex = 0U;
    };

  protected:
    Logger() = default;

    template <typename T>
    void unused(T&&) const
    {
    }

  private:
    // TODO create accessor functions for the global variables
  public:
    static std::atomic<LogLevel> globalLogLevel; // initialized in corresponding cpp file

    // TODO make this a compile time option since if will reduce performance but some logger might want to do the
    // filtering by themself
    static constexpr bool GLOBAL_LOG_ALL{false};

    // TODO compile time option for minimal compiled log level, i.e. all lower log level should be optimized out
    // this is different than GLOBAL_LOG_ALL since globalLogLevel could still be set to off
    static constexpr LogLevel MINIMAL_LOG_LEVEL{LogLevel::TRACE};

  private:
    static constexpr uint32_t BUFFER_SIZE{1024}; // TODO compile time option?
    static constexpr uint32_t NULL_TERMINATED_BUFFER_SIZE{BUFFER_SIZE + 1};
    thread_local static char m_buffer[NULL_TERMINATED_BUFFER_SIZE];
    thread_local static uint32_t m_bufferWriteIndex; // initialized in corresponding cpp file
    // TODO thread local storage with thread id
};

} // namespace ng
} // namespace log
} // namespace iox

#endif // IOX_HOOFS_LOG_NG_LOGGER_HPP
