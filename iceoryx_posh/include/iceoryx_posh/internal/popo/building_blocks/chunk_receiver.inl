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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_INL
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_INL

#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"

namespace iox
{
namespace popo
{
template <typename ChunkReceiverDataType>
inline ChunkReceiver<ChunkReceiverDataType>::ChunkReceiver(
    cxx::not_null<MemberType_t* const> chunkReceiverDataPtr) noexcept
    : Base_t(static_cast<typename ChunkReceiverDataType::ChunkQueueData_t*>(chunkReceiverDataPtr))
{
}

template <typename ChunkReceiverDataType>
inline const typename ChunkReceiver<ChunkReceiverDataType>::MemberType_t*
ChunkReceiver<ChunkReceiverDataType>::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(Base_t::getMembers());
}

template <typename ChunkReceiverDataType>
inline typename ChunkReceiver<ChunkReceiverDataType>::MemberType_t*
ChunkReceiver<ChunkReceiverDataType>::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(Base_t::getMembers());
}

template <typename ChunkReceiverDataType>
inline cxx::expected<cxx::optional<const mepoo::ChunkHeader*>, ChunkReceiveError>
ChunkReceiver<ChunkReceiverDataType>::tryGet() noexcept
{
    auto popRet = this->tryPop();

    if (popRet.has_value())
    {
        auto sharedChunk = *popRet;

        // if the application holds too many chunks, don't provide more
        if (getMembers()->m_chunksInUse.insert(sharedChunk))
        {
            return cxx::success<cxx::optional<const mepoo::ChunkHeader*>>(
                const_cast<const mepoo::ChunkHeader*>(sharedChunk.getChunkHeader()));
        }
        else
        {
            // release the chunk
            sharedChunk = nullptr;
            return cxx::error<ChunkReceiveError>(ChunkReceiveError::TOO_MANY_CHUNKS_HELD_IN_PARALLEL);
        }
    }
    else
    {
        // no new chunk
        return cxx::success<cxx::optional<const mepoo::ChunkHeader*>>(cxx::nullopt_t());
    }
}

template <typename ChunkReceiverDataType>
inline cxx::expected<const mepoo::ChunkHeader*, ChunkReceiveError>
ChunkReceiver<ChunkReceiverDataType>::tryGet2() noexcept
{
    auto popRet = this->tryPop();

    if (popRet.has_value())
    {
        auto sharedChunk = *popRet;

        // if the application holds too many chunks, don't provide more
        if (getMembers()->m_chunksInUse.insert(sharedChunk))
        {
            return cxx::success<const mepoo::ChunkHeader*>(
                const_cast<const mepoo::ChunkHeader*>(sharedChunk.getChunkHeader()));
        }
        else
        {
            // release the chunk
            sharedChunk = nullptr;
            return cxx::error<ChunkReceiveError>(ChunkReceiveError::TOO_MANY_CHUNKS_HELD_IN_PARALLEL);
        }
    }
    else
    {
        // no new chunk
        return cxx::error<ChunkReceiveError>(ChunkReceiveError::NO_CHUNKS_AVAILABLE);
    }
}

template <typename ChunkReceiverDataType>
inline void ChunkReceiver<ChunkReceiverDataType>::release(const mepoo::ChunkHeader* const chunkHeader) noexcept
{
    mepoo::SharedChunk chunk(nullptr);
    // PRQA S 4127 1 # d'tor of SharedChunk will release the memory, we do not have to touch the returned chunk
    if (!getMembers()->m_chunksInUse.remove(chunkHeader, chunk)) // PRQA S 4127
    {
        errorHandler(Error::kPOPO__CHUNK_RECEIVER_INVALID_CHUNK_TO_RELEASE_FROM_USER, nullptr, ErrorLevel::SEVERE);
    }
}

template <typename ChunkReceiverDataType>
inline void ChunkReceiver<ChunkReceiverDataType>::releaseAll() noexcept
{
    getMembers()->m_chunksInUse.cleanup();
    this->clear();
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_INL
