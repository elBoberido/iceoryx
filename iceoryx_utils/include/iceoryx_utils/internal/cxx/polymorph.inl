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
//
// SPDX-License-Identifier: Apache-2.0

#ifndef IOX_UTILS_CXX_POLYMORPH_INL
#define IOX_UTILS_CXX_POLYMORPH_INL

#include "iceoryx_utils/cxx/polymorph.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace iox
{
namespace cxx
{
template <typename Interface, size_t TypeSize, size_t TypeAlignment>
Polymorph<Interface, TypeSize, TypeAlignment>::~Polymorph() noexcept
{
    destruct();
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
template <typename Type, typename... CTorArgs>
Polymorph<Interface, TypeSize, TypeAlignment>::Polymorph(PolymorphType<Type>, CTorArgs&&... ctorArgs) noexcept
{
    emplace<Type>(std::forward<CTorArgs>(ctorArgs)...);
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
template <typename Type, typename... CTorArgs>
void Polymorph<Interface, TypeSize, TypeAlignment>::emplace(CTorArgs&&... ctorArgs) noexcept
{
    static_assert(TypeAlignment >= alignof(Type), "Alignment missmatch! No safe instantiation of Type possible!");
    static_assert(TypeSize >= sizeof(Type), "Size missmatch! Not enough space to instantiate Type!");

    destruct();

    m_instance = new (m_heap) Type(std::forward<CTorArgs>(ctorArgs)...);
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
void Polymorph<Interface, TypeSize, TypeAlignment>::destruct() noexcept
{
    if (m_instance != nullptr)
    {
        m_instance->~Interface();
        m_instance = nullptr;
    }
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
bool Polymorph<Interface, TypeSize, TypeAlignment>::isSpecified() const noexcept
{
    return m_instance != nullptr;
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
Interface* Polymorph<Interface, TypeSize, TypeAlignment>::operator->() const noexcept
{
    return m_instance;
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
Interface& Polymorph<Interface, TypeSize, TypeAlignment>::operator*() const noexcept
{
    return *m_instance;
}

} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_POLYMORPH_INL
