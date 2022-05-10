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


inline constexpr const char* asStringLiteral(const LogLevel value) noexcept
{
    switch (value)
    {
    case LogLevel::OFF:
        return "LogLevel::OFF";
    case LogLevel::FATAL:
        return "LogLevel::FATAL";
    case LogLevel::ERROR:
        return "LogLevel::ERROR";
    case LogLevel::WARN:
        return "LogLevel::WARN";
    case LogLevel::INFO:
        return "LogLevel::INFO";
    case LogLevel::DEBUG:
        return "LogLevel::DEBUG";
    case LogLevel::TRACE:
        return "LogLevel::TRACE";
    }

    return "[Undefined LogLevel]";
}

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

class ConsoleLogger;

class Logger
{
  protected:
    template <uint32_t N>
    static constexpr bool equalStrings(const char* lhs, const char (&rhs)[N])
    {
        return strncmp(lhs, rhs, N) == 0;
    }

    static Logger* activeLogger(Logger* newLogger = nullptr);

  public:
    static Logger& get()
    {
        thread_local static Logger* logger = Logger::activeLogger();
        if (!logger->m_isActive.load(std::memory_order_relaxed))
        {
            // no need to loop until m_isActive is true since this is an inherent race
            //   - the logger needs to be active for the whole lifetime of the application anyway
            //   - if the logger was changed again, the next call will update the logger
            //   - furthermore, it is not recommended to change the logger more than once
            logger = Logger::activeLogger();
        }
        return *logger;
    }

    static constexpr LogLevel minimalLogLevel()
    {
        return Logger::MINIMAL_LOG_LEVEL;
    }

    static constexpr bool ignoreActiveLogLevel()
    {
        return Logger::IGNORE_ACTIVE_LOG_LEVEL;
    }

    static LogLevel activeLogLevel()
    {
        return Logger::m_activeLogLevel.load(std::memory_order_relaxed);
    }

    static LogLevel logLevelFromEnvOr(const LogLevel logLevel)
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
            else
            {
                auto& logger = Logger::get();
                logger.setupNewLogMessage(__FILE__, __LINE__, __FUNCTION__, LogLevel::WARN);
                logger.logString("Invalide value for 'IOX_LOG_LEVEL' environment variable: '");
                logger.logString(logLevelString);
                logger.logString("'");
                logger.flush();
            }
        }
        return logLevel;
    }

    static void init(const LogLevel logLevel = logLevelFromEnvOr(LogLevel::INFO))
    {
        Logger::get().m_activeLogLevel.store(logLevel, std::memory_order_relaxed);
    }

    // TODO add a setLogLevel static function

  protected:
    friend class LogStream;

    virtual void setupNewLogMessage(const char* file, const int line, const char* function, LogLevel logLevel) = 0;

    void logString(const char* message)
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

    // TODO add addBool(const bool), ...
    void logI64Dec(const int64_t value)
    {
        logArithmetik(value, "%li");
    }
    void logU64Dec(const uint64_t value)
    {
        logArithmetik(value, "%lu");
    }
    void logU64Hex(const uint64_t value)
    {
        logArithmetik(value, "%x");
    }
    void logU64Oct(const uint64_t value)
    {
        logArithmetik(value, "%o");
    }

    template <typename T>
    inline void logArithmetik(const T value, const char* format)
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

    virtual void flush() = 0;

  private:
    // TODO with the introduction of m_isActive this shouldn't need to be static -> check ... on the other side, when
    // the log level shall be changed after Logger::init, this needs to stay an atomic and m_isActive needs to be
    // changed to a counter with 0 being inactive
    static std::atomic<LogLevel> m_activeLogLevel; // initialized in corresponding cpp file

    // TODO make this a compile time option since if will reduce performance but some logger might want to do the
    // filtering by themself
    static constexpr bool IGNORE_ACTIVE_LOG_LEVEL{false};

    // TODO compile time option for minimal compiled log level, i.e. all lower log level should be optimized out
    // this is different than IGNORE_ACTIVE_LOG_LEVEL since m_activeLogLevel could still be set to off
    static constexpr LogLevel MINIMAL_LOG_LEVEL{LogLevel::TRACE};

  protected:
    std::atomic<bool> m_isActive{true};
    static constexpr uint32_t BUFFER_SIZE{1024}; // TODO compile time option?
    static constexpr uint32_t NULL_TERMINATED_BUFFER_SIZE{BUFFER_SIZE + 1};
    thread_local static char m_buffer[NULL_TERMINATED_BUFFER_SIZE];
    thread_local static uint32_t m_bufferWriteIndex; // initialized in corresponding cpp file
    // TODO thread local storage with thread id
};

class ConsoleLogger : public Logger
{
    friend class Logger;

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
    ConsoleLogger() = default;

    void setupNewLogMessage(const char* file, const int line, const char* function, LogLevel logLevel) override
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
        constexpr auto TIMESTAMP_BUFFER_SIZE{ConsoleLogger::stringLength(TIME_FORMAT) + YEAR_1M_PROBLEM
                                             + ZERO_TERMINATION};
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

    void flush() override
    {
        if (std::puts(m_buffer) < 0)
        {
            // TODO an error occurred; what to do next? call error handler?
        }
        m_buffer[0] = 0;
        m_bufferWriteIndex = 0U;
    };
};

inline Logger* Logger::activeLogger(Logger* newLogger)
{
    std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    static ConsoleLogger defaultLogger;
    static Logger* logger{&defaultLogger};

    if (newLogger)
    {
        static uint64_t loggerChangeCounter{0};
        ++loggerChangeCounter;
        if (loggerChangeCounter > 1)
        {
            logger->setupNewLogMessage(__FILE__, __LINE__, __FUNCTION__, LogLevel::ERROR);
            logger->logString("Logger backend changed multiple times! This is not recommended! Change counter = ");
            logger->logU64Dec(loggerChangeCounter);
            logger->flush();
            newLogger->setupNewLogMessage(__FILE__, __LINE__, __FUNCTION__, LogLevel::ERROR);
            logger->logString("Logger backend changed multiple times! This is not recommended! Change counter = ");
            logger->logU64Dec(loggerChangeCounter);
            newLogger->flush();
        }
        logger->m_isActive.store(false);
        logger = newLogger;
    }

    return logger;
}

} // namespace ng
} // namespace log
} // namespace iox

#endif // IOX_HOOFS_LOG_NG_LOGGER_HPP
