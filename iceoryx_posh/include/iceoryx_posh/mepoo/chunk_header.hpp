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
/// @note with the new approach, the alignment of the ChunkHeader could be less than 32,
/// but it might be a good idea to keep it that way, to ensure to have the whole struct on the same cache line
struct alignas(32) ChunkHeader
{
    /// @brief ALlocates memory to store the information about the chunks.
    ChunkHeader() noexcept;

    UniquePortId m_originId{popo::CreateInvalidId};
    ChunkInfo m_info;

    /* proposal for new ChunkHeader

    // TODO framing for adjacent payload + divergent alignment
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

    std::uint32_t m_chunkSize{0};
    std::uint8_t m_chunkHeaderVersion{1};
    std::uint8_t m_reserved1{0};
    std::uint8_t m_reserved1{0};
    std::uint8_t m_reserved1{0};
    std::uint64_t m_originId{0};
    std::uint64_t m_sequenceNumber{0};
    std::uint32_t m_payloadSize{0};
    */
    /// @note the offset of m_payloadOffset in the parent must always be
    /// `sizeof(ChunkHeader) - sizeof(decltype(m_payloadOffset))`, thus at the very end of the struct.
    /// This is an optimization when no custom header is used. In this case, the payload is potentially adjacent to the ChunkHeader
    /// and since the payload offset is not static, we need to store the offset in front of the payload
    /// to be able calculate the pointer to the ChunkHeader from a payload pointer. Thus we do not need to store the payload offset twice,
    /// once in the ChunkHeader and once in front of the payload. In the case the payload alignment is larger than the ChunkHeader alignment,
    /// there will be padding bytes in front of the payload which can be used to store the payload offset for the ChunkHeader pointer calculation
    /// without wasting additional memory.
    std::uint32_t m_payloadOffset{0};
    // TODO: test for m_payloadOffset to adjacent payload
    // ```
    // ChunkHeader c;
    // CHECK_THAT(reinterpret_cast<uint64_t>(&c) + (sizof(ChunkHeader) - sizeof(decltype(ChunkHeader::m_payloadOffset))), Eq(reinterpret_cast<uint64_t>(&c.m_payloadOffset)))
    // ```

    // TODO: with the approach that the payload alignment can be bigger than the ChunkHeader alignment,
    // we need to additionally store the payload offset in an uint32_t just in front of the payload.
    // This is necessary since we provide an API where the gets only the pointer for the payload from an allocate call.
    // To be able to calculate the pointer to the ChunkHeader, we need to store the payload offset in a known location,
    // which would be at (payloadPointer - sizeof(uint32_t)). This also means the minimal alignment of the payload must be alignof(uint32_t).
    // This has also to be considered in the maxChunkSizeForPayload calculation

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
        return reinterpret_cast<ChunkHeaderExtension*>(reinterpret_cast<uint64_t>(this) + sizeof(ChunkHeader)); // TODO align the address to alignof(ChunkHeaderExtension)
    }

    static ChunkHeader* fromPayload(const void* const payload) {
        auto payloadAddress = reinterpret_cast<uint64_t>(payload);
        auto payloadOffset = reinterpret_cast<uint32_t*>(payloadAddress - sizeof(decltype(ChunkHeader::m_payloadOffset)));// + m_info.m_payloadOffset
        return reinterpret_cast<ChunkHeader*>(payloadAddress - *payloadOffset);
    }
};

ChunkHeader* convertPayloadPointerToChunkHeader(const void* const payload) noexcept;

} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_CHUNK_HEADER_HPP
