
#ifndef IOX_UTILS_POSIX_WRAPPER_SINGLE_INSTANCE_LOCK_HPP
#define IOX_UTILS_POSIX_WRAPPER_SINGLE_INSTANCE_LOCK_HPP

#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/platform/inet.hpp"
#include "iceoryx_utils/platform/socket.hpp"
#include "iceoryx_utils/platform/types.hpp"

namespace iox
{
namespace posix
{
/// @todo use CreationPattern
class SingleInstanceLock
{
  public:
    SingleInstanceLock(const uint16_t socketPort) noexcept;

    ~SingleInstanceLock() noexcept;

    SingleInstanceLock(const SingleInstanceLock&) noexcept = delete;
    SingleInstanceLock(SingleInstanceLock&&) noexcept = delete;

    SingleInstanceLock& operator=(const SingleInstanceLock&) noexcept = delete;
    SingleInstanceLock& operator=(SingleInstanceLock&&) noexcept = delete;

    enum class LockResult
    {
        SUCCESS,
        SOCKET_FD_FAILED,
        BIND_FAILED,
    };

    LockResult lock() noexcept;

  private:
    const uint16_t m_socketPort{0};
    cxx::optional<int> m_socketFd;
    struct sockaddr_in m_sockserv;
};

} // namespace posix
} // namespace iox

#endif // IOX_UTILS_POSIX_WRAPPER_SINGLE_INSTANCE_LOCK_HPP
