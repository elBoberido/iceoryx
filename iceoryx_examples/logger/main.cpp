// Copyright (c) 2023 by Apex.AI Inc. All rights reserved.
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

#include <atomic>

enum class Category
{
    Foo,
    Bar,
};

const char* asStringLiteral(Category c)
{
    switch (c)
    {
    case Category::Foo:
        return "Foo";
    case Category::Bar:
        return "Bar";
    }
}

template <Category c>
class CategoryConsoleLoggerBase : public iox::log::ConsoleLogger
{
  public:
    static iox::log::LogLevel getLogLevel() noexcept
    {
        return s_activeLogLevel.load(std::memory_order_relaxed);
    }

    static void setLogLevel(const iox::log::LogLevel logLevel) noexcept
    {
        s_activeLogLevel.store(logLevel, std::memory_order_relaxed);
    }

  protected:
    void createLogMessageHeader(const char* file,
                                const int line,
                                const char* function,
                                iox::log::LogLevel logLevel) noexcept override
    {
        iox::log::ConsoleLogger::createLogMessageHeader(file, line, function, logLevel);
        logString("[");
        logString(asStringLiteral(c));
        logString("] ");
    }

  private:
    static std::atomic<iox::log::LogLevel> s_activeLogLevel;
};
template <Category c>
std::atomic<iox::log::LogLevel> CategoryConsoleLoggerBase<c>::s_activeLogLevel{iox::log::LogLevel::INFO};

template <Category c>
using Logger = iox::log::internal::Logger<CategoryConsoleLoggerBase<c>>;

template <Category c>
bool isLogLevelActive(iox::log::LogLevel logLevel) noexcept
{
    return logLevel <= Logger<c>::getLogLevel();
}

#define MY_LOG_INTERNAL(file, line, function, category, level)                                                         \
    iox::log::internal::LogStream<Logger<category>>(file, line, function, level, isLogLevelActive<category>(level))    \
        .self()

#define MY_LOG(category, level)                                                                                        \
    MY_LOG_INTERNAL(                                                                                                   \
        __FILE__, __LINE__, static_cast<const char*>(__FUNCTION__), Category::category, iox::log::LogLevel::level)

int main()
{
    Logger<Category::Foo>::setLogLevel(iox::log::LogLevel::DEBUG);
    Logger<Category::Bar>::setLogLevel(iox::log::LogLevel::ERROR);

    MY_LOG(Foo, INFO) << "aaa";
    MY_LOG(Bar, INFO) << "bbb";

    return EXIT_SUCCESS;
}
