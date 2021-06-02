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

#include "iceoryx_hoofs/platform/socket.hpp"
#include "iceoryx_hoofs/platform/win32_errorHandling.hpp"

#include <iostream>


struct Winsock2ApiInitializer
{
    Winsock2ApiInitializer()
    {
        WORD requestedVersion = MAKEWORD(2, 2);
        WSADATA wsaData;
        auto result = Win32Call(WSAStartup, requestedVersion, &wsaData).value;
        if (result != 0)
        {
            std::cerr << "unable to initialize winsock2" << std::endl;
            std::terminate();
        }

        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
        {
            std::cerr << "required winsock2.dll version is 2.2, found " << HIBYTE(wsaData.wVersion) << "."
                      << LOBYTE(wsaData.wVersion) << std::endl;
            cleanupWinsock();
            std::terminate();
        }
    }

    ~Winsock2ApiInitializer()
    {
        cleanupWinsock();
    }

    void cleanupWinsock()
    {
        Win32Call(WSACleanup);
    }
};

static Winsock2ApiInitializer winsock2ApiInitializer;

int iox_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    return 0;
}

int iox_socket(int domain, int type, int protocol)
{
    return 0;
}

int iox_setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
{
    return 0;
}

ssize_t
iox_sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen)
{
    return 0;
}

ssize_t iox_recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    return 0;
}

int iox_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    return 0;
}

int iox_closesocket(int sockfd)
{
    return Win32Call(closesocket, static_cast<SOCKET>(sockfd)).value;
}
