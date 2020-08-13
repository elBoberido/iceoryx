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
#ifndef IOX_POSH_MEPOO_CHUNK_HEADER_HPP
#define IOX_POSH_MEPOO_CHUNK_HEADER_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/typed_unique_id.hpp"
#include "iceoryx_posh/mepoo/chunk_info.hpp"

#include <cstdint>

namespace iox
{
namespace mepoo
{
/// @brief IMPORTANT the alignment MUST be 32 or less since all mempools are
///         32 byte aligned otherwise we get alignment problems!
struct alignas(32) ChunkHeader
{
    /// @brief ALlocates memory to store the information about the chunks.
    ChunkHeader() noexcept;

    UniquePortId m_originId{popo::CreateInvalidId};
    ChunkInfo m_info;

    /* proposal for new ChunkHeader

     chunk framing:

       sizeof(ChunkHeader)                             m_payloadSize
     __________/\__________                     _____________/\______________
    /                      \                   /                             \
    +-----------------------+-----------------+------------------------------+---------------+
    |                       |   Custom    .   |                              |               |
    |      ChunkHeader      | ChunkHeader .   |           Payload            |    Padding    |
    |                       |  Extension  .   |                              |               |
    +-----------------------+-----------------+------------------------------+---------------+
    \___________________  ____________________/                                              /
    \                   \/                                                                  /
     \            m_payloadOffset                                                          /
      \                                                                                   /
       \_______________________________________  ________________________________________/
                                               \/
                                           m_chunkSize

    std::uint64_t m_originId{0};
    std::uint64_t m_sequenceNumber{0};
    std::uint32_t m_chunkSize{0};
    std::uint32_t m_payloadSize{0};
    std::uint32_t m_padding{0}; <- just a padding to make "address of m_payloadOffset" == "payloadPointer - sizeof(uint32_t)"; this is an optimization for a custom chunk header extension of size 0
    std::uint32_t m_payloadOffset{0};

    // TODO: with the approach that the payload alignment can be bigger than the ChunkHeader alignment,
    // we need to additionally store the payload offset in an uint32_t just in front of the payload.
    // This is necessary since we provide an API where the gets only the pointer for the payload from an allocate call.
    // To be able to calculate the pointer to the ChunkHeader, we need to store the payload offset in a known location,
    // which would be at (payloadPointer - sizeof(uint32_t)). This also means the minimal alignment of the payload must be alignof(uint32_t).
    // This has also to be considered in the maxChunkSizeForPayload calculation
    */

    void* payload() const
    {
        // payload is always located relative to "this" in this way
        return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(this) + m_info.m_payloadOffset);
    }

    /// @todo this is a temporary dummy variable to keep the size of the ChunkHeader at 64 byte for compatibility
    /// reasons
    void* m_payloadDummy{nullptr};

    template <typename ChunkHeaderExtension>
    ChunkHeaderExtension* chunkHeaderExtension()
    {
        // the ChunkHeaderExtension is always located relative to "this" in this way
        return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(this) + sizeof(ChunkHeader));
    }
};

ChunkHeader* convertPayloadPointerToChunkHeader(const void* const payload) noexcept;

} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_CHUNK_HEADER_HPP
