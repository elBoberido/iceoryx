// Copyright (c) 2024 by ekxide IO GmbH. All rights reserved.
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

#ifndef IOX_MIXED_MODE_POC_COMMON_HPP
#define IOX_MIXED_MODE_POC_COMMON_HPP

#include "iox/logging.hpp"
#include "iox/mutex.hpp"
#include "iox/posix_shared_memory_object.hpp"
#include "iox/unnamed_semaphore.hpp"

#include <atomic>
#include <cstdint>
#include <thread>

#ifdef USE_EXPLICIT_ALIGNMENT
#define ALIGNAS(n) alignas(n)
#else
#define ALIGNAS(n)
#endif

constexpr uint64_t ITERATIONS{50000000};

class PoorMansSpinSemaphore
{
  public:
    void post()
    {
        counter++;
    }

    void wait()
    {
        while (counter == 0)
        {
            std::this_thread::yield();
        }
        counter--;
    }

  private:
    std::atomic<int32_t> counter{0};
};

struct SharedData
{
    PoorMansSpinSemaphore leader_barrier;
    PoorMansSpinSemaphore follower_barrier;
    ALIGNAS(8) uint8_t dummy0{0};
    uint8_t dummy1{0};
    ALIGNAS(8) volatile uint64_t non_atomic_counter{0};
    uint8_t dummy2{0};
    ALIGNAS(8) std::atomic<uint64_t> atomic_counter{0};
};

auto print_sizes()
{
    IOX_LOG(INFO, "Size of shared data: " << sizeof(SharedData));
    IOX_LOG(INFO, "Size of iox::UnnamedSemaphore: " << sizeof(iox::UnnamedSemaphore));
    IOX_LOG(INFO, "Size of POSIX sem_t: " << sizeof(sem_t));
    IOX_LOG(INFO, "Size of iox::mutex: " << sizeof(iox::mutex));
    IOX_LOG(INFO, "Size of POSIX pthread_mutex_t: " << sizeof(pthread_mutex_t));
}

auto open_or_create_shm()
{
    constexpr uint64_t MEMORY_SIZE{4096};
    return iox::PosixSharedMemoryObjectBuilder()
        .name("iox-mixed-mode-poc")
        .memorySizeInBytes(MEMORY_SIZE)
        .openMode(iox::OpenMode::OPEN_OR_CREATE)
        .accessMode(iox::AccessMode::READ_WRITE)
        .permissions(iox::perms::owner_all)
        .create();
}

#endif // IOX_MIXED_MODE_POC_COMMON_HPP
