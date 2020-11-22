#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

struct unsafe_t
{
};
static constexpr unsafe_t unsafe{};

enum class Format
{
    ASCII,
    BINARY,
};

/// @todo maybe add two bytes at the begin of the buffer to check for endianness
/// (or maybe just encode it in the container serialization with [size|endianness|data]) and just do a sanity check for
/// NATIVE
/// @code
/// // also have look at std::endian from C++20
/// enum class Endianness : uint8_t {
///     LITTLE = 0x5A,
///     BIG = 0xA5,
/// };
/// uint8_t endiannessBuffer[2];
/// uint16_t endiannessIndicator {(static_cast<uint16_t>(Endianness::BIG) << 8) |
/// static_cast<uint16_t>(Endianness::LITTLE)}; memcpy(endiannessBuffer, &endiannessIndicator, 2); uint8_t buffer[100];
/// buffer[0] = endiannessBuffer[0];
/// return static_cast<Endianness>(buffer[0]);
/// @endcode


template <uint64_t C, Format F = Format::BINARY>
class Buffer
{
  public:
    static constexpr uint64_t END_POSITION = (F == Format::ASCII ? C - 1 : C);

    Buffer() = default;

    static constexpr uint64_t capacity()
    {
        return C;
    }

    bool append(uint8_t data)
    {
        if (m_insertPosition >= END_POSITION)
        {
            return false;
        }
        m_data[m_insertPosition] = data;
        ++m_insertPosition;
        if (F == Format::ASCII)
        {
            m_data[m_insertPosition] = 0;
        }
        return true;
    }

    bool append(const void* data, const uint64_t size)
    {
        if ((data == nullptr) || (m_insertPosition + size > END_POSITION))
        {
            return false;
        }
        std::memcpy(&m_data[m_insertPosition], data, size);
        m_insertPosition += size;
        if (F == Format::ASCII)
        {
            m_data[m_insertPosition] = 0;
        }
        return true;
    }

    bool set(const void* data, const uint64_t size)
    {
        reset();
        return append(data, size);
    }

    void reset()
    {
        m_insertPosition = 0;
        m_data[0] = 0;
    }

    const uint8_t* data() const
    {
        return m_data;
    }

    uint8_t* data(unsafe_t)
    {
        return m_data;
    }

    uint64_t size() const
    {
        return m_insertPosition;
    }

    bool setSize(unsafe_t, uint64_t size)
    {
        if (size > END_POSITION)
        {
            return false;
        }
        m_insertPosition = size;
        if (F == Format::ASCII)
        {
            m_data[m_insertPosition] = 0;
        }
        return true;
    }

  private:
    uint64_t m_insertPosition{0};
    uint8_t m_data[C] = {0};
};

template <uint64_t C, Format F = Format::BINARY>
class SerDe
{
  public:
    SerDe() = default;

    SerDe(const char separator)
        : m_separator(separator)
    {
        static_assert(F != Format::BINARY, "BINARY Format doesn't support separator character");
    }

    operator bool() const
    {
        return !m_serDeFailed;
    }

    using Buffer_t = Buffer<C, F>;
    using ContainerLenghtType_t = uint32_t;

    Buffer_t& buffer()
    {
        return m_buffer;
    }

    SerDe& operator<<(const uint8_t pod);
    SerDe& operator<<(const uint16_t pod);
    SerDe& operator<<(const uint32_t pod);
    SerDe& operator<<(const std::string& str);
    SerDe& operator<<(const std::pair<const void*, const ContainerLenghtType_t> dataContainer);

    const SerDe& operator>>(uint8_t& pod) const;
    const SerDe& operator>>(uint16_t& pod) const;
    const SerDe& operator>>(uint32_t& pod) const;
    const SerDe& operator>>(std::string& str) const;
    /// @todo the might be something like a peek function for the container length interesting,
    /// in order to call this from e.g. `const SerDe& operator>>(std::string& str) const;` and reserve space in or
    /// resize the container
    const SerDe&
    operator>>(std::pair<void*, std::pair<ContainerLenghtType_t /*size*/, const ContainerLenghtType_t /*capacity*/>>&
                   dataContainer) const;

    template <uint64_t CC, Format FF>
    friend std::ostream& operator<<(std::ostream&, const SerDe<CC, FF>&);

  private:
    mutable uint64_t m_deserializeEndPosition{0};
    mutable bool m_serDeFailed{false};
    char m_separator = ':';
    Buffer_t m_buffer;
};

template <uint64_t C, Format F>
std::ostream& operator<<(std::ostream& o, const SerDe<C, F>& s)
{
    if (F == Format::ASCII)
    {
        o << s.m_buffer.data();
    }
    else
    {
        auto flags = o.flags();
        o << "0x[ ";
        o << std::hex << std::uppercase << std::setfill('0');
        for (auto i = 0u; i < s.m_buffer.size(); ++i)
        {
            o << std::setw(2) << static_cast<uint16_t>(s.m_buffer.data()[i]) << " ";
        }
        o << "]";
        o.setf(flags);
    }
    return o;
}

template <uint64_t C, Format F>
SerDe<C, F>& SerDe<C, F>::operator<<(const uint8_t pod)
{
    if (!m_serDeFailed)
    {
        if (F == Format::ASCII)
        {
            auto ascii = std::to_string(pod);
            m_serDeFailed |= !m_buffer.append(ascii.c_str(), ascii.size());
            m_serDeFailed |= !m_buffer.append(m_separator);
        }
        else
        {
            m_serDeFailed |= !m_buffer.append(pod);
        }
    }
    return *this;
}

template <uint64_t C, Format F>
SerDe<C, F>& SerDe<C, F>::operator<<(const uint16_t pod)
{
    if (!m_serDeFailed)
    {
        if (F == Format::ASCII)
        {
            auto ascii = std::to_string(pod);
            m_serDeFailed |= !m_buffer.append(ascii.c_str(), ascii.size());
            m_serDeFailed |= !m_buffer.append(m_separator);
        }
        else
        {
            m_serDeFailed |= !m_buffer.append(static_cast<uint8_t>((pod >> 8) & 0xFF));
            m_serDeFailed |= !m_buffer.append(static_cast<uint8_t>((pod >> 0) & 0xFF));
        }
    }
    return *this;
}

template <uint64_t C, Format F>
SerDe<C, F>& SerDe<C, F>::operator<<(const uint32_t pod)
{
    if (!m_serDeFailed)
    {
        if (F == Format::ASCII)
        {
            auto ascii = std::to_string(pod);
            m_serDeFailed |= !m_buffer.append(ascii.c_str(), ascii.size());
            m_serDeFailed |= !m_buffer.append(m_separator);
        }
        else
        {
            m_serDeFailed |= !m_buffer.append(static_cast<uint8_t>((pod >> 24) & 0xFF));
            m_serDeFailed |= !m_buffer.append(static_cast<uint8_t>((pod >> 16) & 0xFF));
            m_serDeFailed |= !m_buffer.append(static_cast<uint8_t>((pod >> 8) & 0xFF));
            m_serDeFailed |= !m_buffer.append(static_cast<uint8_t>((pod >> 0) & 0xFF));
        }
    }
    return *this;
}

template <uint64_t C, Format F>
SerDe<C, F>& SerDe<C, F>::operator<<(const std::string& str)
{
    if (F == Format::BINARY)
    {
        m_serDeFailed |= str.size() > std::numeric_limits<ContainerLenghtType_t>::max();
    }
    if (!m_serDeFailed)
    {
        *this << std::pair<const void*, ContainerLenghtType_t>(str.c_str(),
                                                               static_cast<ContainerLenghtType_t>(str.size()));
    }
    return *this;
}

template <uint64_t C, Format F>
SerDe<C, F>&
SerDe<C, F>::operator<<(const std::pair<const void*, const ContainerLenghtType_t> dataContainer)
{
    auto data = dataContainer.first;
    auto size = dataContainer.second;
    if (F == Format::BINARY)
    {
        *this << size;
    }
    if (!m_serDeFailed)
    {
        m_serDeFailed |= !m_buffer.append(data, size);
        if (F == Format::ASCII)
        {
            m_serDeFailed |= !m_buffer.append(m_separator);
        }
    }
    return *this;
}


template <uint64_t C, Format F>
const SerDe<C, F>& SerDe<C, F>::operator>>(uint8_t& pod) const
{
    if (m_deserializeEndPosition + sizeof(pod) <= m_buffer.size())
    {
        if (F == Format::ASCII)
        {
            std::cout << "TODO: " << __PRETTY_FUNCTION__ << std::endl;
        }
        else
        {
            auto i = m_deserializeEndPosition;
            pod = m_buffer.data()[i];
        }
    }
    else
    {
        m_serDeFailed = true;
    }
    m_deserializeEndPosition += sizeof(pod);
    return *this;
}

template <uint64_t C, Format F>
const SerDe<C, F>& SerDe<C, F>::operator>>(uint16_t& pod) const
{
    if (m_deserializeEndPosition + sizeof(pod) <= m_buffer.size())
    {
        if (F == Format::ASCII)
        {
            std::cout << "TODO: " << __PRETTY_FUNCTION__ << std::endl;
        }
        else
        {
            auto i = m_deserializeEndPosition;
            pod = (static_cast<uint16_t>(m_buffer.data()[i + 0]) << 8);
            pod |= (static_cast<uint16_t>(m_buffer.data()[i + 1]) << 0);
        }
    }
    else
    {
        m_serDeFailed = true;
    }
    m_deserializeEndPosition += sizeof(pod);
    return *this;
}

template <uint64_t C, Format F>
const SerDe<C, F>& SerDe<C, F>::operator>>(uint32_t& pod) const
{
    if (m_deserializeEndPosition + sizeof(pod) <= m_buffer.size())
    {
        if (F == Format::ASCII)
        {
            std::cout << "TODO: " << __PRETTY_FUNCTION__ << std::endl;
        }
        else
        {
            auto i = m_deserializeEndPosition;
            pod = (static_cast<uint32_t>(m_buffer.data()[i + 0]) << 24);
            pod |= (static_cast<uint32_t>(m_buffer.data()[i + 1]) << 16);
            pod |= (static_cast<uint32_t>(m_buffer.data()[i + 2]) << 8);
            pod |= (static_cast<uint32_t>(m_buffer.data()[i + 3]) << 0);
        }
    }
    else
    {
        m_serDeFailed = true;
    }
    m_deserializeEndPosition += sizeof(pod);
    return *this;
}

template <uint64_t C, Format F>
const SerDe<C, F>& SerDe<C, F>::operator>>(std::string& str) const
{
    if (m_deserializeEndPosition + sizeof(ContainerLenghtType_t) <= m_buffer.size())
    {
        if (F == Format::ASCII)
        {
            std::cout << "TODO: " << __PRETTY_FUNCTION__ << std::endl;
        }
        else
        {
            ContainerLenghtType_t stringLength{0};
            *this >> stringLength;
            if (!m_serDeFailed)
            {
                if ((m_deserializeEndPosition + stringLength) <= m_buffer.size())
                {
                    auto i = m_deserializeEndPosition;
                    str = std::string(reinterpret_cast<const char*>(&m_buffer.data()[i]), stringLength);
                }
                else
                {
                    m_serDeFailed = true;
                }
                m_deserializeEndPosition += stringLength;
            }
        }
    }
    else
    {
        m_serDeFailed = true;
    }
    return *this;
}


enum class RequestType
{
};

class Datagram
{
  public:
    Datagram() = default;

    using SerDe_t = SerDe<1024>;

    SerDe_t& serDe()
    {
        return m_buffer;
    }

    SerDe_t::Buffer_t& buffer()
    {
        return m_buffer.buffer();
    }

  private:
    SerDe_t m_buffer;
};

class IpcChannel
{
  public:
    template <uint64_t C, Format F>
    void send(Buffer<C, F>& buffer)
    {
        send(buffer.data(), buffer.size());
    }

    void send(const uint8_t* data, const uint64_t size)
    {
        m_testData.clear();
        for (auto i = 0u; i < size; ++i)
        {
            m_testData.push_back(data[i]);
        }
    }

    template <uint64_t C, Format F>
    bool receive(Buffer<C, F>& buffer)
    {
        uint64_t size{0};
        auto result = receive(buffer.data(unsafe), size, buffer.capacity());
        if (result)
        {
            buffer.setSize(unsafe, size);
        }

        return result;
    }

    bool receive(uint8_t* buffer, uint64_t& size, const uint64_t capacity)
    {
        if (m_testData.empty())
        {
            std::cout << "No Data!!!" << std::endl;
            return false;
        }

        if (m_testData.size() > capacity)
        {
            return false;
        }

        size = m_testData.size();
        std::memcpy(buffer, m_testData.data(), size);

        m_testData.clear();

        return true;
    }

  private:
    std::vector<uint8_t> m_testData;
};

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

        friend std::ostream& operator<<(std::ostream&, const Dummy::Nested&);

        template <uint64_t C, Format F>
        friend SerDe<C, F>& operator<<(SerDe<C, F>&, const Dummy::Nested&);
        template <uint64_t C, Format F>
        friend const SerDe<C, F>& operator>>(const SerDe<C, F>&, Dummy::Nested&);

      private:
        std::string m_str;
    };

    Dummy(uint16_t u16, uint32_t u32, std::string str)
        : m_u16(u16)
        , m_u32(u32)
        , m_nes(str)
    {
    }

    friend std::ostream& operator<<(std::ostream&, const Dummy&);

    template <uint64_t C, Format F>
    friend SerDe<C, F>& operator<<(SerDe<C, F>&, const Dummy&);
    template <uint64_t C, Format F>
    friend const SerDe<C, F>& operator>>(const SerDe<C, F>&, Dummy&);
    /// @note for classes without default c'tor use a reference to an optional => `cxx::optional<Dummy>&`

  private:
    uint16_t m_u16{0};
    uint32_t m_u32{0};
    Nested m_nes;
    uint8_t m_arr[3] = {0xC0, 0xFF, 0xEE};
};

std::ostream& operator<<(std::ostream& o, const Dummy& d)
{
    auto flags = o.flags();
    o << "Dummy {" << std::endl;
    o << "    m_u16 = 0x" << std::hex << d.m_u16 << std::endl;
    o << "    m_u32 = 0x" << std::hex << d.m_u32 << std::endl;
    o << "    m_nes = " << d.m_nes;
    o << "    m_arr = 0x[" << std::hex;
    o << std::setw(2) << static_cast<uint16_t>(d.m_arr[0]) << " ";
    o << std::setw(2) << static_cast<uint16_t>(d.m_arr[1]) << " ";
    o << std::setw(2) << static_cast<uint16_t>(d.m_arr[2]);
    o << "]" << std::endl;
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

template <uint64_t C, Format F>
SerDe<C, F>& operator<<(SerDe<C, F>& s, const Dummy& d)
{
    s << d.m_u16 << d.m_u32 << d.m_nes;
    return s;
}

template <uint64_t C, Format F>
SerDe<C, F>& operator<<(SerDe<C, F>& s, const Dummy::Nested& d)
{
    s << d.m_str;
    return s;
}

template <uint64_t C, Format F>
const SerDe<C, F>& operator>>(const SerDe<C, F>& s, Dummy& d)
{
    s >> d.m_u16 >> d.m_u32 >> d.m_nes;
    if (!s)
    {
        std::cout << "deserialization of Dummy failed" << std::endl;
    }
    return s;
}

template <uint64_t C, Format F>
const SerDe<C, F>& operator>>(const SerDe<C, F>& s, Dummy::Nested& d)
{
    s >> d.m_str;
    if (!s)
    {
        std::cout << "deserialization of Dummy::Nested failed" << std::endl;
    }
    return s;
}

/// @note create test with the serialized value in the begin/middle and at the end of the buffer

#include "test.hpp"

TEST(SerDe_test, main)
{
    Dummy dummy(0xAFFE, 0xDEADBEEF, "Plumbus");
    std::cout << dummy << std::endl;

    Datagram dg;
    dg.serDe() << dummy;
    std::cout << "Send: " << dg.serDe() << std::endl;
    IpcChannel ch;
    ch.send(dg.buffer());

    Datagram recDg;
    if (ch.receive(recDg.buffer()))
    {
        std::cout << "Received: " << recDg.serDe() << std::endl;
        Dummy d;
        recDg.serDe() >> d;
        std::cout << d << std::endl;
    }

    Datagram recDg2;
    ch.receive(recDg2.buffer());

    SerDe<1024, Format::ASCII> str;
    str << dummy;
    std::cout << str << std::endl;
}
