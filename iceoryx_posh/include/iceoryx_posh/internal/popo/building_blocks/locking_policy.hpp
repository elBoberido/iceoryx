// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_LOCKING_POLICY_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_LOCKING_POLICY_HPP

#include "iox/detail/adaptive_wait.hpp"
#include "iox/mutex.hpp"

namespace iox
{
namespace popo
{
class SpinlockMutex
{
  public:
    SpinlockMutex()
        : m_flag(ATOMIC_FLAG_INIT)
        , m_recursive{Recursive{0, 0}}
    {
    }

    expected<void, MutexLockError> lock() noexcept
    {
        pid_t tid = gettid();

        auto recursive = m_recursive.load();

        if (recursive.tid == tid)
        {
            recursive.count += 1;
            m_recursive.store(recursive);

            return ok();
        }

        detail::adaptive_wait spinner;
        spinner.wait_loop([this] { return this->m_flag.test_and_set(std::memory_order_acquire); });

        m_recursive.store(Recursive{tid, 1});

        return ok();
    }

    expected<void, MutexUnlockError> unlock() noexcept
    {
        pid_t tid = gettid();

        auto recursive = m_recursive.load();

        if (recursive.tid == tid)
        {
            recursive.count -= 1;

            if (recursive.count == 0)
            {
                recursive.tid = 0;
                m_recursive.store(recursive);
                m_flag.clear(std::memory_order_release);
            }
            else
            {
                m_recursive.store(recursive);
            }

            return ok();
        }


        return err(MutexUnlockError::UNKNOWN_ERROR);
    }

    expected<MutexTryLock, MutexTryLockError> try_lock() noexcept
    {
        pid_t tid = gettid();

        auto recursive = m_recursive.load();

        if (recursive.tid == tid)
        {
            recursive.count += 1;
            m_recursive.store(recursive);
            return ok(MutexTryLock::LOCK_SUCCEEDED);
        }

        if (!m_flag.test_and_set(std::memory_order_acquire))
        {
            m_recursive.store(Recursive{tid, 1});

            return ok(MutexTryLock::LOCK_SUCCEEDED);
        }
        return ok(MutexTryLock::FAILED_TO_ACQUIRE_LOCK);
    }

    struct Recursive
    {
        pid_t tid;
        uint32_t count;
    };

  private:
    std::atomic_flag m_flag;
    std::atomic<Recursive> m_recursive;
};

class ThreadSafePolicy
{
  public:
    ThreadSafePolicy() noexcept;

    // needs to be public since we want to use std::lock_guard
    void lock() const noexcept;
    void unlock() const noexcept;
    bool tryLock() const noexcept;

  private:
    mutable optional<SpinlockMutex> m_mutex;
};

class SingleThreadedPolicy
{
  public:
    // needs to be public since we want to use std::lock_guard
    void lock() const noexcept;
    void unlock() const noexcept;
    bool tryLock() const noexcept;
};

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_LOCKING_POLICY_HPP
