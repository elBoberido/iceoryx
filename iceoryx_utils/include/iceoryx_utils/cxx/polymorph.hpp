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

#ifndef IOX_UTILS_CXX_POLYMORPH_HPP
#define IOX_UTILS_CXX_POLYMORPH_HPP

#include <cstddef>
#include <cstdint>
#include <utility>

namespace iox
{
namespace cxx
{
/// This is a proxy which must be used for the non default Polymorph ctor
template <typename T>
class PolymorphType
{
};

template <typename I, uint64_t Size, uint64_t Alignment>
class Polymorph;

/// @brief Reserves space on stack for placement new instantiation
/// @param Interface base type of all classes which should be stored in here
/// @param TypeSize maximum size of a child of Interface
/// @param TypeAlignment alignment which is required for the types
///
/// @code
/// #include "iceoryx_utils/cxx/poor_mans_heap.hpp"
///
/// #include "iceoryx_utils/cxx/helplets.hpp"
///
/// #include <iostream>
///
/// class Base
/// {
///   public:
///     virtual ~Base() = default;
///     virtual void doStuff() = 0;
/// };
///
/// class Foo : public Base
/// {
///   public:
///     Foo(int stuff)
///         : m_stuff(stuff)
///     {
///     }
///
///     void doStuff() override
///     {
///         std::cout << __PRETTY_FUNCTION__ << ": " << m_stuff << std::endl;
///     }
///
///   private:
///     int m_stuff;
/// };
///
/// class Bar : public Base
/// {
///   public:
///     void doStuff() override
///     {
///         std::cout << __PRETTY_FUNCTION__ << std::endl;
///     }
/// };
///
/// int main()
/// {
///     constexpr auto MaxSize = cxx::maxSize<Foo, Bar>();
///     constexpr auto MaxAlignment = cxx::maxAlignment<Foo, Bar>();
///
///     using FooBar = cxx::Polymorph<Base, MaxSize, MaxAlignment>;
///
///     FooBar fooBar1{cxx::PolymorphType<Foo>(), 42};
///     fooBar1->doStuff();
///
///     fooBar1.emplace<Bar>();
///     fooBar1->doStuff();
///
///     fooBar1.emplace<Foo>(13);
///     fooBar1->doStuff();
///
///     return 0;
/// }
/// @endcode
template <typename Interface, size_t TypeSize, size_t TypeAlignment = 8>
class Polymorph
{
  public:
    ~Polymorph() noexcept;

    /// Constructor for a specific instance
    /// @param [in] T the type to instantiate, wrapped in PolymorphType
    /// @param [in] ctorArgs ctor arguments for the type to instantiate
    template <typename T, typename... CTorArgs>
    Polymorph(PolymorphType<T>, CTorArgs&&... ctorArgs) noexcept;

    Polymorph(Polymorph&& other)
    {
        other.m_typeRetention.move(this, &other);
    }
    Polymorph& operator=(Polymorph&& rhs)
    {
        rhs.m_typeRetention.move(this, &rhs);
        return *this;
    }

    Polymorph(const Polymorph&) = delete;
    Polymorph& operator=(const Polymorph&) = delete;

    /// Replaces the current instance with an instance of the specified Type
    /// @param [in] T the type to instantiate
    /// @param [in] ctorArgs ctor arguments for the type to instantiate
    template <typename T, typename... CTorArgs>
    void emplace(CTorArgs&&... ctorArgs) noexcept;

    /// Checks is there is a valid instance
    /// @return true if there is a valid instance
    bool isSpecified() const noexcept;

    /// Returns a pointer to the underlying instance
    /// @return pointer to the underlying instance or nullptr if there is no valid instance
    Interface* operator->() const noexcept;

    /// Returns a reference to the underlying instance. If there is no valid instance, the behaviour is undefined
    /// @return reference to the underlying instance
    Interface& operator*() const noexcept;

  private:
      struct TypeRetention
      {
          template <typename T>
          static void polymorphId() noexcept;

          template <typename P, typename T_base, typename T_rhs>
          static void mover(P* lhs, P* rhs) noexcept;

          void (*id)(){nullptr};
          void (*move)(Polymorph*, Polymorph*){nullptr};
      };

    // helper struct used for situation where the object must be defined but unspecified like after a move
    struct Unspecified
    {
    };

    void destruct() noexcept;

  private:
    alignas(TypeAlignment) uint8_t m_storage[TypeSize];
    Interface* m_instance{nullptr};
    TypeRetention m_typeRetention;
};

} // namespace cxx
} // namespace iox

#include "iceoryx_utils/internal/cxx/polymorph.inl"

#endif // IOX_UTILS_CXX_POLYMORPH_HPP
