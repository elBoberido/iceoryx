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

#include "mixed_mode_poc_common.hpp"

int main()
{
    print_sizes();

    IOX_LOG(INFO, "");

    auto shm_result = open_or_create_shm();
    if (shm_result.has_error())
    {
        IOX_LOG(ERROR, "Could not create shared memory");
        return -1;
    }
    auto& shm = shm_result.value();

    auto* shared_data = static_cast<SharedData*>(shm.getBaseAddress());

    auto& leader_barrier = shared_data->leader_barrier;
    auto& follower_barrier = shared_data->follower_barrier;

    auto& non_atomic_counter = shared_data->non_atomic_counter;
    auto& atomic_counter = shared_data->atomic_counter;

    leader_barrier.post();
    follower_barrier.wait();

    IOX_LOG(INFO, "Racing on the non atomic counter!");

    for (uint64_t i = 0; i < ITERATIONS; ++i)
    {
        non_atomic_counter++;
    }

    leader_barrier.post();
    follower_barrier.wait();

    IOX_LOG(INFO, "Non atomic counter value: " << non_atomic_counter);
    IOX_LOG(INFO, "Expected any value below: " << 2 * ITERATIONS);
    IOX_LOG(INFO, "");
    IOX_LOG(INFO, "Racing on the atomic counter!");

    for (uint64_t i = 0; i < ITERATIONS; ++i)
    {
        // this is intentional more complex than necessary in order to test the CAS loop
        auto old_counter_value = atomic_counter.load(std::memory_order_relaxed);
        while (!atomic_counter.compare_exchange_weak(
            old_counter_value, old_counter_value + 1, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
        }
    }

    leader_barrier.post();
    follower_barrier.wait();

    IOX_LOG(INFO, "Atomic counter value:     " << atomic_counter);
    IOX_LOG(INFO, "Expected counter value:   " << 2 * ITERATIONS);

    if (atomic_counter == 2 * ITERATIONS)
    {
        IOX_LOG(INFO, "Success! Data layout and atomics work!");
    }
    else
    {
        IOX_LOG(ERROR, "Failed! Either data layout issues or atomics do not work!");
    }

    return 0;
}
