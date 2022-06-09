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

//! [iceoryx includes]
#include "user_header_and_payload_types.hpp"

#include "iceoryx_hoofs/posix_wrapper/signal_watcher.hpp"
//! [include differs from typed C++ API]
#include "iceoryx_posh/popo/untyped_publisher.hpp"
//! [include differs from typed C++ API]
#include "iceoryx_posh/runtime/posh_runtime.hpp"
//! [iceoryx includes]

#include <atomic>
#include <iostream>

#include "iceoryx_hoofs/log/logger.hpp"
#include "iceoryx_hoofs/log/logging.hpp"

class MyLogger : public iox::log::Logger
{
  public:
    static void init()
    {
        static MyLogger myLogger;
        iox::log::Logger::setActiveLogger(&myLogger);
        iox::log::Logger::init(iox::log::logLevelFromEnvOr(iox::log::LogLevel::INFO));
    }

  private:
    void setupNewLogMessageHook(const char*, const int, const char*, iox::log::LogLevel logLevel) override
    {
        switch (logLevel)
        {
        case iox::log::LogLevel::FATAL:
            logString("💀: ");
            break;
        case iox::log::LogLevel::ERROR:
            logString("🙈: ");
            break;
        case iox::log::LogLevel::WARN:
            logString("🙀: ");
            break;
        case iox::log::LogLevel::INFO:
            logString("💘: ");
            break;
        case iox::log::LogLevel::DEBUG:
            logString("🐞: ");
            break;
        case iox::log::LogLevel::TRACE:
            logString("🐾: ");
            break;
        default:
            logString("🐔: ");
        }
    }

    void flushHook() override
    {
        const auto buffer = iox::log::Logger::getLogBuffer();
        const auto logEntry = std::get<0>(buffer);
        puts(logEntry);
        iox::log::Logger::assumeFlushed();
    }
};

int main()
{
    MyLogger::init();

    IOX_LOG(FATAL) << "Whoops ... look, over there is a dead seagull flying!";
    IOX_LOG(ERROR) << "Oh no!";
    IOX_LOG(WARN) << "It didn't happen!";
    IOX_LOG(INFO) << "All glory to the hypnotoad!";
    IOX_LOG(DEBUG) << "I didn't do it!";
    IOX_LOG(TRACE) << "Row row row your boat!";

    //     return 0;
    // }
    //     iox::log::Logger::init();
    //! [initialize runtime]
    constexpr char APP_NAME[] = "iox-cpp-user-header-untyped-publisher";
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    //! [initialize runtime]

    //! [create publisher]
    iox::popo::UntypedPublisher publisher({"Example", "User-Header", "Timestamp"});
    //! [create publisher]

    //! [send samples in a loop]
    uint64_t timestamp = 73;
    uint64_t fibonacciLast = 0;
    uint64_t fibonacciCurrent = 1;
    while (!iox::posix::hasTerminationRequested())
    {
        auto fibonacciNext = fibonacciCurrent + fibonacciLast;
        fibonacciLast = fibonacciCurrent;
        fibonacciCurrent = fibonacciNext;

        //! [loan chunk]
        publisher.loan(sizeof(Data), alignof(Data), sizeof(Header), alignof(Header))
            .and_then([&](auto& userPayload) {
                //! [loan was successful]
                auto header = static_cast<Header*>(iox::mepoo::ChunkHeader::fromUserPayload(userPayload)->userHeader());
                header->publisherTimestamp = timestamp;

                auto data = static_cast<Data*>(userPayload);
                data->fibonacci = fibonacciCurrent;

                publisher.publish(userPayload);

                std::cout << APP_NAME << " sent data: " << fibonacciCurrent << " with timestamp " << timestamp << "ms"
                          << std::endl;
                //! [loan was successful]
            })
            .or_else([&](auto& error) {
                //! [loan failed]
                std::cout << APP_NAME << " could not loan chunk! Error code: " << error << std::endl;
                //! [loan failed]
            });
        //! [loan chunk]

        constexpr uint64_t MILLISECONDS_SLEEP{1000U};
        std::this_thread::sleep_for(std::chrono::milliseconds(MILLISECONDS_SLEEP));
        timestamp += MILLISECONDS_SLEEP;
    }
    //! [send samples in a loop]

    return EXIT_SUCCESS;
}
