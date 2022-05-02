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
#include <mutex>

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

    template <typename T>
    inline void unused(T&&) const
    {
    }

  protected:
    static Logger& activeLogger(Logger* newLogger = nullptr)
    {
        std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        static uint64_t loggerChangeCounter{0};
        ++loggerChangeCounter;
        static Logger defaultLogger;
        static Logger* logger{&defaultLogger};

        if (newLogger)
        {
            // TODO if necessary, this could be done in a way that threads which are already logging can detect a new
            // logger, e.g.:
            //    - loggerChangeCounter as atomic in the Logger itself
            //    - Logger::get has a thread_local copy of this value and checks if the global counter changes
            //    - Logger::get stores a pointer instead of a reference and calls Logger::activeLogger() if the counter
            //    are not equal
            // changing the logger should still notify the user with LogLevel::Debug if the logger was changed more than
            // once since this indicates that someone is tampering with the logger
            if (loggerChangeCounter > 1)
            {
                logger->setupNewLogMessage(__FILE__, __LINE__, __PRETTY_FUNCTION__, LogLevel::ERROR);
                logger->putString(
                    "Logger already used before changing the backend! Some threads will not use the new backend!");
                logger->flush();
                newLogger->setupNewLogMessage(__FILE__, __LINE__, __PRETTY_FUNCTION__, LogLevel::ERROR);
                newLogger->putString(
                    "Logger already used before changing the backend! Some threads will not use the new backend!");
                newLogger->flush();
            }
            logger = newLogger;
        }

        return *logger;
    }

  public:
    static Logger& get()
    {
        thread_local static Logger& logger = Logger::activeLogger();
        return logger;
    }

    virtual void setupNewLogMessage(const char* file, const int line, const char* function, LogLevel logLevel)
    {
        // TODO check all pointer for nullptr

        timespec timestamp{0, 0};
        if (clock_gettime(CLOCK_REALTIME, &timestamp) != 0)
        {
            timestamp = {0, 0};
            // intentionally do nothing since a timestamp from 01.01.1970 already indicates  an issue with the clock
        }

        time_t time = timestamp.tv_sec;
        struct tm calendarData;

        // TODO check whether localtime_s would be the better solution
        auto* timeInfo = localtime_r(&time, &calendarData);
        if (timeInfo == nullptr)
        {
            // TODO an error occurred; what to do next? return? don't use the timestamp? call error handler?
        }

        constexpr const char TIME_FORMAT[]{"2002-02-20 22:02:02"};
        constexpr uint32_t ZERO_TERMINATION{1U};
        constexpr uint32_t YEAR_1M_PROBLEM{2U}; // in case iceoryx is still in use, please change to 3
        constexpr auto TIMESTAMP_BUFFER_SIZE{Logger::stringLength(TIME_FORMAT) + YEAR_1M_PROBLEM + ZERO_TERMINATION};
        char timestampString[TIMESTAMP_BUFFER_SIZE]{0};
        auto strftimeRetVal = strftime(timestampString,
                                       TIMESTAMP_BUFFER_SIZE - 1, // TODO check whether the -1 is required
                                       "%Y-%m-%d %H:%M:%S",
                                       timeInfo);
        if (strftimeRetVal == 0)
        {
            // TODO an error occurred; what to do next? return? don't use the timestamp? call error handler?
        }

        constexpr auto MILLISECS_PER_SECOND{1000};
        auto milliseconds = timestamp.tv_nsec % MILLISECS_PER_SECOND;

        // TODO do we also want to always log the iceoryx version and commit sha? Maybe do that only in
        // `initLogger` with LogDebug

        unused(file);
        unused(line);
        unused(function);
        // << "\033[0;90m " << file << ':' << line << " ‘" << function << "’";

        // TODO check whether snprintf_s would be the better solution
        // TODO double check whether snprintf is correctly used
        auto retVal = snprintf(m_buffer,
                               NULL_TERMINATED_BUFFER_SIZE,
                               "\033[0;90m%s.%03ld %s%s\033[m: ",
                               timestampString,
                               milliseconds,
                               LOG_LEVEL_COLOR[static_cast<uint8_t>(logLevel)],
                               LOG_LEVEL_TEXT[static_cast<uint8_t>(logLevel)]);
        if (retVal >= 0)
        {
            m_bufferWriteIndex = static_cast<uint32_t>(retVal);
        }
        else
        {
            // TODO an error occurred; what to do next? call error handler?
        }
    }

    virtual void putString(const char* message)
    {
        auto retVal =
            snprintf(&m_buffer[m_bufferWriteIndex],
                     NULL_TERMINATED_BUFFER_SIZE - m_bufferWriteIndex,
                     "%s",
                     message); // TODO do we need to check whether message is null-terminated at a reasonable length?
        if (retVal >= 0)
        {
            m_bufferWriteIndex += static_cast<uint32_t>(retVal);
        }
        else
        {
            // TODO an error occurred; what to do next? call error handler?
        }
    }

    // TODO add `putU32(const uint32_t)`, putBool(const bool), ...
    virtual void putI64Dec(const int64_t value)
    {
        putArithmetik(value, "%li");
    }
    virtual void putU64Dec(const uint64_t value)
    {
        putArithmetik(value, "%lu");
    }
    virtual void putU64Hex(const uint64_t value)
    {
        putArithmetik(value, "%x");
    }
    virtual void putU64Oct(const uint64_t value)
    {
        putArithmetik(value, "%o");
    }

    template <typename T>
    inline void putArithmetik(const T value, const char* format)
    {
        auto retVal =
            snprintf(&m_buffer[m_bufferWriteIndex],
                     NULL_TERMINATED_BUFFER_SIZE - m_bufferWriteIndex,
                     format,
                     value); // TODO do we need to check whether message is null-terminated at a reasonable length?
        if (retVal >= 0)
        {
            m_bufferWriteIndex += static_cast<uint32_t>(retVal);
        }
        else
        {
            // TODO an error occurred; what to do next? call error handler?
        }
    }

    virtual void flush()
    {
        if (std::puts(m_buffer) < 0)
        {
            // TODO an error occurred; what to do next? call error handler?
        }
        m_buffer[0] = 0;
        m_bufferWriteIndex = 0U;
    };

  protected:
    Logger() = default;

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

  protected:
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
