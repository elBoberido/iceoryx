// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"

namespace iox
{
namespace mepoo
{
SharedChunk::SharedChunk(ChunkManagement* const f_resource)
    : m_chunkManagement(f_resource)
    //, m_chunkHeader(m_chunkManagement == nullptr ? nullptr : m_chunkManagement->m_chunkHeader)
{
}

SharedChunk::SharedChunk(const relative_ptr<ChunkManagement>& f_resource)
    : SharedChunk(f_resource.get())
{
}

SharedChunk::SharedChunk(const SharedChunk& rhs)
{
    *this = rhs;
}

SharedChunk::SharedChunk(SharedChunk&& rhs)
{
    *this = std::move(rhs);
}

SharedChunk::~SharedChunk()
{
    decrementReferenceCounter();
}

void SharedChunk::incrementReferenceCounter()
{
    if (m_chunkManagement != nullptr)
    {
        m_chunkManagement->m_referenceCounter.fetch_add(1u, std::memory_order_relaxed);
    }
}

void SharedChunk::decrementReferenceCounter()
{
    if ((m_chunkManagement != nullptr)
        && (m_chunkManagement->m_referenceCounter.fetch_sub(1u, std::memory_order_relaxed) == 1u))
    {
        freeChunk();
    }
}

void SharedChunk::freeChunk()
{
    m_chunkManagement->m_mempool->freeChunk(m_chunkManagement->m_chunkHeader);
    m_chunkManagement->m_chunkManagementPool->freeChunk(m_chunkManagement);
    m_chunkManagement = nullptr;
//     m_chunkHeader = nullptr;
}

SharedChunk& SharedChunk::operator=(const SharedChunk& rhs)
{
    if (this != &rhs)
    {
        decrementReferenceCounter();
        m_chunkManagement = rhs.m_chunkManagement;
//         m_chunkHeader = rhs.m_chunkHeader;
        incrementReferenceCounter();
    }
    return *this;
}

SharedChunk& SharedChunk::operator=(SharedChunk&& rhs)
{
    if (this != &rhs)
    {
        decrementReferenceCounter();
        m_chunkManagement = rhs.m_chunkManagement;
//         m_chunkHeader = rhs.m_chunkHeader;
        rhs.m_chunkManagement = nullptr;
//         rhs.m_chunkHeader = nullptr;
    }
    return *this;
}

void* SharedChunk::getPayload() const
{
    auto chunkHeader = m_chunkManagement->m_chunkHeader;
    if (chunkHeader == nullptr)
    {
        return nullptr;
    }
    else
    {
        return chunkHeader->payload();
    }
}

bool SharedChunk::operator==(const SharedChunk& rhs) const
{
    return m_chunkManagement == rhs.m_chunkManagement;
}

bool SharedChunk::operator==(const void* const rhs) const
{
    return getPayload() == rhs;
}

bool SharedChunk::operator!=(const SharedChunk& rhs) const
{
    return !(*this == rhs);
}

bool SharedChunk::operator!=(const void* const rhs) const
{
    return !(*this == rhs);
}

bool SharedChunk::hasNoOtherOwners() const
{
    if (m_chunkManagement == nullptr)
    {
        return true;
    }

    return m_chunkManagement->m_referenceCounter.load(std::memory_order_relaxed) == 1u;
}

SharedChunk::operator bool() const
{
    return m_chunkManagement != nullptr;
}

ChunkHeader* SharedChunk::getChunkHeader() const
{
    return m_chunkManagement->m_chunkHeader;
}

ChunkManagement* SharedChunk::release()
{
    ChunkManagement* returnValue = m_chunkManagement;
    m_chunkManagement = nullptr;
//     m_chunkHeader = nullptr;
    return returnValue;
}

// iox::relative_ptr<ChunkManagement> SharedChunk::releaseWithRelativePtr()
// {
//     auto returnValue = m_chunkManagement;
//     m_chunkManagement = nullptr;
//     m_chunkHeader = nullptr;
//     return returnValue;
// }

} // namespace mepoo
} // namespace iox
