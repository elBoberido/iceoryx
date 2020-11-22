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

#include "iceoryx_utils/cxx/serialization.hpp"
#include "test.hpp"

namespace
{
class Dummy
{
  public:
    Dummy() = default;

    class Nested
    {
      public:
        Nested() = default;
        Nested(std::string str)
            : m_str(str)
        {
        }

        Nested(iox::cxx::Serialization&);
        operator iox::cxx::Serialization() const;

        friend std::ostream& operator<<(std::ostream&, const Dummy::Nested&);

      private:
        std::string m_str;
    };

    Dummy(uint16_t u16, uint32_t u32, std::string str)
        : m_u16(u16)
        , m_u32(u32)
        , m_nes(str)
    {
    }

    Dummy(iox::cxx::Serialization&);
    operator iox::cxx::Serialization() const;

    friend std::ostream& operator<<(std::ostream&, const Dummy&);

  private:
    uint16_t m_u16{0};
    uint32_t m_u32{0};
    Nested m_nes;
};

std::ostream& operator<<(std::ostream& o, const Dummy& d)
{
    auto flags = o.flags();
    o << "Dummy {" << std::endl;
    o << "    m_u16 = 0x" << std::hex << d.m_u16 << std::endl;
    o << "    m_u32 = 0x" << std::hex << d.m_u32 << std::endl;
    o << "    m_nes = " << d.m_nes;
    o << "}" << std::endl;
    o.setf(flags);
    return o;
}

std::ostream& operator<<(std::ostream& o, const Dummy::Nested& d)
{
    auto flags = o.flags();
    o << "Dummy::Nested {" << std::endl;
    o << "    m_str = '" << d.m_str << "'" << std::endl;
    o << "}" << std::endl;
    o.setf(flags);
    return o;
}

Dummy::operator iox::cxx::Serialization() const
{
    return iox::cxx::Serialization::create(m_u16, m_u32, static_cast<iox::cxx::Serialization>(m_nes));
}

Dummy::Nested::operator iox::cxx::Serialization() const
{
    return iox::cxx::Serialization::create(m_str);
}

Dummy::Dummy(iox::cxx::Serialization& serial)
{
    std::string nested;
    auto success = serial.extract(m_u16, m_u32, nested);
    auto ser = iox::cxx::Serialization(nested);
    auto nes = Nested(ser);
    // this doesn't work
    // auto nes = Nested(iox::cxx::Serialization(nested));
    m_nes = nes;
    if (!success)
    {
        std::cout << "deserialization of Dummy failed" << std::endl;
    }
}

Dummy::Nested::Nested(iox::cxx::Serialization& serial)
{
    auto success = serial.extract(m_str);
    if (!success)
    {
        std::cout << "deserialization of Dummy::Nested failed" << std::endl;
    }
}
} // namespace

using namespace ::testing;

class Serialization_test : public Test
{
  public:
    void SetUp()
    {
        //         internal::CaptureStderr();
    }
    virtual void TearDown()
    {
        //         std::string output = internal::GetCapturedStderr();
        //         if (Test::HasFailure())
        //         {
        //             std::cout << output << std::endl;
        //         }
    }
};

TEST_F(Serialization_test, Dummy)
{
    Dummy dummy(42, 73, "Plumbus");
    std::cout << dummy << std::endl;
    auto serial = static_cast<iox::cxx::Serialization>(dummy);
    std::cout << serial.toString() << std::endl;

    Dummy dummy2(serial);
    std::cout << dummy2 << std::endl;
}

TEST_F(Serialization_test, CreateSingleEntry)
{
    auto serial = iox::cxx::Serialization::create("hello world");
    EXPECT_THAT(serial.toString(), Eq("11:hello world"));
}

TEST_F(Serialization_test, CreateMultiEntry)
{
    auto serial = iox::cxx::Serialization::create("hello world", 12345);
    EXPECT_THAT(static_cast<std::string>(serial), Eq("11:hello world5:12345"));
}

TEST_F(Serialization_test, ExtractSingleEntry)
{
    auto serial = iox::cxx::Serialization::create(12345);
    int i;
    EXPECT_THAT(serial.extract(i), Eq(true));
    EXPECT_THAT(i, Eq(12345));
}

TEST_F(Serialization_test, ExtractSingleEntryWrongType)
{
    auto serial = iox::cxx::Serialization::create("asd");
    int i;
    EXPECT_THAT(serial.extract(i), Eq(false));
}

TEST_F(Serialization_test, ExtractMultiEntry)
{
    auto serial = iox::cxx::Serialization::create(12345, 'c', "aasd");
    int i;
    char c;
    std::string s;
    EXPECT_THAT(serial.extract(i, c, s), Eq(true));
    EXPECT_THAT(i, Eq(12345));
    EXPECT_THAT(c, Eq('c'));
    EXPECT_THAT(s, Eq("aasd"));
}

TEST_F(Serialization_test, ExtractMultiEntryWrongType)
{
    auto serial = iox::cxx::Serialization::create(12345, 'c', "aasd");
    int i;
    char c;
    char s;
    EXPECT_THAT(serial.extract(i, c, s), Eq(false));
}

TEST_F(Serialization_test, GetNthSingleEntry)
{
    auto serial = iox::cxx::Serialization::create(12345);
    int i;
    EXPECT_THAT(serial.getNth(0, i), Eq(true));
    EXPECT_THAT(i, Eq(12345));
}

TEST_F(Serialization_test, GetNthSingleEntryWrongType)
{
    auto serial = iox::cxx::Serialization::create("a1234a5");
    int i;
    EXPECT_THAT(serial.getNth(0, i), Eq(false));
}

TEST_F(Serialization_test, GetNthMultiEntry)
{
    auto serial = iox::cxx::Serialization::create(12345, "asdasd", 'x', -123);
    int v1;
    std::string v2;
    char v3;
    int v4;
    EXPECT_THAT(serial.getNth(0, v1), Eq(true));
    EXPECT_THAT(serial.getNth(1, v2), Eq(true));
    EXPECT_THAT(serial.getNth(2, v3), Eq(true));
    EXPECT_THAT(serial.getNth(3, v4), Eq(true));

    EXPECT_THAT(v1, Eq(12345));
    EXPECT_THAT(v2, Eq("asdasd"));
    EXPECT_THAT(v3, Eq('x'));
    EXPECT_THAT(v4, Eq(-123));
}

TEST_F(Serialization_test, ExtractFromGivenSerialization)
{
    iox::cxx::Serialization serial("6:hello!4:1234");
    std::string v1;
    int v2;
    EXPECT_THAT(serial.extract(v1, v2), Eq(true));
    EXPECT_THAT(v1, Eq("hello!"));
    EXPECT_THAT(v2, Eq(1234));
}

TEST_F(Serialization_test, SerializeSerializableClass)
{
    struct A
    {
        A()
        {
        }
        A(const iox::cxx::Serialization&)
        {
        }
        operator iox::cxx::Serialization() const
        {
            return iox::cxx::Serialization("5:asdgg");
        }
    };

    A obj;
    auto serial = iox::cxx::Serialization::create(obj, "asd");
    EXPECT_THAT(serial.toString(), Eq("7:5:asdgg3:asd"));
}
