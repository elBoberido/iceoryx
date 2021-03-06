// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
// Copyright (c) 2020 by Robert Bosch GmbH, Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/popo/untyped_publisher.hpp"
#include "mocks/chunk_mock.hpp"
#include "mocks/publisher_mock.hpp"

#include "test.hpp"

using namespace ::testing;
using ::testing::_;

using TestUntypedPublisher = iox::popo::UntypedPublisherImpl<MockBasePublisher<void>>;

class UntypedPublisherTest : public Test
{
  public:
    UntypedPublisherTest()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

  protected:
    ChunkMock<uint64_t> chunkMock;
    TestUntypedPublisher sut{{"", "", ""}};
    MockPublisherPortUser& portMock{sut.mockPort()};
};

TEST_F(UntypedPublisherTest, LoansChunkWithRequestedSize)
{
    constexpr uint32_t ALLOCATION_SIZE = 7U;
    EXPECT_CALL(portMock, tryAllocateChunk(ALLOCATION_SIZE))
        .WillOnce(Return(ByMove(iox::cxx::success<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    // ===== Test ===== //
    auto result = sut.loan(ALLOCATION_SIZE);
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_error());
    // ===== Cleanup ===== //
}

TEST_F(UntypedPublisherTest, LoanFailsIfPortCannotSatisfyAllocationRequest)
{
    constexpr uint32_t ALLOCATION_SIZE = 17U;
    EXPECT_CALL(portMock, tryAllocateChunk(ALLOCATION_SIZE))
        .WillOnce(Return(
            ByMove(iox::cxx::error<iox::popo::AllocationError>(iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS))));
    // ===== Test ===== //
    auto result = sut.loan(ALLOCATION_SIZE);
    // ===== Verify ===== //
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS, result.get_error());
    // ===== Cleanup ===== //
}

TEST_F(UntypedPublisherTest, LoanPreviousChunkSucceeds)
{
    EXPECT_CALL(portMock, tryGetPreviousChunk())
        .WillOnce(Return(ByMove(iox::cxx::optional<iox::mepoo::ChunkHeader*>(chunkMock.chunkHeader()))));
    // ===== Test ===== //
    auto result = sut.loanPreviousChunk();
    // ===== Verify ===== //
    EXPECT_TRUE(result.has_value());
    // ===== Cleanup ===== //
}

TEST_F(UntypedPublisherTest, LoanPreviousChunkFails)
{
    EXPECT_CALL(portMock, tryGetPreviousChunk()).WillOnce(Return(ByMove(iox::cxx::nullopt)));
    // ===== Test ===== //
    auto result = sut.loanPreviousChunk();
    // ===== Verify ===== //
    EXPECT_FALSE(result.has_value());
    // ===== Cleanup ===== //
}

TEST_F(UntypedPublisherTest, PublishesPayloadViaUnderlyingPort)
{
    // ===== Setup ===== //
    EXPECT_CALL(portMock, sendChunk).Times(1);
    // ===== Test ===== //
    sut.publish(chunkMock.chunkHeader()->payload());
    // ===== Verify ===== //
    // ===== Cleanup ===== //
}

// test whether the BasePublisher methods are called

TEST_F(UntypedPublisherTest, OfferDoesOfferServiceOnUnderlyingPort)
{
    EXPECT_CALL(sut, offer).Times(1);
    // ===== Test ===== //
    sut.offer();
}
TEST_F(UntypedPublisherTest, StopOfferDoesStopOfferServiceOnUnderlyingPort)
{
    EXPECT_CALL(sut, stopOffer).Times(1);
    sut.stopOffer();
}

TEST_F(UntypedPublisherTest, isOfferedDoesCheckIfPortIsOfferedOnUnderlyingPort)
{
    EXPECT_CALL(sut, isOffered).Times(1);
    // ===== Test ===== //
    sut.isOffered();
}

TEST_F(UntypedPublisherTest, isOfferedDoesCheckIfUnderylingPortHasSubscribers)
{
    EXPECT_CALL(sut, hasSubscribers).Times(1);
    // ===== Test ===== //
    sut.hasSubscribers();
}

TEST_F(UntypedPublisherTest, GetServiceDescriptionCallForwardedToUnderlyingPublisherPort)
{
    EXPECT_CALL(sut, getServiceDescription).Times(1);
    // ===== Test ===== //
    sut.getServiceDescription();
}
