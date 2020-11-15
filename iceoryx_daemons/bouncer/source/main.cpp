#include "iceoryx_utils/internal/posix_wrapper/single_instance_lock.hpp"
#include "iceoryx_utils/internal/posix_wrapper/unix_domain_socket.hpp"
#include "iceoryx_utils/posix_wrapper/semaphore.hpp"

#include "iceoryx_utils/platform/signal.hpp"
#include "iceoryx_utils/platform/unistd.hpp"

#include <iostream>
#include <thread>

iox::cxx::optional<iox::posix::Semaphore> g_runSemaphore;

void sigHandler(int32_t signal) noexcept
{
    if (signal == SIGHUP)
    {
        char msg[] = "Error! SIGHUP not supported by iox-bouncer!";
        size_t len = strlen(msg);
        write(STDERR_FILENO, msg, len);
        _exit(EXIT_FAILURE);
    }

    g_runSemaphore.and_then([](iox::posix::Semaphore& sem) { sem.post(); }).or_else([]() {
        char msg[] = "Error! Run semaphore not available!";
        size_t len = strlen(msg);
        write(STDERR_FILENO, msg, len);
        _exit(EXIT_FAILURE);
    });
}

void registerSigHandler() noexcept
{
    // register sigHandler for SIGINT, SIGTERM and SIGHUP
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = sigHandler;
    act.sa_flags = 0;
    if (-1 == sigaction(SIGINT, &act, NULL))
    {
        std::cout << "Calling sigaction() failed" << std::endl;
        std::terminate();
    }

    if (-1 == sigaction(SIGTERM, &act, NULL))
    {
        std::cout << "Calling sigaction() failed" << std::endl;
        std::terminate();
    }

    if (-1 == sigaction(SIGHUP, &act, NULL))
    {
        std::cout << "Calling sigaction() failed" << std::endl;
        std::terminate();
    }
}

void executeMinimalReadynessProtocol() noexcept
{
    // minimal readiness protocol implementation
    // https://www.man7.org/linux/man-pages/man3/sd_notify.3.html
    auto notifySocketName = std::getenv("NOTIFY_SOCKET");
    if (notifySocketName)
    {
        // simulating startup
        std::cout << "Startup ... " << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::cout << "notify socket: " << notifySocketName << std::endl;
        auto sdNotify = iox::posix::UnixDomainSocket::create(iox::posix::UnixDomainSocket::NoPathPrefix_t(),
                                                             notifySocketName,
                                                             iox::posix::IpcChannelMode::BLOCKING,
                                                             iox::posix::IpcChannelSide::CLIENT);
        sdNotify.and_then([](iox::posix::UnixDomainSocket& notify) { notify.send("READY=1\n"); })
            .or_else([](iox::posix::IpcChannelError& error) {
                std::cout << "Error opening NOTIFY_SOCKET! Error code: " << static_cast<uint64_t>(error) << std::endl;
            });
    }
    else
    {
        std::cout << "NOTIFY_SOCKET not found" << std::endl;
    }

    /// @todo send a "STOPPING=1\n" at shutdown and a "STATUS=Reason for failure\n" in case of a failure
}

/// To test this with systemd, create a iox-bouncer.service file in `~/.config/systemd/user` with the following content
/// @code
///     [Unit]
///     Description=iceoryx bouncer daemon
///
///     [Service]
///     Type=notify
///     ExecStart=/full/path/to/iox-bouncer
///
///     [Install]
///     WantedBy=multi-user.target
/// @endcode
/// Use `systemctl --user enable iox-bouncer` and `systemctl --user disable iox-bouncer` to enable/disable the service
/// Use `systemctl --user start iox-bouncer` and `systemctl --user stop iox-bouncer` to start/stop the service
/// Use `systemctl --user status iox-bouncer` to get the status of the service
int main()
{
    constexpr uint16_t BOUNCER_LOCKING_PORT{0xCBBBU};
    iox::posix::SingleInstanceLock singleInstanceLock(BOUNCER_LOCKING_PORT);
    switch (singleInstanceLock.lock())
    {
    case iox::posix::SingleInstanceLock::LockResult::SOCKET_FD_FAILED:
        std::cout << "Could not acquire fd for lock!" << std::endl;
        return EXIT_FAILURE;
        break;
    case iox::posix::SingleInstanceLock::LockResult::BIND_FAILED:
        std::cout << "Could not bind to port! Bouncer might be already running" << std::endl;
        return EXIT_FAILURE;
        break;
    default:
        break;
    }

    constexpr unsigned int RUN_SEMAPHORE_VALUE{0};
    auto runSemaphore =
        iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, RUN_SEMAPHORE_VALUE);
    if (runSemaphore.has_error())
    {
        std::cout << "Could not obtain run semaphore!" << std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        g_runSemaphore.emplace(std::move(runSemaphore.get_value()));
    }

    registerSigHandler();

    auto comm = iox::posix::UnixDomainSocket::create(
        "/iox-bouncer", iox::posix::IpcChannelMode::BLOCKING, iox::posix::IpcChannelSide::SERVER);

    if (comm.has_error())
    {
        std::cout << "Could not communication socket!" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Ready ... " << std::flush;

    executeMinimalReadynessProtocol();

    g_runSemaphore.and_then([](iox::posix::Semaphore& sem) { sem.wait(); });

    std::cout << "finished" << std::endl;
}
