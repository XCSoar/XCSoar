// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "StringBuilder.hxx"
#include "StringAPI.hxx"

#include <algorithm>

#ifdef _UNICODE
#include <tchar.h>
#endif

#include <stdarg.h>
#include <stdio.h>

template<typename T>
void
BasicStringBuilder<T>::Append(const_pointer src)
{
	Append(src, StringLength(src));
}

template<typename T>
void
BasicStringBuilder<T>::Append(const_pointer src, size_t length)
{
	CheckAppend(length);

	p = std::copy_n(src, length, p);
	*p = SENTINEL;
}

template class BasicStringBuilder<char>;

template<>
void
BasicStringBuilder<char>::Format(const_pointer fmt, ...)
{
	size_t size = GetRemainingSize();

	va_list ap;
	va_start(ap, fmt);
	size_t n = vsnprintf(GetTail(), size, fmt, ap);
	va_end(ap);

	if (n >= size - 1)
		throw Overflow();

	Extend(n);
}

#ifdef _UNICODE
template class BasicStringBuilder<TCHAR>;
#endif
