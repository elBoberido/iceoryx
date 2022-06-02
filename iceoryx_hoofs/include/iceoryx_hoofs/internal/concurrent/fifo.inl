// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_HOOFS_CONCURRENT_FIFO_INL
#define IOX_HOOFS_CONCURRENT_FIFO_INL

#include "iceoryx_hoofs/internal/concurrent/fifo.hpp"

namespace iox
{
namespace concurrent
{
template <class ValueType, uint64_t Capacity>
inline bool FiFo<ValueType, Capacity>::push(const ValueType& f_param_r) noexcept
{
    return !try_push(f_param_r).has_error();
}

template <class ValueType, uint64_t Capacity>
inline cxx::expected<uint64_t> FiFo<ValueType, Capacity>::try_push(const ValueType& f_param_r) noexcept
{
    if (is_full())
    {
        return cxx::error<uint64_t>(m_write_pos.load(std::memory_order_relaxed));
    }
    else
    {
        auto currentWritePos = m_write_pos.load(std::memory_order_relaxed);
        m_data[currentWritePos % Capacity] = f_param_r;

        // m_write_pos must be increased after writing the new value otherwise
        // it is possible that the value is read by pop while it is written.
        // this fifo is a single producer, multi consumer fifo therefore
        // store is allowed.
        m_write_pos.store(currentWritePos + 1, std::memory_order_release);
        return cxx::success<>();
    }
}

template <class ValueType, uint64_t Capacity>
inline bool FiFo<ValueType, Capacity>::is_full() const noexcept
{
    return m_write_pos.load(std::memory_order_relaxed) == m_read_pos.load(std::memory_order_relaxed) + Capacity;
}

template <class ValueType, uint64_t Capacity>
inline uint64_t FiFo<ValueType, Capacity>::size() const noexcept
{
    return m_write_pos.load(std::memory_order_relaxed) - m_read_pos.load(std::memory_order_relaxed);
}

template <class ValueType, uint64_t Capacity>
inline constexpr uint64_t FiFo<ValueType, Capacity>::capacity() noexcept
{
    return Capacity;
}

template <class ValueType, uint64_t Capacity>
inline bool FiFo<ValueType, Capacity>::empty() const noexcept
{
    return m_read_pos.load(std::memory_order_relaxed) == m_write_pos.load(std::memory_order_relaxed);
}

template <class ValueType, uint64_t Capacity>
inline cxx::optional<ValueType> FiFo<ValueType, Capacity>::pop() noexcept
{
    return try_pop_from_index(cxx::nullopt);
}

template <class ValueType, uint64_t Capacity>
inline cxx::optional<ValueType>
FiFo<ValueType, Capacity>::try_pop_from_index(const cxx::optional<uint64_t> read_pos) noexcept
{
    auto currentReadPos = m_read_pos.load(std::memory_order_acquire);
    if (read_pos && *read_pos != currentReadPos)
    {
        return cxx::nullopt;
    }

    bool isEmpty = (currentReadPos ==
                    // we are not allowed to use the empty method since we have to sync with
                    // the producer pop - this is done here
                    m_write_pos.load(std::memory_order_acquire));
    if (!isEmpty)
    {
        // TODO the ActiveObject uses the FiFo with a non trivially copyable object
        // add template parameter to switch between single/multi producer and then enable the static_assert
        // static_assert(std::is_trivially_copyable<ValueType>::value, "prevent frankenstein objects");
        ValueType out = m_data[currentReadPos % Capacity];

        // m_read_pos must be increased after reading the pop'ed value otherwise
        // it is possible that the pop'ed value is overwritten by push while it is read.
        // Implementing a multi consumer fifo here requires us to use compare_exchange_strong.
        if (m_read_pos.compare_exchange_strong(currentReadPos, currentReadPos + 1, std::memory_order_release))
        {
            return out;
        }
    }

    return cxx::nullopt;
}
} // namespace concurrent
} // namespace iox

#endif // IOX_HOOFS_CONCURRENT_FIFO_INL
