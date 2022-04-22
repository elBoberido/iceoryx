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

#include "iceoryx_hoofs/log/ng/logger.hpp"

namespace iox
{
namespace testing
{
class Logger : public iox::log::ng::Logger
{
  public:
    static void activateTestLogger()
    {
        static Logger logger;
        iox::log::ng::Logger::activeLogger(&logger);
    }

  private:
    Logger() = default;

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;

    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    void flush() override
    {
        // TODO, store the log messages in a ring-buffer and add a static function to flush all of that
        // the ring-buffer will be cleared in the global set-up and flushed in the global tear-down if there was an
        // error
        // https://google.github.io/googletest/advanced.html#global-set-up-and-tear-down

        // do nothing for now
        // iox::log::ng::Logger::flush();
    }
};

} // namespace testing
} // namespace iox
