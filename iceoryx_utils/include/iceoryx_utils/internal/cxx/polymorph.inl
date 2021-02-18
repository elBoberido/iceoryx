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
template <typename T>
void Polymorph<Interface, TypeSize, TypeAlignment>::TypeRetention::polymorphId() noexcept
{
}
template <typename Interface, size_t TypeSize, size_t TypeAlignment>
template <typename P, typename T_base, typename T_rhs>
void Polymorph<Interface, TypeSize, TypeAlignment>::TypeRetention::mover(P* lhs, P* rhs) noexcept
{
    if (lhs == rhs)
    {
        return;
    }

    if (lhs->m_instance && rhs->m_instance && lhs->m_typeRetention.id == rhs->m_typeRetention.id)
    {
        *static_cast<T_rhs*>(lhs->m_instance) = std::move(*static_cast<T_rhs*>(rhs->m_instance));
    }
    else
    {
        if (lhs->m_instance)
        {
            lhs->m_instance->~T_base();
            lhs->m_instance = nullptr;
        }
        if (rhs->m_instance)
        {
            lhs->m_instance = new (lhs->m_storage) T_rhs(std::move(*static_cast<T_rhs*>(rhs->m_instance)));
        }
    }
    lhs->m_typeRetention.id = rhs->m_typeRetention.id;
    rhs->m_typeRetention.id = nullptr;
    lhs->m_typeRetention.move = rhs->m_typeRetention.move;
    rhs->m_typeRetention.move = nullptr;
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
Polymorph<Interface, TypeSize, TypeAlignment>::~Polymorph() noexcept
{
    destruct();
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
template <typename T, typename... CTorArgs>
Polymorph<Interface, TypeSize, TypeAlignment>::Polymorph(PolymorphType<T>, CTorArgs&&... ctorArgs) noexcept
{
    emplace<T>(std::forward<CTorArgs>(ctorArgs)...);
}

template <typename Interface, size_t TypeSize, size_t TypeAlignment>
template <typename T, typename... CTorArgs>
void Polymorph<Interface, TypeSize, TypeAlignment>::emplace(CTorArgs&&... ctorArgs) noexcept
{
    static_assert(TypeAlignment >= alignof(T), "Alignment missmatch! No safe instantiation of Type possible!");
    static_assert(TypeSize >= sizeof(T), "Size missmatch! Not enough space to instantiate Type!");

    static_assert(std::is_base_of<Interface, T>(), "The type T must be derived from Interface!");
    static_assert(std::is_same<Interface, T>() || std::has_virtual_destructor<Interface>::value,
                  "The interface must have a virtual destructor!");

    destruct();

    m_instance = new (m_storage) T(std::forward<CTorArgs>(ctorArgs)...);
    m_typeRetention.id = &TypeRetention::template polymorphId<T>;
    m_typeRetention.move = &TypeRetention::template mover<Polymorph, Interface, T>;
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
