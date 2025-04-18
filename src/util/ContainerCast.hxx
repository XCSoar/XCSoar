// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cstddef>

/**
 * Offset the given pointer by the specified number of bytes.
 */
constexpr void *
OffsetPointer(void *p, ptrdiff_t offset)
{
	return (char *)p + offset;
}

/**
 * Offset the given pointer by the specified number of bytes.
 */
constexpr const void *
OffsetPointer(const void *p, ptrdiff_t offset)
{
	return (const char *)p + offset;
}

template<typename T, typename U>
constexpr T *
OffsetCast(U *p, ptrdiff_t offset)
{
	return reinterpret_cast<T *>(OffsetPointer(p, offset));
}

template<typename T, typename U>
constexpr T *
OffsetCast(const U *p, ptrdiff_t offset)
{
	return reinterpret_cast<const T *>(OffsetPointer(p, offset));
}

template<class C, class A>
constexpr ptrdiff_t
ContainerAttributeOffset(const C *null_c, const A C::*p)
{
	return ptrdiff_t((const char *)null_c - (const char *)&(null_c->*p));
}

template<class C, class A>
constexpr ptrdiff_t
ContainerAttributeOffset(const A C::*p)
{
	return ContainerAttributeOffset<C, A>(nullptr, p);
}

/**
 * Cast the given pointer to a struct member to its parent structure.
 */
template<class C, class A>
constexpr C &
ContainerCast(A &a, A C::*member)
{
	return *OffsetCast<C, A>(&a, ContainerAttributeOffset<C, A>(member));
}

/**
 * Cast the given pointer to a struct member to its parent structure.
 */
template<class C, class A>
constexpr const C &
ContainerCast(const A &a, A C::*member)
{
	return *OffsetCast<const C, const A>(&a, ContainerAttributeOffset<C, A>(member));
}
