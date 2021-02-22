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

#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/polymorph.hpp"
#include "iceoryx_utils/cxx/vector.hpp"

#include "test.hpp"

#include <functional>
#include <iostream>

using namespace ::iox;
using namespace ::testing;
using ::testing::_;

namespace
{
class Engine
{
  public:
    virtual ~Engine() = default;

    virtual void throttle(uint16_t throttleInPercentage) = 0;
    virtual uint16_t speed() = 0;
    virtual void printInfo() = 0;
};

class CombustionEngine : public Engine
{
  public:
    CombustionEngine(uint16_t maxSpeed)
        : m_maxSpeed(maxSpeed){};

    void throttle(uint16_t throttleInPercentage) override
    {
        m_throttleInPercentage = throttleInPercentage;
    }

    uint16_t speed() override
    {
        return m_maxSpeed * (m_throttleInPercentage / 100.);
    }

    void printInfo() override
    {
        std::cout << "CombustionEngine { m_throttleInPercentage = " << m_throttleInPercentage
                  << "; m_maxSpeed = " << m_maxSpeed << "}";
    }

  private:
    uint16_t m_throttleInPercentage{0};
    uint16_t m_maxSpeed{6};
};
class ElectricalEngine : public Engine
{
  public:
    ElectricalEngine(uint16_t maxSpeed)
        : m_maxSpeed(maxSpeed){};

    void throttle(uint16_t throttleInPercentage) override
    {
        m_throttleInPercentage = throttleInPercentage;
    }

    uint16_t speed() override
    {
        return m_maxSpeed * (m_throttleInPercentage / 100.);
    }

    void printInfo() override
    {
        std::cout << "ElectricalEngine { m_throttleInPercentage = " << m_throttleInPercentage
                  << "; m_maxSpeed = " << m_maxSpeed << "}";
    }

  private:
    uint16_t m_throttleInPercentage{0};
    uint16_t m_maxSpeed{6};
};

static constexpr auto MaxSize = iox::cxx::maxSize<CombustionEngine, ElectricalEngine>();
static constexpr auto MaxAlignment = iox::cxx::maxAlignment<CombustionEngine, ElectricalEngine>();

using Engine_t = cxx::Polymorph<Engine, MaxSize, MaxAlignment>;

enum class EngineType
{
    Combustion,
    Electrical
};

class EngineFactory
{
  public:
    static Engine_t create(EngineType type, uint16_t throttleInPercentage)
    {
        return getFactory()(type, throttleInPercentage);
    }

  protected:
    using factory_t = Engine_t (*)(EngineType type, uint16_t throttleInPercentage);

    static factory_t& getFactory()
    {
        static factory_t f = defaultFactory;
        return f;
    }
    static void setFactory(const factory_t& factory)
    {
        if (factory)
        {
            getFactory() = factory;
        }
        else
        {
            std::cout << "Cannot set factory. Passed factory must not be empty!" << std::endl;
        }
    }

    static Engine_t defaultFactory(EngineType type, uint16_t throttleInPercentage)
    {
        switch (type)
        {
        case EngineType::Combustion:
            return Engine_t(cxx::PolymorphType<CombustionEngine>(), throttleInPercentage);
        case EngineType::Electrical:
            return Engine_t(cxx::PolymorphType<ElectricalEngine>(), throttleInPercentage);
        }
    }
};

class Car
{
  public:
    Car(EngineType type)
        : m_engine(EngineFactory::create(type, 250U))
    {
    }

    void throttle(uint16_t percentage)
    {
        m_engine->throttle(percentage);
    }

    uint16_t speed()
    {
        return m_engine->speed();
    }

    void printInfo()
    {
        std::cout << "Car { m_engine = ";
        m_engine->printInfo();
        std::cout << "}";
    }

  private:
    Engine_t m_engine;
};

struct MockEngineDelegate
{
    MOCK_METHOD1(throttle, void(uint16_t));
    MOCK_METHOD0(speed, uint16_t(void));
    MOCK_METHOD0(printInfo, void(void));
};

struct MockEngine : public Engine
{
    MockEngine(MockEngineDelegate* m)
        : mock(m)
    {
    }

    void throttle(uint16_t throttleInPercentage)
    {
        mock->throttle(throttleInPercentage);
    }

    uint16_t speed()
    {
        return mock->speed();
    }

    void printInfo()
    {
        mock->printInfo();
    }

    MockEngineDelegate* mock{nullptr};
};

struct MockEngineFactory : protected EngineFactory
{
    using EngineFactory::factory_t;
    using EngineFactory::setFactory;

    static void setMockFactory()
    {
        EngineFactory::setFactory(&mockFactory);
    }

    static void resetMockFactory()
    {
        EngineFactory::setFactory(&EngineFactory::defaultFactory);

        mocks.clear();
    }

    static Engine_t mockFactory(EngineType, uint16_t)
    {
        if(singleMock.has_value()) {
            auto mock = singleMock.value();
            singleMock.reset();
            return Engine_t(cxx::PolymorphType<MockEngine>(), mock);
        } else {
            mocks.emplace_back();

            return Engine_t(cxx::PolymorphType<MockEngine>(), &mocks.back());
        }
    }

    static cxx::optional<MockEngineDelegate*> singleMock;
    static cxx::vector<MockEngineDelegate, 20> mocks;
};

cxx::optional<MockEngineDelegate*> MockEngineFactory::singleMock;
cxx::vector<MockEngineDelegate, 20> MockEngineFactory::mocks;

TEST(CoffeeAndCoding, main)
{
    MockEngineDelegate mockEngine;
    cxx::vector<Car, 20> speedway;

    // create specific engines
    speedway.emplace_back(EngineType::Combustion);
    speedway.emplace_back(EngineType::Electrical);

    // use a custom factory for the engines and reset to the default at end of scope
    auto mockFactoryGuard =
        cxx::GenericRAII([] { MockEngineFactory::setMockFactory(); }, [] { MockEngineFactory::resetMockFactory(); });

    // create an engine mock in the vector
    speedway.emplace_back(EngineType::Combustion);
    ASSERT_EQ(MockEngineFactory::mocks.size(), 1U);

    EXPECT_CALL(MockEngineFactory::mocks[0], throttle).WillRepeatedly(Return());
    EXPECT_CALL(MockEngineFactory::mocks[0], printInfo).WillRepeatedly(Invoke([&]() { std::cout << "Mock {}"; }));
    EXPECT_CALL(MockEngineFactory::mocks[0], speed).WillOnce(Return(42));

    // this engine mock could already be accessed in the ctor of Car
    MockEngineFactory::singleMock = &mockEngine;
    speedway.emplace_back(EngineType::Combustion);

    EXPECT_CALL(mockEngine, throttle).WillRepeatedly(Return());
    EXPECT_CALL(mockEngine, printInfo).WillRepeatedly(Invoke([&]() { std::cout << "Another mock {}"; }));
    EXPECT_CALL(mockEngine, speed).WillOnce(Return(13));

    // create another engine mock in the vector
    speedway.emplace_back(EngineType::Combustion);
    ASSERT_EQ(MockEngineFactory::mocks.size(), 2U);

    EXPECT_CALL(MockEngineFactory::mocks[1], throttle).WillRepeatedly(Return());
    EXPECT_CALL(MockEngineFactory::mocks[1], printInfo).WillRepeatedly(Invoke([&]() { std::cout << "Yet another mock {}"; }));
    EXPECT_CALL(MockEngineFactory::mocks[1], speed).WillOnce(Return(73));

    int i = 0;
    for (auto& car : speedway)
    {
        std::cout << "####" << std::endl;
        car.throttle(10 * ++i);
        car.printInfo();
        std::cout << " speed: " << car.speed();
        std::cout << std::endl;
    }
}

} // namespace
