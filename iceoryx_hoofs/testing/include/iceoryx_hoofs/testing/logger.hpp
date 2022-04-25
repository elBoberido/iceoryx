// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_HOOFS_TESTING_LOGGER_HPP
#define IOX_HOOFS_TESTING_LOGGER_HPP

#include "iceoryx_hoofs/log/ng/logger.hpp"

#include "test.hpp"

#include <mutex>

namespace iox
{
namespace testing
{
class LogPrinter : public ::testing::EmptyTestEventListener
{
    void OnTestStart(const ::testing::TestInfo&) override;
    void OnTestPartResult(const ::testing::TestPartResult& result) override;
};


class Logger : public iox::log::ng::Logger
{
  public:
    static void activateTestLogger()
    {
        static Logger logger;
        logger.globalLogLevel = iox::log::ng::LogLevel::DEBUG;
        iox::log::ng::Logger::activeLogger(&logger);

        auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
        listeners.Append(new LogPrinter);
    }

    void clearLogBuffer()
    {
        std::lock_guard<std::mutex> lock(m_logBufferMutex);
        m_logBuffer.clear();
    }

    void printLogBuffer()
    {
        std::lock_guard<std::mutex> lock(m_logBufferMutex);
        if (m_logBuffer.empty())
        {
            return;
        }
        puts("#### Log start ####");
        for (const auto& log : m_logBuffer)
        {
            puts(log.c_str());
        }
        puts("#### Log end ####");
    }

  private:
    Logger() = default;

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;

    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    void flush() override
    {
        std::lock_guard<std::mutex> lock(m_logBufferMutex);
        m_logBuffer.emplace_back(m_buffer, m_bufferWriteIndex);

        // iox::log::ng::Logger::flush();

        m_buffer[0] = 0;
        m_bufferWriteIndex = 0U;
    }

    // TODO use smart_lock
    std::mutex m_logBufferMutex;
    std::vector<std::string> m_logBuffer;
};

void LogPrinter::OnTestStart(const ::testing::TestInfo&)
{
    dynamic_cast<Logger&>(iox::log::ng::Logger::get()).clearLogBuffer();
}

void LogPrinter::OnTestPartResult(const ::testing::TestPartResult& result)
{
    if (result.failed())
    {
        dynamic_cast<Logger&>(iox::log::ng::Logger::get()).printLogBuffer();
    }
}

} // namespace testing
} // namespace iox

#endif // IOX_HOOFS_TESTING_LOGGER_HPP
