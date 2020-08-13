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
#ifndef IOX_POSH_MEPOO_DEFAULT_CHUNK_HEADER_EXTENSION_HPP
#define IOX_POSH_MEPOO_DEFAULT_CHUNK_HEADER_EXTENSION_HPP

#include "iceoryx_posh/mepoo/chunk_header.hpp"

#include "iceoryx_utils/cxx/helplets.hpp"

#include <cstdint>

namespace iox
{
namespace mepoo
{

template <uint64_t PAYLOAD_ALIGNMENT = 32>
struct DefaultHeaderExtension {


    uint64_t payloadOffset(const ChunkHeader* chunkHeader, const uint64_t payloadSize) const
    {
        auto startAddress = reinterpret_cast<uint64_t>(chunkHeader);
        return payloadOffset(startAddress, payloadSize);
    }

    uint64_t payloadOffset(const uint64_t chunkHeaderStartAddress, const uint64_t payloadSize) const
    {
        return iox::cxx::align(chunkHeaderStartAddress + sizeof(ChunkHeader), PAYLOAD_ALIGNMENT);
    }

    // TODO do we need a maxPayloadOffset???
    uint64_t maxChunkSizeForPayload(const uint64_t payloadSize)
    {

        // TODO maybe just an assert, since this doesn't change
        if(PAYLOAD_ALIGNMENT > alignof(ChunkHeader))
        {
            return sizeof(ChunkHeader) + payloadSize;
        }
        else
        {
            /*

                   start address of ChunkHeader object with alignment of 2 and size of 2
                    |
                    |      start address of ChunkHeaderExtension with alignment of 1 and size of 1
                    |       |
                    |       |              start address of payload with alignment of 4
                    |       |   |           |
            +---+---+---+---+---+---+---+---+
            0   1   2   3   4   5   6   7   8 <- memory addresses

            In this example, the worst case scenario for wasted memory is to have a padding of
                PAYLOAD_ALIGNMENT - CHUNK_HEADER_EXTENSION_ALIGNMENT
            with
                (CHUNK_HEADER_EXTENSION_ALIGNMENT <= CHUNK_HEADER_ALIGNMENT) && (PAYLOAD_ALIGNMENT > CHUNK_HEADER_EXTENSION_ALIGNMENT)
            We do not support chunk header extensions with a bigger alignment than ChunkHeader since the chunk header extension object musst be placed right after the ChunkHeader object without any padding. If there would be a padding, we would need to store the offset to the chunk header extension the same way like with the payload offset.
            When the payload alignment is not greater than the alignment of the chunk header extension, there is also no padding.
            */

            constexpr uint64_t CHUNK_HEADER_EXTENSION_ALIGNMENT {alignof(ChunkHeader)}; // just in this special case, since we do not have members but usually it would be something like {alignof(DefaultHeaderExtension)};
            static_assert(CHUNK_HEADER_EXTENSION_ALIGNMENT <= alignof(ChunkHeader) && "Chunk header extensions with a greater alignment than ChunkHeader are not supported");

            constexpr uint64_t WORST_CASE_PADDING {PAYLOAD_ALIGNMENT > CHUNK_HEADER_EXTENSION_ALIGNMENT ? PAYLOAD_ALIGNMENT - CHUNK_HEADER_EXTENSION_ALIGNMENT : 0 };

            return sizeof(ChunkHeader) /* + sizeof(DefaultHeaderExtension)*/ + WORST_CASE_PADDING + payloadSize;
        }
    }

};

struct TimestampChunkHeaderExtension
{
    uint64_t payloadOffset(const uint64_t chunkHeaderStartAddress, const uint64_t payloadSize) const
    {
        auto chunkHeaderExtensionStartAddress = iox::cxx::align(chunkHeaderStartAddress + sizeof(ChunkHeader), alignof(TimestampChunkHeaderExtension));
        return iox::cxx::align(chunkHeaderExtensionStartAddress + sizeof(TimestampChunkHeaderExtension), PAYLOAD_ALIGNMENT);
    }

    static constexpr uint64_t PAYLOAD_ALIGNMENT {32u};
    uint64_t m_timestamp{0};
};

// TODO SenderPort/Publisher needs a ctor with either function ref or a reference to a CustomHeaderFilling
// might be problematic for C-API

class CustomHeaderFilling {
    virtual void allocateHook(void* headerExtension) = 0;
    virtual void deliverHook(void* headerExtension) = 0;
};
class TimestampHeaderFilling : public CustomHeaderFilling {
    void allocateHook(void* headerExtension) override {
        auto header = static_cast<TimestampChunkHeaderExtension*>(headerExtension);
        header->m_timestamp = m_synteticTime++;
    }

    void deliverHook(void* headerExtension) override
    {}

    uint64_t m_synteticTime {0u};
};

}
} // namespace iox

#endif // IOX_POSH_MEPOO_DEFAULT_CHUNK_HEADER_EXTENSION_HPP
