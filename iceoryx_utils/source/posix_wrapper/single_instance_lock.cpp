
#include "iceoryx_utils/internal/posix_wrapper/single_instance_lock.hpp"
#include "iceoryx_utils/platform/socket.hpp"
#include "iceoryx_utils/platform/unistd.hpp"

#include <cassert>

namespace iox
{
namespace posix
{
SingleInstanceLock::SingleInstanceLock(const uint16_t socketPort) noexcept : m_socketPort(socketPort)
{
    // according to IANA, private ports must be in the range of 0xC000-0xFFFF
    // https://tools.ietf.org/id/draft-cotton-tsvwg-iana-ports-00.html#privateports
    assert(m_socketPort >= 0xC000U && "According to IANA, dynamic (private) ports should be in the range of 0xC000-0xFFFF!");
}

SingleInstanceLock::~SingleInstanceLock() noexcept
{
    if (m_socketFd.has_value())
    {
        auto closeResult = cxx::makeSmartC(
            closePlatformFileHandle, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, m_socketFd.value());

        if (closeResult.hasErrors())
        {
            std::cerr << "Could not close socket" << std::endl;
        }
    }
}

SingleInstanceLock::LockResult SingleInstanceLock::lock() noexcept
{
    auto openSocket =
        cxx::makeSmartC(socket, cxx::ReturnMode::PRE_DEFINED_ERROR_CODE, {-1}, {}, AF_INET, SOCK_STREAM, 0);
    if (openSocket.hasErrors())
    {
        return SingleInstanceLock::LockResult::SOCKET_FD_FAILED;
    }
    m_socketFd.emplace(openSocket.getReturnValue());

    memset(&m_sockserv, 0, sizeof(sockaddr_in));
    m_sockserv.sin_family = AF_INET;
    m_sockserv.sin_addr.s_addr = inet_addr("127.0.0.1");
    m_sockserv.sin_port = htons(m_socketPort);

    // bind socket for locking
    auto bindResult = cxx::makeSmartC(bind,
                                      cxx::ReturnMode::PRE_DEFINED_SUCCESS_CODE,
                                      {0},
                                      {},
                                      m_socketFd.value(),
                                      reinterpret_cast<struct sockaddr*>(&m_sockserv),
                                      static_cast<unsigned int>(sizeof(m_sockserv)));


    if (bindResult.hasErrors())
    {
        return SingleInstanceLock::LockResult::BIND_FAILED;
    }

    return SingleInstanceLock::LockResult::SUCCESS;
}

} // namespace posix
} // namespace iox
